#include "workflow/StringUtil.h"
#include "workflow/WFFacilities.h"
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include "StrUtil.h"
#include "HttpContent.h"
#include "StringPiece.h"
#include "PathUtil.h"
#include "HttpDef.h"

using namespace wfrest;

const std::string MultiPartForm::k_default_boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

std::map<std::string, std::string> Urlencode::parse_post_kv(const StringPiece &body)
{
    std::map<std::string, std::string> map;

    if (body.empty())
        return map;

    std::vector<StringPiece> arr = StrUtil::split_piece<StringPiece>(body, '&');

    if (arr.empty())
        return map;

    for (const auto &ele: arr)
    {
        if (ele.empty())
            continue;

        std::vector<std::string> kv = StrUtil::split_piece<std::string>(ele, '=');
        size_t kv_size = kv.size();
        std::string &key = kv[0];

        if (key.empty() || map.count(key) > 0)
            continue;

        if (kv_size == 1)
        {
            map.emplace(std::move(key), "");
            continue;
        }

        std::string &val = kv[1];

        if (val.empty())
            map.emplace(std::move(key), "");
        else
            map.emplace(std::move(key), std::move(val));
    }
    return map;
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

    void handle_header();

    void handle_data();
};

void multipart_parser_userdata::handle_header()
{
    if (header_field.empty() || header_value.empty()) return;
    if (strcasecmp(header_field.c_str(), "Content-Disposition") == 0)
    {
        // Content-Disposition: attachment
        // Content-Disposition: attachment; filename="filename.jpg"
        // Content-Disposition: form-data; name="avatar"; filename="user.jpg"
        StringPiece header_val_piece(header_value);
        std::vector<StringPiece> dispo_list = StrUtil::split_piece<StringPiece>(header_val_piece, ';');

        for (auto &dispo: dispo_list)
        {
            auto kv = StrUtil::split_piece<StringPiece>(StrUtil::trim(dispo), '=');
            if (kv.size() == 2)
            {
                // name="file"
                // kv[0] is key(name)
                // kv[1] is value("file")
                StringPiece value = StrUtil::trim_pairs(kv[1], R"(""'')");
                if (kv[0].starts_with(StringPiece("name")))
                {
                    name = value.as_string();
                } else if (kv[0].starts_with(StringPiece("filename")))
                {
                    filename = value.as_string();
                }
            }
        }
    }
    header_field.clear();
    header_value.clear();
}

void multipart_parser_userdata::handle_data()
{
    if (!name.empty())
    {
        std::pair<std::string, std::string> formdata;
        formdata.first = filename;
        formdata.second = part_data;
        (*mp)[name] = formdata;
    }
    name.clear();
    filename.clear();
    part_data.clear();
}

MultiPartForm::MultiPartForm()
{
    settings_ = {
            .on_header_field = header_field_cb,
            .on_header_value = header_value_cb,
            .on_part_data = part_data_cb,
            .on_part_data_begin = part_data_begin_cb,
            .on_headers_complete = headers_complete_cb,
            .on_part_data_end = part_data_end_cb,
            .on_body_end = body_end_cb
    };
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
    std::string boundary = "--" + boundary_;
    multipart_parser *parser = multipart_parser_init(boundary.c_str(), &settings_);
    multipart_parser_userdata userdata;
    userdata.state = MP_START;
    userdata.mp = &form;
    multipart_parser_set_data(parser, &userdata);
    multipart_parser_execute(parser, body.data(), body.size());
    multipart_parser_free(parser);
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
    boundary_ = boundary;
}

void MultiPartEncoder::set_boundary(std::string &&boundary) 
{
    boundary_ = std::move(boundary);
}