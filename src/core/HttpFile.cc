#include "workflow/WFTaskFactory.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <mutex>
#include <unordered_map>
#include <string>
#include <cstring>
#include <functional>
#include <openssl/evp.h>

#include "HttpFile.h"
#include "HttpMsg.h"
#include "PathUtil.h"
#include "HttpServerTask.h"
#include "FileUtil.h"
#include "ErrorCode.h"

namespace wfrest
{

namespace
{

// 文件缓存结构
struct FileCacheEntry {
    std::string content;     // 文件内容
    std::string etag;        // ETag值
    time_t last_modified;    // 最后修改时间
    size_t size;             // 文件大小
};

// 文件缓存
static std::unordered_map<std::string, FileCacheEntry> file_cache;
static std::mutex cache_mutex;

// 小文件阈值 (50KB)
const size_t SMALL_FILE_THRESHOLD = 50 * 1024;
// 中等文件阈值 (1MB)
const size_t MEDIUM_FILE_THRESHOLD = 1024 * 1024;

// 生成ETag (使用 EVP 接口代替已弃用的 MD5 函数)
std::string generate_etag(const std::string& path, time_t mtime, size_t size) {
    std::string data = path + std::to_string(mtime) + std::to_string(size);
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
    
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);
    EVP_DigestUpdate(mdctx, data.c_str(), data.size());
    EVP_DigestFinal_ex(mdctx, md, &md_len);
    EVP_MD_CTX_free(mdctx);
    
    char hex[33];
    for (unsigned int i = 0; i < md_len; i++) {
        sprintf(hex + i * 2, "%02x", md[i]);
    }
    
    return std::string("\"") + hex + "\"";
}

struct SaveFileContext
{
    std::string content;
    std::string notify_msg;
    HttpFile::FileIOArgsFunc fileio_args_func;
};

/*
We do not occupy any thread to read the file, but generate an asynchronous file reading task
and reply to the request after the reading is completed.

We need to read the whole data into the memory before we start replying to the message.
Therefore, it is not suitable for transferring files that are too large.

todo : Any better way to transfer large File?
*/
void pread_callback(WFFileIOTask *pread_task)
{
    const auto *args = pread_task->get_args();
    long ret = pread_task->get_retval();
    auto *resp = static_cast<HttpResp *>(pread_task->user_data);

    if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->Error(StatusFileReadError);
    } else
    {
        resp->append_output_body_nocopy(args->buf, ret);
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

}  // namespace

// note : [start, end)
int HttpFile::send_file(const std::string &path, size_t file_start, size_t file_end, HttpResp *resp)
{
    if(!PathUtil::is_file(path))
    {
        return StatusNotFound;
    }
    
    // 获取文件信息
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) != 0) {
        return StatusFileReadError;
    }
    
    size_t file_size = file_stat.st_size;
    time_t last_modified = file_stat.st_mtime;
    
    int start = file_start;
    int end = file_end;
    
    if (end == -1)
        end = file_size;
    if (start < 0)
        start = file_size + start;

    if (end <= start)
    {
        return StatusFileRangeInvalid;
    }

    // 设置Content-Type
    http_content_type content_type = CONTENT_TYPE_NONE;
    std::string suffix = PathUtil::suffix(path);
    if(!suffix.empty())
    {
        content_type = ContentType::to_enum_by_suffix(suffix);
    }
    if (content_type == CONTENT_TYPE_NONE || content_type == CONTENT_TYPE_UNDEFINED) {
        content_type = APPLICATION_OCTET_STREAM;
    }
    resp->headers["Content-Type"] = ContentType::to_str(content_type);

    // 设置缓存控制头
    char last_modified_str[128];
    struct tm *tm_info = gmtime(&last_modified);
    strftime(last_modified_str, sizeof(last_modified_str), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    
    resp->headers["Last-Modified"] = last_modified_str;
    
    // 生成ETag
    std::string etag = generate_etag(path, last_modified, file_size);
    resp->headers["ETag"] = etag;
    
    // 对于静态资源，设置缓存时间为1天
    resp->headers["Cache-Control"] = "public, max-age=86400";
    
    // 检查条件请求
    HttpServerTask *server_task = task_of(resp);
    HttpReq *req = server_task->get_req();
    
    const std::string &if_none_match = req->header("If-None-Match");
    const std::string &if_modified_since = req->header("If-Modified-Since");
    
    // 如果ETag匹配或文件未修改，返回304
    if ((!if_none_match.empty() && if_none_match == etag) || 
        (!if_modified_since.empty() && strcmp(if_modified_since.c_str(), last_modified_str) == 0)) {
        resp->set_status(304);
        return StatusOK;
    }

    size_t size = end - start;
    
    // 设置Content-Range头
    resp->headers["Content-Range"] = "bytes " + std::to_string(start)
                                          + "-" + std::to_string(end - 1)
                                          + "/" + std::to_string(file_size);
    
    // 优化策略1: 小文件使用缓存
    if (size <= SMALL_FILE_THRESHOLD) {
        // 检查缓存
        std::string cache_key = path + ":" + std::to_string(start) + "-" + std::to_string(end);
        bool use_cache = false;
        std::string cached_content;
        
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            auto it = file_cache.find(cache_key);
            if (it != file_cache.end() && it->second.last_modified == last_modified) {
                cached_content = it->second.content;
                use_cache = true;
            }
        }
        
        if (use_cache) {
            // 使用缓存内容
            resp->append_output_body(cached_content);
            return StatusOK;
        }
        
        // 同步读取小文件
        FILE *fp = fopen(path.c_str(), "rb");
        if (!fp) {
            return StatusFileReadError;
        }
        
        void *buf = malloc(size);
        if (!buf) {
            fclose(fp);
            return StatusFileReadError;
        }
        
        if (fseek(fp, start, SEEK_SET) != 0) {
            free(buf);
            fclose(fp);
            return StatusFileReadError;
        }
        
        size_t read_size = fread(buf, 1, size, fp);
        fclose(fp);
        
        if (read_size != size) {
            free(buf);
            return StatusFileReadError;
        }
        
        // 更新缓存
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            FileCacheEntry entry;
            entry.content = std::string(static_cast<char*>(buf), size);
            entry.etag = etag;
            entry.last_modified = last_modified;
            entry.size = size;
            file_cache[cache_key] = entry;
        }
        
        server_task->add_callback([buf](HttpTask *) {
            free(buf);
        });
        
        resp->append_output_body_nocopy(buf, size);
        return StatusOK;
    }
    
    // 优化策略2: 中等文件使用直接读取
    if (size <= MEDIUM_FILE_THRESHOLD) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) {
            return StatusFileReadError;
        }
        
        void *buf = malloc(size);
        if (!buf) {
            close(fd);
            return StatusFileReadError;
        }
        
        // 使用 pread 直接读取文件
        ssize_t bytes_read = pread(fd, buf, size, start);
        close(fd);
        
        if (bytes_read != (ssize_t)size) {
            free(buf);
            return StatusFileReadError;
        }
        
        server_task->add_callback([buf](HttpTask *) {
            free(buf);
        });
        
        resp->append_output_body_nocopy(buf, size);
        return StatusOK;
    }
    
    // 优化策略3: 大文件仍使用异步方式
    void *buf = malloc(size);

    server_task->add_callback([buf](HttpTask *) {
        free(buf);
    });

    WFFileIOTask *pread_task = WFTaskFactory::create_pread_task(path,
                                                               buf,
                                                               size,
                                                               static_cast<off_t>(start),
                                                               pread_callback);
    pread_task->user_data = resp;
    **server_task << pread_task;
    return StatusOK;
}

// 清除文件缓存
void HttpFile::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    file_cache.clear();
}

// 预加载文件到缓存
void HttpFile::preload_file(const std::string &path) {
    if (!PathUtil::is_file(path)) {
        return;
    }
    
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) != 0) {
        return;
    }
    
    size_t file_size = file_stat.st_size;
    if (file_size > SMALL_FILE_THRESHOLD) {
        return; // 只缓存小文件
    }
    
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) {
        return;
    }
    
    char *buf = new char[file_size];
    size_t read_size = fread(buf, 1, file_size, fp);
    fclose(fp);
    
    if (read_size == file_size) {
        std::string cache_key = path + ":0-" + std::to_string(file_size);
        
        std::lock_guard<std::mutex> lock(cache_mutex);
        FileCacheEntry entry;
        entry.content = std::string(buf, file_size);
        entry.etag = generate_etag(path, file_stat.st_mtime, file_size);
        entry.last_modified = file_stat.st_mtime;
        entry.size = file_size;
        file_cache[cache_key] = entry;
    }
    
    delete[] buf;
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

}  // namespace wfrest
