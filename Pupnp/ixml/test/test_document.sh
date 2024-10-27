#!/bin/sh
g++ -std=c++23 -Wall -Wpedantic -Wextra -Werror -Wuninitialized -Wsuggest-override -Wdeprecated -I ../inc -I ../../upnp/inc test_document.cpp ../../../build/lib/libixml.a -o test_document

./test_document $(find testdata -name *.xml)
