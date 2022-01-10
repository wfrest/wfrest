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
    static int send_file(const std::string &path, size_t start, size_t end, HttpResp *resp);

    static void send_file_for_multi(const std::vector<std::string> &path_list, int path_idx, HttpResp *resp);

    static void save_file(const std::string &dst_path, const std::string &content, HttpResp *resp);

    static void save_file(const std::string &dst_path, std::string&& content, HttpResp *resp);
};

}  // namespace wfrest

#endif // WFREST_HTTPFILE_H_
