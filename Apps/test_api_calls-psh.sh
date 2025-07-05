#!/usr/bin/env -S bash -e

# There is one 'api_calls.c' source file that uses only C statements so it can
# be compiled to a C program, and a C++ program. To verify the SDKs API the
# binary executable must successfuly run with derived libraries as follows.
#
# Executable 'api_calls-psh' compiled with UPnPsdk_pupnp (C++) library:
# ---------------------------------------------------------------------
# must run with its 'UPnPsdk_pupnp' (C++) library
if [ -f build/lib/libupnpsdk-pupnp.so.17 ]; then
    echo "Start: test executable 'api_calls-psh' with its UPnPsdk_NATIVE_PUPNP library."
    build/bin/api_calls-psh
    echo -e "Finished: test executable 'api_calls-psh' with its UPnPsdk_NATIVE_PUPNP library: finished.\n"

    # and must also run with the 'UPnPsdk_compa' (C++) library
    echo  "Start: test executable 'api_calls-psh' with UPnPsdk_compatible library."
    rm -rf build/Apps/api_calls/
    mkdir build/Apps/api_calls/

    (cd build/Apps/api_calls/ && \
    ln -s ../../lib/libupnpsdk-compa.so.0 libupnpsdk-pupnp.so.17)
    LD_LIBRARY_PATH=build/Apps/api_calls build/bin/api_calls-psh
    echo "Finished: test executable 'api_calls-psh' with UPnPsdk_compatible library: finished."
fi

echo "Finished $(basename "${BASH_SOURCE[0]}")"
