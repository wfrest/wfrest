#include "workflow/WFTaskFactory.h"

#include <sys/stat.h>

#include "wfrest/HttpFile.h"
#include "wfrest/HttpMsg.h"
#include "wfrest/PathUtil.h"
#include "wfrest/HttpServerTask.h"
#include "wfrest/FileUtil.h"
#include "wfrest/StatusCode.h"

using namespace wfrest;

namespace
{
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
    auto *resp = static_cast<HttpResp *>(pread_task->user_data);

    if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->Error(StatusFileReadError);
    } else
    {
        resp->append_output_body_nocopy(args->buf, ret);
    }
}

struct pread_multi_context
{
    std::string file_name;
    int file_index;
    bool last = false;
    HttpResp *resp;
    std::string multipart_start;
    std::string multipart_end;
};

std::string build_multipart_start(const std::string &file_name, int idx)
{
    std::string str;
    str.reserve(256);  // reserve space avoid copy
    // todo : how to generate name ?
    str.append("--");
    str.append(MultiPartForm::k_default_boundary);
    str.append("\r\n");
    str.append("Content-Disposition: form-data; name=");
    str.append("\"file");
    str.append(std::to_string(idx));
    str.append("\"; filename= \"");
    str.append(file_name);
    str.append("\"");

    const char *suffix = strrchr(file_name.c_str(), '.');
    if (suffix)
    {
        std::string stype = ContentType::to_str_by_suffix(++suffix);
        if (!stype.empty())
        {
            str.append("\r\n");
            str.append("Content-Type: ");
            str.append(stype);
        }
    }
    str.append("\r\n\r\n");
    return str;
}

std::string build_multipart_end()
{
    std::string multi_part_last;
    multi_part_last.reserve(128);
    multi_part_last.append("--");
    // RFC1521 says that a boundary "must be no longer than 70 characters, not counting the two leading hyphens".
    multi_part_last.append(MultiPartForm::k_default_boundary);
    multi_part_last.append("--");
    return multi_part_last;
}

void pread_multi_callback(WFFileIOTask *pread_task)
{
    FileIOArgs *args = pread_task->get_args();
    long ret = pread_task->get_retval();
    auto *ctx = static_cast<pread_multi_context *>(pread_task->user_data);
    HttpResp *resp = ctx->resp;

    std::string multipart_start = build_multipart_start(ctx->file_name, ctx->file_index);
    ctx->multipart_start = std::move(multipart_start);
    resp->append_output_body_nocopy(ctx->multipart_start.c_str(), ctx->multipart_start.size());

    if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->set_status_code("503");
        resp->append_output_body_nocopy("503 Internal Server Error\n", 26);
    } else
    {
        resp->append_output_body_nocopy(args->buf, ret);
    }

    if (ctx->last)   // last one
    {
        std::string multipart_end = build_multipart_end();
        ctx->multipart_end = std::move(multipart_end);
        resp->append_output_body_nocopy(ctx->multipart_end.c_str(), ctx->multipart_end.size());
    }
}

void pwrite_callback(WFFileIOTask *pwrite_task)
{
    long ret = pwrite_task->get_retval();
    HttpServerTask *server_task = task_of(pwrite_task);
    HttpResp *resp = server_task->get_resp();
    delete static_cast<std::string *>(pwrite_task->user_data);

    if (pwrite_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->set_status_code("503");
        resp->append_output_body_nocopy("503 Internal Server Error\n", 26);
    } else
    {
        resp->append_output_body_nocopy("Save 200 success\n", 17);
    }
}

}  // namespace

// note : [start, end)
int HttpFile::send_file(const std::string &path, size_t file_start, size_t file_end, HttpResp *resp)
{
    int start = file_start;
    int end = file_end;
    if(!PathUtil::is_file(path))
    {
        return StatusNotFile;
    }
    if (end == -1 || start < 0)
    {
        size_t file_size;
        int ret = FileUtil::size(path, OUT &file_size);

        if (ret != StatusOK)
        {
            return ret;
        }
        if (end == -1) end = file_size;
        if (start < 0) start = file_size + start;
    }
    fprintf(stderr, "start = %zu, end = %zu\n", start, end);
    if (end <= start)
    {
        return StatusFileRangeInvalid;
    }

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

    size_t size = end - start;
    void *buf = malloc(size);

    HttpServerTask *server_task = task_of(resp);
    server_task->add_callback([buf](HttpTask *server_task)
                              {
                                  free(buf);
                              });
    // https://datatracker.ietf.org/doc/html/rfc7233#section-4.2
    // Content-Range: bytes 42-1233/1234
    resp->headers["Content-Range"] = "bytes " + std::to_string(start)
                                            + "-" + std::to_string(end)
                                            + "/" + std::to_string(size);
    

    WFFileIOTask *pread_task = WFTaskFactory::create_pread_task(path,
                                                                buf,
                                                                size,
                                                                static_cast<off_t>(start),
                                                                pread_callback);
    pread_task->user_data = resp;   /* pass resp pointer to pread task. */
    **server_task << pread_task;
    return StatusOK;
}

void HttpFile::send_file_for_multi(const std::vector<std::string> &path_list, int path_idx, HttpResp *resp)
{
    HttpServerTask *server_task = task_of(resp);
    const std::string &file_path = path_list[path_idx];


    auto *ctx = new pread_multi_context;
    ctx->file_name = PathUtil::base(file_path);
    ctx->file_index = path_idx;
    ctx->resp = resp;

    if (path_idx == path_list.size() - 1)
    {
        // last one
        ctx->last = true;
    }

    size_t size;
    int ret = FileUtil::size(file_path, OUT &size);
    if (ret == -1)
    {
        resp->set_status(404);
        resp->append_output_body_nocopy("404 File NOT FOUND\n", 19);
        return;
    }
    void *buf = malloc(size);
    server_task->add_callback([buf, ctx](HttpTask *server_task)
                                {
                                    free(buf);
                                    delete ctx;
                                });
    WFFileIOTask *pread_task = WFTaskFactory::create_pread_task(file_path,
                                                                buf,
                                                                size,
                                                                0,
                                                                pread_multi_callback);
    pread_task->user_data = ctx;
    **server_task << pread_task;
}

void HttpFile::save_file(const std::string &dst_path, const std::string &content, HttpResp *resp)
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_content = new std::string;
    *save_content = content;

    WFFileIOTask *pwrite_task = WFTaskFactory::create_pwrite_task(dst_path,
                                                                  static_cast<const void *>(save_content->c_str()),
                                                                  save_content->size(),
                                                                  0,
                                                                  pwrite_callback);
    **server_task << pwrite_task;
    pwrite_task->user_data = save_content;
}

void HttpFile::save_file(const std::string &dst_path, std::string &&content, HttpResp *resp)
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_content = new std::string;
    *save_content = std::move(content);

    WFFileIOTask *pwrite_task = WFTaskFactory::create_pwrite_task(dst_path,
                                                                  static_cast<const void *>(save_content->c_str()),
                                                                  save_content->size(),
                                                                  0,
                                                                  pwrite_callback);
    **server_task << pwrite_task;
    pwrite_task->user_data = save_content;
}




