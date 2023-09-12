# complex #

[![windows](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/windows.yml) [![linux](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/linux.yml) [![macos](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/macos.yml) [![clang-format](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml/badge.svg)](https://github.com/BlueQuartzSoftware/complex/actions/workflows/format_push.yml)

## License and Public Release Notice ##

This software library is directly supported by USAF Contract *FA8650-22-C-5290* and has been cleared as publicly releasable under the following:

    AFRL has completed the review process for your case on 14 Sep 2022:

    Subject: DREAM3D_Project (O) Shah #-11  (Software Code)

    Originator Reference Number: RX22-0852
    Case Reviewer: M. Allen
    Case Number: AFRL-2022-4403

    The material was assigned a clearance of CLEARED on 14 Sep 2022.

This clearance is in effect until 14 SEPT 2025 at which point any new additions created under USAF funding should be cleared. Public additions through the normal PR process will not be affected.

## Introduction ##

`complex` is a rewrite of the [SIMPL](https://www.github.com/bluequartzsoftware/simpl) library upon which [DREAM3D](https://www.github.com/bluequartzsoftware/dream3d) is written. This library aims to be fully C++17 compliant, removes the need for Qt5 at the library level and brings more flexible data organization and grouping options.

## Dependent Libraries ##

| Library Name | GitHub Source | Version |
|--------------|---------------|---------|
| boost-mp11  | <https://github.com/boostorg/mp11>  | 1.77.0 |
| catch2  | <https://github.com/catchorg/Catch2>  | 2.13.6 |
| eigen3  |  <https://gitlab.com/libeigen/eigen.git> | 3.3.9 |
| expected-lite  | <https://github.com/martinmoene/expected-lite>  | 0.5.0 |
| fmt  | <https://github.com/fmtlib/fmt>  | 10.0.0 |
| hdf5  | <https://github.com/HDFGroup/hdf5/>  | 1.12.1 |
| itk  | <https://github.com/InsightSoftwareConsortium/ITK.git>  | 5.2.1 |
| nlohmann-json  | <https://github.com/nlohmann/json/>  | 3.11.2 |
| pybind11  | <https://github.com/pybind/pybind11.git>  | 2.10 |
| span-lite  | <https://github.com/martinmoene/span-lite>  | 0.10.3 |
| tbb  | <https://github.com/oneapi-src/onetbb>  | 2021.10.0 |
| ebsdlib  | <https://www.github.com/bluequartzsoftware/EBSDLib>   | 1.0.23 |
| h5support  | <https://www.github.com/bluequartzsoftware/H5Support>  | 1.0.8 |
| nod  |<https://github.com/fr00b0/nod.git>  | 0.5.2 |
| reproc  |<https://github.com/DaanDeMeyer/reproc>  | 14.2.4 |

## Prerequisites ##

In order to compile `complex` you will need a C++17 compiler suite installed on your computer.

+ Compiler
  + Windows Visual Studio 2019 v142 toolset
  + macOS 11.0 and Xcode 12.4 or higher
  + Linux with GCC Version 9.0 or higher or clang.

## Install vcpkg ##

The `complex` project uses the [vcpkg](https://www.github.com/microsoft/vcpkg) to manage it's dependent libraries. If this is not already installed on your system then you will need to download and compile it.

### Windows ###

Clone the vcpkg repository into a location that it will be used from. inside your home directory or at `C:/vcpkg` is a reasonable spot. *DO NOT USE A PATH WITH SPACES IN ANY OF THE FOLDERS*.

    cd C:/Users/[USERNAME]/Applications
    git clone https://www.github.com/microsoft/vcpkg

The `bootstrap-vcpkg.bat` file should be run automatically by CMake the first time. This will create the `vcpkg.exe` file. Additionally CMake should automatically find `vcpkg.exe`. If CMake does not find it, you may need to add it to your `PATH` variable.

## Clone Appropriate Repositories ##

Within the folder DREAM3D-Dev clone both the `complex` and `DREAM3D_Data` repositories. The `DREAM3D_Data` repo is optional but does contain testing data.

Create a location to keep the `complex` repositories and make builds. You can do either in-source our out-of-source builds.

    git clone --recursive https://github.com/bluequartzsoftware/complex

## Configure complex with CMake ##

For this example we are going to do an "in-source" build. By default git will ignore some basic names for build directories such as `Debug, Release, x64`. CMake can generate lots of project files from ninja, to nmake to Visual Studio. For this example we are going to use the `ninja` generator so we will need to keep the Debug and Release builds separated.

The first time `complex` is configured with CMake, VCPKG will download, build and install the needed dependent libraries. This can take a few minutes so be patient. After `complex` is configured you can build it using your IDE (Visual Studio) or ninja (QtCreator, CLion) or cmake itself.

### Windows with Visual Studio IDE ###

This example shows how to configure complex to build using Visual Studio IDE

    set BUILD_DIR=VisualStudio
    set VCPKG_INSTALL_ROOT=C:/Appications/vcpkg
    cd complex
    mkdir %BUILD_DIR%
    cd %BUILD_DIR%

    cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALL_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=%BUILD_DIR% -DVCPKG_MANIFEST_FEATURES="tests;parallel"  -DDREAM3D_Data_Dir=../../DREAM3D_Data ../complex

### Windows with Ninja and IDE of choice (QtCreator, CLion, command line) ###

Adjust the below commands to the sytle of your shell (The below example is done in Windows Command Prompt Batch file style).

In this example we are building a Release version of compled into a directory called `Release` inside the source directory. We will be using the `ninja` generator so be sure that ninja.exe is on your PATH

    set NINJA_INSTALL=C:/Applications/ninja-win
    set PATH=%PATH%:%NINJA_INSTALL%
    set BUILD_DIR=Release
    set VCPKG_INSTALL_ROOT=C:/Appications/vcpkg
    cd complex
    mkdir %BUILD_DIR%
    cd %BUILD_DIR%

    cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALL_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=%BUILD_DIR% -DVCPKG_MANIFEST_FEATURES="tests;parallel" -DDREAM3D_Data_Dir=../../DREAM3D_Data ../complex

### macOS ARM64/M1 Compile and IDE of choice (QtCreator, CLion, command line) ###

    export NINJA_INSTALL=/opt/local/bin
    export PATH=$PATH:$NINJA_INSTALL
    export BUILD_DIR=Release
    export VCPKG_INSTALL_ROOT=/opt/local/vcpkg
    # This is used for Apple Silicon ARM64
    export VCPKG_TARGET_TRIPLET=arm64-osx-dynamic
    # This is used for Intel x64
    export VCPKG_TARGET_TRIPLET=x64-osx-dynamic
    cd complex
    mkdir $BUILD_DIR
    cd $BUILD_DIR

    cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALL_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=$BUILD_DIR -DVCPKG_MANIFEST_FEATURES="tests;parallel" -DDREAM3D_DATA_DIR=$HOME/Workspace1/DREAM3D_Data -DVCPKG_TARGET_TRIPLET:STRING=$VCPKG_TARGET_TRIPLET ../complex

## Required CMake Arguments ##

+ `-G` and one of the supported CMake generator strings
  + `Ninja` works very well on all platforms if you have [ninja](https://github.com/ninja-build/ninja/releases)
+ `-DCMAKE_TOOLCHAIN_FILE=[path to vcpkg.cmake file]`
+ `-DCMAKE_BUILD_TYPE=[Debug|Release]` but **ONLY** if you are using Ninja or 'Unix MakeFiles' as the generator
+ `-DVCPKG_MANIFEST_FEATURES=....` build options
+ `-DDREAM3D_DATA_DIR=....` Absolute path to a Directory to store downloaded test files. This can be shared between build directories
+ `-DVCPKG_TARGET_TRIPLET=....` Correct VCPKG triplet for your platform/operating system
  + MacOS: [x64-osx-dynamic | arm64-osx-dynamic]
  + Linux: x64-linux-dynamic
  + Windows:

## Adding Plugins ##

By default the **only** plugin that is compiled is the `ComplexCore` plugin. If you would like to build any of the additional plugins you can provide additional arguments to the configuration command. **NOTE** For the `COMPLEX_EXTRA_PLUGINS` CMake argument, if you have multiple plugins separate each with a `;` character (which is why we double quote the value for the COMPLEX_EXTRA_PLUGINS variable.)

### ITKImageProcessing ###

This plugin gives complex access to the ability to read/write images and use the **ITK** library to process images.

There are 2 arguments that need to be added to the CMake configuration command

+ `-DCOMPLEX_EXTRA_PLUGINS="ITKImageProcessing"`
+ `-DVCPKG_MANIFEST_FEATURES="tests;parallel;itk"`

### OrientationAnalysis ###

This plugin gives complex the ability to process typical EBSD style of data.

+ `-DCOMPLEX_EXTRA_PLUGINS="OrientationAnalysis"`
+ `-DVCPKG_MANIFEST_FEATURES="tests;parallel;ebsd"`

## VCPKG Options ##

### Defining where VCPKG installs the dependent libraries ###

By default VCPKG will install any library that it compiles into vcpkg specific and platform specific locations. If you would like to specifically set where those libraries are installed the following cmake code will allow that:

+ `-DVCPKG_INSTALLED_DIR=$HOME/workspace/vcpkg-installed`

### Disable VCPKG from checking for updates with each configuration ###

Be default VCPKG will check for updates to the libraries that it compiles. If you would like to skip this step each time the following CMake code is needed:

+ `-DVCPKG_MANIFEST_INSTALL=OFF`

## Defining where test data is stored ##

Complex and its plugins require test files to be able to perform the unit tests. By default these will be store inside the build directory. This means that if you have multiple build directories, a separate copy of all the test files will be downloaded for each build directory. The developer can set the `DREAM3D_DATA_DIR` variable to a path that will be used. They will need to set this for **each** build directory. You **MUST** define `DREAM3D_DATA_DIR` using an absolute path. Relative paths **will not work**.

+ `-DDREAM3D_DATA_DIR=/opt/local/DREAM3D_Data`

The developer can also turn off the downloading of any test data with the following:

+ `-DCOMPLEX_DOWNLOAD_TEST_FILES=OFF`

## Python Bindings ##

Python bindings are available for complex. To install them, please use an Anaconda virtual environment like the following:

```shell
conda config --add channels conda-forge
conda config --set channel_priority strict
conda create -n cxpython python=3.8
conda activate cxpython
conda install -c bluequartzsoftware complex
```
