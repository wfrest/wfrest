#include "workflow/WFTaskFactory.h"

#include <sys/stat.h>

#include "HttpFile.h"
#include "HttpMsg.h"
#include "PathUtil.h"
#include "HttpServerTask.h"

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

    // clear output body
    if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->set_status_code("503");
        resp->append_output_body_nocopy("<html>503 Internal Server Error.</html>\r\n", 41);
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
};

std::string build_multi_part_one(const std::string &file_name, int idx)
{
    std::string str;
    str.reserve(256);  // reserve space avoid copy
    // todo : how to generate name ?
    str.append("--");
    str.append(MultiPartForm::default_boundary);
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
        std::string stype = ContentType::to_string_by_suffix(++suffix);
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

void pread_multi_callback(WFFileIOTask *pread_task)
{
    FileIOArgs *args = pread_task->get_args();
    long ret = pread_task->get_retval();
    auto *ctx = static_cast<pread_multi_context *>(pread_task->user_data);
    auto *resp = static_cast<HttpResp *>(series_of(pread_task)->get_context());

    std::string part_one = build_multi_part_one(ctx->file_name, ctx->file_index);
    resp->append_output_body(part_one);

    if (pread_task->get_state() != WFT_STATE_SUCCESS || ret < 0)
    {
        resp->set_status_code("503");
        resp->append_output_body_nocopy("<html>503 Internal Server Error.</html>\r\n", 41);
    } else
    {
        resp->append_output_body_nocopy(args->buf, ret);
        resp->append_output_body_nocopy("\r\n", 2);
    }
    if (ctx->last)   // last one
    {
        std::string multi_part_end;
        multi_part_end.reserve(128);
        multi_part_end.append("--");
        // RFC1521 says that a boundary "must be no longer than 70 characters, not counting the two leading hyphens".
        multi_part_end.append(MultiPartForm::default_boundary);
        multi_part_end.append("--");
        resp->append_output_body(multi_part_end);
    }
    delete ctx;
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


}  // namespace

// static member init
std::string HttpFile::root = ".";

void HttpFile::send_file(const std::string &path, size_t start, size_t end, HttpResp *resp)
{
    auto *server_task = resp->get_task();
    std::string file_path = concat_path(root, path);
    fprintf(stderr, "file path : %s\n", file_path.c_str());

    if (end == -1 || start < 0)
    {
        struct stat st;

        int ret = stat(file_path.c_str(), &st);
        if (ret == -1)
        {
            fprintf(stderr, "File has something wrong\n");
            resp->append_output_body_nocopy("File has something wrong", 24);
            return;
        }
        size_t file_size = st.st_size;
        if (end == -1) end = file_size;
        if (start < 0) start = file_size + start;
    }

    if (end <= start)
    {
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

void HttpFile::send_file_for_multi(const std::vector<std::string> &path_list, int path_idx, HttpResp *resp)
{
    auto *server_task = resp->get_task();
    std::string file_path = concat_path(root, path_list[path_idx]);
    fprintf(stderr, "file path : %s\n", file_path.c_str());

    auto *ctx = new pread_multi_context;
    ctx->file_name = PathUtil::base(path_list[path_idx]);
    ctx->file_index = path_idx;
    if (path_idx == path_list.size() - 1)
    {
        // last one
        ctx->last = true;
    }

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
    pread_task->user_data = ctx;
    auto series = series_of(server_task);
    series->push_back(pread_task);
    if (path_idx == 0) series->set_context(resp);
}

void HttpFile::mount(std::string &&root)
{
    if (root.front() != '.' and root.front() != '/')
    {
        root = "./" + root;
    } else if (root.front() != '.')
    {
        root = "." + root;
    } else
    {
        root = std::move(root);
    }
    if (root.back() == '/') root.pop_back();
    // ./xxx/xx
}

void HttpFile::save_file(const std::string &dst_path, const void *content, size_t size, HttpResp *resp)
{
    auto *server_task = resp->get_task();

    std::string file_path = concat_path(root, dst_path);

    // fprintf(stderr, "content :: %s to %s\n", static_cast<const char *>(content), file_path.c_str());

    WFFileIOTask *pwrite_task = WFTaskFactory::create_pwrite_task(file_path,
                                                                  content,
                                                                  size,
                                                                  0,
                                                                  pwrite_callback);
    pwrite_task->user_data = resp;
    **server_task << pwrite_task;

}




