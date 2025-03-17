#ifndef WFREST_HTTPFILE_H_
#define WFREST_HTTPFILE_H_

#include <string>
#include <vector>

namespace wfrest
{
class HttpResp;

class HttpFile
{
public:
    using FileIOArgsFunc = std::function<void(const struct FileIOArgs*)>;

public:
    static int send_file(const std::string &path, size_t start, size_t end, HttpResp *resp);

    static int send_cached_file(const std::string &path, size_t start, size_t end, HttpResp *resp);

    static void save_file(const std::string &dst_path, const std::string &content, HttpResp *resp);

    static void save_file(const std::string &dst_path, const std::string &content, 
                                            HttpResp *resp, const std::string &notify_msg);

    static void save_file(const std::string &dst_path, const std::string &content, 
                          HttpResp *resp, const FileIOArgsFunc &func);

    static void save_file(const std::string &dst_path, std::string&& content, HttpResp *resp);

    static void save_file(const std::string &dst_path, std::string&& content, 
                                            HttpResp *resp, const std::string &notify_msg);

    static void save_file(const std::string &dst_path, std::string &&content, 
                          HttpResp *resp, const FileIOArgsFunc &func);

private:
    static void save_file(const std::string &dst_path, const std::string &content, 
                          HttpResp *resp, const std::string &notify_msg, 
                          const FileIOArgsFunc &func);

    static void save_file(const std::string &dst_path, std::string &&content, 
                          HttpResp *resp, const std::string &notify_msg, 
                          const FileIOArgsFunc &func);
};

}  // namespace wfrest

#endif // WFREST_HTTPFILE_H_
