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

using namespace wfrest;

// Create test file directory structure
bool prepare_test_files(const std::string& test_dir) {
    // Ensure directory exists
    if (!FileUtil::create_directories(test_dir)) {
        std::cerr << "Failed to create directory: " << test_dir << std::endl;
        return false;
    }
    
    // Create test files of different sizes
    std::vector<std::pair<std::string, size_t>> files = {
        {"tiny.txt", 1 * 1024},        // 1KB
        {"small.txt", 10 * 1024},      // 10KB
        {"medium.txt", 30 * 1024},     // 30KB
        {"threshold.txt", 50 * 1024},  // 50KB（threshold）
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

// Client for executing tests
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
    // Check if wrk tool is installed
    int check_wrk = system("which wrk > /dev/null 2>&1");
    if (check_wrk != 0) {
        std::cerr << "Error: 'wrk' tool not found, please install it first" << std::endl;
        std::cerr << "  On Debian/Ubuntu: sudo apt-get install wrk" << std::endl;
        std::cerr << "  On macOS: brew install wrk" << std::endl;
        return 1;
    }

    // Default parameters
    int port = 8888;
    std::string test_dir = "./static_test_files";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--port" && i + 1 < argc) {
            port = std::stoi(argv[i + 1]);
            i++;
        } else if (std::string(argv[i]) == "--dir" && i + 1 < argc) {
            test_dir = argv[i + 1];
            i++;
        }
    }

    // Prepare test files
    if (!prepare_test_files(test_dir)) {
        std::cerr << "Failed to prepare test files, exiting test" << std::endl;
        return 1;
    }

    // Start HTTP server
    HttpServer server;
    
    // Set static file directory
    server.Static("/static", test_dir.c_str());
    
    // Add a status endpoint
    server.GET("/status", [](const HttpReq *req, HttpResp *resp) {
        Json json;
        json["status"] = "ok";
        json["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        
        resp->Json(json);
    });

    if (server.start(port) == 0) {
        std::cout << "Server started on port " << port << "..." << std::endl;
        std::cout << "Test files located at directory: " << test_dir << std::endl;
        std::cout << "\nAccess the following URLs for testing:\n";
        
        // List all test URLs
        std::vector<std::string> files = {
            "tiny.txt", "small.txt", "medium.txt", "threshold.txt", "large.txt", "huge.txt"
        };
        
        for (const auto& file : files) {
            std::string url = "http://localhost:" + std::to_string(port) + "/static/" + file;
            std::cout << url << std::endl;
        }

        std::cout << "\n=== Performance Test Results ===\n";
        
        // Execute tests
        for (const auto& file : files) {
            std::string url = "http://localhost:" + std::to_string(port) + "/static/" + file;
            std::cout << "\nTest file: " << file << std::endl;
            std::cout << "---------------------------\n";
            
            BenchmarkClient client(url, 10000, 10); 
            client.run();
            
            std::cout << "---------------------------\n";
        }
        
        // Test completed
        std::cout << "\nPerformance test completed. Press Ctrl+C to exit...\n";
        
        // Wait for user interruption
        WFFacilities::WaitGroup wait_group(1);
        wait_group.wait();
        
        // Clean up test files
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