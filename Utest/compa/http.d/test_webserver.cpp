// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-04-28

// Include source code for testing. So we have also direct access to static
// functions which need to be tested.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
#include <Pupnp/upnp/src/genlib/net/http/webserver.cpp>
#else
#include <Compa/src/genlib/net/http/webserver.cpp>
#endif

#include <UPnPsdk/synclog.hpp>
#include <UPnPsdk/upnptools.hpp> // for errStrEx

#include <utest/utest.hpp>

#if false
// The web_server functions call stack
//====================================
/*
     web_server_init()
01)  |__ media_list_init()
02)  |__ membuffer_init()
03)  |__ glob_alias_init()
04)  |__ Initialize callbacks
05)  |__ pthread_mutex_init()

01) Tested with MediaListTestSuite.
    On old code we have a vector 'gMediaTypeList' that contains members of
    document types (document_type_t), that maps a file extension to the content
    type and content subtype of a document. 'gMediaTypeList' is initialized with
    pointers into C-string 'gEncodedMediaTypes' which contains the media types.
    Understood ;-) ? I simplify it on the compatible code. --Ingo
02) Tested with ./test_membuffer.cpp
03) Tested with testsuites XMLalias*
04) Nothing to test. There are only pointers set to nullptr.
05) Nothing to test. This uses pthreads.

Functions to manage the pupnp global XML alias structure 'gAliasDoc'.
---------------------------------------------------------------------
These functions are re-engeneered to become part of the new XML alias structure
as methods. They are tested with the XMLaliasTestSuite. */
static UPNP_INLINE void glob_alias_init();
    // Not needed anymore. This will be done by instantiation of the structure
    // and with its constructor.
int web_server_set_alias();              // Will become method set().
static void alias_grab();                // will become method get().
static UPNP_INLINE int is_valid_alias(); // Will become method is_valid().
static void alias_release();             // Will become method release().
#endif

#ifndef UPnPsdk_WITH_NATIVE_PUPNP
namespace {
/*!
 * \brief Check for the validity of the XML object buffer.
 *
 * \returns **true** if valid alias, **false** otherwise.
 */
UPNP_INLINE bool is_valid_alias(
    /*! [in] XML alias object. */
    const xml_alias_t* alias) {
    TRACE("Executing is_valid_alias()")
    if (alias == nullptr)
        return false;
    return alias->is_valid();
}

} // anonymous namespace
#endif

namespace utest {

using ::testing::_;
using ::testing::ExitedWithCode;
using ::UPnPsdk::errStrEx;


class CEnableTrace {
#ifdef UPnPsdk_WITH_TRACE
  public:
    CEnableTrace() { std::clog.clear(); }
    ~CEnableTrace() { std::clog.setstate(std::ios_base::failbit); }
#endif
};


// Little helper
// =============
class CgWebMutex {
    // There are some Units with mutex locks and unlocks that will throw
    // exceptions on WIN32 if not initialized. I need this class in conjunction
    // with other helper classes to ensure that the mutex is destructed at last.
    // --Ingo
  public:
    CgWebMutex() { pthread_mutex_init(&gWebMutex, NULL); }
    ~CgWebMutex() { pthread_mutex_destroy(&gWebMutex); }
};

class CUpnpFileInfo {
    // Use this simple helper class to ensure to always free an allocated
    // UpnpFileInfo.
  public:
    UpnpFileInfo* info{};

    CUpnpFileInfo() { this->info = UpnpFileInfo_new(); }
    ~CUpnpFileInfo() { UpnpFileInfo_delete(this->info); }
};


// Mocked system calls
//====================
class StdlibMock : public umock::StdlibInterface {
  public:
    virtual ~StdlibMock() override {}
    MOCK_METHOD(void*, malloc, (size_t size), (override));
    MOCK_METHOD(void*, calloc, (size_t nmemb, size_t size), (override));
    MOCK_METHOD(void*, realloc, (void* ptr, size_t size), (override));
    MOCK_METHOD(void, free, (void* ptr), (override));
};


// Testsuites
// ==========
TEST(WebServerTestSuite, init_and_destroy) {
    // Initialize needed global variable.
    bWebServerState = WEB_SERVER_DISABLED;

    // Test Unit init
    EXPECT_EQ(web_server_init(), UPNP_E_SUCCESS);

    EXPECT_EQ(bWebServerState, WEB_SERVER_ENABLED);

    // Check if MediaList is initialized
    const char* con_type{};
    const char* con_subtype{};
    EXPECT_EQ(search_extension("aif", &con_type, &con_subtype), 0);
    EXPECT_STREQ(con_type, "audio");
    EXPECT_STREQ(con_subtype, "aiff");

    // Check if gDocumentRootDir is initialized
    EXPECT_EQ(gDocumentRootDir.buf, nullptr);
    EXPECT_EQ(gDocumentRootDir.length, (size_t)0);
    EXPECT_EQ(gDocumentRootDir.capacity, (size_t)0);
    EXPECT_EQ(gDocumentRootDir.size_inc, (size_t)5);

    // Check if the global alias document is initialized
    EXPECT_EQ(gAliasDoc.doc.buf, nullptr);
    EXPECT_EQ(gAliasDoc.name.buf, nullptr);
    EXPECT_EQ(gAliasDoc.ct, nullptr);
    EXPECT_EQ(gAliasDoc.last_modified, 0);

    // Check if the virtual directory callback list is initialized
    EXPECT_EQ(pVirtualDirList, nullptr);
    EXPECT_EQ(virtualDirCallback.get_info, nullptr);
    EXPECT_EQ(virtualDirCallback.open, nullptr);
    EXPECT_EQ(virtualDirCallback.read, nullptr);
    EXPECT_EQ(virtualDirCallback.write, nullptr);
    EXPECT_EQ(virtualDirCallback.seek, nullptr);
    EXPECT_EQ(virtualDirCallback.close, nullptr);

    // Test Unit destroy
    EXPECT_EQ(bWebServerState, WEB_SERVER_ENABLED);
    web_server_destroy();

    EXPECT_EQ(bWebServerState, WEB_SERVER_DISABLED);
}

TEST(WebServerTestSuite, set_root_dir) {
    // Initialize global variable to avoid side effects
    membuffer_init(&gDocumentRootDir);

    // Test Unit
    EXPECT_EQ(web_server_set_root_dir(SAMPLE_SOURCE_DIR "/web"), 0);
    EXPECT_STREQ(gDocumentRootDir.buf, SAMPLE_SOURCE_DIR "/web");
}

TEST(WebServerTestSuite, set_root_dir_with_trailing_slash) {
    // Initialize global variable to avoid side effects
    membuffer_init(&gDocumentRootDir);

    // Test Unit
    EXPECT_EQ(web_server_set_root_dir(SAMPLE_SOURCE_DIR "/web/"), 0);
    EXPECT_STREQ(gDocumentRootDir.buf, SAMPLE_SOURCE_DIR "/web");
}

TEST(WebServerTestSuite, set_root_dir_empty_string) {
    // SKIP on Github Actions
    if (github_actions && !old_code)
        GTEST_SKIP() << "             known failing test on Github Actions";

    // Initialize global variable to avoid side effects
    membuffer_init(&gDocumentRootDir);

    // Test Unit
    EXPECT_EQ(web_server_set_root_dir(""), 0);

    if (old_code) {
        // This must be fixed within membuffer. Look at
        // TEST(MembufferTestSuite, membuffer_assign_str_empty_string)
        std::cout << CRED "[ BUG      ] " CRES << __LINE__
                  << ": Assign an empty string must point to an empty string "
                     "but not having a nullptr.\n";
        EXPECT_EQ(gDocumentRootDir.buf, nullptr); // Wrong

    } else {

        EXPECT_STREQ(gDocumentRootDir.buf, "");
    }
}

TEST(WebServerDeathTest, set_root_dir_nullptr) {
    // SKIP on Github Actions
    if (github_actions && !old_code)
        GTEST_SKIP() << "             known failing test on Github Actions";

    // Test Unit
    if (old_code) {
        // This must be fixed within membuffer.
        std::cout << CRED "[ BUG      ] " CRES << __LINE__
                  << ": web_server_set_root_dir called with nullptr must not "
                     "segfault.\n";
        // This expects segfault.
        ASSERT_DEATH(web_server_set_root_dir(nullptr), ".*");

    } else {

        // This expects NO segfault.
        ASSERT_EXIT(
            {
                web_server_set_root_dir(nullptr);
                exit(0);
            },
            ExitedWithCode(0), ".*");
        int ret_set_root_dir{UPNP_E_INTERNAL_ERROR};
        ret_set_root_dir = web_server_set_root_dir(nullptr);
        EXPECT_EQ(ret_set_root_dir, UPNP_E_INVALID_ARGUMENT)
            << errStrEx(ret_set_root_dir, UPNP_E_INVALID_ARGUMENT);
    }
}

TEST(MediaListTestSuite, init) {
    if (old_code) {
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
        // Destroy gMediaTypeList to avoid side effects from other tests.
        memset(&gMediaTypeList, 0xAA, sizeof(gMediaTypeList));

        // Test Unit
        ::media_list_init();

        EXPECT_STREQ(gMediaTypeList[0].file_ext, "aif");
        EXPECT_STREQ(gMediaTypeList[0].content_type, "audio");
        EXPECT_STREQ(gMediaTypeList[0].content_subtype, "aiff");

        EXPECT_STREQ(gMediaTypeList[NUM_MEDIA_TYPES - 1].file_ext, "zip");
        EXPECT_STREQ(gMediaTypeList[NUM_MEDIA_TYPES - 1].content_type,
                     "application");
        EXPECT_STREQ(gMediaTypeList[NUM_MEDIA_TYPES - 1].content_subtype,
                     "zip");
#endif
    } else {

        // With new code there is no media list to initialize. We have a
        // constant array there, once initialized by the compiler on startup.
        // This is only a dummy function for compatibility.
    }
}

TEST(MediaListTestSuite, search_extension) {
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    // Destroy gMediaTypeList to avoid side effects from other tests.
    memset(&gMediaTypeList, 0xAA, sizeof(gMediaTypeList));
    ::media_list_init();
#endif

    const char* con_unused{"<unused>"};
    const char* con_type{con_unused};
    const char* con_subtype{con_unused};

    // Test Unit, first entry
    EXPECT_EQ(search_extension("aif", &con_type, &con_subtype), 0);
    EXPECT_STREQ(con_type, "audio");
    EXPECT_STREQ(con_subtype, "aiff");
    // Last entry
    EXPECT_EQ(search_extension("zip", &con_type, &con_subtype), 0);
    EXPECT_STREQ(con_type, "application");
    EXPECT_STREQ(con_subtype, "zip");

    // Looking for non existing entries.
    con_type = con_unused;
    con_subtype = con_unused;
    EXPECT_EQ(search_extension("@%§&", &con_type, &con_subtype), -1);
    EXPECT_STREQ(con_type, "<unused>");
    EXPECT_STREQ(con_subtype, "<unused>");

    EXPECT_EQ(search_extension("", &con_type, &con_subtype), -1);
    EXPECT_STREQ(con_type, "<unused>");
    EXPECT_STREQ(con_subtype, "<unused>");
}

TEST(MediaListTestSuite, get_content_type) {
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    // Destroy gMediaTypeList to avoid side effects from other tests.
    memset(&gMediaTypeList, 0xAA, sizeof(gMediaTypeList));
    ::media_list_init();
#endif
    CUpnpFileInfo f;

    EXPECT_EQ(get_content_type("tvdevicedesc.xml", f.info), 0);
    EXPECT_STREQ((DOMString)UpnpFileInfo_get_ContentType(f.info), "text/xml");

    EXPECT_EQ(get_content_type("unknown_ext.???", f.info), 0);
    EXPECT_STREQ((DOMString)UpnpFileInfo_get_ContentType(f.info),
                 "application/octet-stream");

    EXPECT_EQ(get_content_type("no_ext", f.info), 0);
    EXPECT_STREQ((DOMString)UpnpFileInfo_get_ContentType(f.info),
                 "application/octet-stream");

    EXPECT_EQ(get_content_type("incomplete_ext.", f.info), 0);
    EXPECT_STREQ((DOMString)UpnpFileInfo_get_ContentType(f.info),
                 "application/octet-stream");

    EXPECT_EQ(get_content_type(".html", f.info), 0);
    EXPECT_STREQ((DOMString)UpnpFileInfo_get_ContentType(f.info), "text/html");

    EXPECT_EQ(get_content_type("double_ext.html.zip", f.info), 0);
    EXPECT_STREQ((DOMString)UpnpFileInfo_get_ContentType(f.info),
                 "application/zip");

    EXPECT_EQ(get_content_type(".html.tar", f.info), 0);
    EXPECT_STREQ((DOMString)UpnpFileInfo_get_ContentType(f.info),
                 "application/tar");
}

TEST(MediaListDeathTest, get_content_type_with_no_filename) {
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    // Destroy gMediaTypeList to avoid side effects from other tests.
    memset(&gMediaTypeList, 0xAA, sizeof(gMediaTypeList));
#endif
    CUpnpFileInfo f;

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": get_content_type called with nullptr to filename must "
                     "not segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(::get_content_type(nullptr, f.info), ".*");

    } else {

        // This expects NO segfault.
        ASSERT_EXIT((get_content_type(nullptr, f.info), exit(0)),
                    ExitedWithCode(0), ".*");
        int ret_get_content_type{UPNP_E_INTERNAL_ERROR};
        ret_get_content_type = get_content_type(nullptr, f.info);
        EXPECT_EQ(ret_get_content_type, UPNP_E_FILE_NOT_FOUND)
            << errStrEx(ret_get_content_type, UPNP_E_FILE_NOT_FOUND);
    }
}

TEST(MediaListDeathTest, get_content_type_with_no_fileinfo) {
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    // Destroy gMediaTypeList to avoid side effects from other tests.
    memset(&gMediaTypeList, 0xAA, sizeof(gMediaTypeList));
    ::media_list_init();
#endif

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": get_content_type called with nullptr to fileinfo must "
                     "not segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(::get_content_type("filename.txt", nullptr), ".*");

    } else {

        // This expects NO segfault.
        ASSERT_EXIT((get_content_type("filename.txt", nullptr), exit(0)),
                    ExitedWithCode(0), ".*");
        int ret_get_content_type{UPNP_E_INTERNAL_ERROR};
        ret_get_content_type = get_content_type("filename.txt", nullptr);
        EXPECT_EQ(ret_get_content_type, UPNP_E_INVALID_ARGUMENT)
            << errStrEx(ret_get_content_type, UPNP_E_INVALID_ARGUMENT);
    }
}

class XMLaliasFTestSuite : public ::testing::Test {
  protected:
    XMLaliasFTestSuite() {
        TRACE2(this, " Construct XMLaliasFTestSuite");
        // There are mutexes used, so we have to initialize it.
        pthread_mutex_init(&gWebMutex, NULL);

        // There is a problem with the global structure ::gAliasDoc due to side
        // effects on other tests. So we always provide a fresh initialized and
        // unused ::gAliasDoc for each test.

        // Next does not allocate memory, it only nullify pointer so we do
        // not have to release it.
        ::glob_alias_init();
    }

    ~XMLaliasFTestSuite() {
        TRACE2(this, " Destruct XMLaliasFTestSuite");
        // Always unlock a possible locked mutex in case of an aborted function
        // within a locked mutex. This avoids a deadlock in next
        // alias_release() by waiting to unlock the mutex.
        pthread_mutex_unlock(&gWebMutex);

        // This frees all allocated resources.
        while (is_valid_alias(&gAliasDoc)) {
            alias_release(&gAliasDoc);
        }
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
        memset(&::gAliasDoc, 0xAA, sizeof(::gAliasDoc));
#endif

        pthread_mutex_destroy(&gWebMutex);
    }
};
typedef XMLaliasFTestSuite XMLaliasFDeathTest;


TEST(XMLaliasTestSuite, glob_alias_init_and_release) {
    // Because we test glob_alias_init() and alias_release() here, we cannot use
    // the fixture class with TEST_F that also uses glob_alias_init().

    // There are some mutex locks and unlocks. This must be first in scope.
    CgWebMutex global_web_mutex;

    // With the old code we may see uninitialized but plausible values on
    // preused global variables so we overwrite them with unwanted values.
    // With new code the structure is initialized with its constructor, no
    // need to call an initialization function but we do it also for
    // compatibility.
#ifdef UPnPsdk_WITH_NATIVE_PUPNP
    memset(&::gAliasDoc, 0xAA, sizeof(::gAliasDoc));
#else
    gAliasDoc.clear();
#endif

    // Test Unit init.
    glob_alias_init();

    xml_alias_t* alias = &gAliasDoc;
    // An initialized empty structure does not contain a valid alias.
    ASSERT_FALSE(is_valid_alias(alias));

    // Check the empty alias.
    EXPECT_EQ(alias->doc.buf, nullptr);
    EXPECT_EQ(alias->doc.length, (size_t)0);
    EXPECT_EQ(alias->doc.capacity, (size_t)0);
    EXPECT_EQ(alias->doc.size_inc, (size_t)5);

    EXPECT_EQ(alias->name.buf, nullptr);
    EXPECT_EQ(alias->name.length, (size_t)0);
    EXPECT_EQ(alias->name.capacity, (size_t)0);
    EXPECT_EQ(alias->name.size_inc, (size_t)5);

    EXPECT_EQ(alias->ct, nullptr);
    EXPECT_EQ(alias->last_modified, 0);

    // An empty alias structure hasn't allocated memory so there is also nothing
    // to free.
    StdlibMock mock_stdlibObj;
    umock::Stdlib stdlib_injectObj(&mock_stdlibObj);
    EXPECT_CALL(mock_stdlibObj, free(_)).Times(0);

    // Test Unit release
    alias_release(alias);

    EXPECT_EQ(alias->doc.buf, nullptr);
    EXPECT_EQ(alias->doc.length, (size_t)0);
    EXPECT_EQ(alias->doc.capacity, (size_t)0);
    EXPECT_EQ(alias->doc.size_inc, (size_t)5);

    EXPECT_EQ(alias->name.buf, nullptr);
    EXPECT_EQ(alias->name.length, (size_t)0);
    EXPECT_EQ(alias->name.capacity, (size_t)0);
    EXPECT_EQ(alias->name.size_inc, (size_t)5);

    EXPECT_EQ(alias->ct, nullptr);
    EXPECT_EQ(alias->last_modified, 0);
}

TEST_F(XMLaliasFTestSuite, copy_empty_structure) {
    // Test Unit, gAliasDoc is initialized by the fixture.
    xml_alias_t aliasDoc{gAliasDoc};

    // Check the copied alias.
    EXPECT_EQ(aliasDoc.doc.buf, nullptr);
    EXPECT_EQ(aliasDoc.doc.length, (size_t)0);
    EXPECT_EQ(aliasDoc.doc.capacity, (size_t)0);
    EXPECT_EQ(aliasDoc.doc.size_inc, (size_t)5);

    EXPECT_EQ(aliasDoc.name.buf, nullptr);
    EXPECT_EQ(aliasDoc.name.length, (size_t)0);
    EXPECT_EQ(aliasDoc.name.capacity, (size_t)0);
    EXPECT_EQ(aliasDoc.name.size_inc, (size_t)5);

    EXPECT_EQ(aliasDoc.ct, nullptr);
    EXPECT_EQ(aliasDoc.last_modified, 0);
}

TEST_F(XMLaliasFTestSuite, copy_structure) {
    // For details look at test [..].set_alias_with_content_length_zero.

    char alias_name[]{"valid_alias_name1"}; // length = 18
    char content[]{"XML Dokument string1"}; // length = 20
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content, 20, 0), 0);
    *gAliasDoc.ct = 2;

    // Test Unit
    xml_alias_t aliasDoc{gAliasDoc};

    EXPECT_EQ(*aliasDoc.ct, 2);

    EXPECT_EQ(aliasDoc.name.length, (size_t)18);
    EXPECT_EQ(aliasDoc.name.capacity, (size_t)22);
    EXPECT_STREQ(aliasDoc.name.buf, "/valid_alias_name1");

    EXPECT_EQ(aliasDoc.doc.length, (size_t)20);
    EXPECT_EQ(aliasDoc.doc.capacity, (size_t)20);
    EXPECT_STREQ(aliasDoc.doc.buf, "XML Dokument string1");
}

TEST_F(XMLaliasFTestSuite, assign_empty_structure) {
    // Test Unit, gAliasDoc is initialized by the fixture.
    xml_alias_t aliasDoc;
    aliasDoc = gAliasDoc;

    // Check the copied alias.
    EXPECT_EQ(aliasDoc.doc.buf, nullptr);
    EXPECT_EQ(aliasDoc.doc.length, (size_t)0);
    EXPECT_EQ(aliasDoc.doc.capacity, (size_t)0);
    EXPECT_EQ(aliasDoc.doc.size_inc, (size_t)5);

    EXPECT_EQ(aliasDoc.name.buf, nullptr);
    EXPECT_EQ(aliasDoc.name.length, (size_t)0);
    EXPECT_EQ(aliasDoc.name.capacity, (size_t)0);
    EXPECT_EQ(aliasDoc.name.size_inc, (size_t)5);

    EXPECT_EQ(aliasDoc.ct, nullptr);
    EXPECT_EQ(aliasDoc.last_modified, 0);
}

TEST_F(XMLaliasFTestSuite, assign_structure) {
    // For details look at test [..].set_alias_with_content_length_zero.

    char alias_name[]{"valid_alias_name2"}; // length = 18
    char content[]{"XML Dokument string2"}; // length = 20
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content, 20, 0), 0);
    *gAliasDoc.ct = 3;

    // Test Unit
    xml_alias_t aliasDoc;
    aliasDoc = gAliasDoc;

    EXPECT_EQ(*aliasDoc.ct, 3);

    EXPECT_EQ(aliasDoc.name.length, (size_t)18);
    EXPECT_EQ(aliasDoc.name.capacity, (size_t)22);
    EXPECT_STREQ(aliasDoc.name.buf, "/valid_alias_name2");

    EXPECT_EQ(aliasDoc.doc.length, (size_t)20);
    EXPECT_EQ(aliasDoc.doc.capacity, (size_t)20);
    EXPECT_STREQ(aliasDoc.doc.buf, "XML Dokument string2");
}

TEST_F(XMLaliasFDeathTest, alias_release_nullptr) {
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": alias_release a nullptr must not segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(::alias_release(nullptr), ".*");

    } else {

        // This expects NO segfault.
        EXPECT_EXIT((alias_release(nullptr), exit(0)), ExitedWithCode(0), ".*");
    }
}

TEST_F(XMLaliasFDeathTest, is_valid_alias_nullptr) {
    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": is_valid_alias(nullptr) must not segfault.\n";
        // This expects segfault.
        EXPECT_DEATH(
            {
                int rc = ::is_valid_alias((::xml_alias_t*)nullptr);
                // Next statement is only executed if there was no segfault but
                // it's needed to suppress optimization to remove unneeded
                // return code.
                std::cout << "No segfault with rc = " << rc << "\n";
            },
            ".*");

    } else {

        // This expects NO segfault.
        ASSERT_EXIT((is_valid_alias((xml_alias_t*)nullptr), exit(0)),
                    ExitedWithCode(0), ".*");
        bool ret_is_valid_alias{true};
        ret_is_valid_alias = is_valid_alias((xml_alias_t*)nullptr);
        EXPECT_FALSE(ret_is_valid_alias);
    }
}

TEST_F(XMLaliasFTestSuite, is_valid_alias_empty_structure) {
    // An empty alias is provided by the fixture.
    // Test Unit empty alias structure not to be a valid alias.
    bool ret_is_valid_alias{true};
    ret_is_valid_alias = is_valid_alias(&gAliasDoc);
    EXPECT_FALSE(ret_is_valid_alias);
}

TEST(XMLaliasTestSuite, is_valid_alias_uninitialized_structure) {
    // Because we test an uninitialzed structure we cannot use the fixture that
    // provide an initialized structure. is_valid_alias() does not use mutex.

    bool ret_is_valid_alias{true};

    if (old_code) {
        // Because it is impossible to test for invalid pointers it is important
        // to "nullify" the structure.
        ::xml_alias_t alias{};
        // memset(&alias, 0xA5, sizeof(xml_alias_t));
        // This "random" structure setting will report a valid alias

        // Test Unit
        ret_is_valid_alias = ::is_valid_alias(&alias);
        EXPECT_FALSE(ret_is_valid_alias);

    } else {

        // Here we have a cnstructor of the struct that is doing initialization
        // so we never get an uninitialized alias. But an unset initialized
        // alias is also not valid.
        xml_alias_t alias;

        // Test Unit
        ret_is_valid_alias = is_valid_alias(&alias);
        EXPECT_FALSE(ret_is_valid_alias);
    }
}

TEST_F(XMLaliasFTestSuite, set_and_is_valid_and_release_global_alias) {
    char alias_name[]{"is_valid_alias"}; // length = 14

    // The content string must be allocated on the heap because it is freed by
    // the unit.
    const char content[]{"Test for a valid alias"}; // length = 22
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);

    // Test Unit web_server_set_alias()
    // alias_content_length without terminating '\0'
    // time_t 1668095500 sec is 2022-11-10T16:51:40
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content,
                                   sizeof(content) - 1, 1668095500),
              0);

    EXPECT_STREQ(gAliasDoc.doc.buf, "Test for a valid alias");
    EXPECT_EQ(gAliasDoc.doc.length, strlen(gAliasDoc.doc.buf));
    EXPECT_EQ(gAliasDoc.doc.length, sizeof(content) - 1);
    EXPECT_EQ(gAliasDoc.doc.capacity, (size_t)22);
    EXPECT_EQ(gAliasDoc.doc.size_inc, (size_t)5);

    EXPECT_STREQ(gAliasDoc.name.buf, "/is_valid_alias");
    EXPECT_EQ(strlen(gAliasDoc.name.buf), (size_t)15); // with added '/'
    // *.length is without NULL byte, sizeof with NULL byte
    EXPECT_EQ(gAliasDoc.name.length, sizeof('/') + sizeof(alias_name) - 1);
    EXPECT_EQ(gAliasDoc.name.capacity, (size_t)19);
    EXPECT_EQ(gAliasDoc.name.size_inc, (size_t)5);

    EXPECT_EQ(*gAliasDoc.ct, 1);
    EXPECT_EQ(gAliasDoc.last_modified, 1668095500);

    // Test Unit is_valid_alias()
    EXPECT_TRUE(is_valid_alias((xml_alias_t*)&gAliasDoc));

    // Test Unit alias_release()
    alias_release((xml_alias_t*)&gAliasDoc);

    EXPECT_STREQ(gAliasDoc.doc.buf, nullptr);
    EXPECT_EQ(gAliasDoc.doc.length, (size_t)0);
    EXPECT_EQ(gAliasDoc.doc.capacity, (size_t)0);
    EXPECT_EQ(gAliasDoc.doc.size_inc, (size_t)5);

    EXPECT_STREQ(gAliasDoc.name.buf, nullptr);
    EXPECT_EQ(gAliasDoc.name.length, (size_t)0);
    EXPECT_EQ(gAliasDoc.name.capacity, (size_t)0);
    EXPECT_EQ(gAliasDoc.name.size_inc, (size_t)5);

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Clear webserver alias with 'alias_release()' "
                     "should clear all fields of the structure.\n";
        EXPECT_NE(gAliasDoc.ct, nullptr);               // This is wrong
        EXPECT_EQ(gAliasDoc.last_modified, 1668095500); // This is wrong

    } else {

        EXPECT_EQ(gAliasDoc.ct, nullptr);
        EXPECT_EQ(gAliasDoc.last_modified, 0);
    }

    // Test Unit is_valid_alias()
    EXPECT_FALSE(is_valid_alias((xml_alias_t*)&gAliasDoc));
}

TEST_F(XMLaliasFTestSuite, set_and_remove_alias) {
    char alias_name[]{"alias_name"};

    // The content string must be allocated on the heap because it is freed by
    // the unit.
    const char content[]{"This is an alias content"};
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);

    // Test Unit set_alias
    // time_t 1668095500 sec is 2022-11-10T16:51:40
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content,
                                   sizeof(content) - 1, 1668095500),
              0);

    // Test Unit remove alias with setting alias name to nullptr.
    // time_t 1668095500 sec is 2022-11-10T16:51:40
    EXPECT_EQ(web_server_set_alias(nullptr, alias_content, sizeof(content) - 1,
                                   1668095500),
              0);

    EXPECT_STREQ(gAliasDoc.doc.buf, nullptr);
    EXPECT_EQ(gAliasDoc.doc.length, (size_t)0);
    EXPECT_EQ(gAliasDoc.doc.capacity, (size_t)0);
    EXPECT_EQ(gAliasDoc.doc.size_inc, (size_t)5);

    EXPECT_STREQ(gAliasDoc.name.buf, nullptr);
    EXPECT_EQ(gAliasDoc.name.length, (size_t)0);
    EXPECT_EQ(gAliasDoc.name.capacity, (size_t)0);
    EXPECT_EQ(gAliasDoc.name.size_inc, (size_t)5);

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Clear webserver alias with 'web_server_set_alias()' "
                     "should clear all fields of the structure.\n";
        EXPECT_NE(gAliasDoc.ct, nullptr);               // This is wrong
        EXPECT_EQ(gAliasDoc.last_modified, 1668095500); // This is wrong

    } else {

        EXPECT_EQ(gAliasDoc.ct, nullptr);
        EXPECT_EQ(gAliasDoc.last_modified, 0);
    }
}

TEST_F(XMLaliasFDeathTest, set_alias_with_nullptr_to_alias_content) {
    char alias_name[]{"alias_Name"};

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": web_server_set_alias() with nullptr to "
                     "alias_content must not abort or leave invalid alias.\n";
#ifndef NDEBUG
        // This expects an abort with failed assertion only with DEBUG build.
        // There is an assert used in the function.
        ASSERT_DEATH(::web_server_set_alias(alias_name, nullptr, 0, 0), ".*");
#else
        int ret_set_alias{UPNP_E_INTERNAL_ERROR};
        ret_set_alias = ::web_server_set_alias(alias_name, nullptr, 0, 0);
        EXPECT_EQ(ret_set_alias, UPNP_E_SUCCESS)
            << errStrEx(ret_set_alias, UPNP_E_SUCCESS);

        // This is an invalid alias with name but no pointer to its document.
        EXPECT_EQ(::gAliasDoc.doc.buf, nullptr);
        EXPECT_EQ(::gAliasDoc.doc.length, (size_t)0);
        EXPECT_EQ(::gAliasDoc.doc.capacity, (size_t)0);
        EXPECT_EQ(::gAliasDoc.doc.size_inc, (size_t)5);

        EXPECT_STREQ(::gAliasDoc.name.buf, "/alias_Name");
        EXPECT_EQ(::gAliasDoc.name.length, sizeof(alias_name));
        EXPECT_EQ(::gAliasDoc.name.capacity, (size_t)15);
        EXPECT_EQ(::gAliasDoc.name.size_inc, (size_t)5);
#endif

    } else {

        // This expects NO abort with failed assertion.
        ASSERT_EXIT((web_server_set_alias(alias_name, nullptr, 0, 0), exit(0)),
                    ExitedWithCode(0), ".*");
        int ret_set_alias{UPNP_E_INTERNAL_ERROR};
        ret_set_alias = web_server_set_alias(alias_name, nullptr, 0, 0);
        EXPECT_EQ(ret_set_alias, UPNP_E_INVALID_ARGUMENT)
            << errStrEx(ret_set_alias, UPNP_E_INVALID_ARGUMENT);
    }
}

TEST_F(XMLaliasFDeathTest, set_alias_with_nullptr_but_length_to_alias_content) {
    char alias_name[]{"alias_with_length"};

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": web_server_set_alias() with nullptr to "
                     "alias_content but length must not abort or leave invalid "
                     "alias.\n";
#ifndef NDEBUG
        // This expects an abort with failed assertion only with DEBUG build.
        // There is an assert used in the function.
        ASSERT_DEATH(::web_server_set_alias(alias_name, nullptr, 1, 0), ".*");
#else
        int ret_set_alias{UPNP_E_INTERNAL_ERROR};
        ret_set_alias = ::web_server_set_alias(alias_name, nullptr, 1, 0);
        EXPECT_EQ(ret_set_alias, UPNP_E_SUCCESS)
            << errStrEx(ret_set_alias, UPNP_E_SUCCESS);

        // This is an invalid alias with name and doc length but no pointer to
        // its document.
        EXPECT_EQ(::gAliasDoc.doc.buf, nullptr);
        EXPECT_EQ(::gAliasDoc.doc.length, (size_t)1);
        EXPECT_EQ(::gAliasDoc.doc.capacity, (size_t)1);
        EXPECT_EQ(::gAliasDoc.doc.size_inc, (size_t)5);

        EXPECT_STREQ(::gAliasDoc.name.buf, "/alias_with_length");
        EXPECT_EQ(::gAliasDoc.name.length, sizeof(alias_name));
        EXPECT_EQ(::gAliasDoc.name.capacity, (size_t)22);
        EXPECT_EQ(::gAliasDoc.name.size_inc, (size_t)5);
#endif

    } else {

        // This expects NO abort with failed assertion.
        ASSERT_EXIT((web_server_set_alias(alias_name, nullptr, 1, 0), exit(0)),
                    ExitedWithCode(0), ".*");
        int ret_set_alias{UPNP_E_INTERNAL_ERROR};
        ret_set_alias = web_server_set_alias(alias_name, nullptr, 1, 0);
        EXPECT_EQ(ret_set_alias, UPNP_E_INVALID_ARGUMENT)
            << errStrEx(ret_set_alias, UPNP_E_INVALID_ARGUMENT);
    }
}

TEST_F(XMLaliasFTestSuite, set_alias_with_content_length_zero) {
    // alias_content is defined to be a XML document which is a character string
    // with string length. So alias_content_length isn't really needed. It
    // should only truncate the alias_content but never exceed its length even
    // if it is set greater.

    char alias_name[]{"valid_alias_name"};

    // The content string must be allocated on the heap because it is freed by
    // the unit.
    char content[]{"XML Dokument string"};
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);

    // Test Unit with alias_content_length = 0
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content, 0, 0), 0);

    // The alias_name is set.
    EXPECT_STREQ(gAliasDoc.name.buf, "/valid_alias_name");
    EXPECT_EQ(strlen(gAliasDoc.name.buf), (size_t)17);
    // *.length is without NULL byte, sizeof with NULL byte
    EXPECT_EQ(gAliasDoc.name.length, sizeof('/') + sizeof(alias_name) - 1);
    EXPECT_EQ(gAliasDoc.name.capacity, (size_t)21);
    EXPECT_EQ(gAliasDoc.name.size_inc, (size_t)5);

    EXPECT_EQ(*gAliasDoc.ct, 1);
    EXPECT_EQ(gAliasDoc.last_modified, 0);

    // The content length must be 0.
    EXPECT_EQ(gAliasDoc.doc.length, (size_t)0);
    EXPECT_EQ(gAliasDoc.doc.capacity, (size_t)0);
    EXPECT_EQ(gAliasDoc.doc.size_inc, (size_t)5);

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": web_server_set_alias() must never copy a longer but "
                     "an empty string with length zero.\n";
        EXPECT_STREQ(::gAliasDoc.doc.buf, "XML Dokument string"); // Wrong

    } else {

        EXPECT_STREQ(gAliasDoc.doc.buf, "");
    }
}

TEST_F(XMLaliasFTestSuite, set_alias_with_content_length_one) {
    // For details look at test [..].set_alias_with_content_length_zero.

    char alias_name[]{"valid_alias_name"};
    char content[]{"XML Dokument string"};
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);

    // Test Unit with alias_content_length = 1
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content, 1, 0), 0);

    EXPECT_EQ(*gAliasDoc.ct, 1);
    EXPECT_EQ(gAliasDoc.doc.length, (size_t)1);
    EXPECT_EQ(gAliasDoc.doc.capacity, (size_t)1);

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": web_server_set_alias() must never copy a longer string "
                     "than with length one.\n";
        EXPECT_STREQ(::gAliasDoc.doc.buf, "XML Dokument string"); // Wrong

    } else {

        EXPECT_STREQ(gAliasDoc.doc.buf, "X");
    }
}

TEST_F(XMLaliasFTestSuite, set_alias_with_content_length) {
    // For details look at test [..].set_alias_with_content_length_zero.

    char alias_name[]{"valid_alias_name"};
    char content[]{"XML Dokument string"}; // length = 19
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);

    // Test Unit with alias_content_length
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content, 19, 0), 0);

    EXPECT_EQ(*gAliasDoc.ct, 1);
    EXPECT_EQ(gAliasDoc.doc.length, (size_t)19);
    EXPECT_EQ(gAliasDoc.doc.capacity, (size_t)19);
    EXPECT_STREQ(gAliasDoc.doc.buf, "XML Dokument string");
}

TEST_F(XMLaliasFTestSuite, set_alias_with_content_length_greater) {
    // For details look at test [..].set_alias_with_content_length_zero.

    char alias_name[]{"valid_alias_name"};
    char content[]{"XML Dokument string"}; // length = 19
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);

    // Test Unit with alias_content_length greater than length of content.
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content, 20, 0), 0);

    EXPECT_EQ(*gAliasDoc.ct, 1);
    EXPECT_STREQ(gAliasDoc.doc.buf, "XML Dokument string");

    if (old_code) {
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": web_server_set_alias() must never set wrong content "
                     "length of alias document.\n";
        EXPECT_EQ(::gAliasDoc.doc.length, (size_t)20);   // Wrong
        EXPECT_EQ(::gAliasDoc.doc.capacity, (size_t)20); // Wrong

    } else {

        EXPECT_EQ(gAliasDoc.doc.length, (size_t)19);
        EXPECT_EQ(gAliasDoc.doc.capacity, (size_t)19);
    }
}

TEST_F(XMLaliasFTestSuite, set_alias_with_zero_modified_date) {
    const char alias_name[]{"valid_alias_name"};
    // The content string must be allocated on the heap because it is freed by
    // the unit.
    const char content[]{"Some valid content"};
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);

    // Preset modifired date to 2022-11-10T16:51:41
    gAliasDoc.last_modified = 1668095501;

    // Test Unit
    EXPECT_EQ(
        web_server_set_alias(alias_name, alias_content, sizeof(content) - 1, 0),
        0);

    EXPECT_EQ(gAliasDoc.last_modified, 0);
}

TEST_F(XMLaliasFTestSuite, set_alias_with_negative_modified_date) {
    const char alias_name[]{"valid_alias_name"};
    // The content string must be allocated on the heap because it is freed by
    // the unit.
    const char content[]{"Some valid content"};
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);

    // Preset modifired date to 2022-11-10T16:51:42
    gAliasDoc.last_modified = 1668095502;

    // Test Unit
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content,
                                   sizeof(content) - 1, -1),
              0);

    EXPECT_EQ(gAliasDoc.last_modified, -1);
}

TEST_F(XMLaliasFTestSuite, set_alias_two_times) {
    const char alias_name[]{"valid_alias_name"}; // length = 16
    // The content string must be allocated on the heap because it is freed by
    // the unit.
    const char content[]{"Some valid content"}; // length = 18
    char* alias_content1 = (char*)malloc(sizeof(content));
    strcpy(alias_content1, content);

    // Test Unit first time
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content1,
                                   sizeof(content) - 1, 1),
              0);

    // Check the alias.
    // The alias_name has been coppied to the heap.
    EXPECT_NE(gAliasDoc.name.buf, alias_name);
    // The document buffer is pointing to the allocated memory above.
    EXPECT_EQ(gAliasDoc.doc.buf, alias_content1);

    EXPECT_EQ(gAliasDoc.last_modified, 1);
    EXPECT_EQ(*gAliasDoc.ct, 1);

    // The allocated alias_content1 is still valid now but will be freed with
    // setting the alias document again, means overwrite it. We need a new
    // allocated alias_content2, otherwise we will get undefined behaviour with
    // segfault or "double free detected in tcache 2" on old_code.
    EXPECT_STREQ(alias_content1, "Some valid content");

    char* alias_content2 = (char*)malloc(sizeof(content));
    strcpy(alias_content2, content);

    // Test Unit second time
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content2,
                                   sizeof(content) - 1, 2),
              0);

    EXPECT_STREQ(gAliasDoc.name.buf, "/valid_alias_name");
    EXPECT_STREQ(gAliasDoc.doc.buf, "Some valid content");
    EXPECT_EQ(gAliasDoc.last_modified, 2);
    EXPECT_EQ(*gAliasDoc.ct, 1);
}

TEST_F(XMLaliasFDeathTest, set_alias_more_times_with_same_content) {
    const char alias_name[]{"valid_alias_name"}; // length = 16
    const char content[]{"Some valid content"};  // length = 18

    if (old_code) {
        // Using the same unallocated content again results in undefined
        // behaviour with segfault or "double free detected in tcache 2" or
        // just hang with no return, but may also succeed.
        std::cout << CYEL "[ BUGFIX   ] " CRES << __LINE__
                  << ": Set alias again to overwrite previous setting with "
                     "same content must not segfault.\n";
#ifdef DEBUG
        // This expects segfault.
        ASSERT_DEATH(
            {
                // The content string must be allocated on the heap because it
                // is freed by the unit.
                char* alias_content = (char*)malloc(sizeof(content));
                strcpy(alias_content, content);
                ::web_server_set_alias(alias_name, alias_content,
                                       sizeof(content) - 1, 1);
                // alias_content is freed by the Unit. Using it just again is
                // undefined behavior but may succeed sometimes. I do not can
                // test random succeeding or failing, so I have to clear the
                // pointer for a defined test condition.
                alias_content = nullptr;
                ::web_server_set_alias(alias_name, alias_content,
                                       sizeof(content) - 1, 2);
            },
            ".*");
#else
        // This expects NO abort.
        ASSERT_EXIT(
            {
                // The content string must be allocated on the heap because it
                // is freed by the unit.
                char* alias_content = (char*)malloc(sizeof(content));
                strcpy(alias_content, content);
                ::web_server_set_alias(alias_name, alias_content,
                                       sizeof(content) - 1, 1);
                // alias_content is freed by the Unit. Using it just again is
                // undefined behavior but may succeed sometimes. I do not can
                // test random succeeding or failing, so I have to clear the
                // pointer for a defined test condition.
                alias_content = nullptr;
                ::web_server_set_alias(alias_name, alias_content,
                                       sizeof(content) - 1, 2);
                exit(0);
            },
            ExitedWithCode(0), ".*");
#endif

    } else {

        // This expects NO abort.
        ASSERT_EXIT(
            {
                // The content string must be allocated on the heap because it
                // is freed by the unit.
                char* alias_content = (char*)malloc(sizeof(content));
                strcpy(alias_content, content);
                web_server_set_alias(alias_name, alias_content,
                                     sizeof(content) - 1, 1);
                web_server_set_alias(alias_name, alias_content,
                                     sizeof(content) - 1, 2);
                web_server_set_alias(alias_name, alias_content,
                                     sizeof(content) - 1, 3);
                exit(0);
            },
            ExitedWithCode(0), ".*");

        // Test Unit when it hasn't failed.
        char* alias_content = (char*)malloc(sizeof(content));
        strcpy(alias_content, content);

        int ret_set_alias{UPNP_E_INTERNAL_ERROR};
        ret_set_alias = web_server_set_alias(alias_name, alias_content,
                                             sizeof(content) - 1, 4);
        EXPECT_EQ(ret_set_alias, UPNP_E_SUCCESS)
            << errStrEx(ret_set_alias, UPNP_E_SUCCESS);

        ret_set_alias = web_server_set_alias(alias_name, alias_content,
                                             sizeof(content) - 1, 5);
        EXPECT_EQ(ret_set_alias, UPNP_E_INVALID_ARGUMENT)
            << errStrEx(ret_set_alias, UPNP_E_INVALID_ARGUMENT);

        // This indicates that the second wrong setting attempt hasn't changed
        // the first valid setting.
        EXPECT_EQ(gAliasDoc.last_modified, 4);
        EXPECT_EQ(*gAliasDoc.ct, 1);
    }
}

TEST_F(XMLaliasFTestSuite, alias_grab_valid_structure) {
    char alias_name[]{"is_valid_alias"};            // length = 14
    const char content[]{"Test for a valid alias"}; // length = 22
    char* alias_content = (char*)malloc(sizeof(content));
    strcpy(alias_content, content);

    // Set global alias
    // time_t 1668095500 sec is 2022-11-10T16:51:40
    EXPECT_EQ(web_server_set_alias(alias_name, alias_content,
                                   sizeof(content) - 1, 1668095500),
              0);
    // Provide dstination structure.
    xml_alias_t gAliasDoc_dup;

    // Test Unit
    alias_grab(&gAliasDoc_dup);

    EXPECT_STREQ(gAliasDoc_dup.doc.buf, "Test for a valid alias");
    EXPECT_EQ(gAliasDoc_dup.doc.length, sizeof(content) - 1);
    EXPECT_EQ(gAliasDoc_dup.doc.capacity, (size_t)22);
    EXPECT_EQ(gAliasDoc_dup.doc.size_inc, (size_t)5);

    EXPECT_STREQ(gAliasDoc_dup.name.buf, "/is_valid_alias");
    EXPECT_EQ(strlen(gAliasDoc_dup.name.buf), (size_t)15);
    // *.length is without NULL byte, sizeof with NULL byte
    EXPECT_EQ(gAliasDoc_dup.name.length, sizeof('/') + sizeof(alias_name) - 1);
    EXPECT_EQ(gAliasDoc_dup.name.capacity, (size_t)19);
    EXPECT_EQ(gAliasDoc_dup.name.size_inc, (size_t)5);

    EXPECT_EQ(gAliasDoc_dup.last_modified, 1668095500);
    ASSERT_NE(gAliasDoc_dup.ct, nullptr);
    EXPECT_EQ(*gAliasDoc_dup.ct, 2);
}

TEST_F(XMLaliasFDeathTest, alias_grab_empty_structure) {
    // An empty gAliasDoc is provided by the fixture.

    // Test Unit
    if (old_code) {
        std::cout << CYEL "[ FIX      ] " CRES << __LINE__
                  << ": alias_grab an empty structure should not abort the "
                     "program due to failed assertion.\n";
        // Provide destination structure.
        xml_alias_t gAliasDoc_dup;

        // This expects abort with failed assertion.
        ASSERT_DEATH(alias_grab(&gAliasDoc_dup), ".*");

    } else {

        // This expects NO abort.
        // Provide destination structure.
        xml_alias_t gAliasDoc_dup;

        ASSERT_EXIT(
            {
                alias_grab(&gAliasDoc_dup);
                exit(0);
            },
            ExitedWithCode(0), ".*");

        alias_grab(&gAliasDoc_dup);
        EXPECT_STREQ(gAliasDoc_dup.doc.buf, nullptr);
        EXPECT_EQ(gAliasDoc_dup.doc.length, (size_t)0);
        EXPECT_EQ(gAliasDoc_dup.doc.capacity, (size_t)0);
        EXPECT_EQ(gAliasDoc_dup.doc.size_inc, (size_t)5);

        EXPECT_STREQ(gAliasDoc_dup.name.buf, nullptr);
        EXPECT_EQ(gAliasDoc_dup.name.length, (size_t)0);
        EXPECT_EQ(gAliasDoc_dup.name.capacity, (size_t)0);
        EXPECT_EQ(gAliasDoc_dup.name.size_inc, (size_t)5);

        EXPECT_EQ(gAliasDoc_dup.last_modified, 0);
        ASSERT_EQ(gAliasDoc_dup.ct, nullptr);
    }
}

TEST_F(XMLaliasFDeathTest, alias_grab_nullptr) {
    // Test Unit
    if (old_code) {
        std::cout << CYEL "[ FIX      ] " CRES << __LINE__
                  << ": alias_grab(nullptr) must not segfault or abort with "
                     "failed assertion.\n";
        ASSERT_DEATH(::alias_grab(nullptr), ".*");

    } else {

        ASSERT_EXIT(
            {
                alias_grab(nullptr);
                exit(0);
            },
            ExitedWithCode(0), ".*");

        alias_grab(nullptr);
    }
}

} // namespace utest


int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
#include <utest/utest_main.inc>
    return gtest_return_code; // managed in gtest_main.inc
}
