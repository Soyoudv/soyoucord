#!/usr/bin/env bash
set -e

if gcc client.c -lreadline -lpthread -o client && gcc server.c -o server; then
    echo "Compilation successful."
else
    echo "Compilation failed."
    exit 1
fi