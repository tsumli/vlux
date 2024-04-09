#!/bin/bash
cargo run --manifest-path tools/shader_compiler/Cargo.toml --bin compile --release -- -s ./src/shader -o ./processed $1