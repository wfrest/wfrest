FROM ubuntu:18.04
MAINTAINER author "ysyfrank@gmail.com"

RUN  sed -i s@/archive.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list \
    && apt-get clean && apt-get update \
    && apt-get install -y vim git g++ cmake make libssl-dev \
    && git clone https://github.com/chanchann/wfrest.git \
    && cd wfrest && mkdir build && cd build \
    && cmake .. && make 
