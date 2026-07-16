#include "workflow/WFTaskFactory.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <sys/stat.h>
#include <type_traits>

#include "HttpFile.h"
#include "HttpMsg.h"
#include "PathUtil.h"
#include "HttpServerTask.h"
#include "ErrorCode.h"
#include "FileCache.h"

namespace wfrest
{

namespace
{

struct SaveFileContext
{
    std::string content;
    std::string notify_msg;
    HttpFile::FileIOArgsFunc fileio_args_func;
};

struct FileRange
{
    size_t start;
    size_t end;
    bool partial;
};

struct FileMetadata
{
    size_t size;
    std::time_t last_modified;
};

struct ReadContext
{
    HttpResp *resp;
    size_t expected_size;
};

struct CacheContext
{
    HttpResp *resp;
    std::string path;
    size_t file_size;
    size_t start;
    size_t end;
    std::time_t last_modified;
};

bool normalize_file_range(size_t file_size, size_t encoded_start,
                          size_t encoded_end, FileRange *range)
{
    using SignedSize = std::make_signed<size_t>::type;
    const size_t max_absolute =
        static_cast<size_t>(std::numeric_limits<SignedSize>::max());

    size_t start;
    if (encoded_start > max_absolute)
    {
        const size_t suffix_size = ~encoded_start + 1;
        if (suffix_size == 0 || suffix_size > file_size)
            return false;
        start = file_size - suffix_size;
    }
    else
    {
        start = encoded_start;
    }

    size_t end;
    if (encoded_end == static_cast<size_t>(-1))
    {
        end = file_size;
    }
    else
    {
        if (encoded_end > max_absolute)
            return false;
        end = std::min(encoded_end, file_size);
    }

    if (file_size == 0)
    {
        const bool valid_empty_end =
            encoded_end == 0 || encoded_end == static_cast<size_t>(-1);
        if (start != 0 || end != 0 || !valid_empty_end)
            return false;
    }
    else if (start >= file_size || end <= start)
    {
        return false;
    }

    range->start = start;
    range->end = end;
    range->partial = start != 0 || end != file_size;
    return true;
}

int prepare_file_response(const std::string& path, size_t encoded_start,
                          size_t encoded_end, HttpResp *resp,
                          FileMetadata *metadata, FileRange *range)
{
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) != 0 || !S_ISREG(file_stat.st_mode))
        return StatusNotFound;

    if (file_stat.st_size < 0 ||
        static_cast<uintmax_t>(file_stat.st_size) >
            static_cast<uintmax_t>(std::numeric_limits<size_t>::max()))
    {
        return StatusFileReadError;
    }

    metadata->size = static_cast<size_t>(file_stat.st_size);
    metadata->last_modified = file_stat.st_mtime;

    if (!normalize_file_range(metadata->size, encoded_start, encoded_end, range))
    {
        resp->headers["Content-Range"] =
            "bytes */" + std::to_string(metadata->size);
        return StatusFileRangeInvalid;
    }

    http_content_type content_type = CONTENT_TYPE_NONE;
    const std::string suffix = PathUtil::suffix(path);
    if (!suffix.empty())
        content_type = ContentType::to_enum_by_suffix(suffix);
    if (content_type == CONTENT_TYPE_NONE || content_type == CONTENT_TYPE_UNDEFINED)
        content_type = APPLICATION_OCTET_STREAM;
    resp->headers["Content-Type"] = ContentType::to_str(content_type);

    resp->headers.erase("Content-Range");
    if (range->partial)
    {
        resp->set_status(206);
        resp->headers["Content-Range"] =
            "bytes " + std::to_string(range->start) + "-" +
            std::to_string(range->end - 1) + "/" +
            std::to_string(metadata->size);
    }

    return StatusOK;
}

void clear_file_response_metadata(HttpResp *resp)
{
    resp->headers.erase("Content-Range");
    resp->headers.erase("Content-Type");
}

/*
We do not occupy any thread to read the file, but generate an asynchronous file reading task
and reply to the request after the reading is completed.

We need to read the whole data into the memory before we start replying to the message.
Therefore, it is not suitable for transferring files that are too large.

todo : Any better way to transfer large File?
*/
void pread_callback(WFFileIOTask *pread_task)
{
    FileIOArgs *args = pread_task->get_args();
    long ret = pread_task->get_retval();
    auto *read_ctx = static_cast<ReadContext *>(pread_task->user_data);
    HttpResp *resp = read_ctx->resp;

    if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0 ||
        static_cast<uintmax_t>(ret) !=
            static_cast<uintmax_t>(read_ctx->expected_size))
    {
        clear_file_response_metadata(resp);
        resp->Error(StatusFileReadError);
    } else
    {
        resp->append_output_body_nocopy(args->buf, static_cast<size_t>(ret));
    }
}

void pwrite_callback(WFFileIOTask *pwrite_task)
{
    long ret = pwrite_task->get_retval();
    HttpServerTask *server_task = task_of(pwrite_task);
    HttpResp *resp = server_task->get_resp();
    auto *save_context = static_cast<SaveFileContext *>(pwrite_task->user_data);
    if(save_context->fileio_args_func)
    {
        save_context->fileio_args_func(pwrite_task->get_args());
    }
    if (pwrite_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->Error(StatusFileWriteError);
    } else
    {
        if(!save_context->notify_msg.empty())
        {
            resp->append_output_body_nocopy(save_context->notify_msg.c_str(), save_context->notify_msg.size());
        }
    }
}

// Callback for asynchronous file reading in cached mode
void pread_cache_callback(WFFileIOTask *pread_task)
{
    FileIOArgs *args = pread_task->get_args();
    long ret = pread_task->get_retval();
    auto *cache_ctx = static_cast<CacheContext *>(pread_task->user_data);
    HttpResp *resp = cache_ctx->resp;

    if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0 ||
        static_cast<uintmax_t>(ret) !=
            static_cast<uintmax_t>(cache_ctx->file_size))
    {
        clear_file_response_metadata(resp);
        resp->Error(StatusFileReadError);
        return;
    }

    std::string content(static_cast<char*>(args->buf), cache_ctx->file_size);
    FileCache::instance().add_file(cache_ctx->path, content,
                                   cache_ctx->last_modified);
    resp->String(content.substr(cache_ctx->start,
                                cache_ctx->end - cache_ctx->start));
}

}  // namespace


// note : [start, end)
int HttpFile::send_file(const std::string &path, size_t file_start, size_t file_end, HttpResp *resp)
{
    FileMetadata metadata;
    FileRange range;
    int ret = prepare_file_response(path, file_start, file_end, resp,
                                    &metadata, &range);
    if (ret != StatusOK)
        return ret;

    const size_t size = range.end - range.start;
    if (size == 0)
        return StatusOK;

    void *buf = malloc(size);
    if (!buf)
    {
        clear_file_response_metadata(resp);
        return StatusFileReadError;
    }

    HttpServerTask *server_task = task_of(resp);
    auto *read_ctx = new ReadContext{resp, size};
    server_task->add_callback([buf, read_ctx](HttpTask *) {
        free(buf);
        delete read_ctx;
    });

    WFFileIOTask *pread_task = WFTaskFactory::create_pread_task(path,
                                                                buf,
                                                                size,
                                                                static_cast<off_t>(range.start),
                                                                pread_callback);
    pread_task->user_data = read_ctx;
    **server_task << pread_task;
    return StatusOK;
}


void HttpFile::save_file(const std::string &dst_path, const std::string &content,
                        HttpResp *resp, const std::string &notify_msg,
                        const FileIOArgsFunc &func)
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_context = new SaveFileContext;
    save_context->content = content;    // copy
    save_context->notify_msg = notify_msg;  // copy
    if (func)
    {
        save_context->fileio_args_func = func;
    }
    WFFileIOTask *pwrite_task = WFTaskFactory::create_pwrite_task(dst_path,
                                                                  static_cast<const void *>(save_context->content.c_str()),
                                                                  save_context->content.size(),
                                                                  0,
                                                                  pwrite_callback);
    **server_task << pwrite_task;
    server_task->add_callback([save_context](HttpTask *) {
        delete save_context;
    });
    pwrite_task->user_data = save_context;
}

void HttpFile::save_file(const std::string &dst_path, std::string &&content,
                        HttpResp *resp, const std::string &notify_msg,
                        const FileIOArgsFunc &func)
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_context = new SaveFileContext;
    save_context->content = std::move(content);
    save_context->notify_msg = std::move(notify_msg);
    if (func)
    {
        save_context->fileio_args_func = func;
    }
    WFFileIOTask *pwrite_task = WFTaskFactory::create_pwrite_task(dst_path,
                                                                  static_cast<const void *>(save_context->content.c_str()),
                                                                  save_context->content.size(),
                                                                  0,
                                                                  pwrite_callback);
    **server_task << pwrite_task;
    server_task->add_callback([save_context](HttpTask *) {
        delete save_context;
    });
    pwrite_task->user_data = save_context;
}


void HttpFile::save_file(const std::string &dst_path, const std::string &content, HttpResp *resp)
{
    return save_file(dst_path, content, resp, "", nullptr);
}

void HttpFile::save_file(const std::string &dst_path, const std::string &content,
                                    HttpResp *resp, const std::string &notify_msg)
{
    return save_file(dst_path, content, resp, notify_msg, nullptr);
}

void HttpFile::save_file(const std::string &dst_path, const std::string &content,
                    HttpResp *resp, const FileIOArgsFunc &func)
{
    return save_file(dst_path, content, resp, "", func);
}

void HttpFile::save_file(const std::string &dst_path, std::string&& content, HttpResp *resp)
{
    return save_file(dst_path, std::move(content), resp, "", nullptr);
}

void HttpFile::save_file(const std::string &dst_path, std::string&& content,
                                    HttpResp *resp, const std::string &notify_msg)
{
    return save_file(dst_path, std::move(content), resp, notify_msg, nullptr);
}

void HttpFile::save_file(const std::string &dst_path, std::string &&content,
                    HttpResp *resp, const FileIOArgsFunc &func)
{
    return save_file(dst_path, std::move(content), resp, "", func);
}

int HttpFile::send_cached_file(const std::string &path, size_t file_start, size_t file_end, HttpResp *resp)
{
    FileCache& cache = FileCache::instance();
    if (!cache.is_enabled())
        return send_file(path, file_start, file_end, resp);

    FileMetadata metadata;
    FileRange range;
    int ret = prepare_file_response(path, file_start, file_end, resp,
                                    &metadata, &range);
    if (ret != StatusOK)
        return ret;

    if (metadata.size == 0)
    {
        cache.add_file(path, "", metadata.last_modified);
        resp->String(std::string());
        return StatusOK;
    }
    
    std::string file_content;
    if (cache.get_file(path, file_content, range.start, range.end))
    {
        resp->String(std::move(file_content));
        return StatusOK;
    }

    HttpServerTask *server_task = task_of(resp);
    void *buf = malloc(metadata.size);
    if (!buf)
    {
        clear_file_response_metadata(resp);
        return StatusFileReadError;
    }

    auto *cache_ctx = new CacheContext{resp, path, metadata.size,
                                       range.start, range.end,
                                       metadata.last_modified};
    server_task->add_callback([buf, cache_ctx](HttpTask *) {
        free(buf);
        delete cache_ctx;
    });

    WFFileIOTask *pread_task = WFTaskFactory::create_pread_task(path,
                                                                buf,
                                                                metadata.size,
                                                                0,
                                                                pread_cache_callback);
    pread_task->user_data = cache_ctx;
    **server_task << pread_task;
    return StatusOK;
}

}  // namespace wfrest
