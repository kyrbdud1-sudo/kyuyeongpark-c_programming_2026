FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

# 시스템 패키지 업데이트 및 기본 도구 설치
RUN apt-get update && apt-get install -y \
    curl \
    wget \
    git \
    git-flow \
    vim \
    nano \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# C/C++ 개발 도구 설치
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    gdb \
    make \
    cmake \
    clang \
    clang-format \
    clangd \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace