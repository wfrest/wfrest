#ifndef WFREST_HTTPCONTENT_H_
#define WFREST_HTTPCONTENT_H_

#include <string>
#include <map>
#include "wfrest/MultiPartParser.h"
#include "wfrest/Macro.h"

namespace wfrest
{

class StringPiece;

class Urlencode
{
public:
    static std::map<std::string, std::string> parse_post_kv(const StringPiece &body);
};

// struct FormData
// {
//     std::string filename;
//     std::string body;

//     bool is_file() const
//     { return !filename.empty(); }
// };

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST
// <name ,<filename, body>>
using Form = std::map<std::string, std::pair<std::string, std::string>>;

// Modified From libhv
class MultiPartForm
{
public:
    MultiPartForm();

    Form parse_multipart(const StringPiece &body) const;

    void set_boundary(std::string &&boundary)
    { boundary_ = std::move(boundary); }

    void set_boundary(const std::string &boundary)
    { boundary_ = boundary; }

public:
    static const std::string k_default_boundary;
private:
    static int header_field_cb(multipart_parser *parser, const char *buf, size_t len);

    static int header_value_cb(multipart_parser *parser, const char *buf, size_t len);

    static int part_data_cb(multipart_parser *parser, const char *buf, size_t len);

    static int part_data_begin_cb(multipart_parser *parser);

    static int headers_complete_cb(multipart_parser *parser);

    static int part_data_end_cb(multipart_parser *parser);

    static int body_end_cb(multipart_parser *parser);

private:
    multipart_parser *parser_;
    std::string boundary_;

    multipart_parser_settings settings_;
};



}  // wfrest

#endif // WFREST_HTTPCONTENT_H_
