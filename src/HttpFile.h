//
// Created by Chanchan on 11/8/21.
//

#ifndef _HTTPFILE_H_
#define _HTTPFILE_H_

#include <string>
#include <vector>
#include <workflow/HttpMessage.h>

namespace wfrest
{
    class HttpResp;

    class HttpFile
    {
    public:
        static HttpFile *get_instance()
        {
            static HttpFile kInstance;
            return &kInstance;
        }

        void send_file(const std::string &path, size_t start, size_t end, HttpResp *resp);
        void send_file_for_multi(const std::vector<std::string>& path_list, int path_idx, HttpResp *resp);

        void mount(std::string &&root);

        void save_file(const std::string &dst_path, const void* content, size_t size, HttpResp *resp);

    private:
        HttpFile() = default;
        ~HttpFile() = default;
    private:
        std::string root_ = ".";
    };

}  // namespace wfrest




#endif //_HTTPFILE_H_
