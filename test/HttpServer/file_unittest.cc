#include "workflow/WFFacilities.h"
#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "wfrest/StatusCode.h"
#include "wfrest/FileUtil.h"
#include "FileTestUtil.h"

using namespace wfrest;
using namespace protocol;

class FileTest
{
public:
    static void process(const std::string &path, size_t start, size_t end)
    {
        HttpServer svr;
        WFFacilities::WaitGroup wait_group(1);

        std::string *file_body = generate_file_content();
        bool write_ok = FileTestUtil::write_file(path, *file_body);
        EXPECT_TRUE(write_ok);

        svr.GET("/file", [path, start, end](const HttpReq *req, HttpResp *resp)
        {
            resp->File(path, start, end);
        });

        EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

        WFHttpTask *client_task = create_http_task("file");
        client_task->set_callback([&wait_group, file_body, path, start, end](WFHttpTask *task)
        {
            const void *body;
            size_t body_len;
            HttpResponse *resp = task->get_resp();
            resp->get_parsed_body(&body, &body_len);
            HttpHeaderMap header(resp);
            std::string content_type = header.get("Content-Type");
            EXPECT_EQ(content_type, "text/plain");
            std::string content_range = header.get("Content-Range");
            
            int exp_start = start;
            int exp_end = end;
            if(exp_end == -1) exp_end = file_body->size();
            if(exp_start < 0) exp_start = file_body->size() + start;

            EXPECT_EQ(content_range, "bytes " + std::to_string(exp_start) 
                                    + "-" + std::to_string(exp_end) 
                                    + "/" + std::to_string(exp_end - exp_start));

            std::string file_body_range = file_body->substr(exp_start, exp_end - exp_start); 
            EXPECT_EQ(file_body_range, std::string(static_cast<const char *>(body), body_len));

            std::remove(path.c_str());
            EXPECT_FALSE(FileUtil::file_exists(path));
            wait_group.done();
            delete file_body;
        });

        client_task->start();
        wait_group.wait();
        svr.stop();
    }
private:
    static WFHttpTask *create_http_task(const std::string &path)
    {
        return WFTaskFactory::create_http_task("http://127.0.0.1:8888/" + path, 4, 2, nullptr);
    }

    static std::string *generate_file_content()
    {
        auto *str = new std::string;
        for (size_t i = 0; i < 100; i++)
        {
            str->append(std::to_string(i % 5));
        }
        return str;
    }
};

TEST(HttpServer, file_range1)
{
    FileTest::process("test.txt", 10, 20);
}

TEST(HttpServer, file_range2)
{
    FileTest::process("test.txt", 0, 20);
}

TEST(HttpServer, file_range3)
{
    FileTest::process("test.txt", 10, -1);
}

TEST(HttpServer, file_range4)
{
    FileTest::process("test.txt", -10, -1);
}

TEST(HttpServer, file_range5)
{
    FileTest::process("test.txt", -10, 95);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}