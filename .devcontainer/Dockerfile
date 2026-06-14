# syntax=docker/dockerfile:1.7

# Candidate takehome C++ development environment.
#
# Base: Standard Ubuntu 24.04 image with Clang 18, CMake, Conan 2, Ninja,
# GoogleTest tooling, and the sanitizers' supporting runtimes.
#
# FIPS-friendly: this image intentionally avoids the apt.llvm.org and
# Kitware HTTPS bootstraps. On FIPS-enabled hosts, OpenSSL 3 inside the
# container can fail TLS handshakes with `curl: (35) Insufficient
# randomness`, because libssl picks up `/proc/sys/crypto/fips_enabled=1`
# from the host kernel. Everything we install therefore comes from the
# Ubuntu noble archive (HTTP) or from PyPI (HTTPS) after we've disabled
# host-FIPS detection via `OPENSSL_FORCE_FIPS_MODE=0`.
FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG CLANG_VERSION=18
ARG CONAN_VERSION=2.7.1

# Disable FIPS detection inside the container for the entire image
# build *and* runtime. Ubuntu's patched OpenSSL honors this env var to
# skip the FIPS DRBG instantiation that fails when no FIPS provider is
# installed.
ENV OPENSSL_FORCE_FIPS_MODE=0

ARG DEBIAN_FRONTEND=noninteractive
ARG CLANG_VERSION=18
ARG CONAN_VERSION=2.7.1

# Disable FIPS detection inside the container for the entire image
# build *and* runtime. Ubuntu's patched OpenSSL honors this env var to
# skip the FIPS DRBG instantiation that fails when no FIPS provider is
# installed.
ENV OPENSSL_FORCE_FIPS_MODE=0

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# --- System setup ---------------------------------------------------------
# Create the non-root vscode user with sudo access
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        sudo \
        git \
    && \
    groupadd -r vscode && \
    useradd -r -g vscode -m -d /home/vscode vscode && \
    echo "vscode ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/vscode && \
    chmod 0440 /etc/sudoers.d/vscode && \
    rm -rf /var/lib/apt/lists/*

# --- System packages ----------------------------------------------------------
# Everything below is in Ubuntu 24.04 noble (main/universe). Clang 18,
# clangd-18, clang-tidy-18, clang-format-18, libc++-18-dev, lld-18,
# lldb-18 are all packaged. CMake 3.28 is new enough for the
# top-level `cmake_minimum_required(VERSION 3.24)`.
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        bash-completion \
        build-essential \
        ca-certificates \
        ccache \
        clang-${CLANG_VERSION} \
        clang-format-${CLANG_VERSION} \
        clang-tidy-${CLANG_VERSION} \
        clang-tools-${CLANG_VERSION} \
        clangd-${CLANG_VERSION} \
        cmake \
        cppcheck \
        curl \
        gdb \
        git \
        gnupg \
        jq \
        libc++-${CLANG_VERSION}-dev \
        libc++abi-${CLANG_VERSION}-dev \
        libclang-rt-${CLANG_VERSION}-dev \
        lld-${CLANG_VERSION} \
        lldb-${CLANG_VERSION} \
        llvm-${CLANG_VERSION} \
        lsb-release \
        make \
        ninja-build \
        pkg-config \
        python3 \
        python3-pip \
        python3-venv \
        pipx \
        strace \
        tree \
        unzip \
        valgrind \
        vim \
        xz-utils \
        zip \
    && \
    rm -rf /var/lib/apt/lists/*

# --- Pin clang/clang++/clang-tidy/clang-format alternatives -------------------
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-${CLANG_VERSION} 100 && \
    update-alternatives --install /usr/bin/llvm-symbolizer llvm-symbolizer /usr/bin/llvm-symbolizer-${CLANG_VERSION} 100

# --- Conan (installed system-wide via pipx, available to every user) ---------
# pipx hits PyPI over HTTPS; this works on FIPS hosts because the env
# var above neutralizes the libssl FIPS code path. If your environment
# blocks PyPI entirely, fetch Conan from your internal mirror and
# `pip install` it instead.
ENV PIPX_HOME=/opt/pipx \
    PIPX_BIN_DIR=/usr/local/bin
RUN pipx install --pip-args="--no-cache-dir" "conan==${CONAN_VERSION}" && \
    conan --version

# --- Non-root user setup ------------------------------------------------------
# The base image already creates `vscode` (uid 1000). Make sure Conan/ccache
# directories are writable for that user without further intervention.
USER vscode
WORKDIR /home/vscode

RUN mkdir -p /home/vscode/.conan2 /home/vscode/.ccache && \
    conan profile detect --force && \
    # Force Clang + libstdc++11 ABI + C++20 in the auto-detected profile so
    # candidates do not have to remember to edit it themselves.
    sed -i -E \
        -e 's/^compiler=.*/compiler=clang/' \
        -e "s/^compiler.version=.*/compiler.version=${CLANG_VERSION}/" \
        -e 's/^compiler.libcxx=.*/compiler.libcxx=libstdc++11/' \
        -e 's/^compiler.cppstd=.*/compiler.cppstd=20/' \
        /home/vscode/.conan2/profiles/default && \
    cat /home/vscode/.conan2/profiles/default

ENV CC=clang \
    CXX=clang++ \
    CMAKE_GENERATOR=Ninja \
    CCACHE_DIR=/home/vscode/.ccache \
    CONAN_HOME=/home/vscode/.conan2 \
    PATH=/home/vscode/.local/bin:${PATH}

# Default shell prompt: keep the base image's behavior so users see the
# familiar VS Code prompt. No CMD override -- the devcontainer runtime
# manages the lifecycle.
