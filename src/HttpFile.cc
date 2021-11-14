//
// Created by Chanchan on 11/8/21.
//

#include "HttpFile.h"
#include <sys/stat.h>
#include "HttpMsg.h"
#include <workflow/WFTaskFactory.h>

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

        // todo : give this process to user
        if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
        {
            resp->set_status_code("503");
            resp->append_output_body("<html>503 Internal Server Error.</html>");
        } else
        {
            resp->append_output_body_nocopy(args->buf, ret);
        }
    }

    void pwrite_callback(WFFileIOTask *pwrite_task)
    {
        long ret = pwrite_task->get_retval();
        auto *resp = static_cast<HttpResp *>(pwrite_task->user_data);

        if (pwrite_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
        {
            resp->set_status_code("503");
            resp->append_output_body("<html>503 Internal Server Error.</html>\r\n");
        }
        else
        {
            resp->set_status_code("200");
            resp->append_output_body("<html>save 200 success.</html>\r\n");
        }
    }

    std::string concat_path(std::string& root, const std::string& path)
    {
        std::string res;

        if (path.front() != '/')
        {
            res = root + "/" + path;
        } else
        {
            res = root + path;
        }
        return res;
    }

}  // namespace

void HttpFile::send_file(const std::string &path, size_t start, size_t end, HttpResp *resp)
{
    assert(resp);
    auto *server_task = resp->get_task();
    assert(server_task);

    std::string file_path = concat_path(root_, path);

    fprintf(stderr, "file path : %s\n", file_path.c_str());

    if (end == 0)
    {
        struct stat st{};
        stat(file_path.c_str(), &st);
        end = st.st_size;
    }
    size_t size = end - start;
    void *buf = malloc(size);
    // https://datatracker.ietf.org/doc/html/rfc7233#section-4.2
    // Content-Range: bytes 42-1233/1234
    resp->add_header_pair("Content-Range",
                          "bytes " + std::to_string(start)
                          + "-" + std::to_string(end)
                          + "/" + std::to_string(size));

    WFFileIOTask *pread_task = WFTaskFactory::create_pread_task(file_path,
                                                                buf,
                                                                size,
                                                                static_cast<off_t>(start),
                                                                pread_callback);
    server_task->user_data = buf; /* to free() in callback() */
    pread_task->user_data = resp;   /* pass resp pointer to pread task. */
    server_task->add_callback([](HttpTask *server_task)
                              {
                                  free(server_task->user_data);
                              });
    **server_task << pread_task;
}

void HttpFile::mount(std::string &&root)
{
    if(root.front() != '.' and root.front() != '/')
    {
        root_ = "./" + root;
    } else if(root.front() != '.')
    {
        root_ = "." + root;
    } else
    {
        root_ = std::move(root);
    }
    if(root_.back() == '/') root_.pop_back();
    // ./xxx/xx
}

void HttpFile::save_file(const std::string &dst_path, const void *content, size_t size, HttpResp *resp)
{
    auto *server_task = resp->get_task();

    std::string file_path = concat_path(root_, dst_path);

    // fprintf(stderr, "content :: %s to %s\n", static_cast<const char *>(content), file_path.c_str());

    WFFileIOTask *pwrite_task = WFTaskFactory::create_pwrite_task(file_path,
                                                                  content,
                                                                  size,
                                                                  0,
                                                                  pwrite_callback);
    pwrite_task->user_data = resp;
    **server_task << pwrite_task;
}


