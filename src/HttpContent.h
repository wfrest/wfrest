//
// Created by Chanchan on 11/5/21.
//

#ifndef _HTTPCONTENT_H_
#define _HTTPCONTENT_H_

#include <string>
#include <unordered_map>
#include "MultiPartParser.h"
#include "Macro.h"
#include "StringPiece.h"

namespace wfrest
{
    class Urlencode
    {
    public:
        using KV = std::unordered_map<std::string, std::string>;

        static void parse_query_params(const StringPiece &body, OUT KV &res);

    };

    struct FormData
    {
        std::string filename;
        std::string content;
    };

    // Taken From
    // libhv : https://github.com/ithewei/libhv
    class MultiPartForm
    {
    public:
        using MultiPart = std::unordered_map<std::string, FormData>;

        MultiPartForm();

        int parse_multipart(const StringPiece &body, OUT MultiPart &form);

        void set_boundary(std::string &&boundary)
        { boundary_ = std::move(boundary); }

        void set_boundary(const std::string &boundary)
        { boundary_= boundary; }

    public:
        static const std::string default_boundary;
    private:
        static int header_field_cb(multipart_parser *parser, const char *buf, size_t len);

        static int header_value_cb(multipart_parser *parser, const char *buf, size_t len);

        static int part_data_cb(multipart_parser *parser, const char *buf, size_t len);

        static int part_data_begin_cb(multipart_parser *parser);

        static int headers_complete_cb(multipart_parser *parser);

        static int part_data_end_cb(multipart_parser *parser);

        static int body_end_cb(multipart_parser *parser);
    private:
        multipart_parser *parser_{};
        std::string boundary_;

        multipart_parser_settings settings_{};
    };
}  // wfrest

#endif //_HTTPCONTENT_H_
