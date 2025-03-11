#include "workflow/WFFacilities.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "wfrest/HttpServer.h"
#include "wfrest/Json.h"
#include "wfrest/FileUtil.h"
#include "wfrest/PathUtil.h"
#include "wfrest/HttpFile.h"

using namespace wfrest;

// 创建测试文件目录结构
bool prepare_test_files(const std::string& test_dir) {
    // 确保目录存在
    if (!FileUtil::create_directories(test_dir)) {
        std::cerr << "Failed to create directory: " << test_dir << std::endl;
        return false;
    }
    
    // 创建不同大小的测试文件
    std::vector<std::pair<std::string, size_t>> files = {
        {"tiny.txt", 1 * 1024},        // 1KB
        {"small.txt", 10 * 1024},      // 10KB
        {"medium.txt", 30 * 1024},     // 30KB
        {"threshold.txt", 50 * 1024},  // 50KB（阈值）
        {"large.txt", 100 * 1024},     // 100KB
        {"huge.txt", 1024 * 1024}      // 1MB
    };

    for (const auto& file : files) {
        std::string path = test_dir + "/" + file.first;
        if (!FileUtil::create_file_with_size(path, file.second)) {
            return false;
        }
        std::cout << "Created test file: " << path << " (" << file.second << " bytes)" << std::endl;
    }
    return true;
}

// 执行测试的客户端
class BenchmarkClient {
public:
    BenchmarkClient(const std::string& url, int num_requests, int concurrency)
        : url_(url), num_requests_(num_requests), concurrency_(concurrency) {}

    void run() {
        std::string cmd = "wrk -t" + std::to_string(concurrency_) + 
                          " -c" + std::to_string(concurrency_) + 
                          " -d10s --latency " + url_;
        
        std::cout << "Executing command: " << cmd << std::endl;
        int result = system(cmd.c_str());
        if (result != 0) {
            std::cerr << "Command execution failed, return code: " << result << std::endl;
        }
    }

private:
    std::string url_;
    int num_requests_;
    int concurrency_;
};

int main(int argc, char *argv[]) {
    // 检查 wrk 工具是否已安装
    int check_wrk = system("which wrk > /dev/null 2>&1");
    if (check_wrk != 0) {
        std::cerr << "Error: 'wrk' tool not found, please install it first" << std::endl;
        std::cerr << "  On Debian/Ubuntu: sudo apt-get install wrk" << std::endl;
        std::cerr << "  On macOS: brew install wrk" << std::endl;
        return 1;
    }

    // 默认参数
    int port = 8888;
    std::string test_dir = "./static_test_files";
    bool use_cache = true;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--port" && i + 1 < argc) {
            port = std::stoi(argv[i + 1]);
            i++;
        } else if (std::string(argv[i]) == "--dir" && i + 1 < argc) {
            test_dir = argv[i + 1];
            i++;
        } else if (std::string(argv[i]) == "--no-cache") {
            use_cache = false;
        }
    }

    // 准备测试文件
    if (!prepare_test_files(test_dir)) {
        std::cerr << "Failed to prepare test files, exiting test" << std::endl;
        return 1;
    }

    // 启动 HTTP 服务器
    HttpServer server;
    
    // 设置静态文件目录
    server.Static("/static", test_dir.c_str());
    
    // 如果启用缓存，预加载文件
    if (use_cache) {
        std::cout << "Preloading files into cache..." << std::endl;
        std::vector<std::string> files = {
            "tiny.txt", "small.txt", "medium.txt", "threshold.txt"
        };
        
        for (const auto& file : files) {
            std::string path = test_dir + "/" + file;
            HttpFile::preload_file(path);
            std::cout << "Preloaded: " << file << std::endl;
        }
    } else {
        std::cout << "Running without file cache" << std::endl;
    }
    
    // 添加状态端点
    server.GET("/status", [](const HttpReq *req, HttpResp *resp) {
        Json json;
        json["status"] = "ok";
        json["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        
        resp->Json(json);
    });
    
    // 添加缓存控制端点
    server.GET("/cache/clear", [](const HttpReq *req, HttpResp *resp) {
        HttpFile::clear_cache();
        Json json;
        json["status"] = "ok";
        json["message"] = "Cache cleared";
        
        resp->Json(json);
    });

    if (server.start(port) == 0) {
        std::cout << "Server started on port " << port << "..." << std::endl;
        std::cout << "Test files located at directory: " << test_dir << std::endl;
        std::cout << "\nAccess the following URLs for testing:\n";
        
        // 列出所有测试 URL
        std::vector<std::string> files = {
            "tiny.txt", "small.txt", "medium.txt", "threshold.txt", "large.txt", "huge.txt"
        };
        
        for (const auto& file : files) {
            std::string url = "http://localhost:" + std::to_string(port) + "/static/" + file;
            std::cout << url << std::endl;
        }

        std::cout << "\n=== Performance Test Results ===\n";
        
        // 执行测试
        for (const auto& file : files) {
            std::string url = "http://localhost:" + std::to_string(port) + "/static/" + file;
            std::cout << "\nTest file: " << file << std::endl;
            std::cout << "---------------------------\n";
            
            // 第一次测试（可能是冷启动）
            std::cout << "First run (cold start):" << std::endl;
            BenchmarkClient client1(url, 10000, 10); 
            client1.run();
            
            // 清除缓存（如果不使用缓存）
            if (!use_cache) {
                system(("curl -s http://localhost:" + std::to_string(port) + "/cache/clear > /dev/null").c_str());
            }
            
            // 第二次测试（热启动）
            std::cout << "\nSecond run (warm start):" << std::endl;
            BenchmarkClient client2(url, 10000, 10); 
            client2.run();
            
            std::cout << "---------------------------\n";
        }
        
        // 测试完成
        std::cout << "\nPerformance test completed. Press Ctrl+C to exit...\n";
        
        // 等待用户中断
        WFFacilities::WaitGroup wait_group(1);
        wait_group.wait();
        
        // 清理测试文件
        if (FileUtil::remove_directory(test_dir)) {
            std::cout << "Test file directory cleaned" << std::endl;
        } else {
            std::cerr << "Failed to clean test file directory" << std::endl;
        }
    } else {
        std::cerr << "Unable to start server" << std::endl;
        return 1;
    }

    return 0;
} 