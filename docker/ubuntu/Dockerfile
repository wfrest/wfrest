FROM ubuntu:18.04
MAINTAINER Chanchan <ysyfrank@gmail.com>

WORKDIR /home/project

RUN sed -i s@/archive.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list \
    && apt-get clean && apt-get update \
    && apt-get install -y vim git g++ cmake make libssl-dev \
    && git clone https://github.com/chanchann/wfrest.git \
    && cd wfrest && mkdir build && cd build \
    && cmake .. && make 
