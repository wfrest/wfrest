#ifndef WFREST_HTTPCONTENT_H_
#define WFREST_HTTPCONTENT_H_

#include <string>
#include <map>
#include <vector>
#include "MultiPartParser.h"
#include "Noncopyable.h"

namespace wfrest
{

class StringPiece;

class Urlencode
{
public:
    static std::map<std::string, std::string> parse_post_kv(const StringPiece &body);
};

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods/POST
// <name ,<filename, body>>
using Form = std::map<std::string, std::pair<std::string, std::string>>;

// Modified From libhv
class MultiPartForm 
{
public:
    Form parse_multipart(const StringPiece &body) const;

    void set_boundary(std::string &&boundary)
    { boundary_ = std::move(boundary); }

    void set_boundary(const std::string &boundary)
    { boundary_ = boundary; }

public:
    static const std::string k_default_boundary;

    MultiPartForm();
private:
    static int header_field_cb(multipart_parser *parser, const char *buf, size_t len);

    static int header_value_cb(multipart_parser *parser, const char *buf, size_t len);

    static int part_data_cb(multipart_parser *parser, const char *buf, size_t len);

    static int part_data_begin_cb(multipart_parser *parser);

    static int headers_complete_cb(multipart_parser *parser);

    static int part_data_end_cb(multipart_parser *parser);

    static int body_end_cb(multipart_parser *parser);

private:
    std::string boundary_;

    multipart_parser_settings settings_;
};

class MultiPartEncoder 
{
public:
    using ParamList = std::vector<std::pair<std::string, std::string>>;
    using FileList = std::vector<std::pair<std::string, std::string>>;

    MultiPartEncoder();

    ~MultiPartEncoder() = default;

    void add_param(const std::string &name, const std::string &value);

    void add_file(const std::string &file_name, const std::string &file_path);

    const ParamList &params() const { return params_; }

    const FileList &files() const { return files_; }

    const std::string &boundary() const { return boundary_; }

    void set_boundary(const std::string &boundary);

    void set_boundary(std::string &&boundary);

private:
    std::string boundary_;
    std::string content_;
    std::vector<std::pair<std::string, std::string>> params_;
    std::vector<std::pair<std::string, std::string>> files_;
};

}  // wfrest

#endif // WFREST_HTTPCONTENT_H_
