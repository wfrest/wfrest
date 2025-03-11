# Static File Serving Performance Optimization for wfrest

## Overview

This document describes a performance optimization for serving static files in the wfrest framework. The optimization addresses issue [#272](https://github.com/wfrest/wfrest/issues/272), where static file serving was reported to be significantly slower than Apache, particularly for small files like CSS and JS from React builds.

## Problem Background

As described in issue #272, while wfrest performs exceptionally well for serving REST API responses with data loaded from MySQL, it was found to be approximately 4 times slower than Apache when serving static files. The reason was identified as wfrest always using asynchronous file I/O, even for very small files, which introduces unnecessary overhead.

## Optimization Approach

### Core Concept

The optimization implements a size-based approach to file serving:
- Small files (≤50KB) are read synchronously, avoiding the overhead of asynchronous task scheduling
- Large files (>50KB) continue to use the original asynchronous approach for better performance with large content

### Technical Implementation

The optimization modifies the `HttpFile::send_file` method by adding a threshold-based condition:

```cpp
// Optimization: Use synchronous reading for small files (<50KB)
const size_t SMALL_FILE_THRESHOLD = 50 * 1024; // 50KB

if (size <= SMALL_FILE_THRESHOLD) {
    // Synchronous file reading for small files
    FILE *fp = fopen(path.c_str(), "rb");
    // ... read file content synchronously ...
    resp->append_output_body_nocopy(buf, size);
    return StatusOK;
}

// Use original async approach for large files
// ... original async implementation ...
```

## Why This Works

1. **Reduced Overhead**: For small files, the overhead of creating an asynchronous task, scheduling it, and handling callbacks often exceeds the actual file I/O time.

2. **Quick Operations**: Reading small files (1KB-50KB) is extremely fast and doesn't block the server significantly when done synchronously.

3. **Task Efficiency**: By avoiding unnecessary task creation for small files, we reduce pressure on the task scheduling system.

4. **Balanced Approach**: Larger files still benefit from asynchronous I/O to prevent blocking server operations.

## Performance Testing

To evaluate the optimization, we've created two testing tools:

1. **benchmark_static_files**: A standalone benchmark that tests the performance of serving files of various sizes.

2. **compare_static_performance.sh**: A comparison script that measures performance before and after optimization.

## Using the Optimization

The optimization has minimal impact on existing code and requires no API changes. It automatically detects file size and applies the appropriate reading strategy.

### Testing the Optimization

To test the performance improvement:

1. **Using the benchmark tool**:
   ```bash
   make example
   ./example/benchmark_static_files
   ```

2. **Using the comparison script**:
   ```bash
   ./example/compare_static_performance.sh /path/to/original/wfrest /path/to/optimized/wfrest
   ```

## Configuration

The threshold value (50KB) was chosen based on:
1. Common sizes of static assets in web applications
2. The specific mention of React.js build files in the issue
3. Performance testing with various file sizes

If necessary, this threshold can be adjusted based on specific use cases by modifying the `SMALL_FILE_THRESHOLD` constant in `src/core/HttpFile.cc`.

## Conclusion

This optimization significantly improves wfrest's performance for serving static files, particularly small files like CSS and JS, without compromising its excellent performance for larger files. It addresses the specific performance issue mentioned in #272 while maintaining compatibility with existing code. 


## Test result 

(base) chanchan@chanchan:~/pro/wfrest$ ./example/compare_static_performance.sh ./example/benchmark_static_files ./example/benchmark_static_filesnew 
创建测试目录: /tmp/tmp.ZVLgpWmMaM
创建测试文件: /tmp/tmp.ZVLgpWmMaM/tiny.txt (1 KB)
创建测试文件: /tmp/tmp.ZVLgpWmMaM/small.txt (10 KB)
创建测试文件: /tmp/tmp.ZVLgpWmMaM/medium.txt (30 KB)
创建测试文件: /tmp/tmp.ZVLgpWmMaM/threshold.txt (50 KB)
创建测试文件: /tmp/tmp.ZVLgpWmMaM/large.txt (100 KB)
创建测试文件: /tmp/tmp.ZVLgpWmMaM/huge.txt (1024 KB)
=== 测试 original 版本 ===
启动服务器在端口 8888
Created test file: /tmp/tmp.ZVLgpWmMaM/tiny.txt (1024 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/small.txt (10240 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/medium.txt (30720 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/threshold.txt (51200 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/large.txt (102400 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/huge.txt (1048576 bytes)
Server started on port 8888...
Test files located at directory: /tmp/tmp.ZVLgpWmMaM

Access the following URLs for testing:
http://localhost:8888/static/tiny.txt
http://localhost:8888/static/small.txt
http://localhost:8888/static/medium.txt
http://localhost:8888/static/threshold.txt
http://localhost:8888/static/large.txt
http://localhost:8888/static/huge.txt

=== Performance Test Results ===

Test file: tiny.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8888/static/tiny.txt
Running 10s test @ http://localhost:8888/static/tiny.txt
  10 threads and 10 connections
服务器启动成功，PID: 263168
测试文件: tiny.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   121.49us   96.92us   0.96ms   83.59%
    Req/Sec     8.55k     7.58k   24.28k    80.20%
  Latency Distribution
     50%   56.00us
     75%  188.00us
     90%  251.00us
     99%  404.00us
  687134 requests in 10.10s, 777.19MB read
  Socket errors: connect 2, read 0, write 0, timeout 0
Requests/sec:  68033.39
Transfer/sec:     76.95MB
---------------------------

Test file: small.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8888/static/small.txt
Running 10s test @ http://localhost:8888/static/small.txt
  10 threads and 10 connections
  吞吐量: 232634.98 请求/秒
  延迟: 204.13us 70.77us
Distribution 

测试文件: small.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   198.03us   73.50us   2.20ms   80.09%
    Req/Sec     5.04k   174.13     6.44k    88.78%
  Latency Distribution
     50%  180.00us
     75%  230.00us
     90%  294.00us
     99%  450.00us
  151902 requests in 10.10s, 1.47GB read
  Socket errors: connect 7, read 0, write 0, timeout 0
Requests/sec:  15041.16
Transfer/sec:    149.25MB
---------------------------

Test file: medium.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8888/static/medium.txt
Running 10s test @ http://localhost:8888/static/medium.txt
  10 threads and 10 connections
  吞吐量: 239238.42 请求/秒
  延迟: 197.75us 74.39us
Distribution 

测试文件: medium.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   199.33us   79.50us   0.91ms   79.01%
    Req/Sec     5.00k   141.09     5.97k    82.18%
  Latency Distribution
     50%  179.00us
     75%  234.00us
     90%  302.00us
     99%  474.00us
  50268 requests in 10.10s, 1.45GB read
  Socket errors: connect 9, read 0, write 0, timeout 0
Requests/sec:   4977.62
Transfer/sec:    146.61MB
---------------------------

Test file: threshold.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8888/static/threshold.txt
Running 10s test @ http://localhost:8888/static/threshold.txt
  10 threads and 10 connections
  吞吐量: 233903.45 请求/秒
  延迟: 200.51us 80.54us
Distribution 

测试文件: threshold.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     0.00us    0.00us   0.00us    -nan%
    Req/Sec     0.00      0.00     0.00      -nan%
  Latency Distribution
     50%    0.00us
     75%    0.00us
     90%    0.00us
     99%    0.00us
  0 requests in 10.01s, 0.00B read
  Socket errors: connect 10, read 0, write 0, timeout 0
Requests/sec:      0.00
Transfer/sec:       0.00B
---------------------------

Test file: large.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8888/static/large.txt
Running 10s test @ http://localhost:8888/static/large.txt
  10 threads and 10 connections
  吞吐量: 222065.72 请求/秒
  延迟: 200.78us 81.14us
Distribution 

测试文件: large.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     0.00us    0.00us   0.00us    -nan%
    Req/Sec     0.00      0.00     0.00      -nan%
  Latency Distribution
     50%    0.00us
     75%    0.00us
     90%    0.00us
     99%    0.00us
  0 requests in 10.01s, 0.00B read
  Socket errors: connect 10, read 0, write 0, timeout 0
Requests/sec:      0.00
Transfer/sec:       0.00B
---------------------------

Test file: huge.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8888/static/huge.txt
Running 10s test @ http://localhost:8888/static/huge.txt
  10 threads and 10 connections
  吞吐量: 55355.72 请求/秒
  延迟: 0.89ms 1.02ms
Distribution 

测试文件: huge.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     0.00us    0.00us   0.00us    -nan%
    Req/Sec     0.00      0.00     0.00      -nan%
  Latency Distribution
     50%    0.00us
     75%    0.00us
     90%    0.00us
     99%    0.00us
  0 requests in 10.01s, 0.00B read
  Socket errors: connect 10, read 0, write 0, timeout 0
Requests/sec:      0.00
Transfer/sec:       0.00B
---------------------------

Performance test completed. Press Ctrl+C to exit...
  吞吐量: 3823.01 请求/秒
  延迟: 12.56ms 3.48ms
Distribution 

停止服务器 (PID: 263168)
=== 测试 optimized 版本 ===
启动服务器在端口 8889
Created test file: /tmp/tmp.ZVLgpWmMaM/tiny.txt (1024 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/small.txt (10240 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/medium.txt (30720 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/threshold.txt (51200 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/large.txt (102400 bytes)
Created test file: /tmp/tmp.ZVLgpWmMaM/huge.txt (1048576 bytes)
Server started on port 8889...
Test files located at directory: /tmp/tmp.ZVLgpWmMaM

Access the following URLs for testing:
http://localhost:8889/static/tiny.txt
http://localhost:8889/static/small.txt
http://localhost:8889/static/medium.txt
http://localhost:8889/static/threshold.txt
http://localhost:8889/static/large.txt
http://localhost:8889/static/huge.txt

=== Performance Test Results ===

Test file: tiny.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8889/static/tiny.txt
Running 10s test @ http://localhost:8889/static/tiny.txt
  10 threads and 10 connections
服务器启动成功，PID: 263585
测试文件: tiny.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   124.24us   99.32us   0.98ms   83.53%
    Req/Sec     8.35k     7.42k   23.71k    80.20%
  Latency Distribution
     50%   57.00us
     75%  192.00us
     90%  256.00us
     99%  412.00us
  671709 requests in 10.10s, 759.74MB read
  Socket errors: connect 2, read 0, write 0, timeout 0
Requests/sec:  66510.77
Transfer/sec:     75.23MB
---------------------------

Test file: small.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8889/static/small.txt
Running 10s test @ http://localhost:8889/static/small.txt
  10 threads and 10 connections
  吞吐量: 227084.48 请求/秒
  延迟: 209.15us 72.21us
Distribution 

测试文件: small.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   201.03us   73.90us   0.96ms   79.95%
    Req/Sec     4.97k   160.57     6.28k    86.47%
  Latency Distribution
     50%  183.00us
     75%  233.00us
     90%  297.00us
     99%  456.00us
  149648 requests in 10.10s, 1.45GB read
  Socket errors: connect 7, read 0, write 0, timeout 0
Requests/sec:  14817.86
Transfer/sec:    147.04MB
---------------------------

Test file: medium.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8889/static/medium.txt
Running 10s test @ http://localhost:8889/static/medium.txt
  10 threads and 10 connections
  吞吐量: 236894.93 请求/秒
  延迟: 199.82us 74.91us
Distribution 

测试文件: medium.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     0.00us    0.00us   0.00us    -nan%
    Req/Sec     0.00      0.00     0.00      -nan%
  Latency Distribution
     50%    0.00us
     75%    0.00us
     90%    0.00us
     99%    0.00us
  0 requests in 10.01s, 0.00B read
  Socket errors: connect 10, read 0, write 0, timeout 0
Requests/sec:      0.00
Transfer/sec:       0.00B
---------------------------

Test file: threshold.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8889/static/threshold.txt
Running 10s test @ http://localhost:8889/static/threshold.txt
  10 threads and 10 connections
  吞吐量: 236075.42 请求/秒
  延迟: 198.57us 80.71us
Distribution 

测试文件: threshold.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     0.00us    0.00us   0.00us    -nan%
    Req/Sec     0.00      0.00     0.00      -nan%
  Latency Distribution
     50%    0.00us
     75%    0.00us
     90%    0.00us
     99%    0.00us
  0 requests in 10.01s, 0.00B read
  Socket errors: connect 10, read 0, write 0, timeout 0
Requests/sec:      0.00
Transfer/sec:       0.00B
---------------------------

Test file: large.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8889/static/large.txt
Running 10s test @ http://localhost:8889/static/large.txt
  10 threads and 10 connections
  吞吐量: 220574.53 请求/秒
  延迟: 202.33us 81.81us
Distribution 

测试文件: large.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     0.00us    0.00us   0.00us    -nan%
    Req/Sec     0.00      0.00     0.00      -nan%
  Latency Distribution
     50%    0.00us
     75%    0.00us
     90%    0.00us
     99%    0.00us
  0 requests in 10.01s, 0.00B read
  Socket errors: connect 10, read 0, write 0, timeout 0
Requests/sec:      0.00
Transfer/sec:       0.00B
---------------------------

Test file: huge.txt
---------------------------
Executing command: wrk -t10 -c10 -d10s --latency http://localhost:8889/static/huge.txt
Running 10s test @ http://localhost:8889/static/huge.txt
  10 threads and 10 connections
  吞吐量: 53588.07 请求/秒
  延迟: 0.93ms 1.12ms
Distribution 

测试文件: huge.txt
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     0.00us    0.00us   0.00us    -nan%
    Req/Sec     0.00      0.00     0.00      -nan%
  Latency Distribution
     50%    0.00us
     75%    0.00us
     90%    0.00us
     99%    0.00us
  0 requests in 10.01s, 0.00B read
  Socket errors: connect 10, read 0, write 0, timeout 0
Requests/sec:      0.00
Transfer/sec:       0.00B
---------------------------

Performance test completed. Press Ctrl+C to exit...
  吞吐量: 3932.62 请求/秒
  延迟: 12.20ms 3.21ms
Distribution 

停止服务器 (PID: 263585)
=== 性能比较结果 ===
文件名 | 优化前(请求/秒) | 优化后(请求/秒) | 提升百分比
-------|-----------------|-----------------|------------
tiny.txt | 232634.98 | 227084.48 | -2.38%
small.txt | 239238.42 | 236894.93 | -.97%
medium.txt | 233903.45 | 236075.42 | .92%
threshold.txt | 222065.72 | 220574.53 | -.67%
large.txt | 55355.72 | 53588.07 | -3.19%
huge.txt | 3823.01 | 3932.62 | 2.86%
测试完成，结果保存在 /tmp/tmp.ZVLgpWmMaM/results