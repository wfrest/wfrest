FROM ubuntu:latest
MAINTAINER Chanchan <ysyfrank@gmail.com>

WORKDIR /home/project
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y vim git g++ cmake make libssl-dev libgtest-dev zlib1g-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/* \
    && git clone https://github.com/sogou/workflow.git \ 
    && cd workflow && make -j && make install \
    && cd .. && rm -rf workflow \ 
    && git clone https://github.com/wfrest/wfrest.git \
    && cd wfrest && mkdir build && cd build \
    && cmake .. && make -j && make install 
