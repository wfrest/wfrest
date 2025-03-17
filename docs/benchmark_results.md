# wfrest File Cache Benchmark Results

## Overview

The wfrest File Cache Benchmark compares the performance of normal static file serving versus cached static file serving. This benchmark demonstrates the significant performance improvements that can be achieved by using wfrest's file caching system.

## Benchmark Setup

The benchmark tests the following:

- **Normal Static File Serving**: Files are read directly from disk for each request
- **Cached Static File Serving**: Files are stored in memory after first access

The test uses the `wrk` HTTP benchmarking tool to measure:
- **Requests per Second (RPS)**: How many requests the server can handle per second
- **Latency**: The response time in milliseconds

## Benchmark Results

Based on the results shown, we can observe:

| File | Size | Normal Req/s | Normal Latency (ms) | Cached Req/s | Cached Latency (ms) | RPS Speedup | Latency Improvement |
|------|------|--------------|---------------------|--------------|---------------------|-------------|---------------------|
| test_1024.dat | 1.00 KB | 135614.00 | 0.00 | 270770.00 | 0.00 | 2.00x | 1.00x |
| test_10240.dat | 10.00 KB | 123302.00 | 0.00 | 249637.00 | 0.00 | 2.02x | 1.00x |
| test_51200.dat | 50.00 KB | 62879.20 | 0.00 | 186190.00 | 0.00 | 2.13x | 1.00x |
| test_102400.dat | 100.00 KB | 86171.30 | 0.00 | 129868.00 | 0.00 | 1.51x | 1.00x |
| test_512000.dat | 500.00 KB | 11523.40 | 0.00 | 38390.80 | 0.00 | 3.33x | 1.00x |
| test_1048576.dat | 1.00 MB | 4615.52 | 0.00 | 17077.30 | 0.00 | 3.78x | 1.00x |

### Key Findings

1. **Overall Performance Gain**: Cached static file serving consistently outperforms normal static file serving across all file sizes.

2. **RPS Improvements**:
   - Small files (1KB - 50KB): 2.00x - 2.13x performance improvement
   - Medium files (100KB): 1.51x performance improvement
   - Large files (500KB - 1MB): 3.33x - 3.78x performance improvement

3. **Performance by File Size**:
   - The largest performance improvements are seen with larger files (3.78x speedup for 1MB files)
   - Even small files see significant performance gains (2.00x for 1KB files)

## Visual Comparison

The bar chart in the benchmark results clearly illustrates the performance difference between normal and cached file serving:

- Green bars (Cached Static) consistently higher than pink bars (Normal Static)
- The performance difference is particularly noticeable for larger files
- Both methods show decreasing requests per second as file size increases (expected behavior)

## How to Run the Benchmark

The benchmark can be run using the included benchmark tool:

```bash
./30_cache_benchmark 8888 [duration_seconds] [concurrency]
```

Then open http://localhost:8888/ in a web browser to see the interactive benchmark interface.

You can customize:
- Test duration (seconds)
- Concurrency level (number of parallel connections)

## Technical Implementation

The benchmark:
1. Generates test files of various sizes (1KB - 1MB)
2. Uses two routes - one with normal file serving and one with cached file serving
3. Runs wrk benchmarks against both routes for each file
4. Calculates performance metrics (RPS, latency, speedup)

## Conclusion

The benchmark results demonstrate that using wfrest's file caching system provides significant performance improvements for static file serving:

- 2.0x - 3.8x higher requests per second
- Consistent performance gains across all file sizes
- Larger improvements for larger files

This makes cached static file serving an excellent choice for frequently accessed static assets in production environments.

For configuration details and implementation, see the [cached_static_files.md](cached_static_files.md) and [static_file_optimization.md](static_file_optimization.md) documentation. 