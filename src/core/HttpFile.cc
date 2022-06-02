#include "workflow/WFTaskFactory.h"

#include <sys/stat.h>

#include "HttpFile.h"
#include "HttpMsg.h"
#include "PathUtil.h"
#include "HttpServerTask.h"
#include "FileUtil.h"
#include "ErrorCode.h"

using namespace wfrest;

namespace
{

struct SaveFileContext 
{
    std::string content;
    std::string notify_msg;
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

void pwrite_callback(WFFileIOTask *pwrite_task)
{
    long ret = pwrite_task->get_retval();
    HttpServerTask *server_task = task_of(pwrite_task);
    HttpResp *resp = server_task->get_resp();
    auto *save_context = static_cast<SaveFileContext *>(pwrite_task->user_data);

    if (pwrite_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->Error(StatusFileWriteError);
    } else
    {
        if(save_context->notify_msg.empty()) 
        {
            resp->append_output_body_nocopy("Save File success\n", 18);
        } else 
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
    int start = file_start;
    int end = file_end;
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
    pread_task->user_data = resp;  
    **server_task << pread_task;
    return StatusOK;
}

void HttpFile::save_file(const std::string &dst_path, const std::string &content, HttpResp *resp)
{
    HttpFile::save_file(dst_path, content, resp, "");
}

void HttpFile::save_file(const std::string &dst_path, const std::string &content,
                                        HttpResp *resp, const std::string &notify_msg) 
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_context = new SaveFileContext; 
    save_context->content = content;    // copy
    save_context->notify_msg = notify_msg;  // copy

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

void HttpFile::save_file(const std::string &dst_path, const std::string &content,
                                        HttpResp *resp, std::string &&notify_msg) 
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_context = new SaveFileContext; 
    save_context->content = content;    // copy
    save_context->notify_msg = std::move(notify_msg);  

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

void HttpFile::save_file(const std::string &dst_path, std::string &&content, HttpResp *resp)
{
    HttpFile::save_file(dst_path, std::move(content), resp, "");   
}

void HttpFile::save_file(const std::string &dst_path, std::string &&content, 
                                        HttpResp *resp, const std::string &notify_msg) 
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_context = new SaveFileContext; 
    save_context->content = std::move(content);  
    save_context->notify_msg = notify_msg;  // copy

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
                                        HttpResp *resp, std::string &&notify_msg) 
{
    HttpServerTask *server_task = task_of(resp);

    auto *save_context = new SaveFileContext; 
    save_context->content = std::move(content);  
    save_context->notify_msg = std::move(notify_msg); 

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
