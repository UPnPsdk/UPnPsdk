#!/bin/bash -e

# api_calls-csh
if [ ! -d build/bin ]; then
    mkdir -p build/bin
fi
/usr/bin/g++ -std=c++23 \
-Wall -Wpedantic -Wextra -Werror -Wuninitialized -Wsuggest-override -Wdeprecated \
-Wl,-rpath,build/lib,--enable-new-dtags \
-ICompa/inc \
-IUPnPsdk/include \
-Ibuild/include \
Apps/src/api_calls.c \
-obuild/bin/api_calls-csh \
-Lbuild/lib \
-lupnpsdk-compa
