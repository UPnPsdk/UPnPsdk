var Compa_2src_2inc_2httpparser_8hpp =
[
    [ "scanner_t", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#dc/d0a/structscanner__t", [
      [ "msg", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a9bd6e912cddf69f4193762dfae3885cb", null ],
      [ "cursor", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ac1c73fcb21c8500bf51a076f5083f65f", null ],
      [ "entire_msg_loaded", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a515e85ca47e62b5bc422c60080fec30e", null ]
    ] ],
    [ "http_header_t", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#df/d1d/structhttp__header__t", [
      [ "name", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a39ccc6b29221ffaf48476329eb43b00c", null ],
      [ "name_id", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#acd520c762356814ae21b9de9446f9397", null ],
      [ "value", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a6a05e8e1ea8ad4f889d635b469dc53ee", null ],
      [ "name_buf", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a785af325a54ed965881f17cf52c29f90", null ]
    ] ],
    [ "http_message_t", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#de/d66/structhttp__message__t", [
      [ "initialized", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ab586e7ef7512057314a980f391ecea9b", null ],
      [ "method", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a226234f2adefb0980a78b004f927c852", null ],
      [ "uri", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a13c130c56a7461ec81a060deb0764164", null ],
      [ "request_method", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a8ad4b5caee28078c616859bf127846a5", null ],
      [ "status_code", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a6e533777011a4bd0c5bbdc9c303f04f6", null ],
      [ "status_msg", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a4fa4a604b2e97efb692201e479c698ae", null ],
      [ "amount_discarded", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a6bf8c1b722e09bfd03df6f4a586d687e", null ],
      [ "is_request", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2d335ffbfa533def795b026483da4125", null ],
      [ "major_version", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a494431d42bc8dd024f0a2737a4ee2392", null ],
      [ "minor_version", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ad6562806be41838f35ae41d6cb6742ba", null ],
      [ "headers", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ad25a924f2f27dc1000fc080fe408a0c1", null ],
      [ "entity", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a55fe88fcfc0ffc5c0967c0db2c082f19", null ],
      [ "msg", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a755470277c80a2a1fed7a3993b3996ec", null ],
      [ "urlbuf", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a5813a4478c701c3eac633ca5b9f9e215", null ]
    ] ],
    [ "http_parser_t", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#d7/ddd/structhttp__parser__t", [
      [ "msg", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a0ae25c4f785752a83e6d1b32a5e85c12", null ],
      [ "http_error_code", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a76839ca93d37b8bc1bbfed9c4a641a9e", null ],
      [ "valid_ssdp_notify_hack", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#abcd4f2abec0f3372317746201af4f85a", null ],
      [ "position", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a94111f06d30bf833358daa3e891217be", null ],
      [ "ent_position", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#acf9cc63cdca9cfdd71704d35750cd21f", null ],
      [ "content_length", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ab5b768a5b15c4cb3a52356b55a591b6e", null ],
      [ "chunk_size", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a91ab50a5e0edc2fb271b620a7a988d2a", null ],
      [ "entity_start_position", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aeb959ba1bd2245e16ff4b722199d6ff2", null ],
      [ "scanner", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a8ee1d56dc0af6df7d8c7bbfefa55917a", null ]
    ] ],
    [ "ENTREAD_DETERMINE_READ_METHOD", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a698500b7b207ca90f8b0fb4788c7a1a0", null ],
    [ "ENTREAD_USING_CLEN", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#acd3cae2622055961a1c0f8d3417b149e", null ],
    [ "ENTREAD_USING_CHUNKED", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ac9609dadb3b25ea2a20aed1e31dc25bd", null ],
    [ "ENTREAD_UNTIL_CLOSE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#addd20b8843cdf986d81692204ee00af3", null ],
    [ "ENTREAD_CHUNKY_BODY", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a126c180d33b74b2c2e8f67e7e580284f", null ],
    [ "ENTREAD_CHUNKY_HEADERS", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#afc1ba3ff37e12803e384923cde5cd9cd", null ],
    [ "HDR_UNKNOWN", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ada5380a79e4f620c182903ee828009d0", null ],
    [ "HDR_CACHE_CONTROL", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a914b617229718c70112993c2b89d03a6", null ],
    [ "HDR_CALLBACK", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a92a906003e7f3ab811219ba721461cc4", null ],
    [ "HDR_CONTENT_LENGTH", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#afda372773b7b22dcf4a2511710ec6dfd", null ],
    [ "HDR_CONTENT_TYPE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#acca4a785448102527eaa5bda427500bb", null ],
    [ "HDR_DATE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a5be755408093a8fc9c43d857f7899b0c", null ],
    [ "HDR_EXT", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a6412b751586afc028de92676bdc43d22", null ],
    [ "HDR_HOST", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a850c4ab3df7244d2fd1a77b85eaebede", null ],
    [ "HDR_LOCATION", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa52672857ec618af4bfc97b3e2a15466", null ],
    [ "HDR_MAN", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa79d3550137ace421c440f7f0cf953d6", null ],
    [ "HDR_MX", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a5e75b1c70e5235d14e97d2ba36791c06", null ],
    [ "HDR_NT", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a5d23b50bb3f45d4b85e69a04a46a6f71", null ],
    [ "HDR_NTS", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a5f80a3685cf243196afaf16ab80cdb52", null ],
    [ "HDR_SERVER", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a727159e5876062f67d4d1c02764fc1b1", null ],
    [ "HDR_SEQ", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a9b40a1ab8ac093563a3bddeb126b8b37", null ],
    [ "HDR_SID", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#afe5d3ab378b44ed5e2d480c8b514b338", null ],
    [ "HDR_SOAPACTION", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ac75c0ec85995d0aca73cff116040fd77", null ],
    [ "HDR_ST", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a8217432ea411c52a4cebef25e5ecb962", null ],
    [ "HDR_TIMEOUT", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#af06ee19445ba64c1257f085308451187", null ],
    [ "HDR_TRANSFER_ENCODING", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a54cd755d8b2dd6d30dafca1744df2a89", null ],
    [ "HDR_USN", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a56e9ecad1d42ee4012f5aa147b1b327a", null ],
    [ "HDR_USER_AGENT", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aadee92777b096085e98d344620edd451", null ],
    [ "HDR_ACCEPT", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a6fbad6cdbde28b5b63fd3f8c6a97c8af", null ],
    [ "HDR_ACCEPT_ENCODING", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a3afc4afe656f1f155be817a9a9548352", null ],
    [ "HDR_ACCEPT_CHARSET", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a8bbe82a9d27ea9e63f8842262501e14f", null ],
    [ "HDR_ACCEPT_LANGUAGE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a6111ed154390056077b67620ac3bf302", null ],
    [ "HDR_ACCEPT_RANGE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a59a1124abfd8ca5ac89b3b3ab8e6d500", null ],
    [ "HDR_CONTENT_ENCODING", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a139baf6629644eec9fe1c3577f8086a1", null ],
    [ "HDR_CONTENT_LANGUAGE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a3bbc71c4fe0a90a18d9f8d3bbe650361", null ],
    [ "HDR_CONTENT_LOCATION", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a17e260795ddf3fc6ac5f1ba410a27b1f", null ],
    [ "HDR_CONTENT_RANGE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aff26b4e5e80b47092fa723966412dbbc", null ],
    [ "HDR_IF_RANGE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ab2f6f295de5f36eeaae53810c3e7b5cd", null ],
    [ "HDR_RANGE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#af9d04a35751176742736ffe2473584fd", null ],
    [ "HDR_TE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aece0f946c47e9de1a78a7f3d350b4a70", null ],
    [ "parser_pos_t", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa87468a6ef98f54ebd919c9ba6ea0f32", [
      [ "POS_REQUEST_LINE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa87468a6ef98f54ebd919c9ba6ea0f32ab1e31ed965f8f14b44c13400b2c0d19b", null ],
      [ "POS_RESPONSE_LINE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa87468a6ef98f54ebd919c9ba6ea0f32a53dae8c9702c5b1fbf5c6f76ab947cb9", null ],
      [ "POS_HEADERS", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa87468a6ef98f54ebd919c9ba6ea0f32afafcf8b5b9f5e60d9a76452d3e5974ce", null ],
      [ "POS_ENTITY", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa87468a6ef98f54ebd919c9ba6ea0f32a7aca91dc31fab4816740615dfce6ddb9", null ],
      [ "POS_COMPLETE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa87468a6ef98f54ebd919c9ba6ea0f32a03f2adb1c5f5dde625f972a145c75002", null ]
    ] ],
    [ "http_method_t", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2", [
      [ "HTTPMETHOD_PUT", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2a17597dbe94c390770d8513e18c2231a0", null ],
      [ "HTTPMETHOD_DELETE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2a8001615724f318b4fb620b38e4adc1c1", null ],
      [ "HTTPMETHOD_GET", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2acf941e299d45833d1df89328cc493dff", null ],
      [ "HTTPMETHOD_HEAD", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2afbab61bc9f961011f78f7388f3e316bc", null ],
      [ "HTTPMETHOD_POST", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2a227d645fdc722ba4153a6c9914b12f91", null ],
      [ "HTTPMETHOD_MPOST", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2a03f527c3c141dfeb6d270a171a6accdd", null ],
      [ "HTTPMETHOD_SUBSCRIBE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2adb25aed5fa91288da7e43e2c5e4a7295", null ],
      [ "HTTPMETHOD_UNSUBSCRIBE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2ae849a93b943939df75618cb1e2ab0664", null ],
      [ "HTTPMETHOD_NOTIFY", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2a46718299289b5eae0b48d15e755d8676", null ],
      [ "HTTPMETHOD_MSEARCH", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2a75e95c9228e2b0a8b4663fbf3ce7271b", null ],
      [ "HTTPMETHOD_UNKNOWN", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2ac8fc3465ff686177e0573772ceef4e9f", null ],
      [ "SOAPMETHOD_POST", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2a9688aa06b649251b6c50710b0e1048a0", null ],
      [ "HTTPMETHOD_SIMPLEGET", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a2fd930fba66817097e73ba9b75800eb2a87d3b7abaf2af24e434e2e372597fd50", null ]
    ] ],
    [ "parse_status_t", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aaec739c6f0ff93b139062af2ba4b0f1d", [
      [ "PARSE_SUCCESS", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aaec739c6f0ff93b139062af2ba4b0f1da9674022bcca440239f8c53bcbd4101a0", null ],
      [ "PARSE_INCOMPLETE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aaec739c6f0ff93b139062af2ba4b0f1da5900ec47943a7136371702edb225d4a3", null ],
      [ "PARSE_INCOMPLETE_ENTITY", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aaec739c6f0ff93b139062af2ba4b0f1da952f16db1d3b76fc3ce841a7ca836de6", null ],
      [ "PARSE_FAILURE", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aaec739c6f0ff93b139062af2ba4b0f1da8a5eaa538a709e6b7baf4eab139afaaf", null ],
      [ "PARSE_OK", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aaec739c6f0ff93b139062af2ba4b0f1dac6100102562753aa9aa43bd7885baa6e", null ],
      [ "PARSE_NO_MATCH", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aaec739c6f0ff93b139062af2ba4b0f1da5a7b1c01cd870b96dde1b5610fae2206", null ],
      [ "PARSE_CONTINUE_1", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aaec739c6f0ff93b139062af2ba4b0f1dafa49a399f330e793f8b8d6a7c5a05bec", null ]
    ] ],
    [ "httpmsg_destroy", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a01540e038a0e2f3aab83c0876715ec6f", null ],
    [ "httpmsg_find_hdr_str", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa3bec615c3b93c4115110df5cad170ef", null ],
    [ "httpmsg_find_hdr", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a6136244ecac6f1942c0b48fd75e59840", null ],
    [ "parser_request_init", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a5b6fce55ef2bc9415001527cf3323a41", null ],
    [ "parser_response_init", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a9349fffebb03eb45bff46ccac9fe357c", null ],
    [ "parser_parse", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a4eff37cf0315cb3beccc9e9d03e834c7", null ],
    [ "parser_parse_responseline", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a976f5873d7e6cd97db075b87f47c74c6", null ],
    [ "parser_parse_headers", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ae885549e0e5aaf2a07f09c011e006ba5", null ],
    [ "parser_parse_entity", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aa2001c56d3ad1bc1d89995338850ada4", null ],
    [ "parser_get_entity_read_method", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ae6f3ead0905d31409977a3ba203ebfa4", null ],
    [ "parser_append", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a94377ad173dc8fd327dcec217ec46544", null ],
    [ "matchstr", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#adb6f04b4682aae5e4690b672025b6856", null ],
    [ "raw_to_int", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#ab6f3362dea50f1228e5a6b7f99acc7ce", null ],
    [ "raw_find_str", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a6f102e766ab6d15cdd7c7c31d550903f", null ],
    [ "method_to_str", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a34ffcb549b62290faab9e8f46a030bdb", null ],
    [ "print_http_headers", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#a779cc41615042d7c56e4df25fb4c4a3e", null ],
    [ "Http_Header_Names", "df/d6f/Compa_2src_2inc_2httpparser_8hpp.html#aaa47881083ab4dcedd80b73631fa6d4f", null ]
];