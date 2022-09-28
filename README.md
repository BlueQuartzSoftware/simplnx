# complex #

[![windows](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml) [![linux](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml) [![macos](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml) [![clang-format](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml)

## License and Public Release Notice ##

This software library is directly supported by USAF Contract *FA8650-22-C-5290* and has been cleared as publicly releasable under the following:

```
AFRL has completed the review process for your case on 14 Sep 2022:

Subject: DREAM3D_Project (O) Shah #-11  (Software Code)

Originator Reference Number: RX22-0852
Case Reviewer: M. Allen
Case Number: AFRL-2022-4403

The material was assigned a clearance of CLEARED on 14 Sep 2022.
```

This clearance is in effect until 14 SEPT 2025 at which point any new additions created under USAF funding should be cleared. Public additions through the normal PR process will not be affected.


## Introduction ##

`complex` is a rewrite of the [SIMPL](https://www.github.com/bluequartzsoftware/simpl) library upon which [DREAM3D](https://www.github.com/bluequartzsoftware/dream3d) is written. This library aims to be fully C++17 compliant, removes the need for Qt5 at the library level and brings more flexible data organization and grouping options.

## Dependent Libraries ##

| Library Name | GitHub Source | Version |
|--------------|---------------|---------|
| boost-mp11  | https://github.com/boostorg/mp11  | 1.77.0 |
| catch2  | https://github.com/catchorg/Catch2  | 2.13.6 |
| eigen3  |  https://gitlab.com/libeigen/eigen.git | 3.3.9 |
| expected-lite  | https://github.com/martinmoene/expected-lite  | 0.5.0 |
| fmt  | https://github.com/fmtlib/fmt  | 7.1.3 |
| hdf5  | https://github.com/HDFGroup/hdf5/  | 1.12.1 |
| itk  | https://github.com/InsightSoftwareConsortium/ITK.git  | 5.2.1 |
| nlohmann-json  | https://github.com/nlohmann/json/  | 3.9.1 |
| pybind11  | https://github.com/pybind/pybind11.git  | 2.6.2 |
| span-lite  | https://github.com/martinmoene/span-lite  | 0.10.3 |
| tbb  | https://github.com/oneapi-src/onetbb  | 2021.5.0 |
| ebsdlib  | https://www.github.com/bluequartzsoftware/EBSDLib   | 1.0.16 |
| h5support  | https://www.github.com/bluequartzsoftware/H5Support  | 1.0.8 |
| nod  |https://github.com/fr00b0/nod.git  | 0.5.2 |


## Prerequisites ##

In order to compile `complex` you will need a C++17 compiler suite installed on your computer.

+ Compiler
  + Windows Visual Studio 2017 v141 toolset
  + macOS 10.15 and Xcode 12.4 or higher
  + Linux with GCC Version 9.0 or higher or clang.

## Install vcpkg ##

The `complex` project uses the [vcpkg](https://www.github.com/microsoft/vcpkg) to manage it's dependent libraries. If this is not already installed on your system then you will need to download and compile it.

### Windows ###

Clone the vcpkg repository into a location that it will be used from. inside your home directory or at `C:/vcpkg` is a reasonable spot. *DO NOT USE A PATH WITH SPACES IN ANY OF THE FOLDERS*.

```(lang-console)
cd C:/Users/[USERNAME]/Applications
git clone https://www.github.com/microsoft/vcpkg
```

The `bootstrap-vcpkg.bat` file should be run automatically by CMake the first time. This will create the `vcpkg.exe` file. Additionally CMake should automatically find `vcpkg.exe`. If CMake does not find it, you may need to add it to your `PATH` variable.

### MacOS/Linux ###

Select an install location (the example below uses `/Users/Shared/DREAM3DNX_SDK/`) and then clone, build and install vcpkg.

```(lang-console)
mkdir -p /Users/Shared/DREAM3DNX_SDK/
cd /Users/Shared/DREAM3DNX_SDK/
git clone https://github.com/microsoft/vcpkg
export PATH=/Users/Shared/DREAM3DNX_SDK/vcpkg:$PATH
```

## Clone Appropriate Repositories ##

Within the folder DREAM3D-Dev clone both the `complex` and `DREAM3D_Data` repositories. The `DREAM3D_Data` repo is optional but does contain testing data.

Create a location to keep the `complex` repositories and make builds. You can do either in-source our out-of-source builds.

```(lang-console)
git clone --recursive https://github.com/bluequartzsoftware/complex
git clone --recursive https://github.com/DREAM3D/DREAM3D_Data
```

## Configure complex with CMake (Windows & MSVC) ##

For this example we are going to do an "in-source" build. By default git will ignore some basic names for build directories such as `Debug, Release, x64`. CMake can generate lots of project files from ninja, to nmake to Visual Studio. For this example we are going to use the `ninja` generator so we will need to keep the Debug and Release builds separated.

The first time `complex` is configured with CMake, VCPKG will download, build and install the needed dependent libraries. This can take a few minutes so be patient. After `complex` is configured you can build it using your IDE (Visual Studio) or ninja (QtCreator, CLion) or cmake itself.

```(lang-console)
cd complex
mkdir Release
cd Release

cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/Applications/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DVCPKG_MANIFEST_FEATURES="tests;parallel" ../../complex
```

Once the project is configured it should build using **ninja.exe**

Stop the VCPKG from checking for updates for each build adjust the cmake variable **-DVCPKG_MANIFEST_INSTALL=OFF**

