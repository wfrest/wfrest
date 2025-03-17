#include "wfrest/HttpServer.h"
#include "wfrest/FileCache.h"
#include "wfrest/ErrorCode.h"
#include "workflow/Workflow.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"
#include <chrono>
#include <fstream>
#include <random>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <numeric> // For std::accumulate
#include <iostream>
#include <sstream>
#include <memory>

using namespace wfrest;

// Generate a random file of specified size
bool generate_test_file(const std::string& path, size_t size_bytes)
{
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);
    
    const size_t CHUNK_SIZE = 4096;
    std::vector<char> buffer(CHUNK_SIZE);
    
    size_t bytes_left = size_bytes;
    while (bytes_left > 0) {
        size_t chunk = std::min(CHUNK_SIZE, bytes_left);
        for (size_t i = 0; i < chunk; i++) {
            buffer[i] = static_cast<char>(dist(gen));
        }
        file.write(buffer.data(), chunk);
        bytes_left -= chunk;
    }
    
    file.close();
    return true;
}

// File sizes to test
const std::vector<size_t> TEST_FILE_SIZES = {
    1 * 1024,      // 1 KB
    10 * 1024,     // 10 KB
    50 * 1024,     // 50 KB
    100 * 1024,    // 100 KB
    500 * 1024,    // 500 KB
    1 * 1024 * 1024 // 1 MB
};

// Structure to hold benchmark results
struct BenchmarkResult {
    std::string file_name;
    size_t file_size;
    int num_requests;
    
    // Standard file serving
    double normal_rps;
    double normal_latency_ms;
    
    // Cached file serving
    double cached_rps;
    double cached_latency_ms;
    
    // Speedup
    double rps_speedup;
    double latency_speedup;
};

// Format the size in human-readable form
std::string format_size(size_t bytes)
{
    const char* suffixes[] = {"B", "KB", "MB", "GB"};
    int suffix_idx = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024 && suffix_idx < 3) {
        size /= 1024;
        suffix_idx++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << suffixes[suffix_idx];
    return oss.str();
}

// Print results in a nice table format
void print_results(const std::vector<BenchmarkResult>& results)
{
    printf("\n%-10s %-10s %-15s %-15s %-15s %-15s %-15s %-15s\n",
           "File", "Size", "Normal RPS", "Normal Latency", "Cached RPS", "Cached Latency", "RPS Speedup", "Latency Speedup");
    printf("%-10s %-10s %-15s %-15s %-15s %-15s %-15s %-15s\n",
           "--------", "--------", "-------------", "-------------", "-------------", "-------------", "-------------", "-------------");
           
    for (const auto& result : results) {
        printf("%-10s %-10s %-15.2f %-15.2f %-15.2f %-15.2f %-15.2fx %-15.2fx\n",
               result.file_name.c_str(),
               format_size(result.file_size).c_str(),
               result.normal_rps,
               result.normal_latency_ms,
               result.cached_rps,
               result.cached_latency_ms,
               result.rps_speedup,
               result.latency_speedup);
    }
    printf("\n");
}

// JSON representation of results for API endpoint
Json results_to_json(const std::vector<BenchmarkResult>& results)
{
    Json json;
    for (const auto& result : results) {
        Json result_json;
        result_json["file_name"] = result.file_name;
        result_json["file_size"] = result.file_size;
        result_json["file_size_human"] = format_size(result.file_size);
        result_json["num_requests"] = result.num_requests;
        result_json["normal_rps"] = result.normal_rps;
        result_json["normal_latency_ms"] = result.normal_latency_ms;
        result_json["cached_rps"] = result.cached_rps;
        result_json["cached_latency_ms"] = result.cached_latency_ms;
        result_json["rps_speedup"] = result.rps_speedup;
        result_json["latency_speedup"] = result.latency_speedup;
        
        json.push_back(result_json);
    }
    return json;
}

// Client for executing wrk tests
class BenchmarkClient {
public:
    BenchmarkClient(const std::string& url, int duration_seconds, int concurrency)
        : url_(url), duration_seconds_(duration_seconds), concurrency_(concurrency) {}

    // Run the benchmark and return {requests_per_second, latency_ms}
    std::pair<double, double> run() {
        std::string cmd = "wrk -t" + std::to_string(concurrency_) + 
                          " -c" + std::to_string(concurrency_) + 
                          " -d" + std::to_string(duration_seconds_) + 
                          "s --latency " + url_ + " 2>&1";
        
        std::cout << "Executing command: " << cmd << std::endl;
        
        // Use popen to capture the output
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            std::cerr << "Command execution failed" << std::endl;
            return {0.0, 0.0}; // Return zero values on error
        }
        
        char buffer[1024];
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
        pclose(pipe);
        
        // Parse the wrk output to extract RPS and latency
        double requests_per_second = 0.0;
        double latency_ms = 0.0;
        
        std::istringstream iss(result);
        std::string line;
        while (std::getline(iss, line)) {
            // Example line: "Requests/sec:   1234.56"
            if (line.find("Requests/sec") != std::string::npos) {
                size_t pos = line.find(":");
                if (pos != std::string::npos) {
                    std::string value = line.substr(pos + 1);
                    // Use strtod for safer conversion without exceptions
                    char* end;
                    double val = strtod(value.c_str(), &end);
                    
                    // Check if conversion was successful
                    if (end != value.c_str()) {
                        // At least some part of the string was converted
                        requests_per_second = val;
                    } else {
                        std::cerr << "Error parsing RPS value: " << value << std::endl;
                    }
                }
            }
            // Example line: "    Latency   123.45ms    11.22ms   33.44ms   99.99%"
            else if (line.find("Latency") != std::string::npos && line.find("ms") != std::string::npos) {
                std::istringstream latency_iss(line);
                std::string token;
                std::getline(latency_iss, token, ' '); // Skip "Latency" 
                while (token.empty() && std::getline(latency_iss, token, ' ')); // Skip spaces
                
                if (!token.empty()) {
                    // Extract the average latency
                    size_t ms_pos = token.find("ms");
                    if (ms_pos != std::string::npos) {
                        std::string latency_str = token.substr(0, ms_pos);
                        
                        // Use strtod for safer conversion without exceptions
                        char* end;
                        double val = strtod(latency_str.c_str(), &end);
                        
                        // Check if conversion was successful
                        if (end != latency_str.c_str()) {
                            // At least some part of the string was converted
                            latency_ms = val;
                        } else {
                            std::cerr << "Error parsing latency value: " << token << std::endl;
                        }
                    }
                }
            }
        }
        
        std::cout << "Results: " << requests_per_second << " req/s, " 
                  << latency_ms << "ms latency" << std::endl;
        
        return {requests_per_second, latency_ms};
    }

private:
    std::string url_;
    int duration_seconds_;
    int concurrency_;
};

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port> [duration_seconds] [concurrency]\n", argv[0]);
        return 1;
    }

    // Check if wrk tool is installed
    int check_wrk = system("which wrk > /dev/null 2>&1");
    if (check_wrk != 0) {
        std::cerr << "Error: 'wrk' tool not found, please install it first" << std::endl;
        std::cerr << "  On Debian/Ubuntu: sudo apt-get install wrk" << std::endl;
        std::cerr << "  On macOS: brew install wrk" << std::endl;
        return 1;
    }

    int port = atoi(argv[1]);
    int duration_seconds = 5; // Default duration (shorter for interactive testing)
    int concurrency = 10;     // Default concurrency
    
    if (argc >= 3) {
        duration_seconds = atoi(argv[2]);
    }
    
    if (argc >= 4) {
        concurrency = atoi(argv[3]);
    }
    
    std::string test_dir = "./benchmark_files";
    
    // Create test directory if it doesn't exist
    struct stat st = {0};
    if (stat(test_dir.c_str(), &st) == -1) {
        if (mkdir(test_dir.c_str(), 0700) != 0) {
            fprintf(stderr, "Failed to create directory: %s\n", test_dir.c_str());
            return 1;
        }
    }
    
    // Generate test files
    std::vector<std::string> test_files;
    for (size_t size : TEST_FILE_SIZES) {
        std::string filename = "test_" + std::to_string(size) + ".dat";
        std::string filepath = test_dir + "/" + filename;
        
        if (generate_test_file(filepath, size)) {
            test_files.push_back(filename);
            printf("Generated test file: %s (%s)\n", filename.c_str(), format_size(size).c_str());
        } else {
            fprintf(stderr, "Failed to generate test file: %s\n", filepath.c_str());
        }
    }
    
    if (test_files.empty()) {
        fprintf(stderr, "No test files could be generated. Exiting.\n");
        return 1;
    }
    
    // Setup server
    HttpServer svr;
    
    // Configure file cache
    FileCache& cache = FileCache::instance();
    cache.set_max_size(20 * 1024 * 1024); // 20MB cache size
    cache.enable();
    
    // Set up routes for normal and cached file access
    svr.Static("/static", test_dir.c_str());
    svr.CachedStatic("/cached", test_dir.c_str());
    
    // Cache control routes
    svr.GET("/clear-cache", [](const HttpReq *req, HttpResp *resp) {
        FileCache::instance().clear();
        resp->String("Cache cleared");
    });
    
    svr.GET("/cache-info", [](const HttpReq *req, HttpResp *resp) {
        size_t cache_size = FileCache::instance().size();
        bool enabled = FileCache::instance().is_enabled();
        
        Json json;
        json["cache_size_bytes"] = cache_size;
        json["cache_size_mb"] = cache_size / (1024.0 * 1024.0);
        json["enabled"] = enabled;
        
        resp->Json(json);
    });

    // API endpoint to run the benchmark using wrk
    svr.GET("/run-benchmark", [&test_dir, &test_files, port, duration_seconds, concurrency](const HttpReq *req, HttpResp *resp) {
        int bench_duration = duration_seconds;
        int bench_concurrency = concurrency;
        
        // Allow overriding parameters
        if (req->has_query("duration")) {
            const std::string& dur_val = req->query("duration");
            char* end;
            long val = strtol(dur_val.c_str(), &end, 10);
            if (end != dur_val.c_str() && *end == '\0' && val > 0) {
                bench_duration = static_cast<int>(std::min(val, 30L)); // Limit to 30 seconds max
            }
        }
        
        if (req->has_query("concurrency")) {
            const std::string& conc_val = req->query("concurrency");
            char* end;
            long val = strtol(conc_val.c_str(), &end, 10);
            if (end != conc_val.c_str() && *end == '\0' && val > 0) {
                bench_concurrency = static_cast<int>(std::min(val, 100L)); // Limit to 100 concurrent
            }
        }
        
        // Clear cache before testing
        FileCache::instance().clear();
        
        std::vector<BenchmarkResult> results;
        
        for (const auto& filename : test_files) {
            struct stat file_stat;
            std::string filepath = test_dir + "/" + filename;
            
            if (stat(filepath.c_str(), &file_stat) == -1 || !S_ISREG(file_stat.st_mode)) {
                fprintf(stderr, "File not found or not a regular file: %s\n", filepath.c_str());
                continue;
            }
            
            BenchmarkResult result;
            result.file_name = filename;
            result.file_size = file_stat.st_size;
            result.num_requests = 0; // wrk doesn't specify this directly
            
            // Test standard file serving (non-cached)
            std::string normal_url = "http://localhost:" + std::to_string(port) + "/static/" + filename;
            BenchmarkClient normal_client(normal_url, bench_duration, bench_concurrency);
            auto normal_result = normal_client.run();
            double normal_rps = normal_result.first;
            double normal_latency = normal_result.second;
            
            result.normal_rps = normal_rps;
            result.normal_latency_ms = normal_latency;
            
            // Clear cache before cached test
            FileCache::instance().clear();
            
            // Test cached file serving
            std::string cached_url = "http://localhost:" + std::to_string(port) + "/cached/" + filename;
            BenchmarkClient cached_client(cached_url, bench_duration, bench_concurrency);
            auto cached_result = cached_client.run();
            double cached_rps = cached_result.first;
            double cached_latency = cached_result.second;
            
            result.cached_rps = cached_rps;
            result.cached_latency_ms = cached_latency;
            
            // Calculate speedups (protect against divide by zero)
            result.rps_speedup = (normal_rps > 0) ? (cached_rps / normal_rps) : 1.0;
            result.latency_speedup = (cached_latency > 0) ? (normal_latency / cached_latency) : 1.0;
            
            results.push_back(result);
        }
        
        // Print results to server console
        print_results(results);
        
        // Return results as JSON
        resp->Json(results_to_json(results));
    });
    
    // HTML page with interactive benchmark UI
    svr.GET("/", [port, duration_seconds, concurrency](const HttpReq *req, HttpResp *resp) {
        // Use a standard string instead of a raw string to avoid C++ standard issues
        std::string html = 
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head>\n"
            "    <title>wfrest File Cache Benchmark</title>\n"
            "    <style>\n"
            "        body { font-family: Arial, sans-serif; max-width: 1000px; margin: 0 auto; padding: 20px; }\n"
            "        h1 { color: #333; }\n"
            "        table { border-collapse: collapse; width: 100%; margin-top: 20px; }\n"
            "        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n"
            "        th { background-color: #f2f2f2; }\n"
            "        tr:nth-child(even) { background-color: #f9f9f9; }\n"
            "        .controls { margin: 20px 0; }\n"
            "        .controls label { margin-right: 10px; }\n"
            "        button { padding: 8px 16px; background-color: #4CAF50; color: white; border: none; cursor: pointer; }\n"
            "        button:hover { background-color: #45a049; }\n"
            "        .loading { display: none; margin-left: 10px; }\n"
            "        .speedup { font-weight: bold; color: #4CAF50; }\n"
            "        code { background-color: #f5f5f5; padding: 2px 4px; border-radius: 4px; }\n"
            "    </style>\n"
            "</head>\n"
            "<body>\n"
            "    <h1>wfrest File Cache Benchmark</h1>\n"
            "    <p>This benchmark compares normal static file serving vs cached static file serving using <code>wrk</code>.</p>\n"
            "    \n"
            "    <div class=\"controls\">\n"
            "        <label for=\"duration\">Test Duration (seconds):</label>\n"
            "        <input type=\"number\" id=\"duration\" min=\"1\" max=\"30\" value=\"" + std::to_string(duration_seconds) + "\">\n"
            "        \n"
            "        <label for=\"concurrency\">Concurrency:</label>\n"
            "        <input type=\"number\" id=\"concurrency\" min=\"1\" max=\"100\" value=\"" + std::to_string(concurrency) + "\">\n"
            "        \n"
            "        <button onclick=\"runBenchmark()\">Run Benchmark</button>\n"
            "        <span class=\"loading\" id=\"loading\">Running benchmark... (this may take a while)</span>\n"
            "    </div>\n"
            "    \n"
            "    <div id=\"results\">\n"
            "        <p>Click \"Run Benchmark\" to start the test.</p>\n"
            "    </div>\n"
            "    \n"
            "    <div id=\"chart-container\" style=\"margin-top: 20px; display: none;\">\n"
            "        <h2>Performance Comparison</h2>\n"
            "        <canvas id=\"speedupChart\" width=\"900\" height=\"400\"></canvas>\n"
            "    </div>\n"
            "    \n"
            "    <div style=\"margin-top: 40px;\">\n"
            "        <h2>Test Files</h2>\n"
            "        <table>\n"
            "            <tr>\n"
            "                <th>File</th>\n"
            "                <th>Size</th>\n"
            "                <th>Normal URL</th>\n"
            "                <th>Cached URL</th>\n"
            "            </tr>\n";
        
        for (size_t size : TEST_FILE_SIZES) {
            std::string filename = "test_" + std::to_string(size) + ".dat";
            std::string normal_url = "http://localhost:" + std::to_string(port) + "/static/" + filename;
            std::string cached_url = "http://localhost:" + std::to_string(port) + "/cached/" + filename;
            
            html += 
                "            <tr>\n"
                "                <td>" + filename + "</td>\n"
                "                <td>" + format_size(size) + "</td>\n"
                "                <td><a href=\"" + normal_url + "\" target=\"_blank\">" + normal_url + "</a></td>\n"
                "                <td><a href=\"" + cached_url + "\" target=\"_blank\">" + cached_url + "</a></td>\n"
                "            </tr>\n";
        }
        
        html +=
            "        </table>\n"
            "    </div>\n"
            "    \n"
            "    <script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n"
            "    <script>\n"
            "        let benchmarkData = null;\n"
            "        let myChart = null;\n"
            "        \n"
            "        function runBenchmark() {\n"
            "            const duration = document.getElementById(\"duration\").value;\n"
            "            const concurrency = document.getElementById(\"concurrency\").value;\n"
            "            document.getElementById(\"loading\").style.display = \"inline\";\n"
            "            document.getElementById(\"chart-container\").style.display = \"none\";\n"
            "            \n"
            "            fetch(`/run-benchmark?duration=${duration}&concurrency=${concurrency}`)\n"
            "                .then(response => response.json())\n"
            "                .then(data => {\n"
            "                    benchmarkData = data;\n"
            "                    displayResults();\n"
            "                    document.getElementById(\"loading\").style.display = \"none\";\n"
            "                    document.getElementById(\"chart-container\").style.display = \"block\";\n"
            "                    createChart();\n"
            "                })\n"
            "                .catch(error => {\n"
            "                    console.error(\"Error:\", error);\n"
            "                    document.getElementById(\"loading\").style.display = \"none\";\n"
            "                    document.getElementById(\"results\").innerHTML = \"<p>Error running benchmark.</p>\";\n"
            "                });\n"
            "        }\n"
            "        \n"
            "        function displayResults() {\n"
            "            let html = \n"
            "                \"<h2>Benchmark Results</h2>\" +\n"
            "                \"<table>\" +\n"
            "                    \"<tr>\" +\n"
            "                        \"<th>File</th>\" +\n"
            "                        \"<th>Size</th>\" +\n"
            "                        \"<th>Normal Req/s</th>\" +\n"
            "                        \"<th>Normal Latency (ms)</th>\" +\n"
            "                        \"<th>Cached Req/s</th>\" +\n"
            "                        \"<th>Cached Latency (ms)</th>\" +\n"
            "                        \"<th>RPS Speedup</th>\" +\n"
            "                        \"<th>Latency Improvement</th>\" +\n"
            "                    \"</tr>\";\n"
            "            \n"
            "            for (let i = 0; i < benchmarkData.length; i++) {\n"
            "                const result = benchmarkData[i];\n"
            "                html += \n"
            "                    \"<tr>\" +\n"
            "                        \"<td>\" + result.file_name + \"</td>\" +\n"
            "                        \"<td>\" + result.file_size_human + \"</td>\" +\n"
            "                        \"<td>\" + result.normal_rps.toFixed(2) + \"</td>\" +\n"
            "                        \"<td>\" + result.normal_latency_ms.toFixed(2) + \"</td>\" +\n"
            "                        \"<td>\" + result.cached_rps.toFixed(2) + \"</td>\" +\n"
            "                        \"<td>\" + result.cached_latency_ms.toFixed(2) + \"</td>\" +\n"
            "                        \"<td class=\\\"speedup\\\">\" + result.rps_speedup.toFixed(2) + \"x</td>\" +\n"
            "                        \"<td class=\\\"speedup\\\">\" + result.latency_speedup.toFixed(2) + \"x</td>\" +\n"
            "                    \"</tr>\";\n"
            "            }\n"
            "            \n"
            "            html += \"</table>\";\n"
            "            document.getElementById(\"results\").innerHTML = html;\n"
            "        }\n"
            "        \n"
            "        function createChart() {\n"
            "            const ctx = document.getElementById(\"speedupChart\").getContext(\"2d\");\n"
            "            \n"
            "            // Clean up previous chart if exists\n"
            "            if (myChart) {\n"
            "                myChart.destroy();\n"
            "            }\n"
            "            \n"
            "            const labels = [];\n"
            "            const normalRpsData = [];\n"
            "            const cachedRpsData = [];\n"
            "            \n"
            "            for (let i = 0; i < benchmarkData.length; i++) {\n"
            "                labels.push(benchmarkData[i].file_size_human);\n"
            "                normalRpsData.push(benchmarkData[i].normal_rps);\n"
            "                cachedRpsData.push(benchmarkData[i].cached_rps);\n"
            "            }\n"
            "            \n"
            "            myChart = new Chart(ctx, {\n"
            "                type: \"bar\",\n"
            "                data: {\n"
            "                    labels: labels,\n"
            "                    datasets: [\n"
            "                        {\n"
            "                            label: \"Normal Static (Req/s)\",\n"
            "                            data: normalRpsData,\n"
            "                            backgroundColor: \"rgba(255, 99, 132, 0.7)\",\n"
            "                            borderColor: \"rgba(255, 99, 132, 1)\",\n"
            "                            borderWidth: 1\n"
            "                        },\n"
            "                        {\n"
            "                            label: \"Cached Static (Req/s)\",\n"
            "                            data: cachedRpsData,\n"
            "                            backgroundColor: \"rgba(75, 192, 192, 0.7)\",\n"
            "                            borderColor: \"rgba(75, 192, 192, 1)\",\n"
            "                            borderWidth: 1\n"
            "                        }\n"
            "                    ]\n"
            "                },\n"
            "                options: {\n"
            "                    scales: {\n"
            "                        y: {\n"
            "                            beginAtZero: true,\n"
            "                            title: {\n"
            "                                display: true,\n"
            "                                text: \"Requests per Second\"\n"
            "                            }\n"
            "                        },\n"
            "                        x: {\n"
            "                            title: {\n"
            "                                display: true,\n"
            "                                text: \"File Size\"\n"
            "                            }\n"
            "                        }\n"
            "                    },\n"
            "                    plugins: {\n"
            "                        title: {\n"
            "                            display: true,\n"
            "                            text: \"Performance Comparison: Normal vs Cached Static File Serving\"\n"
            "                        }\n"
            "                    }\n"
            "                }\n"
            "            });\n"
            "        }\n"
            "    </script>\n"
            "</body>\n"
            "</html>";
        
        resp->headers["Content-Type"] = "text/html";
        resp->String(html);
    });
    
    if (svr.start(port) == 0) {
        printf("\nCache Benchmark Server started on port %d\n", port);
        printf("Open http://localhost:%d/ in your browser to run the benchmark\n\n", port);
        printf("Benchmark parameters: default duration=%d seconds, concurrency=%d\n", duration_seconds, concurrency);
        printf("(These can be changed in the web interface)\n\n");
        
        printf("Available URLs:\n");
        printf("  http://localhost:%d/ - Interactive benchmark page\n", port);
        printf("  http://localhost:%d/run-benchmark?duration=5&concurrency=10 - Run benchmark via API\n", port);
        printf("  http://localhost:%d/static/test_10240.dat - Test normal file access\n", port);
        printf("  http://localhost:%d/cached/test_10240.dat - Test cached file access\n", port);
        printf("  http://localhost:%d/clear-cache - Clear the file cache\n", port);
        printf("  http://localhost:%d/cache-info - Get cache statistics\n\n", port);
        
        // Wait for user interruption
        WFFacilities::WaitGroup wait_group(1);
        wait_group.wait();
    } else {
        fprintf(stderr, "Failed to start server on port %d\n", port);
        return 1;
    }
    
    return 0;
}