#!/bin/bash

# 用于比较wfrest静态文件服务性能优化前后的差异
# 使用方法: ./compare_static_performance.sh [优化前的二进制文件] [优化后的二进制文件]

set -e

ORIGINAL_BIN=$1
OPTIMIZED_BIN=$2

if [ -z "$ORIGINAL_BIN" ] || [ -z "$OPTIMIZED_BIN" ]; then
    echo "使用方法: $0 [优化前的二进制文件] [优化后的二进制文件]"
    exit 1
fi

# 检查二进制文件是否存在且可执行
if [ ! -f "$ORIGINAL_BIN" ] || [ ! -x "$ORIGINAL_BIN" ]; then
    echo "错误: 优化前的二进制文件不存在或不可执行: $ORIGINAL_BIN"
    exit 1
fi

if [ ! -f "$OPTIMIZED_BIN" ] || [ ! -x "$OPTIMIZED_BIN" ]; then
    echo "错误: 优化后的二进制文件不存在或不可执行: $OPTIMIZED_BIN"
    exit 1
fi

# 检查wrk工具
which wrk > /dev/null 2>&1 || { 
    echo "错误: 未找到'wrk'工具，请安装它"
    echo "  在Debian/Ubuntu上: sudo apt-get install wrk"
    echo "  在macOS上: brew install wrk"
    exit 1
}

# 创建临时目录
TEST_DIR=$(mktemp -d)
echo "创建测试目录: $TEST_DIR"

# 函数：生成测试文件
generate_test_files() {
    sizes=("1" "10" "30" "50" "100" "1024")
    names=("tiny.txt" "small.txt" "medium.txt" "threshold.txt" "large.txt" "huge.txt")
    
    for i in "${!sizes[@]}"; do
        size=${sizes[$i]}
        name=${names[$i]}
        path="$TEST_DIR/$name"
        
        # 生成指定大小的文件（以KB为单位）
        dd if=/dev/urandom of="$path" bs=1024 count="$size" 2>/dev/null
        echo "创建测试文件: $path ($size KB)"
    done
}

# 函数：测试单个版本
test_version() {
    binary_path=$1
    version_name=$2
    port=$3
    
    echo "=== 测试 $version_name 版本 ==="
    
    # 启动服务器（后台运行）
    echo "启动服务器在端口 $port"
    $binary_path --port $port --dir $TEST_DIR &
    server_pid=$!
    
    # 等待服务器启动
    sleep 2
    
    # 获取进程状态，确保服务器正在运行
    if ! kill -0 $server_pid 2>/dev/null; then
        echo "服务器启动失败!"
        exit 1
    fi
    
    echo "服务器启动成功，PID: $server_pid"
    
    # 测试文件列表
    files=("tiny.txt" "small.txt" "medium.txt" "threshold.txt" "large.txt" "huge.txt")
    
    # 创建结果目录
    mkdir -p "$TEST_DIR/results"
    
    # 对每个文件运行测试
    for file in "${files[@]}"; do
        echo "测试文件: $file"
        result_file="$TEST_DIR/results/${version_name}_${file}.txt"
        
        # 运行wrk测试
        wrk -t4 -c50 -d10s --latency http://localhost:$port/static/$file > "$result_file"
        
        # 提取并显示主要结果
        requests=$(grep "Requests/sec" "$result_file" | awk '{print $2}')
        latency=$(grep "Latency" "$result_file" | awk '{print $2" "$3}')
        
        echo "  吞吐量: $requests 请求/秒"
        echo "  延迟: $latency"
        echo ""
    done
    
    # 停止服务器
    echo "停止服务器 (PID: $server_pid)"
    kill $server_pid
    wait $server_pid 2>/dev/null || true
}

# 函数：比较结果
compare_results() {
    echo "=== 性能比较结果 ==="
    echo "文件名 | 优化前(请求/秒) | 优化后(请求/秒) | 提升百分比"
    echo "-------|-----------------|-----------------|------------"
    
    files=("tiny.txt" "small.txt" "medium.txt" "threshold.txt" "large.txt" "huge.txt")
    
    for file in "${files[@]}"; do
        original_file="$TEST_DIR/results/original_${file}.txt"
        optimized_file="$TEST_DIR/results/optimized_${file}.txt"
        
        original_rps=$(grep "Requests/sec" "$original_file" | awk '{print $2}')
        optimized_rps=$(grep "Requests/sec" "$optimized_file" | awk '{print $2}')
        
        # 计算提升百分比
        if [ $(echo "$original_rps > 0" | bc) -eq 1 ]; then
            improvement=$(echo "scale=2; ($optimized_rps - $original_rps) * 100 / $original_rps" | bc)
            echo "$file | $original_rps | $optimized_rps | ${improvement}%"
        else
            echo "$file | $original_rps | $optimized_rps | 无法计算"
        fi
    done
}

# 主流程
generate_test_files

# 测试原始版本
test_version "$ORIGINAL_BIN" "original" 8888

# 测试优化版本
test_version "$OPTIMIZED_BIN" "optimized" 8889

# 比较结果
compare_results

# 整理
echo "测试完成，结果保存在 $TEST_DIR/results"
echo "清理测试文件? (y/n)"
read clean_up

if [ "$clean_up" = "y" ]; then
    rm -rf "$TEST_DIR"
    echo "测试文件已清理"
else
    echo "测试文件保留在 $TEST_DIR"
fi

exit 0 