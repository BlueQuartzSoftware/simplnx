# Guide to Building From Source #

## Dependent Libraries ##

| Library Name | GitHub Source | Version |
|--------------|---------------|---------|
| boost-mp11  | <https://github.com/boostorg/mp11>  | 1.77.0 |
| catch2  | <https://github.com/catchorg/Catch2>  | 2.13.6 |
| eigen3  |  <https://gitlab.com/libeigen/eigen.git> | 3.4.0 |
| expected-lite  | <https://github.com/martinmoene/expected-lite>  | 0.5.1 |
| fmt  | <https://github.com/fmtlib/fmt>  | 10.1.1 |
| hdf5  | <https://github.com/HDFGroup/hdf5/>  | 1.14.4.3 |
| itk  | <https://github.com/InsightSoftwareConsortium/ITK.git>  | 5.2.1 |
| nlohmann-json  | <https://github.com/nlohmann/json/>  | 3.11.12 |
| pybind11  | <https://github.com/pybind/pybind11.git>  | 2.12 |
| span-lite  | <https://github.com/martinmoene/span-lite>  | 0.10.3 |
| tbb  | <https://github.com/oneapi-src/onetbb>  | 2021.4.0 |
| ebsdlib  | <https://www.github.com/bluequartzsoftware/EBSDLib>   | 1.0.30 |
| h5support  | <https://www.github.com/bluequartzsoftware/H5Support>  | 1.0.13 |
| nod  | <https://github.com/fr00b0/nod.git>  | 0.5.4 |
| reproc  | <https://github.com/DaanDeMeyer/reproc>  | 14.2.4 |

## Prerequisites ##

In order to compile `simplnx` you will need a C++17 compiler suite installed on your computer.

+ Compiler
  + Windows Visual Studio 2019 v142 toolset
  + macOS 12.0 and Xcode 14.2 or higher
  + Linux with GCC Version 11.0 or higher or clang 14.

## Install vcpkg ##

The `simplnx` project uses the [vcpkg](https://www.github.com/microsoft/vcpkg) to manage it's dependent libraries. If this is not already installed on your system then you will need to download and compile it.

### Windows ###

Clone the vcpkg repository into a location that it will be used from. inside your home directory or at `C:/vcpkg` is a reasonable spot. *DO NOT USE A PATH WITH SPACES IN ANY OF THE FOLDERS*.

```shell
cd C:/Users/[USERNAME]/Applications
git clone https://www.github.com/microsoft/vcpkg
```

The `bootstrap-vcpkg.bat` file should be run automatically by CMake the first time. This will create the `vcpkg.exe` file. Additionally CMake should automatically find `vcpkg.exe`. If CMake does not find it, you may need to add it to your `PATH` variable.

## Clone Appropriate Repositories ##

Within the folder DREAM3D-Dev clone both the `simplnx` and `DREAM3D_Data` repositories. The `DREAM3D_Data` repo is optional but does contain testing data.

Create a location to keep the `simplnx` repositories and make builds. You can do either in-source our out-of-source builds.

```shell
git clone --recursive https://github.com/bluequartzsoftware/simplnx
```

## Configure simplnx with CMake ##

For this example we are going to do an "in-source" build. By default git will ignore some basic names for build directories such as `Debug, Release, x64`. CMake can generate lots of project files from ninja, to nmake to Visual Studio. For this example we are going to use the `ninja` generator so we will need to keep the Debug and Release builds separated.

The first time `simplnx` is configured with CMake, VCPKG will download, build and install the needed dependent libraries. This can take a few minutes so be patient. After `simplnx` is configured you can build it using your IDE (Visual Studio) or ninja (QtCreator, CLion) or cmake itself.

### Windows with Visual Studio IDE ###

This example shows how to configure simplnx to build using Visual Studio IDE

```shell
set BUILD_DIR=VisualStudio
set VCPKG_INSTALL_ROOT=C:/Appications/vcpkg
cd simplnx
mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALL_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=%BUILD_DIR% -DVCPKG_MANIFEST_FEATURES="tests;parallel"  -DDREAM3D_Data_Dir=../../DREAM3D_Data ../simplnx
```

### Windows with Ninja and IDE of choice (QtCreator, CLion, command line) ###

Adjust the below commands to the sytle of your shell (The below example is done in Windows Command Prompt Batch file style).

In this example we are building a Release version of compled into a directory called `Release` inside the source directory. We will be using the `ninja` generator so be sure that ninja.exe is on your PATH

```shell
set NINJA_INSTALL=C:/Applications/ninja-win
set PATH=%PATH%:%NINJA_INSTALL%
set BUILD_DIR=Release
set VCPKG_INSTALL_ROOT=C:/Appications/vcpkg
cd simplnx
mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALL_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=%BUILD_DIR% -DVCPKG_MANIFEST_FEATURES="tests;parallel" -DDREAM3D_Data_Dir=../../DREAM3D_Data ../simplnx
```

### macOS ARM64/M1 Compile and IDE of choice (QtCreator, CLion, command line) ###

```shell
export NINJA_INSTALL=/opt/local/bin
export PATH=$PATH:$NINJA_INSTALL
export BUILD_DIR=Release
export VCPKG_INSTALL_ROOT=/opt/local/vcpkg
# This is used for Apple Silicon ARM64
export VCPKG_TARGET_TRIPLET=arm64-osx-dynamic
# This is used for Intel x64
export VCPKG_TARGET_TRIPLET=x64-osx-dynamic
cd simplnx
mkdir $BUILD_DIR
cd $BUILD_DIR
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALL_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=$BUILD_DIR -DVCPKG_MANIFEST_FEATURES="tests;parallel" -DDREAM3D_DATA_DIR=$HOME/Workspace1/DREAM3D_Data -DVCPKG_TARGET_TRIPLET:STRING=$VCPKG_TARGET_TRIPLET ../simplnx
```

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

By default the **only** plugin that is compiled is the `SimplnxCore` plugin. If you would like to build any of the additional plugins you can provide additional arguments to the configuration command. **NOTE** For the `SIMPLNX_EXTRA_PLUGINS` CMake argument, if you have multiple plugins separate each with a `;` character (which is why we double quote the value for the SIMPLNX_EXTRA_PLUGINS variable.)

### ITKImageProcessing ###

This plugin gives simplnx access to the ability to read/write images and use the **ITK** library to process images.

There are 2 arguments that need to be added to the CMake configuration command

+ `-DSIMPLNX_EXTRA_PLUGINS="ITKImageProcessing"`
+ `-DVCPKG_MANIFEST_FEATURES="tests;parallel;itk"`

### OrientationAnalysis ###

This plugin gives simplnx the ability to process typical EBSD style of data.

+ `-DSIMPLNX_EXTRA_PLUGINS="OrientationAnalysis"`
+ `-DVCPKG_MANIFEST_FEATURES="tests;parallel;ebsd"`

## VCPKG Options ##

### Defining where VCPKG installs the dependent libraries ###

By default VCPKG will install any library that it compiles into vcpkg specific and platform specific locations. If you would like to specifically set where those libraries are installed the following cmake code will allow that:

+ `-DVCPKG_INSTALLED_DIR=$HOME/workspace/vcpkg-installed`

### Disable VCPKG from checking for updates with each configuration ###

Be default VCPKG will check for updates to the libraries that it compiles. If you would like to skip this step each time the following CMake code is needed:

+ `-DVCPKG_MANIFEST_INSTALL=OFF`

## Defining where test data is stored ##

Simplnx and its plugins require test files to be able to perform the unit tests. By default these will be store inside the build directory. This means that if you have multiple build directories, a separate copy of all the test files will be downloaded for each build directory. The developer can set the `DREAM3D_DATA_DIR` variable to a path that will be used. They will need to set this for **each** build directory. You **MUST** define `DREAM3D_DATA_DIR` using an absolute path. Relative paths **will not work**.

+ `-DDREAM3D_DATA_DIR=/opt/local/DREAM3D_Data`

The developer can also turn off the downloading of any test data with the following:

+ `-DSIMPLNX_DOWNLOAD_TEST_FILES=OFF`

## Python Bindings ##

Python bindings are available for simplnx. To install them, please use an Anaconda virtual environment like the following:

```shell
conda config --add channels conda-forge
conda config --set channel_priority strict
conda create -n cxpython python=3.8
conda activate cxpython
conda install -c bluequartzsoftware simplnx
```
