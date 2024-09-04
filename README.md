# The UPnP+™ Software Development Kit
**Under development!**

## 1. Overview
This is a fork of the <a href="https://github.com/pupnp/pupnp">Portable SDK for UPnP Devices (pupnp)</a> with the aim of a complete re-engeneering based on <!-- <a href="https://openconnectivity.org/upnp-specs/UPnP-arch-DeviceArchitecture-v2.0-20200417.pdf">UPnP™ Device Architecture 2.0</a>. --><a href="https://upnpsdk.github.io/UPnPsdk/UPnP-arch-DeviceArchitecture-v2.0-20200417.pdf">UPnP™ Device Architecture 2.0</a>.

The following general goals are in progress or planned:
- Suporting main Unix derivates (e.g. Debian/Ubuntu, macOS), MS Windows
- Drop in compatibillity with the old pUPnP library
- Continued optimization for embedded devices
- Based on C++ with exceptions instead of C
- Object oriented programming
- Unit-Test driven development
- Integrating CMake for managing the code
- Focus on IPv6 support
- Mandatory OpenSSL support

## 2. Technical Documentation
Here you can find the [Technical Documentation](https://upnpsdk.github.io/UPnPsdk/).<br/>
If you want to be compatible with the classic pUPnP library here you find the <a href="https://upnpsdk.github.io/UPnPsdk/d9/d54/group__compaAPI.html">Compatible API</a>.
To use the new written object oriented part of the library here you find its <a href="https://upnpsdk.github.io/UPnPsdk/d6/d14/group__upnplibAPI.html">UPnPsdk API</a>.

## 3. Version numbering
We follow the [Semantic Versioning](https://semver.org/spec/v2.0.0.html#semantic-versioning-200). In short it defines
- MAJOR version when you make incompatible API changes,
- MINOR version when you add or modify functionality in a backwards compatible manner, and
- PATCH version when you make backwards compatible bug fixes.

The UPnPsdk version starts with 0.1.0. Major version 0 means it's not a productive version but under development. Because we will use CMake to manage the code it can be seen as integral part of the project. The UPnPsdk version number will also reflect changes to the CMake configuration.

This fork is based on [release-1.14.19](https://github.com/pupnp/pupnp/releases/tag/release-1.14.19) of the pupnp library.

## 4. Milestones
- Ongoing: create extensive Unit Tests without modification of the old source code
- Ongoing: define C++ interfaces for the API
- Ongoing: change old C program to C++ objects but preserve drop in compatibility
- Ongoing: support IP6
- Ongoing: support OpenSSL

<!--
## 4. Cmake subprojects
                                      UPnPsdk
                                         |
            +---------------+------------+-------------+----------------+
            |               |            |             |                |
       UPNPLIB_CORE    UPNPLIB_IXML    PUPNP    UPNPLIB_GTESTS    UPNPLIB_SAMPLE
                                       /   \
                              PUPNP_UPNP   PUPNP_IXML
                                              \
                                             PUPNP_IXML_TEST

These names are also the names of the CMake subprojects.
-->

## 5. Build Instructions
You need to have `git` installed. Just clone this repository:

    ~$ git clone https://github.com/UPnPsdk/UPnPsdk.git UPnPsdk-project/
    ~$ cd UPnPsdk-project/

You are now in the relative root directory of the program source tree. All project directory references are relative to its root directory (CMAKE_SOURCE_DIR) that is `UPnPsdk-project/` if you haven't used another directory name with the clone command above. If in daubt with file access or with executing you should first ensure that you are in the projects root directory.

### 5.1. Linux and MacOS build
Be in the projects root directory. First configure then build:

    UPnPsdk-project$ cmake -S . -B build/
    UPnPsdk-project$ cmake --build build/

To install the include files and libraries to /usr/local/include and /usr/local/lib

    UPnPsdk-project$ cmake --install build/

For shared linked applications the dynamic link loader must know where to find e.g. <code>/usr/local/lib/libUPnPsdk.so</code>. It has a cache where it stores such paths. To update its cache with the info to the new installed shared library you can just execute one time for all subsequent calls of your application now and later even after a reboot

    ~$ ldconfig
    ~$ yourUPnPapp1       # now
    ~/local$ yourUPnPapp2 # next day

If you do not like to modify your system environment you can instead prefix the call to your application with the hint

    ~$ LD_LIBRARY_PATH=/usr/local/lib yourUPnPapp1

Of course you can also export the environment variable to the current volatile environment so you do not have to prefix every call in this environment.

    ~$ export LD_LIBRARY_PATH=/usr/local/lib
    ~$ yourUPnPapp1
    ~$ yourUPnPapp2

This all doesn't matter if you link against the static libraries.

To uninstall the include files and libraries as long as the build/ directory is available and clean up a build just do

    UPnPsdk-project$ sudo xargs rm < build/install_manifest.txt
    UPnPsdk-project$ rm -rf build/

If you like you can backup the file <code>build/install_manifest.txt</code> for later uninstallation without build/ directory.

### 5.2. Microsoft Windows build
The development of this UPnP Software Development Kit has started on Linux. So for historical reasons it uses POSIX threads (pthreads) as specified by [The Open Group](http://get.posixcertified.ieee.org/certification_guide.html). Unfortunately Microsoft Windows does not support it so I have to use a third party library. I use the well known and well supported [pthreads4w library](https://sourceforge.net/p/pthreads4w). It will be downloaded on Microsoft Windows and compiled with building the project and should do out of the box. To build the UPnPsdk you need a Developer Command Prompt. How to install it is out of scope of this description. Microsoft has good documentation about it. For example this is the prompt I used (example, maybe outdated):

    **********************************************************************
    ** Visual Studio 2019 Developer Command Prompt v16.9.5
    ** Copyright (c) 2021 Microsoft Corporation
    **********************************************************************
    [vcvarsall.bat] Environment initialized for: 'x64'

    ingo@WIN10-DEVEL C:\Users\ingo> pwsh
    PowerShell 7.4.1
    PS C:\Users\ingo>

First configure then build:

    PS C: UPnPsdk-project> cmake -S . -B build/
    PS C: UPnPsdk-project> cmake --build build/ --config Release

After build don't forget to copy the needed `build/_deps/pthreads4w-build/pthread*.dll` library file to a location where your program can find it. Copying it to your programs directory or to the system directory `Windows\System32` will always do. Within the project development directory tree (default root UPnPsdk-project/) built programs and libraries find its dll files. There is nothing to do.

To clean up a build just delete the build/ folder:

    PS C: UPnPsdk-project> rm -rf build/

If you need more details about installation of POSIX threads on Microsoft Windows I have made an example at [github pthreadsWinLin](https://github.com/upnplib/pthreadsWinLin.git).

<!--
### 5.3 Googletest build
I strongly recommend to use shared gtest libraries for this project because there are situations where static and shared libraries are linked together. Using static linked Googletest libraries may fail then. If you know what you ar doing and you are able to manage possible linker errors you can try to use static built Googletest libraries.

    # strongly recommended shared libs
    UPnPsdk-project$ cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Debug -D UPNPLIB_WITH_GOOGLETEST=ON
    UPnPsdk-project$ cmake --build build/ --config Debug

    # or alternative static libs
    UPnPsdk-project$ cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Debug -D UPNPLIB_WITH_GOOGLETEST=ON -D GTESTS_WITH_SHARED_LIBS=OFF
    UPnPsdk-project$ cmake --build build/ --config Debug

Using build type "Debug" is not necessary for Googletest but it will enable additional debug messages from the library. if you don't need it you can just use "Release" instead of "Debug" above as option.

## 5. Configure Options for cmake
Option prefixed with -D | Default | Description
-------|---------|---
UPNP_GOOGLETEST=[ON\|OFF] | OFF | Enables installation of GoogleTest for Unit-Tests. For details look at section *Googletest build*.
BUILD_SHARED_LIBS=[ON\|OFF] | OFF | This option affects only Googletest to build it with shared gtest libraries. UPnPsdk is always build shared and static.
CMAKE_BUILD_TYPE=[Debug\| Release\| MinSizeRel\| RelWithDebInfo] | Release | If you set this option to **Debug** you will have additional development support. The mnemonic program symbols are compiled into the binary programs so you can better examine the code and simply debug it. But I think it is better to write a Unit Test instead of using a debugger. Compiling with symbols increases the program size a big amount. With focus on embedded devices this is a bad idea.
PT4W_BUILD_TESTING=[ON\|OFF] | OFF | Runs the testsuite of pthreads4w (PT4W) with nearly 1000 tests. It will take some time but should be done at least one time.

- -D DEVEL=OFF          This enables some additional information for development. It preserves installation options that normaly will be deleted after Installation for Optimisation so you can examine them. These are mainly the installation directory from **pthread4w** and its temporary installation files even on a non MS Windows environment.
-->

<pre>
Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, \<Ingo\@Hoeft-online.de\>
Redistribution only with this Copyright remark. Last modified: 2024-09-05</pre>
