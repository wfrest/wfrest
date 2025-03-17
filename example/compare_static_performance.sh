#!/bin/bash

# For comparing performance differences of wfrest static file service before and after optimization
# Usage: ./compare_static_performance.sh [pre-optimization wfrest path] [post-optimization wfrest path]

set -e

ORIGINAL_PATH=$1
OPTIMIZED_PATH=$2

if [ -z "$ORIGINAL_PATH" ] || [ -z "$OPTIMIZED_PATH" ]; then
    echo "Usage: $0 [pre-optimization wfrest path] [post-optimization wfrest path]"
    exit 1
fi

# Check wrk tool
which wrk > /dev/null 2>&1 || { 
    echo "Error: 'wrk' tool not found, please install it"
    echo "  On Debian/Ubuntu: sudo apt-get install wrk"
    echo "  On macOS: brew install wrk"
    exit 1
}

# Create temporary directory
TEST_DIR=$(mktemp -d)
echo "Creating test directory: $TEST_DIR"

# Function: Generate test files
generate_test_files() {
    sizes=("1" "10" "30" "50" "100" "1024")
    names=("tiny.txt" "small.txt" "medium.txt" "threshold.txt" "large.txt" "huge.txt")
    
    for i in "${!sizes[@]}"; do
        size=${sizes[$i]}
        name=${names[$i]}
        path="$TEST_DIR/$name"
        
        # Generate file of specified size (in KB)
        dd if=/dev/urandom of="$path" bs=1024 count="$size" 2>/dev/null
        echo "Created test file: $path ($size KB)"
    done
}

# Function: Test single version
test_version() {
    version_path=$1
    version_name=$2
    port=$3
    
    echo "=== Testing $version_name version ==="
    
    # Build and start server (assuming there's a simple startup script)
    cd "$version_path"
    echo "Building..."
    make -j$(nproc) 28_benchmark_static_files
    
    # Start server (run in background)
    echo "Starting server on port $port"
    ./28_benchmark_static_files --port $port --dir $TEST_DIR &
    server_pid=$!
    
    # Wait for server to start
    sleep 2
    
    # Get process status, ensure server is running
    if ! kill -0 $server_pid 2>/dev/null; then
        echo "Server failed to start!"
        exit 1
    fi
    
    echo "Server started successfully, PID: $server_pid"
    
    # Test file list
    files=("tiny.txt" "small.txt" "medium.txt" "threshold.txt" "large.txt" "huge.txt")
    
    # Create results directory
    mkdir -p "$TEST_DIR/results"
    
    # Run tests for each file
    for file in "${files[@]}"; do
        echo "Testing file: $file"
        result_file="$TEST_DIR/results/${version_name}_${file}.txt"
        
        # Run wrk test
        wrk -t4 -c50 -d10s --latency http://localhost:$port/static/$file > "$result_file"
        
        # Extract and display main results
        requests=$(grep "Requests/sec" "$result_file" | awk '{print $2}')
        latency=$(grep "Latency" "$result_file" | awk '{print $2" "$3}')
        
        echo "  Throughput: $requests requests/sec"
        echo "  Latency: $latency"
        echo ""
    done
    
    # Stop server
    echo "Stopping server (PID: $server_pid)"
    kill $server_pid
    wait $server_pid 2>/dev/null || true
}

# Function: Compare results
compare_results() {
    echo "=== Performance Comparison Results ==="
    echo "Filename | Pre-optimization(req/sec) | Post-optimization(req/sec) | Improvement %"
    echo "---------|---------------------------|---------------------------|-------------"
    
    files=("tiny.txt" "small.txt" "medium.txt" "threshold.txt" "large.txt" "huge.txt")
    
    for file in "${files[@]}"; do
        original_file="$TEST_DIR/results/original_${file}.txt"
        optimized_file="$TEST_DIR/results/optimized_${file}.txt"
        
        original_rps=$(grep "Requests/sec" "$original_file" | awk '{print $2}')
        optimized_rps=$(grep "Requests/sec" "$optimized_file" | awk '{print $2}')
        
        # Calculate improvement percentage
        if [ $(echo "$original_rps > 0" | bc) -eq 1 ]; then
            improvement=$(echo "scale=2; ($optimized_rps - $original_rps) * 100 / $original_rps" | bc)
            echo "$file | $original_rps | $optimized_rps | ${improvement}%"
        else
            echo "$file | $original_rps | $optimized_rps | Cannot calculate"
        fi
    done
}

# Main process
generate_test_files

# Test original version
test_version "$ORIGINAL_PATH" "original" 8888

# Test optimized version
test_version "$OPTIMIZED_PATH" "optimized" 8889

# Compare results
compare_results

# Clean up
echo "Tests completed, results saved in $TEST_DIR/results"
echo "Clean up test files? (y/n)"
read clean_up

if [ "$clean_up" = "y" ]; then
    rm -rf "$TEST_DIR"
    echo "Test files cleaned up"
else
    echo "Test files retained in $TEST_DIR"
fi

exit 0 