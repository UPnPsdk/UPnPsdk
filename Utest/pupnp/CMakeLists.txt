# Copyright (C) 2026+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
# Redistribution only with this Copyright remark. Last modified: 2026-03-15

cmake_minimum_required(VERSION 3.18)
include(UPnPsdk-ProjectHeader)

project(GTESTS_COMPA VERSION 0001
                  DESCRIPTION "Tests for pupnp and compatible code"
                  HOMEPAGE_URL "https://github.com/UPnPsdk")


add_subdirectory(uri.d)
