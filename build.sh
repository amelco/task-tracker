#!/bin/bash

# comment if wanna use it via vim's make command
#set -xe

mkdir -p build

clang -Wall -Wextra -g -o build/tatr main.c
