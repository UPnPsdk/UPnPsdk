#!/usr/bin/env -S bash -e

# There is one 'api_calls.c' source file that uses only C statements so it can
# be compiled to a C program, and a C++ program. To verify the SDKs API the
# binary executable must successfuly run with derived libraries as follows.
#
# Executable 'api_calls_pre_fork-psh' compiled with old libupnp (C) library:
# --------------------------------------------------------------------------
# must run with its old libupnp (C) library
echo "Start: test executable 'api_calls_pre_fork-psh' with its original libupnp library."
LD_LIBRARY_PATH=Apps/bin Apps/bin/api_calls_pre_fork-psh
echo -e "Finished: test executable 'api_calls_pre_fork-psh' with its original libupnp library.\n"

# and must also run with the 'UPnPsdk_pupnp' (C++) library
if [ -f build/lib/libupnpsdk-pupnp.so.17 ]; then
    echo "Start: test executable 'api_calls_pre_fork-psh' with UPnPsdk_NATIVE_PUPNP library."
    rm -rf build/Apps/api_calls/
    mkdir build/Apps/api_calls/

    (cd build/Apps/api_calls/ && \
    ln -s ../../lib/libupnpsdk-pupnp.so.17 libupnp.so.17 && \
    ln -s ../../lib/libupnpsdk-pupnp.so.17 libixml.so.11)
    LD_LIBRARY_PATH=build/Apps/api_calls Apps/bin/api_calls_pre_fork-psh
    echo -e "Finished: test executable 'api_calls_pre_fork-psh' with UPnPsdk_NATIVE_PUPNP library.\n"
fi

# and must also run with the 'UPnPsdk_compa' (C++) library
echo "Start: test executable 'api_calls_pre_fork-psh' with UPnPsdk compatible library."
rm -rf build/Apps/api_calls/
mkdir build/Apps/api_calls/

(cd build/Apps/api_calls/ && \
ln -s ../../lib/libupnpsdk-compa.so.0 libupnp.so.17 && \
ln -s ../../lib/libupnpsdk-compa.so.0 libixml.so.11)
LD_LIBRARY_PATH=build/Apps/api_calls Apps/bin/api_calls_pre_fork-psh
echo "Finished: test executable 'api_calls_pre_fork-psh' with UPnPsdk compatible library."

echo "Finished $(basename "${BASH_SOURCE[0]}")"
