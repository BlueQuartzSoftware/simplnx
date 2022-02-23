# complex #

[![windows](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml) [![linux](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml) [![macos](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml) [![clang-format](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml)

## Introduction ##

`complex` is a rewrite of the [SIMPL](https://www.github.com/bluequartzsoftware/simpl) library upon which [DREAM3D](https://www.github.com/bluequartzsoftware/dream3d) is written. This library aims to be fully C++17 compliant, removes the need for Qt5 at the library level and brings more flexible data organization and grouping options.

## Prerequisites ##

In order to compile `complex` you will need a C++17 compiler suite installed on your computer.

+ Compiler
  + Windows Visual Studio 2017 v141 toolset
  + macOS 10.15 and Xcode 12.4 or higher
  + Linux with GCC Version 9.0 or higher

## Install vcpkg ##

The `complex` project uses the [vcpkg](https://www.github.com/microsoft/vcpkg) to manage it's dependent libraries. If this is not already installed on your system then you will need to download and compile it.

### Windows ###

Clone the vcpkg repository into a location that it will be used from. inside your home directory or at `C:/vcpkg` is a reasonable spot. *DO NOT USE A PATH WITH SPACES IN ANY OF THE FOLDERS*.

```(lang-console)
cd C:/Users/[USERNAME]/Applications
git clone ssh://git@github.com/microsoft/vcpkg
```

Use the Windows Explorer to open the vcpkg directory and double click the `bootstrap-vcpkg.bat` file. This will create the `vcpkg.exe` file. You will need to ensure that the path to the `vcpkg.exe` is on your `PATH` environment variable. There are several ways to do this in Windows, select the one that is right for your environment.

### MacOS/Linux ###

Select an install location (the example below uses `/Users/Shared/DREAM3DNX_SDK/`) and then clone, build and install vcpkg.

```(lang-console)
mkdir -p /Users/Shared/DREAM3DNX_SDK/
cd /Users/Shared/DREAM3DNX_SDK/
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.sh -disableMetrics
export PATH=/Users/Shared/DREAM3DNX_SDK/vcpkg:$PATH
```

## Clone Appropriate Repositories ##

Within the folder DREAM3D-Dev clone both the `complex` and `DREAM3DNX` repositories

Create a location to keep the `complex` repositories and make builds. You can do either in-source our out-of-source builds.

```(lang-console)
git clone --recursive ssh://git@github.com/bluequartzsoftware/complex
git clone --recursive ssh://git@github.com:DREAM3D/DREAM3D_Data.git
```

## Configure complex with CMake (Windows & MSVC) ##

For this example we are going to do an "in-source" build. By default git will ignore some basic names for build directories such as `Debug, Release, x64`. CMake can generate lots of project files from ninja, to nmake to Visual Studio. For this example we are going to use the `ninja` generator so we will need to keep the Debug and Release builds separated.

The first time `complex` is configured with CMake, VCPKG will download, build and install the needed dependent libraries. This can take a few minutes so be patient. After `complex` is configured you can build it using your IDE (Visual Studio) or ninja (QtCreator, CLion) or cmake itself.

```(lang-console)
cd complex
mkdir Release
cd Release

cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/Applications/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Releas -DVCPKG_MANIFEST_DIR=C:/Users/mjackson/Workspace1/complex -DVCPKG_MANIFEST_FEATURES="tests;parallel" -DVCPKG_INSTALLED_DIR=C:/Users/mjackson/Workspace1/vcpkg-installed -DVCPKG_MANIFEST_INSTALL=ON -DCMAKE_MAKE_PROGRAM=C:/Applications/ninja-win/ninja.exe ../
```

Once the project is configured it should build using **ninja.exe**

Stop the VCPKG from checking for updates for each build adjust the cmake variable **-DVCPKG_MANIFEST_INSTALL=OFF**

