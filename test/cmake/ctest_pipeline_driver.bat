
@echo off
:: Check to see if we were built using the Visual Studio generators, if so
:: then we need an additional argument to tell this script which configuration
:: directory to use as the source for the compiled files.
:: This file is auto generated from:
:: '@CMAKE_CURRENT_LIST_FILE@' 
::
:: If configured using an actual Visual Studio IDE then 'MSVC_IDE' will be defined to '1'
:: and we can use that to figure out if we need to inject an intermediate configuration
:: directory based on the configuration type. If running from the command line then
:: the developer would need to use:
::  ```ctest -C Release -R "Some_Test_Case"
::
:: If we used a different generator then those are single configuration generators
:: and we just use the path to the `Bin` directory.
::
:: The generator used for this script was: '@CMAKE_GENERATOR@'
::
:: If the Visual Studio generator was used then the CMAKE_CONFIG_TYPE is set as
:: an environment variable when this script is called so it will be defined and
:: we can use that to generate the proper intermediate directory

@echo "! CMake Generator: " @CMAKE_GENERATOR@

IF "@MSVC_IDE@" == "1" (
    set CONFIG_DIR=/%CMAKE_CONFIG_TYPE%
) else (
    set CONFIG_DIR=
    @echo "! CMAKE_BUILD_TYPE: " @CMAKE_BUILD_TYPE@
) 

::-----------------------------------------------------------------------------
IF "%CMAKE_CONFIG_TYPE%" == "Debug" (
    set EXE_SFX=@COMPLEX_DEBUG_POSTFIX@
)

::-----------------------------------------------------------------------------
IF "%CMAKE_CONFIG_TYPE%" == "Release" (
    set EXE_SFX=
)

echo on
@echo "! Prebuilt Pipeline Test Starting"
@echo "!     @test@.d3dpipeline"

:: Put the binary directory on the PATH so it can be found without needing to
:: know the absolute path to it. 
@set PATH=%PATH%;@CMAKE_RUNTIME_OUTPUT_DIRECTORY@%CONFIG_DIR%\
@echo "! Running Executable at @CMAKE_RUNTIME_OUTPUT_DIRECTORY@%CONFIG_DIR%\PipelineRunner%EXE_SFX%@EXE_EXT@"
@echo "! Change directory to the proper directory..."
cd @CMAKE_RUNTIME_OUTPUT_DIRECTORY@%CONFIG_DIR%
@echo "! Execute the pipeline...."
PipelineRunner%EXE_SFX%@EXE_EXT@ "@pipeline_file_path@"
