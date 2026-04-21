#!/bin/bash

# comment if wanna use it via vim's make command
#set -xe

BIN_FOLDER=bin

mkdir -p $BIN_FOLDER

clang -Wall -Wextra -g -o $BIN_FOLDER/tatr main.c
