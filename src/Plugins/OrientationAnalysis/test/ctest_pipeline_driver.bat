
@echo off
:: Check to see if we were built using the Visual Studio generators, if so
:: then we need an additional argument to tell this script which configuration
:: directory to use as the source for the compiled files.
:: This file is auto generated from @CMAKE_CURRENT_LIST_FILE@. 
:: set CONFIG_DIR=%CMAKE_CONFIG_TYPE%
echo CMAKE_CONFIG_TYPE %CMAKE_CONFIG_TYPE%

IF "@MSVC_IDE@" == "1" (
    set CONFIG_DIR=%CMAKE_CONFIG_TYPE%
    ::-----------------------------------------------------------------------------
    IF "%CMAKE_CONFIG_TYPE%" == "Debug" (
        set EXE_SFX=@PIPELINE_RUNNER_DEBUG@
    )

    ::-----------------------------------------------------------------------------
    IF "%CMAKE_CONFIG_TYPE%" == "Release" (
        set EXE_SFX=
    )

) else (
    set CONFIG_DIR=
    ::-----------------------------------------------------------------------------
    IF "@CMAKE_BUILD_TYPE@" == "Debug" (
        set CONFIG_DIR=
        set EXE_SFX=@PIPELINE_RUNNER_DEBUG@
    )

    ::-----------------------------------------------------------------------------
    IF "@CMAKE_BUILD_TYPE@" == "Release" (
        set CONFIG_DIR=
        set EXE_SFX=
    )
) 
echo CONFIG_DIR %CONFIG_DIR%
echo EXE_SFX %EXE_SFX%

::echo on
@echo Prebuilt Pipeline Test Starting
@echo     @test@.d3dpipeline

::-----------------------------------------------------------------------------
:: Check the input file
::-----------------------------------------------------------------------------
@INPUT_FILE_COMMANDS@


::-----------------------------------------------------------------------------
:: Execute nxrunner
::-----------------------------------------------------------------------------
@echo Running Executable at '@CMAKE_RUNTIME_OUTPUT_DIRECTORY@/%CONFIG_DIR%/@PIPELINE_RUNNER_NAME@%EXE_SFX%@EXE_EXT@'

cd @CMAKE_RUNTIME_OUTPUT_DIRECTORY@\%CONFIG_DIR%

@PIPELINE_RUNNER_NAME@%EXE_SFX%@EXE_EXT@ --execute "@ARGS_PIPELINE_PATH@"
IF %ERRORLEVEL% NEQ 0 EXIT 1

::-----------------------------------------------------------------------------
:: Check the Output file
::-----------------------------------------------------------------------------
@OUTPUT_FILE_COMMANDS@

::-----------------------------------------------------------------------------
:: These files need to be deleted after the test has completed
::-----------------------------------------------------------------------------
@DELETE_FILE_COMMANDS@
