FROM ubuntu:18.04
MAINTAINER Chanchan <ysyfrank@gmail.com>

WORKDIR /home/project

RUN RUN sed -i s@/archive.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list \
    && apt-get clean && apt-get update \
    && apt-get install -y vim git g++ cmake make libssl-dev libgtest-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/* \
    && git clone https://github.com/sogou/workflow.git \ 
    && cd workflow && make && make install \
    && cd .. && rm -rf workflow \ 
    && git clone https://github.com/chanchann/wfrest.git \
    && cd wfrest && mkdir build && cd build \
    && cmake .. && make && make install 
