//
// Created by Chanchan on 11/8/21.
//

#include <workflow/WFTaskFactory.h>

#include <sys/stat.h>

#include "HttpFile.h"
#include "HttpMsg.h"
#include "PathUtil.h"

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
            resp->set_status_code("503");
            resp->append_output_body_nocopy("<html>503 Internal Server Error.</html>\r\n", 41);
        } else
        {
            resp->append_output_body_nocopy(args->buf, ret);
        }
    }

    void pread_multi_callback(WFFileIOTask *pread_task)
    {
        FileIOArgs *args = pread_task->get_args();
        long ret = pread_task->get_retval();
        auto *resp = static_cast<HttpResp *>(series_of(pread_task)->get_context());
        auto* last = static_cast<int *>(pread_task->user_data);
        if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
        {
            resp->set_status_code("503");
            resp->append_output_body_nocopy("<html>503 Internal Server Error.</html>\r\n", 41);
        } else
        {
            resp->append_output_body_nocopy(args->buf, ret);
            resp->append_output_body_nocopy("\r\n", 2);
            if(!last)   // last one
            {
                std::string multi_part_end = "--";
                multi_part_end.append(MultiPartForm::default_boundary);
                multi_part_end.append("--");
                resp->append_output_body(multi_part_end);
                delete last;
            }
        }
    }

    void pwrite_callback(WFFileIOTask *pwrite_task)
    {
        long ret = pwrite_task->get_retval();
        auto *resp = static_cast<HttpResp *>(pwrite_task->user_data);

        if (pwrite_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
        {
            resp->set_status_code("503");
            resp->append_output_body_nocopy("<html>503 Internal Server Error.</html>\r\n", 41);
        } else
        {
            resp->set_status_code("200");
            resp->append_output_body_nocopy("<html>save 200 success.</html>\r\n", 32);
        }
    }

    std::string concat_path(std::string &root, const std::string &path)
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

    std::string build_multi_part_one(const std::string& path, int i)
    {
        std::string str;
        // todo : how to generate name ?
        std::string file_name = PathUtil::base(path);
        str.append("--");
        str.append(MultiPartForm::default_boundary);
        str.append("\r\n");
        str.append("Content-Disposition: form-data; name=");
        str.append("\"file");
        str.append(std::to_string(i));
        str.append("\"; filename= \"");
        str.append(file_name);
        str.append("\"");

        const char* suffix = strrchr(file_name.c_str(), '.');
        if (suffix) {
            std::string stype = ContentType::to_string_by_suffix(++suffix);
            if (!stype.empty()) {
                str.append("\r\n");
                str.append("Content-Type: ");
                str.append(stype);
            }
        }
        str.append("\r\n\r\n");
        return str;
    }

}  // namespace

void HttpFile::send_file(const std::string &path, int start, int end, HttpResp *resp)
{
    if (start < 0)
    {
        fprintf(stderr, "start parameter should not be negative\n");
        resp->append_output_body_nocopy("start parameter should not be negative\n", 39);
        return;
    }
    auto *server_task = resp->get_task();
    std::string file_path = concat_path(root_, path);
    fprintf(stderr, "file path : %s\n", file_path.c_str());

    if (end == -1)
    {
        struct stat st{};
        stat(file_path.c_str(), &st);
        end = st.st_size;
    }

    if(end < start)
    {
        fprintf(stderr, "File size should greater or equal than zero\n");
        resp->append_output_body_nocopy("File size should greater or equal than zero\n", 44);
        return;
    }
    size_t size = end - start;
    void *buf = malloc(size);
    server_task->add_callback([buf](HttpTask *server_task)
                              {
                                  free(buf);
                              });
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
    pread_task->user_data = resp;   /* pass resp pointer to pread task. */
    **server_task << pread_task;
}

void HttpFile::send_file_for_multi(const std::vector<std::string>& path_list, int i, HttpResp *resp)
{
    auto *server_task = resp->get_task();
    std::string file_path = concat_path(root_, path_list[i]);
    fprintf(stderr, "file path : %s\n", file_path.c_str());

    std::string part_one = build_multi_part_one(path_list[i], i);
    resp->append_output_body(part_one);

    struct stat st{};
    stat(file_path.c_str(), &st);
    size_t size = st.st_size;

    void *buf = malloc(size);
    server_task->add_callback([buf](HttpTask *server_task)
                              {
                                  free(buf);
                              });
    WFFileIOTask *pread_task = WFTaskFactory::create_pread_task(file_path,
                                                                buf,
                                                                size,
                                                                0,
                                                                pread_multi_callback);
    if(i == path_list.size() - 1)
    {
        int *last = new int;
        pread_task->user_data = last;
    }
    auto series = series_of(server_task);
    series->push_back(pread_task);
    if(i == 0) series->set_context(resp);
}

void HttpFile::mount(std::string &&root)
{
    if (root.front() != '.' and root.front() != '/')
    {
        root_ = "./" + root;
    } else if (root.front() != '.')
    {
        root_ = "." + root;
    } else
    {
        root_ = std::move(root);
    }
    if (root_.back() == '/') root_.pop_back();
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




