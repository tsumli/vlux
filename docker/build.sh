#!/bin/bash
NAME="vulkan-rendering"
docker build \
    -t $NAME:latest \
    --file docker/Dockerfile \
     --network host \
     --build-arg USER=$(whoami) \
     .