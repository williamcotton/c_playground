FROM mcr.microsoft.com/devcontainers/base:ubuntu

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends openssl libssl-dev git clang-format clang-tidy clang-tools clang clangd libc++-dev libc++1 libc++abi-dev \
    libc++abi1 libclang-dev libclang1 liblldb-dev libomp-dev libomp5 lld lldb llvm-dev llvm-runtime llvm python3-clang libcurl4-openssl-dev libblocksruntime-dev \
    libkqueue-dev libpthread-workqueue-dev git build-essential python3 cmake ninja-build systemtap-sdt-dev libbsd-dev linux-libc-dev apache2-utils

WORKDIR /vscode
RUN pwd
RUN git clone https://github.com/apple/swift-corelibs-libdispatch.git
WORKDIR /vscode/swift-corelibs-libdispatch
RUN ls -al
RUN cmake -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .
RUN ninja install
