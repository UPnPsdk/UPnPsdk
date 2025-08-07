#!/usr/bin/env -S bash -e

# There is one 'api_calls.c' source file that uses only C statements so it can
# be compiled to a C program, and a C++ program. To verify the SDKs API the
# binary executable must successfuly run with derived libraries as follows.
#
# Executable 'api_calls-csh' compiled with UPnPsdk_compa (C++) library:
# ---------------------------------------------------------------------
# must run with its 'UPnPsdk_compa' (C++) library
echo "Start: test executable 'api_calls-csh' with its UPnPsdk compatible library."
build/bin/api_calls-csh
echo "Finished: test executable 'api_calls-csh' with its UPnPsdk compatible library."

echo "Finished $(basename "${BASH_SOURCE[0]}")"
