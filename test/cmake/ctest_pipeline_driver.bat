
@echo off
:: Check to see if we were built using the Visual Studio generators, if so
:: then we need an additional argument to tell this script which configuration
:: directory to use as the source for the compiled files.
:: This file is auto generated from @CMAKE_CURRENT_LIST_FILE@. 
:: set CONFIG_DIR=%CMAKE_CONFIG_TYPE%
IF "@MSVC_IDE@" == "1" (
    set CONFIG_DIR=%CMAKE_CONFIG_TYPE%
) else (
    set CONFIG_DIR=
) 

::-----------------------------------------------------------------------------
IF "%CMAKE_CONFIG_TYPE%" == "Debug" (
    set EXE_SFX=@COMPLEX_DEBUG_POSTFIX@
)

::-----------------------------------------------------------------------------
IF "%CMAKE_CONFIG_TYPE%" == "Release" (
    set EXE_SFX=
)

::-----------------------------------------------------------------------------
IF "@CMAKE_BUILD_TYPE@" == "Debug" (
    set CONFIG_DIR=
    set EXE_SFX=@COMPLEX_DEBUG_POSTFIX@
)

::-----------------------------------------------------------------------------
IF "@CMAKE_BUILD_TYPE@" == "Release" (
    set CONFIG_DIR=
    set EXE_SFX=
)

echo on
@echo "Prebuilt Pipeline Test Starting"
@echo "    @test@.d3dpipeline"

:: Put the binary directory on the PATH so it can be found without needing to
:: know the absolute path to it. 
@set PATH=%PATH%;@CMAKE_RUNTIME_OUTPUT_DIRECTORY@\%CONFIG_DIR%\
@echo "Running Executable at @CMAKE_RUNTIME_OUTPUT_DIRECTORY@\%CONFIG_DIR%\PipelineRunner%EXE_SFX%@EXE_EXT@"

cd @CMAKE_RUNTIME_OUTPUT_DIRECTORY@\%CONFIG_DIR%

PipelineRunner%EXE_SFX%@EXE_EXT@ "@pipeline_file_path@"
