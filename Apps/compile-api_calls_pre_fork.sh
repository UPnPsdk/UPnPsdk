#!/usr/bin/bash

PUPNP_BASE_DIR="../../pupnp-dev/pupnp"

# api_calls_pre_fork
/usr/bin/gcc \
-Wall -Wpedantic -Wextra -Werror -Wuninitialized -Wdeprecated \
-I"$PUPNP_BASE_DIR"/build/upnp/inc \
-I"$PUPNP_BASE_DIR"/upnp/inc \
-I"$PUPNP_BASE_DIR"/ixml/inc \
Apps/src/api_calls.c \
-oApps/bin/api_calls_pre_fork-psh \
-L"$PUPNP_BASE_DIR"/build/ixml \
-L"$PUPNP_BASE_DIR"/build/upnp \
-lixml -lupnp

#-Wl,-rpath,"$PUPNP_BASE_DIR"/build/upnp,--enable-new-dtags \
