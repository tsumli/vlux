FROM nvidia/cuda:12.2.2-cudnn8-devel-ubuntu22.04
SHELL ["/bin/bash", "-c"]

RUN apt update
RUN apt upgrade -y

ENV DEBIAN_FRONTEND=noninteractive
RUN apt install -y \
    clangd \
    file \
    flex \
    git \
    wget \
    curl \
    cmake \
    software-properties-common \
    unzip \
    vim \
    ninja-build \
    xserver-xorg

# install gcc/gdb
RUN wget https://github.com/gcc-mirror/gcc/archive/refs/tags/releases/gcc-13.2.0.tar.gz &&\
    tar xvf gcc-13.2.0.tar.gz
WORKDIR /gcc-releases-gcc-13.2.0
RUN ./contrib/download_prerequisites
RUN mkdir build && \
    cd build && \
    ../configure --enable-languages=c,c++ --prefix=/usr/local --disable-bootstrap --disable-multilib && \
    make > /dev/null && \
    make install > /dev/null
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt update
RUN apt install --only-upgrade -y libstdc++6
RUN apt install -y gdb

# doxygen
RUN apt install -y doxygen graphviz

# install vulkan
RUN wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | tee /etc/apt/trusted.gpg.d/lunarg.asc
RUN wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list http://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
RUN apt update
RUN apt install -y vulkan-sdk

## tools
RUN apt install -y \
    dpkg-dev \
    libvulkan1-dbgsym \
    vulkan-tools-dbgsym
RUN apt source vulkan-loader vulkan-tools

## glfm/glm/xxf86vm/xi
RUN apt install -y \
    libglfw3-dev \
    libglm-dev \
    libxxf86vm-dev \
    libxi-dev

# update cmake
# https://apt.kitware.com/
RUN apt-get update
RUN apt-get install ca-certificates gpg wget
RUN test -f /usr/share/doc/kitware-archive-keyring/copyright || \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | \
    tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | \
    tee /etc/apt/sources.list.d/kitware.list >/dev/null
RUN apt-get update
RUN apt-get install -y cmake

# modify permission
ARG USER
ARG UID=1000
ARG GID=1000
RUN groupadd -g $GID $USER && \
    useradd -m -u $UID -g $GID $USER
USER ${USER}
ENV HOME /home/${USER}

# install rust
RUN curl https://sh.rustup.rs | sh -s -- --default-toolchain nightly -y
ENV PATH=/home/${USER}/.cargo/bin:${PATH}
RUN rustup update
RUN rustup install nightly
RUN rustup component add clippy rls rust-analysis rust-src rust-docs rustfmt rust-analyzer
