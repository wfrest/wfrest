#include <strings.h>
#include <utility>
#include "HttpContent.h"
#include "MultipartUtil.h"
#include "StringPiece.h"
#include "UriUtil.h"

using namespace wfrest;

const std::string MultiPartForm::k_default_boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

namespace
{

bool is_ows(char ch)
{
    return ch == ' ' || ch == '\t';
}

bool is_token_char(unsigned char ch)
{
    if ((ch >= '0' && ch <= '9') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= 'a' && ch <= 'z'))
    {
        return true;
    }

    switch (ch)
    {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '*':
        case '+':
        case '-':
        case '.':
        case '^':
        case '_':
        case '`':
        case '|':
        case '~':
            return true;
        default:
            return false;
    }
}

unsigned char ascii_lower(unsigned char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return static_cast<unsigned char>(ch + ('a' - 'A'));
    return ch;
}

bool ascii_iequals(const std::string &value, const char *expected)
{
    size_t i = 0;
    while (i < value.size() && expected[i] != '\0')
    {
        if (ascii_lower(static_cast<unsigned char>(value[i])) !=
            ascii_lower(static_cast<unsigned char>(expected[i])))
        {
            return false;
        }
        ++i;
    }
    return i == value.size() && expected[i] == '\0';
}

void skip_ows(const std::string &value, size_t *cursor)
{
    while (*cursor < value.size() && is_ows(value[*cursor]))
        ++(*cursor);
}

bool read_token(const std::string &input, size_t *cursor, std::string *value)
{
    const size_t begin = *cursor;
    while (*cursor < input.size() &&
           is_token_char(static_cast<unsigned char>(input[*cursor])))
    {
        ++(*cursor);
    }
    if (*cursor == begin)
        return false;

    value->assign(input.data() + begin, *cursor - begin);
    return true;
}

bool is_quoted_text(unsigned char ch)
{
    return ch == '\t' || ch == ' ' || ch == '!' ||
           (ch >= '#' && ch <= '[') ||
           (ch >= ']' && ch <= '~') || ch >= 0x80;
}

bool is_quoted_pair_char(unsigned char ch)
{
    return ch == '\t' || (ch >= ' ' && ch != 0x7f);
}

bool read_quoted_value(const std::string &input,
                       size_t *cursor,
                       std::string *value)
{
    if (*cursor >= input.size() || input[*cursor] != '"')
        return false;

    ++*cursor;
    value->clear();
    while (*cursor < input.size())
    {
        const unsigned char ch =
            static_cast<unsigned char>(input[(*cursor)++]);
        if (ch == '"')
            return true;

        if (ch == '\\')
        {
            if (*cursor >= input.size())
                return false;
            const unsigned char escaped =
                static_cast<unsigned char>(input[(*cursor)++]);
            if (!is_quoted_pair_char(escaped))
                return false;
            value->push_back(static_cast<char>(escaped));
        }
        else
        {
            if (!is_quoted_text(ch))
                return false;
            value->push_back(static_cast<char>(ch));
        }
    }
    return false;
}

bool read_parameter_value(const std::string &input,
                          size_t *cursor,
                          std::string *value)
{
    if (*cursor < input.size() && input[*cursor] == '"')
        return read_quoted_value(input, cursor, value);
    return read_token(input, cursor, value);
}

bool parse_content_disposition(const std::string &input,
                               std::string *name,
                               std::string *filename)
{
    size_t cursor = 0;
    skip_ows(input, &cursor);

    std::string disposition;
    if (!read_token(input, &cursor, &disposition) ||
        !ascii_iequals(disposition, "form-data"))
    {
        return false;
    }

    bool has_name = false;
    bool has_filename = false;
    std::string parsed_name;
    std::string parsed_filename;
    skip_ows(input, &cursor);
    while (cursor < input.size())
    {
        if (input[cursor] != ';')
            return false;
        ++cursor;
        skip_ows(input, &cursor);

        std::string key;
        if (!read_token(input, &cursor, &key))
            return false;
        skip_ows(input, &cursor);
        if (cursor >= input.size() || input[cursor] != '=')
            return false;
        ++cursor;
        skip_ows(input, &cursor);

        std::string value;
        if (!read_parameter_value(input, &cursor, &value))
            return false;
        skip_ows(input, &cursor);
        if (cursor < input.size() && input[cursor] != ';')
            return false;

        if (ascii_iequals(key, "name"))
        {
            if (has_name)
                return false;
            has_name = true;
            parsed_name = std::move(value);
        }
        else if (ascii_iequals(key, "filename"))
        {
            if (has_filename)
                return false;
            has_filename = true;
            parsed_filename = std::move(value);
        }
    }

    if (!has_name || parsed_name.empty())
        return false;

    *name = std::move(parsed_name);
    *filename = std::move(parsed_filename);
    return true;
}

} // namespace

std::map<std::string, std::string> Urlencode::parse_post_kv(const StringPiece &body)
{
    return UriUtil::split_query(body);
}

enum multipart_parser_state_e
{
    MP_START,
    MP_PART_DATA_BEGIN,
    MP_HEADER_FIELD,
    MP_HEADER_VALUE,
    MP_HEADERS_COMPLETE,
    MP_PART_DATA,
    MP_PART_DATA_END,
    MP_BODY_END
};

struct multipart_parser_userdata
{
    Form *mp;
    multipart_parser_state_e state;
    std::string header_field;
    std::string header_value;
    std::string part_data;
    std::string name;
    std::string filename;
    bool disposition_seen;
    bool disposition_valid;

    void handle_header();

    void handle_data();

    void reset_part();
};

void multipart_parser_userdata::handle_header()
{
    if (!header_field.empty() &&
        strcasecmp(header_field.c_str(), "Content-Disposition") == 0)
    {
        if (disposition_seen)
        {
            disposition_valid = false;
            name.clear();
            filename.clear();
        }
        else
        {
            disposition_seen = true;
            disposition_valid = parse_content_disposition(
                header_value, &name, &filename);
        }
    }
    header_field.clear();
    header_value.clear();
}

void multipart_parser_userdata::handle_data()
{
    if (disposition_seen && disposition_valid)
    {
        std::pair<std::string, std::string> formdata;
        formdata.first = filename;
        formdata.second = part_data;
        (*mp)[name] = formdata;
    }
    reset_part();
}

void multipart_parser_userdata::reset_part()
{
    header_field.clear();
    header_value.clear();
    name.clear();
    filename.clear();
    part_data.clear();
    disposition_seen = false;
    disposition_valid = false;
}

MultiPartForm::MultiPartForm()
{
    settings_.on_header_field = header_field_cb;
    settings_.on_header_value = header_value_cb;
    settings_.on_part_data = part_data_cb;
    settings_.on_part_data_begin = part_data_begin_cb;
    settings_.on_headers_complete = headers_complete_cb;
    settings_.on_part_data_end = part_data_end_cb;
    settings_.on_body_end = body_end_cb;
}

void MultiPartForm::set_boundary(const std::string &boundary)
{
    if (detail::is_valid_multipart_boundary(boundary))
        boundary_ = boundary;
    else
        boundary_.clear();
}

void MultiPartForm::set_boundary(std::string &&boundary)
{
    if (detail::is_valid_multipart_boundary(boundary))
        boundary_ = std::move(boundary);
    else
        boundary_.clear();
}

int MultiPartForm::header_field_cb(multipart_parser *parser, const char *buf, size_t len)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->handle_header();
    userdata->state = MP_HEADER_FIELD;
    userdata->header_field.append(buf, len);
    return 0;
}

int MultiPartForm::header_value_cb(multipart_parser *parser, const char *buf, size_t len)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->state = MP_HEADER_VALUE;
    userdata->header_value.append(buf, len);
    return 0;
}

int MultiPartForm::part_data_cb(multipart_parser *parser, const char *buf, size_t len)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->state = MP_PART_DATA;
    userdata->part_data.append(buf, len);
    return 0;
}

int MultiPartForm::part_data_begin_cb(multipart_parser *parser)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->reset_part();
    userdata->state = MP_PART_DATA_BEGIN;
    return 0;
}

int MultiPartForm::headers_complete_cb(multipart_parser *parser)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->handle_header();
    userdata->state = MP_HEADERS_COMPLETE;
    return 0;
}

int MultiPartForm::part_data_end_cb(multipart_parser *parser)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->state = MP_PART_DATA_END;
    userdata->handle_data();
    return 0;
}

int MultiPartForm::body_end_cb(multipart_parser *parser)
{
    auto *userdata = static_cast<multipart_parser_userdata *>(multipart_parser_get_data(parser));
    userdata->state = MP_BODY_END;
    return 0;
}

Form MultiPartForm::parse_multipart(const StringPiece &body) const
{
    Form form;
    if (boundary_.empty() || body.empty())
        return form;

    std::string boundary = "--" + boundary_;
    multipart_parser *parser = multipart_parser_init(boundary.c_str(), &settings_);
    if (parser == nullptr)
        return form;

    multipart_parser_userdata userdata{};
    userdata.state = MP_START;
    userdata.mp = &form;
    multipart_parser_set_data(parser, &userdata);
    const size_t parsed = multipart_parser_execute(parser, body.data(), body.size());
    multipart_parser_free(parser);

    if (parsed != body.size() || userdata.state != MP_BODY_END)
        form.clear();

    return form;
}

MultiPartEncoder::MultiPartEncoder()
    : boundary_(MultiPartForm::k_default_boundary)
{
}

void MultiPartEncoder::add_param(const std::string &name, const std::string &value) 
{
    params_.push_back({name, value});
}

void MultiPartEncoder::add_file(const std::string &file_name, const std::string &file_path)
{
    files_.push_back({file_name, file_path});
}

void MultiPartEncoder::set_boundary(const std::string &boundary)
{
    if (detail::is_valid_multipart_boundary(boundary))
        boundary_ = boundary;
}

void MultiPartEncoder::set_boundary(std::string &&boundary) 
{
    if (detail::is_valid_multipart_boundary(boundary))
        boundary_ = std::move(boundary);
}
