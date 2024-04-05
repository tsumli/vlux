#!/bin/bash
SIDPLAY=:0 docker compose -f docker/docker-compose.yaml run --service-ports $1
