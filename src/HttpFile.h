//
// Created by Chanchan on 11/8/21.
//

#ifndef _HTTPFILE_H_
#define _HTTPFILE_H_

#include <string>
#include <workflow/HttpMessage.h>

namespace wfrest
{
    class HttpFile
    {
    public:
        explicit HttpFile(protocol::HttpMessage *msg) : msg_(msg) {}

        void send_file(const std::string &path, size_t start, size_t end);
        void mount(std::string &&root);

        void save_file(const std::string &dst_path, const void* content, size_t size);
    private:
        std::string root_ = ".";
        protocol::HttpMessage *msg_;
    };

}  // namespace wfrest




#endif //_HTTPFILE_H_
