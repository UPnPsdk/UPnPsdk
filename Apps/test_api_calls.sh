#!/bin/bash -e

# There is one 'api_calls.c' source file that uses only C statements so it can
# be compiled to a C program, and a C++ program. To verify the SDKs API the
# binary executable must successfuly compile and run with derived libraries as
# follows.
#
# Executable 'api_calls_pre_fork-psh' compiled with old libupnp (C) library:
# --------------------------------------------------------------------------
# must run with its old libupnp (C) library
if [ -d ../../pupnp-dev/pupnp/build/upnp ]; then
    LD_LIBRARY_PATH=../../pupnp-dev/pupnp/build/upnp:../../pupnp-dev/pupnp/build/ixml Apps/bin/api_calls_pre_fork-psh
    echo -e "Test executable 'api_calls_pre_fork-psh' with its original libupnp library: finished.\n"
fi

# and must also run with the 'UPnPsdk_pupnp' (C++) library
if [ -f build/lib/libupnpsdk-pupnp.so.17 ]; then
    rm -rf build/Apps/api_calls/
    mkdir build/Apps/api_calls/

    (cd build/Apps/api_calls/ && \
    ln -s ../../lib/libupnpsdk-pupnp.so.17 libupnp.so.17 && \
    ln -s ../../lib/libupnpsdk-pupnp.so.17 libixml.so.11)
    LD_LIBRARY_PATH=build/Apps/api_calls Apps/bin/api_calls_pre_fork-psh
    echo -e "Test executable 'api_calls_pre_fork-psh' with UPnPsdk_NATIVE_PUPNP library: finished.\n"
fi

# and must also run with the 'UPnPsdk_compa' (C++) library
rm -rf build/Apps/api_calls/
mkdir build/Apps/api_calls/

(cd build/Apps/api_calls/ && \
ln -s ../../lib/libupnpsdk-compa.so.0 libupnp.so.17 && \
ln -s ../../lib/libupnpsdk-compa.so.0 libixml.so.11)
LD_LIBRARY_PATH=build/Apps/api_calls Apps/bin/api_calls_pre_fork-psh
echo -e "Test executable 'api_calls_pre_fork-psh' with UPnPsdk compatible library: finished.\n"


# Executable 'api_calls-psh' compiled with UPnPsdk_pupnp (C++) library:
# ---------------------------------------------------------------------
# must run with its 'UPnPsdk_pupnp' (C++) library
if [ -f build/lib/libupnpsdk-pupnp.so.17 ]; then
    build/bin/api_calls-psh
    echo -e "Test executable 'api_calls-psh' with its UPnPsdk_NATIVE_PUPNP library: finished.\n"

    # and must also run with the 'UPnPsdk_compa' (C++) library
    rm -rf build/Apps/api_calls/
    mkdir build/Apps/api_calls/

    (cd build/Apps/api_calls/ && \
    ln -s ../../lib/libupnpsdk-compa.so.0 libupnp.so.17)
    LD_LIBRARY_PATH=build/Apps/api_calls build/bin/api_calls-psh
    echo -e "Test executable 'api_calls-psh' with UPnPsdk_compatible library: finished.\n"
fi


# Executable 'api_calls-csh' compiled with UPnPsdk_compa (C++) library:
# ---------------------------------------------------------------------
# must run with its 'UPnPsdk_compa' (C++) library
build/bin/api_calls-csh
echo "Test executable 'api_calls-csh' with its UPnPsdk compatible library: finished."
