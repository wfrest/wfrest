#include "workflow/WFFacilities.h"
#include <gtest/gtest.h>
#include "wfrest/HttpServer.h"
#include "wfrest/ErrorCode.h"
#include "wfrest/PathUtil.h"
#include "wfrest/FileUtil.h"
#include "wfrest/Json.h"
#include "../ClientUtil.h"
#include "../FileTestUtil.h"

using namespace wfrest;
using namespace protocol;

class StaticTest : public testing::Test
{
protected:
    void SetUp() override
    {
        // create dir
        dir_path_ = "./www";
        FileTestUtil::create_dir(dir_path_.c_str(), 0777);
        EXPECT_TRUE(PathUtil::is_dir(dir_path_));
        // create file
        file_path_ = dir_path_ + "/test.txt";
        bool write_ok = FileTestUtil::write_file(file_path_, "123456788890");
        EXPECT_TRUE(write_ok);
        EXPECT_TRUE(FileUtil::file_exists(file_path_));
    }

    void TearDown() override
    {
        FileTestUtil::recursive_delete(dir_path_.c_str());
        EXPECT_FALSE(FileUtil::file_exists(file_path_));
    }
protected:
    std::string dir_path_;
    std::string file_path_;
};

TEST_F(StaticTest, serve_static_dir)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.Static("/public", "./www");

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task = ClientUtil::create_http_task("public/test.txt");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;
        task->get_resp()->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("123456788890", static_cast<const char *>(body)) == 0);
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
}

TEST_F(StaticTest, serve_static_file)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.Static("/public", "./www/test.txt");

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task = ClientUtil::create_http_task("public");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;

        task->get_resp()->get_parsed_body(&body, &body_len);
        EXPECT_TRUE(strcmp("123456788890", static_cast<const char *>(body)) == 0);
        wait_group.done();
    });
    svr.list_routes();
    client_task->start();
    wait_group.wait();
    svr.stop();
}

TEST_F(StaticTest, serve_static_file_error)
{
    HttpServer svr;
    WFFacilities::WaitGroup wait_group(1);

    svr.Static("/public", "./www/test.txt");

    EXPECT_TRUE(svr.start("127.0.0.1", 8888) == 0) << "http server start failed";

    WFHttpTask *client_task = ClientUtil::create_http_task("public/test.txt");
    client_task->set_callback([&wait_group](WFHttpTask *task)
    {
        const void *body;
        size_t body_len;

        task->get_resp()->get_parsed_body(&body, &body_len);
        std::string body_str(static_cast<const char *>(body));
        Json js = Json::parse(body_str);
        EXPECT_EQ(js["errmsg"].get<std::string>(), "Route Not Found : GET /public/test.txt");
        wait_group.done();
    });

    client_task->start();
    wait_group.wait();
    svr.stop();
}
