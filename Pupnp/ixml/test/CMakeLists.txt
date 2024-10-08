# Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2024-09-09

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(PUPNP_IXML_TEST VERSION 0005
                   DESCRIPTION "IXML library tests"
                   HOMEPAGE_URL "https://github.com/UPnPsdk")


#file (GLOB XML_FILES
#	${PROJECT_SOURCE_DIR}/testdata/*.xml
#)

# ixml test executable linked with shared library
#------------------------------------------------
add_executable (test_ixml_shared
	test_document.cpp
)
target_include_directories(test_ixml_shared
    # Next variable is needed for one step configure. Don't use
    # PRIVATE ${PUPNP_UPNP_SOURCE_DIR}/inc
    PRIVATE ${PUPNP_SOURCE_DIR}/upnp/inc
)
target_compile_definitions (test_ixml_shared
    PRIVATE
        $<$<BOOL:${script_support}>:IXML_HAVE_SCRIPTSUPPORT>
        $<$<CONFIG:Debug>:DEBUG>
        UPnPsdk_SHARE
)
target_link_libraries (test_ixml_shared
    PRIVATE
        ixml_shared
)

#add_test (NAME test_ixml
#	COMMAND test_ixml_shared ${XML_FILES}
#)

#set_tests_properties (test_ixml PROPERTIES
#	ENVIRONMENT "PATH=$<TARGET_FILE_DIR:ixml_shared>\;%PATH%"
#)


# ixml test executable linked with static library
#------------------------------------------------
add_executable (test_ixml_static
	test_document.cpp
)
target_include_directories(test_ixml_static
    # Next variable is needed for one step configure. Don't use
    # PRIVATE ${PUPNP_UPNP_SOURCE_DIR}/inc
    PRIVATE ${PUPNP_SOURCE_DIR}/upnp/inc
)
target_compile_definitions (test_ixml_static
    PRIVATE
        $<$<BOOL:${script_support}>:IXML_HAVE_SCRIPTSUPPORT>
        $<$<CONFIG:Debug>:DEBUG>
)
target_link_libraries(test_ixml_static
    PRIVATE
        ixml_static
)

#add_test (NAME test_ixml_static
#	COMMAND test_ixml_static ${XML_FILES}
#)
