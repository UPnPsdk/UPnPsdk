<!--
!!! ATTENTION !!!
Don't edit file README.md. It will be overwritten by cmake/README.md.cmake and
your work is lost on next CMake configuration. Only edit cmake/README.md.cmake.

-->
# The UPnP+™ Software Development Kit
**Under development!**

## 1. Overview
This is a fork of the <a href="https://github.com/pupnp/pupnp">Portable SDK for UPnP Devices (pupnp)</a> with the aim of a complete re-engeneering based on <!-- <a href="https://openconnectivity.org/upnp-specs/UPnP-arch-DeviceArchitecture-v2.0-20200417.pdf">UPnP™ Device Architecture 2.0</a>. --><a href="https://upnpsdk.github.io/UPnPsdk/UPnP-arch-DeviceArchitecture-v2.0-20200417.pdf">UPnP™ Device Architecture 2.0</a>.

The following general goals are in progress or planned:
- Suporting main Unix derivates (e.g. Debian/Ubuntu, macOS), MS Windows
- Drop in compatibillity with the old pUPnP library
- Continued optimization for embedded devices
- Including support of 32 bit architectures
- Based on C++ with exceptions instead of C
- Object oriented programming
- Unit-Test driven development
- Integrating CMake for managing the code
- Focus on IPv6 support
- Mandatory OpenSSL support

## 2. Limitations
- Development for old **pupnp** code is not supported. No header files for **pupnp** are provided, only for the **UPnPsdk**.
- On the old <a href="https://github.com/pupnp/pupnp">Portable SDK for UPnP Devices (pupnp)</a> already deprecated functions are not supported anymore. These are the functions `UpnpSetContentLength(), handle_query_variable(), UpnpGetServiceVarStatus(), UpnpGetServiceVarStatusAsync()`.
- Large-file support on 32 bit architectures is always enabled. Disabling **lfs** is not supported.

## 3. Technical Documentation
Here you can find the [Technical Documentation](https://upnpsdk.github.io/UPnPsdk/).<br/>
If you want to be compatible with the classic pUPnP library here you find the <a href="https://upnpsdk.github.io/UPnPsdk/d9/d54/group__compaAPI.html">Compatible API</a>.
To use the new written object oriented part of the library here you find its <a href="https://upnpsdk.github.io/UPnPsdk/d6/d14/group__upnplibAPI.html">UPnPsdk API</a>.

## 4. Version numbering
We follow the [Semantic Versioning](https://semver.org/spec/v2.0.0.html#semantic-versioning-200). In short it defines
- MAJOR version when you make incompatible API changes,
- MINOR version when you add or modify functionality in a backwards compatible manner, and
- PATCH version when you make backwards compatible bug fixes.

The UPnPsdk version starts with 0.1.0. Major version 0 means it's not a productive version but under development. Because we will use CMake to manage the code it can be seen as integral part of the project. The UPnPsdk version number will also reflect changes to the CMake configuration.

This fork is based on [release-1.14.24](https://github.com/pupnp/pupnp/releases/tag/release-1.14.24) of the pupnp library.

## 5. Milestones
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

## 6. Build Instructions
You need to have `git` installed. On Microsoft Windows you must "Enable symbolic links" when asked for "Configuring extra options" while running the <a href="https://git-scm.com/downloads/win">git installation program</a>. Then just clone this repository:

    ~$ git clone https://github.com/UPnPsdk/UPnPsdk.git UPnPsdk-project/
    ~$ cd UPnPsdk-project/

You are now in the relative root directory of the program source tree. On Microsoft Windows you should first provide symbolic links with `git config core.symlinks true` on the command line. More details and troubleshooting at <a href="https://stackoverflow.com/a/59761201/5014688">Git symbolic links in Windows</a>.

All project directory references are relative to its root directory (CMAKE_SOURCE_DIR) that is `UPnPsdk-project/` if you haven't used another directory name with the clone command above. If in daubt with file access or with executing you should first ensure that you are in the projects root directory.

### 6.1. Linux and MacOS build
Be in the projects root directory. First configure:

    UPnPsdk-project$ cmake -S . -B build/

If you want to compile for a 32 bit architecture on a 64 bit host do:

    UPnPsdk-project$ cmake -S . -B build/ -D CMAKE_C_FLAGS=-m32 -D CMAKE_CXX_FLAGS=-m32

Then build the project:

    UPnPsdk-project$ cmake --build build/

To install the libraries and test-executables to `/usr/local/bin` and `/usr/local/lib`

    UPnPsdk-project$ sudo cmake --install build/

To install the header files for development to `/usr/local/include/`

    UPnPsdk-project$ sudo cmake --install build/ --component Development

To uninstall all as long as the `build/` directory is available

    UPnPsdk-project$ for f in build/install_manifest*.txt do cat $f && echo done | sudo xargs rm

To uninstall only components:

    # Uninstall libraries and executable
    UPnPsdk-project$ sudo xargs rm < build/install_manifest.txt

    # Unistall Development components
    UPnPsdk-project$ sudo xargs rm < build/install_manifest_Development.txt

Remove all CMake configuration and build. Consider to backup the install manifest files before doing this.

    UPnPsdk-project$ rm -rf build/

Little helper:

    UPnPsdk-project$ tree /usr/local

    # Check the stored RPATH or RUNPATH needed to find the shared libraries, for example
    UPnPsdk-project$ objdump -x /usr/local/bin/api_calls-csh | grep PATH

If you like you can backup the files <code>build/install_manifest*.txt</code> for later uninstallation without build/ directory.

### 6.2. Microsoft Windows build
The development of this UPnP Software Development Kit has started on Linux. So for historical reasons it uses POSIX threads (pthreads) as specified by [The Open Group](http://get.posixcertified.ieee.org/certification_guide.html). Unfortunately Microsoft Windows does not support it so we have to use a third party library. I use the well known and well supported [pthreads4w library](https://sourceforge.net/p/pthreads4w). It will be downloaded on Microsoft Windows and compiled with building the project and should do out of the box.

To build the UPnPsdk you need a Developer Command Prompt. How to install it is out of scope of this description. Microsoft has good documentation about it. You can find it at [Use the Microsoft C++ toolset from the command line](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170). For example this is the prompt I used (example, maybe outdated):

    **********************************************************************
    ** Visual Studio 2022 Developer Command Prompt v17.11.1
    ** Copyright (c) 2022 Microsoft Corporation
    **********************************************************************
    [vcvarsall.bat] Environment initialized for: 'x86_x64'

    ingo@WIN11-DEVEL C:\Users\ingo>pwsh
    PowerShell 7.4.6
    PS C:\Users\ingo>

First configure then build. Because CMake is confusing the architecture without hint it is important to always specify it with `-A x64` even if it's the default. Otherwise CMake will install the SDK with 64 bit architecture to `"C:\Program Files (x86)"`.

    PS C: UPnPsdk-project> cmake -S . -B build/ -A x64
    PS C: UPnPsdk-project> cmake --build build/ --config Release

 As shown above you have used the "**x64** Native Tools Command Prompt for VS 20xx" for the default 64 bit architecture. To compile for a 32 bit architecture you must use the "**x86** Native Tools Command Prompt for VS 20xx" and specify the architecture with option `-A Win32` to cmake as follows:

    PS C: UPnPsdk-project> cmake -S . -B build/ -A Win32
    PS C: UPnPsdk-project> cmake --build build/ --config Release

To install the SDK with test-executables do:

    PS C: UPnPsdk-project> cmake --install build/ --config Release

To install the header files for development do:

    PS C: UPnPsdk-project> cmake --install build/ --config Release --component Development

To uninstall the SDK do:

    PS C: UPnPsdk-project> Remove-Item -Path "C:\Program Files\UPnPsdk\" -Recurse
    PS C: UPnPsdk-project> Remove-Item -Path "C:\Program Files (x86)\UPnPsdk\" -Recurse

To uninstall only components as long as the `build/` directory is available:

    # Uninstall libraries and executable
    PS C: UPnPsdk-project> Get-Content .\build\install_manifest.txt | %{Remove-Item -Path $_}

    # Unistall Development components
    PS C: UPnPsdk-project> Get-Content .\build\install_manifest_Development.txt | %{Remove-Item -Path $_}

Remove all CMake configuration and build. Consider to backup the install manifest files before doing this.

    PS C: UPnPsdk-project> Remove-Item -Path ".\build\" -Recurse -Force

Little helper:

    # Get the architecture (32 or 64 bit) from an executable or a library, for example:
    PS C: UPnPsdk-project> dumpbin.exe -headers .\build\bin\Release\upnpsdk-compa.dll | Select-String -Pattern 'magic #'

If you need more details about installation of POSIX threads on Microsoft Windows I have made an example at [github pthreadsWinLin](https://github.com/upnplib/pthreadsWinLin.git).

### 6.3 OpenSSL
UPnPsdk uses [OpenSSL](https://www.openssl.org/) by default if it is supported by the underlaying operating system. You have always the option to [build OpenSSL from the sources](https://openssl-library.org/source/index.html). But nearly all distributions for Unix like platforms support OpenSSL out of the box or have it available in their program repositories ready to install. Unfortunately Microsoft Windows does not support OpenSSL. You may use the build-from-source option or you look for a third party that provide pre compiled binaries for installation. For example I use the [Win32/Win64 OpenSSL Installation](https://slproweb.com/products/Win32OpenSSL.html) from Shining Light Productions. With this installer I need to tell Windows where to find it after installation. The usual way is to add the program location to the PATH environment variable that would be in my case `"C:/Program Files/OpenSSL-Win64"`. Look for the right place on your system. This makes OpenSSL available on the whole system. But I prefer to modify the environment of the platform as less as possible. So I only use the option `"-D OPENSSL_ROOT_DIR"` that CMake understands, for example:

    cmake -S . -B build -A x64 -D OPENSSL_ROOT_DIR="C:/Program Files/OpenSSL-Win64"

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
Copyright (C) 2021+ GPL 3 and higher by Ingo Höft, <Ingo\@Hoeft-online.de>
Redistribution only with this Copyright remark. Last modified: 2025-05-01</pre>
