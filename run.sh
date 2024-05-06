#!/bin/bash
#
XAUTH=/tmp/.docker.xauth
touch $XAUTH
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -
DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY docker compose -f docker/docker-compose.yaml run --service-ports $1
