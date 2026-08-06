# Guide to Building From Source

## Dependent Libraries

| Library Name | GitHub Source | Version |
|--------------|---------------|---------|
| benchmark | <https://github.com/google/benchmark> | 1.8.4 |
| blosc | <https://github.com/Blosc/c-blosc> | 1.18.1 |
| boost-mp11 | <https://github.com/boostorg/mp11>  | 1.77.0 |
| catch2 | <https://github.com/catchorg/Catch2>  | 2.13.6 |
| ebsdlib | <https://www.github.com/bluequartzsoftware/EBSDLib>   | 1.0.40 |
| eigen3 |  <https://gitlab.com/libeigen/eigen.git> | 3.4.0 |
| expected-lite | <https://github.com/martinmoene/expected-lite>  | 0.8.0 |
| fmt | <https://github.com/fmtlib/fmt>  | 11.1.4 |
| h5support  | <https://www.github.com/bluequartzsoftware/H5Support>  | 1.0.13 |
| hdf5 | <https://github.com/HDFGroup/hdf5/>  | 1.14.4.3 |
| itk | <https://github.com/InsightSoftwareConsortium/ITK.git>  | 5.4.4 |
| lz4 | <https://github.com/lz4/lz4> | 1.9.4 |
| nlohmann-json | <https://github.com/nlohmann/json/>  | 3.11.2 |
| nod | <https://github.com/fr00b0/nod.git>  | 0.5.4 |
| nxcommon | <https://github.com/BlueQuartzSoftware/NXCommon> | 0.1.1 |
| nxh5support | <https://github.com/BlueQuartzSoftware/NXH5Support> | 0.1.0 |
| pugixml | <https://github.com/zeux/pugixml> | 1.11.4 |
| pybind11 | <https://github.com/pybind/pybind11.git>  | 2.12.0 |
| snappy | <https://github.com/google/snappy> | 1.1.9 |
| span-lite | <https://github.com/martinmoene/span-lite>  | 0.10.3 |
| spdlog | <https://github.com/gabime/spdlog> | 1.15.3 |
| tbb | <https://github.com/oneapi-src/onetbb>  | 2021.4.0 |
| reproc | <https://github.com/DaanDeMeyer/reproc>  | 14.2.5 |
| zlib | <https://www.zlib.net/> | 1.3.1 |
| zstd | <https://facebook.github.io/zstd/> | 1.5.2 |

## Prerequisites

In order to compile `simplnx` you will need a C++20 compiler suite installed on your computer.

+ Compiler
  + Windows Visual Studio 2022 v143 toolset
  + macOS 12.5 and Xcode 14.2 or higher
  + Linux with GCC Version 11.0 or higher or clang 14.

For Python Bindings a Python version of 3.8 is the minimum version, but internally 3.11 and 3.12 are the actively used versions.

## Install vcpkg

The `simplnx` project uses the [vcpkg](https://www.github.com/microsoft/vcpkg) to manage it's dependent libraries. If this is not already installed on your system then you will need to download and compile it.

### Windows

Clone the vcpkg repository into a location that it will be used from. inside your home directory or at `C:/vcpkg` is a reasonable spot. *DO NOT USE A PATH WITH SPACES IN ANY OF THE FOLDERS*.

```shell
cd C:/Users/[USERNAME]/Applications
git clone https://www.github.com/microsoft/vcpkg
```

The `bootstrap-vcpkg.bat` file should be run automatically by CMake the first time. This will create the `vcpkg.exe` file. Additionally CMake should automatically find `vcpkg.exe`. If CMake does not find it, you may need to add it to your `PATH` variable.

## Clone Appropriate Repositories

Within the folder DREAM3D-Dev clone both the `simplnx` and `DREAM3D_Data` repositories. The `DREAM3D_Data` repo is optional but does contain testing data.

Create a location to keep the `simplnx` repositories and make builds. You can do either in-source our out-of-source builds.

```shell
git clone --recursive https://github.com/bluequartzsoftware/simplnx
```

## Configure simplnx with CMake

For this example we are going to do an "in-source" build. By default git will ignore some basic names for build directories such as `Debug, Release, x64`. CMake can generate lots of project files from ninja, to nmake to Visual Studio. For this example we are going to use the `ninja` generator so we will need to keep the Debug and Release builds separated.

The first time `simplnx` is configured with CMake, VCPKG will download, build and install the needed dependent libraries. This can take a few minutes so be patient. After `simplnx` is configured you can build it using your IDE (Visual Studio) or ninja (QtCreator, CLion) or cmake itself.

### Windows with Visual Studio IDE

This example shows how to configure simplnx to build using Visual Studio IDE

```shell
set BUILD_DIR=VisualStudio
set VCPKG_INSTALL_ROOT=C:/Applications/vcpkg
cd simplnx
mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALL_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=%BUILD_DIR% -DVCPKG_MANIFEST_FEATURES="tests;parallel"  -DDREAM3D_Data_Dir=../../DREAM3D_Data ../simplnx
```

### Windows with Ninja and IDE of choice (QtCreator, CLion, command line)

Adjust the below commands to the style of your shell (The below example is done in Windows Command Prompt Batch file style).

In this example we are building a Release version compiled into a directory called `Release` inside the source directory. We will be using the `ninja` generator so be sure that ninja.exe is on your PATH

```shell
set NINJA_INSTALL=C:/Applications/ninja-win
set PATH=%PATH%:%NINJA_INSTALL%
set BUILD_DIR=Release
set VCPKG_INSTALL_ROOT=C:/Applications/vcpkg
cd simplnx
mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=%VCPKG_INSTALL_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=%BUILD_DIR% -DVCPKG_MANIFEST_FEATURES="tests;parallel" -DDREAM3D_Data_Dir=../../DREAM3D_Data ../simplnx
```

### macOS ARM64/M1 Compile and IDE of choice (QtCreator, CLion, command line)

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
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALL_ROOT/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=$BUILD_DIR -DVCPKG_MANIFEST_FEATURES="tests;parallel" -DDREAM3D_DATA_DIR=$HOME/Workspace6/DREAM3D_Data -DVCPKG_TARGET_TRIPLET:STRING=$VCPKG_TARGET_TRIPLET ../simplnx
```

## Required CMake Arguments

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
+ `-DVCPKG_HOST_TRIPLET=....` Correct VCPKG triplet for your platform/operating system
  + MacOS: [x64-osx-dynamic | arm64-osx-dynamic]
  + Linux: x64-linux-dynamic
  + Windows:

### Notable CMake Arguments

| Identifier | Type | Explanation |
|------------|------|-------------|
| `ITKImageProcessing_USE_JOB_POOL` | `BOOL` | When `ON` the amount of threads used to build `ITK` library plugin is capped (see `ITKImageProcessing_JOB_POOL`). Some of the `ITK` files more 2+ GB of memory to compile due to heavy templating. This is to prevent systems from running out of memory during compilation. |
| `ITKImageProcessing_JOB_POOL` | `STRING` | If `ITKImageProcessing_USE_JOB_POOL` is `ON`, this variable is the maximum number of threads available to the `ITK` plugin during compilation. For example on a system with 32 threads and 64GB of RAM a value of `16` will prevent out-of-memory errors during compilation. |
| `SIMPLNX_BUILD_TESTS` | `BOOL` | Turning this on will compile the filter/library tests and include them in the all ctests target |
| `SIMPLNX_BUILD_DOCS` | `BOOL` | Turning this on will build the library/filters documentation, `SPHINX_BUILD_EXECUTABLE` must be defined |
| `SPHINX_BUILD_EXECUTABLE` | `PATH` | The absolute path to to the sphinx executable; *required* for `SIMPLNX_BUILD_PYTHON_DOCS` and/or `SIMPLNX_BUILD_DOCS` |
| `SIMPLNX_EXTRA_PLUGINS` | `STRING` | A list of plugin targets (separated by `;`) to include into the library. Currently the only official additional plugin not bundled is `SimplnxReview` which is untested filters |
| `SIMPLNX_ENABLE_BENCHMARK_UTILITY` | `BOOL` | Turning this on will include the preferred benchmarking library; Be sure to **add** `benchmark` **to** `VCPKG_MANIFEST_FEATURES` **list** |

### Python Binding CMake Arguments

*NOTE: Be sure to **add** `python` **to** `VCPKG_MANIFEST_FEATURES` **list** if you are enabling python bindings*. Remember to separate each manifest feature with a `;`. Example: `tests;parallel;itk;ebsd;benchmark;python`

| Identifier | Type | Explanation |
|------------|------|-------------|
| `SIMPLNX_BUILD_PYTHON` | `BOOL` | Turning this on will enable Python Bindings, but several other variables must be supplied to get it to configure. |
| `Python_EXECUTABLE | `PATH` | The absolute path to to the python executable; *required* for Python Bindings |
| `SIMPLNX_PY_DISABLE_HIDDEN_VISIBILITY` | `BOOL` | Turning this on is *required* for Python Bindings on Unix-based systems |
| `SIMPLNX_EMBED_PYTHON` | `BOOL` | Turning this on will allow you to write python filters/plugins that will be loaded/included into the final compiled library executable |
| `SIMPLNX_BUILD_PYTHON_TESTS` | `BOOL` | Turning this on will also compile the python tests and include them in the all ctests target |
| `SIMPLNX_BUILD_PYTHON_DOCS` | `BOOL` | Turning this on will build the python documentation, `SPHINX_BUILD_EXECUTABLE` must be defined |

### ITKImageProcessing

This plugin gives simplnx access to the ability to read/write images and use the **ITK** library to process images.

There are 1 arguments that need to be added to the CMake configuration command

+ `-DVCPKG_MANIFEST_FEATURES="tests;parallel;itk"`

### OrientationAnalysis

This plugin gives simplnx the ability to process typical EBSD style of data.

+ `-DVCPKG_MANIFEST_FEATURES="tests;parallel;ebsd"`

## VCPKG Options

### Defining where VCPKG installs the dependent libraries

By default VCPKG will install any library that it compiles into vcpkg specific and platform specific locations. If you would like to specifically set where those libraries are installed the following cmake code will allow that:

+ `-DVCPKG_INSTALLED_DIR=$HOME/workspace/vcpkg-installed`

### Disable VCPKG from checking for updates with each configuration

Be default VCPKG will check for updates to the libraries that it compiles. If you would like to skip this step each time the following CMake code is needed:

+ `-DVCPKG_MANIFEST_INSTALL=OFF`

## Defining where test data is stored

Simplnx and its plugins require test files to be able to perform the unit tests. By default these will be store inside the build directory. This means that if you have multiple build directories, a separate copy of all the test files will be downloaded for each build directory. The developer can set the `DREAM3D_DATA_DIR` variable to a path that will be used. They will need to set this for **each** build directory. You **MUST** define `DREAM3D_DATA_DIR` using an absolute path. Relative paths **will not work**.

+ `-DDREAM3D_DATA_DIR=/opt/local/DREAM3D_Data`

The developer can also turn off the downloading of any test data with the following:

+ `-DSIMPLNX_DOWNLOAD_TEST_FILES=OFF`

## Python Bindings

Python bindings are available for simplnx. To install them, please use an Anaconda virtual environment like the following:

```shell
conda config --add channels conda-forge
conda config --set channel_priority strict
conda create -n cxpython python=3.8
conda activate cxpython
conda install -c bluequartzsoftware simplnx
```

## Example Linux/Ninja CMakeUserPresets.json Template

Be sure to update `Python_EXECUTABLE`, `SPHINX_BUILD_EXECUTABLE`, and `VCPKG_INSTALLATION_ROOT`.

Update `generator` with preferred generator if you wish to avoid Ninja.

If adapting for another platform, be sure to update `VCPKG_TARGET_TRIPLET` and `VCPKG_HOST_TRIPLET`, then search and replace "linux" with current OS.

```console
{
  "version": 2,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 20,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "default-all",
      "displayName": "default-all",
      "description": "Build configuration for Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/../DREAM3D-Build/dream3dnx",
      "hidden": true,
      "cacheVariables": {
        "VCPKG_MANIFEST_DIR": {
          "type": "STRING",
          "value": "${sourceDir}/../simplnx"
        },
        "VCPKG_MANIFEST_FEATURES": {
          "type": "STRING",
          "value": "tests;parallel;itk;ebsd;benchmark;python"
        },
        "VCPKG_INSTALLED_DIR": {
          "type": "STRING",
          "value": "${sourceDir}/../vcpkg-installed"
        },
        "VCPKG_MANIFEST_INSTALL": {
          "type": "BOOL",
          "value": "ON"
        },
        "SIMPLNX_EXTRA_PLUGINS": {
          "type": "STRING",
          "value": "SimplnxReview"
        },
        "CMAKE_TOOLCHAIN_FILE": {
          "type": "FILEPATH",
          "value": "$env{VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake"
        },
        "SIMPLNX_BUILD_TESTS": {
          "type": "BOOL",
          "value": "ON"
        },
        "SIMPLNX_BUILD_DOCS": {
          "type": "BOOL",
          "value": "OFF"
        },
        "ITKImageProcessing_USE_JOB_POOL": {
          "type": "BOOL",
          "value": "ON"
        },
        "ITKImageProcessing_JOB_POOL": {
          "type": "STRING",
          "value": "16"
        },
        "SIMPLNX_BUILD_PYTHON": {
          "type": "BOOL",
          "value": "ON"
        },
        "SIMPLNX_EMBED_PYTHON": {
          "type": "BOOL",
          "value": "ON"
        },
        "SIMPLNX_BUILD_PYTHON_TESTS": {
          "type": "BOOL",
          "value": "ON"
        },
        "SIMPLNX_PY_DISABLE_HIDDEN_VISIBILITY": {
          "type": "BOOL",
          "value": "ON"
        },
        "Python_EXECUTABLE": {
          "type": "PATH",
          "value": "absolute/path/to/python"
        },
        "SPHINX_BUILD_EXECUTABLE": {
          "type": "PATH",
          "value": "absolute/path/to/sphinx"
        },
        "DREAM3D_DATA_DIR": {
          "type": "STRING",
          "value": "${sourceDir}/../DREAM3D_Data"
        },
        "SIMPLNX_ENABLE_BENCHMARK_UTILITY": {
          "type": "BOOL",
          "value": "ON"
        }
      },
      "environment": {"VCPKG_INSTALLATION_ROOT":"absolute/path/to/vcpkg"}
    },
    {
      "name": "x64",
      "inherits": "default-all",
      "displayName": "x64",
      "description": "x64 Arch",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/../DREAM3D-Build/DREAM3DNX-Linux-x64",
      "hidden": true,
      "cacheVariables": {
        "VCPKG_TARGET_TRIPLET": {
          "type": "STRING",
          "value": "x64-linux-dynamic"
        },
        "VCPKG_HOST_TRIPLET": {
          "type": "STRING",
          "value": "x64-linux-dynamic"
        }
      }
    },
    {
      "name": "Linux-Debug-x64",
      "inherits": "x64",
      "displayName": "Linux-Debug-x64",
      "description": "Build configuration for Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/../DREAM3D-Build/DREAM3DNX-Debug-Linux-x64",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": {
          "type": "STRING",
          "value": "Debug"
          }
       }
    },
    {
      "name": "Linux-Release-x64",
      "inherits": "x64",
      "displayName": "Linux-Release-x64",
      "description": "Build configuration for Linux",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/../DREAM3D-Build/DREAM3DNX-Release-Linux-x64",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": {
          "type": "STRING",
          "value": "Release"
        }
      }
    }
  ],
  "buildPresets": [
    {
      "name": "Debug-x64",
      "displayName": "Debug-x64",
      "description": "Build configuration for Debug-x64",
      "configurePreset": "Linux-Debug-x64",
      "configuration": "Debug"
    },
    {
      "name": "Release-x64",
      "displayName": "Release-x64",
      "description": "Build configuration for Release-x64",
      "configurePreset": "Linux-Release-x64",
      "configuration": "Release"

    }
  ]
}

```
