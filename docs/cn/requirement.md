## 依赖要求

* workflow，版本v0.9.9或更新
* Linux，如ubuntu 18.04或更新版本
* Cmake
* zlib1g-dev
* libssl-dev
* libgtest-dev
* gcc和g++或llvm + clang，在ubuntu 20.04上测试通过

如果您使用的是ubuntu 20.04，可以通过以下命令安装这些依赖：

```bash
apt-get install build-essential cmake zlib1g-dev libssl-dev libgtest-dev -y
```

您需要检查libpthread.so是否存在，如果不存在，您可能会看到错误日志：
```bash
cannot find -lpthread.so
```

对于amd64，如果不存在，您可能需要创建一个符号链接：
```bash
ln -s /lib/x86_64-linux-gnu/libpthread.so.0 /lib/x86_64-linux-gnu/libpthread.so
```

对于ARM64，您可以尝试：
```bash
ln -s /lib/aarch64-linux-gnu/libpthread.so.0 /lib/aarch64-linux-gnu/libpthread.so
``` 