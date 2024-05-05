FROM nvidia/cuda:12.4.1-cudnn-devel-ubuntu22.04
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
    libvulkan-dev \
    software-properties-common \
    unzip \
    vim \
    ninja-build \
    xserver-xorg \
    libx11-dev \
    zlib1g-dev \
    libxext-dev \
    xdg-user-dirs \
    libgl1-mesa-dev \
    freeglut3-dev \
    mesa-utils \
    vulkan-tools \
    libvulkan-dev \
    libglfw3-dev \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

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

COPY docker/nvidia_icd.json /usr/share/vulkan/icd.d/
ARG XDG_RUNTIME_DIR="/tmp/xdg_runtime_dir"
RUN mkdir -p ${XDG_RUNTIME_DIR} && chmod 777 ${XDG_RUNTIME_DIR}

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
ARG GROUP
ARG UID=1000
ARG GID=1000
RUN groupadd -g $GID $GROUP
RUN useradd -m -u $UID -g $GID $USER && \
        echo "$USER:$GID" | chpasswd && \
        usermod --shell /bin/bash $USER && \
        usermod -aG sudo $USER && \
        mkdir /etc/sudoers.d && \
        echo "$USER ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/$USER && \
        chmod 0440 /etc/sudoers.d/$USER && \
        usermod  --uid $UID $USER && \
        groupmod --gid $GID $USER
RUN apt install sudo
USER ${USER}
ENV HOME /home/${USER}

# install rust
RUN curl https://sh.rustup.rs | sh -s -- --default-toolchain nightly -y
ENV PATH=/home/${USER}/.cargo/bin:${PATH}
RUN rustup update
RUN rustup install nightly
RUN rustup component add clippy rls rust-analysis rust-src rust-docs rustfmt rust-analyzer

# post process
SHELL ["/bin/bash", "-l", "-c"]
RUN echo "export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/nvidia_icd.json" >> ~/.bashrc
RUN echo "export XDG_RUNTIME_DIR=${XDG_RUNTIME_DIR}" >> ~/.bashrc
ENV NVIDIA_VISIBLE_DEVICES ${NVIDIA_VISIBLE_DEVICES:-all}
ENV NVIDIA_DRIVER_CAPABILITIES ${NVIDIA_DRIVER_CAPABILITIES:+$NVIDIA_DRIVER_CAPABILITIES,}graphics
