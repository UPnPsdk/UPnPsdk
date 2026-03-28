#!/bin/sh
g++ -std=c++23 -Wall -Wpedantic -Wextra -Werror -Wuninitialized -Wsuggest-override -Wdeprecated -I ../inc -I ../../upnp/inc test_document.cpp ../../../build/lib/libupnpsdk-pupnp.a -o test_document-pst
g++ -std=c++23 -Wall -Wpedantic -Wextra -Werror -Wuninitialized -Wsuggest-override -Wdeprecated -I ../inc -I ../../upnp/inc test_document.cpp ../../../build/lib/libupnpsdk-compa.a -o test_document-cst
g++ -std=c++23 -Wall -Wpedantic -Wextra -Werror -Wuninitialized -Wsuggest-override -Wdeprecated -I ../inc -I ../../upnp/inc poc_gh_506.cpp ../../../build/lib/libupnpsdk-pupnp.a -o test_poc_gh_506-pst

echo "\nRunning Pupnp test_document-pst\n###############################"
./test_document-pst $(find testdata -name *.xml)
echo "\nRunning Compa test_document-cst\n###############################"
./test_document-cst $(find testdata -name *.xml)
echo "\nRunning Pupnp test_poc_gh_506-pst\n#################################"
./test_poc_gh_506-pst
