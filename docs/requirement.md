## Requirement

* workflow, version v0.9.9 or newer
* Linux , like ubuntu 18.04 or newer
* Cmake
* zlib1g-dev
* libssl-dev
* libgtest-dev
* gcc  and g++ or llvm + clang, tested with ubuntu 20.04

If you are on ubuntu 20.04, you may install them by command:

```bash
apt-get install build-essential cmake zlib1g-dev libssl-dev libgtest-dev -y
```

you need check if the libpthread.so is exists, if not exists, you can see error logs: 
```bash
cannot find -lpthread.so
```

for amd64 , if not exists, you may want to ln -s it.
```bash
ln -s /lib/x86_64-linux-gnu/libpthread.so.0 /lib/x86_64-linux-gnu/libpthread.so
```

for ARM64 , you can try 
```bash
ln -s /lib/aarch64-linux-gnu/libpthread.so.0 /lib/aarch64-linux-gnu/libpthread.so
```