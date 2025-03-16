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