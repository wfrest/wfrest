#ifndef WFREST_HTTPFILE_H_
#define WFREST_HTTPFILE_H_

#include <string>
#include <vector>
#include "workflow/HttpMessage.h"

namespace wfrest
{
class HttpResp;

class HttpFile
{
public:
    static void send_file(const std::string &path, size_t start, size_t end, HttpResp *resp);

    static void send_file_for_multi(const std::vector<std::string> &path_list, int path_idx, HttpResp *resp);

    static void save_file(const std::string &dst_path, const std::string &content, HttpResp *resp);

    static void save_file(const std::string &dst_path, std::string&& content, HttpResp *resp);

public: 
    static void mount(const char *root_path);

    static void add_static_map(const char *relative_path, const char *root);

    static void remove_static_map(const char *relative_path);

    static const std::map<std::string, std::string>& static_map() { return static_file_map; }
private:
    static std::string root;

    static std::map<std::string, std::string> static_file_map;
};

}  // namespace wfrest

#endif // WFREST_HTTPFILE_H_
