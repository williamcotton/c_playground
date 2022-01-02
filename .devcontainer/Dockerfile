# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.208.0/containers/ubuntu/.devcontainer/base.Dockerfile

# [Choice] Ubuntu version (use hirsuite or bionic on local arm64/Apple Silicon): hirsute, focal, bionic
ARG VARIANT="hirsute"
# FROM mcr.microsoft.com/vscode/devcontainers/base:0-${VARIANT}
FROM ubuntu:${VARIANT}

# [Optional] Uncomment this section to install additional OS packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends clang-format clang-tidy clang-tools clang clangd libc++-dev libc++1 libc++abi-dev \
    libc++abi1 libclang-dev libclang1 liblldb-dev libomp-dev libomp5 lld lldb llvm-dev llvm-runtime llvm python-clang libcurl4-openssl-dev libblocksruntime-dev \
    libkqueue-dev libpthread-workqueue-dev git build-essential python cmake

WORKDIR /vscode
RUN pwd
RUN git clone git://github.com/nickhutchinson/libdispatch.git
WORKDIR /vscode/libdispatch
RUN ls -al
RUN ./configure
RUN make
RUN sudo make install
