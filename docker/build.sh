#!/bin/bash
NAME="vulkan-rendering"
docker build \
    -t $NAME:latest \
    --file docker/Dockerfile \
    --network host \
    --no-cache \
    --build-arg USER=$(id -un) \
    --build-arg GROUP=$(id -gn) \
    .
