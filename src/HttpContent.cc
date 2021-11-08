//
// Created by Chanchan on 11/5/21.
//

#include "HttpContent.h"
#include "workflow/StringUtil.h"
#include <cstring>
#include "StrUtil.h"

using namespace wfrest;

const std::string MultiPartForm::default_boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

void Urlencode::parse_query_params(const StringPiece &body, Urlencode::KV &res)
{
    if (body.empty())
        return;

    std::vector<StringPiece> arr = StrUtil::split_piece<StringPiece>(body, '&');

    if (arr.empty())
        return;

    for (const auto &ele: arr)
    {
        if (ele.empty())
            continue;

        std::vector<std::string> kv = StrUtil::split_piece<std::string>(ele, '=');
        size_t kv_size = kv.size();
        std::string &key = kv[0];

        if (key.empty() || res.count(key) > 0)
            continue;

        if (kv_size == 1)
        {
            res.emplace(std::move(key), "");
            continue;
        }

        std::string &val = kv[1];

        if (val.empty())
            res.emplace(std::move(key), "");
        else
            res.emplace(std::move(key), std::move(val));
    }
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
    MultiPartForm::MultiPart *mp;
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

//        auto dispo_list2 = StrUtil::split(header_value, ';');
//        for(auto &li2 : dispo_list2)
//        {
//            fprintf(stderr, "dispo2 : %s\n", li2.c_str());
//        }
        for (auto &dispo: dispo_list)
        {
//            fprintf(stderr, "dispo : %s\n", dispo.data());
            auto kv = StrUtil::split_piece<StringPiece>(StrUtil::trim(dispo), '=');
//            auto kv = StrUtil::split(StrUtil::trim(dispo, " "), '=');
            if (kv.size() == 2)
            {
                const char *key = kv[0].data();
                StringPiece value_piece = *(kv.begin() + 1);
                // todo
                StringPiece value = StrUtil::trim_pairs(value_piece, R"(""'')");
//                std::string value = StrUtil::trim_pairs(kv[1], R"(""'')");

//                std::string value = *(kv.begin() + 1);
//                StringPiece value_piece(value);
//                value = StrUtil::trim_pairs(value_piece, R"(""'')");

                if (strcmp(key, "name") == 0)
                {
//                    name = std::move(value);
                    name = value.as_string();
                } else if (strcmp(key, "filename") == 0)
                {
//                    filename = std::move(value);
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
        FormData formdata;
        formdata.content = part_data;
        formdata.filename = filename;
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

int MultiPartForm::parse_multipart(const StringPiece &body, OUT MultiPartForm::MultiPart &form)
{
//    std::string boundary = "--" + boundary_;
    boundary_.insert(0, "--");
    fprintf(stderr, "-- bounday : %s\n", boundary_.c_str());
    multipart_parser *parser = multipart_parser_init(boundary_.c_str(), &settings_);
    multipart_parser_userdata userdata;
    userdata.state = MP_START;
    userdata.mp = &form;
    multipart_parser_set_data(parser, &userdata);
    size_t num_parse = multipart_parser_execute(parser, body.data(), body.size());
    multipart_parser_free(parser);
    fprintf(stderr, "num parse : %zu || body size : %zu\n", num_parse, body.size());
    fprintf(stderr, "form size : %zu\n", form.size());
    return num_parse == body.size() ? 0 : -1;
}






