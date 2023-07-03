#!/bin/bash

# This file is auto generated from @CMAKE_CURRENT_LIST_FILE@. 

#set -e

echo "[@PLUGIN_NAME@_@ARGS_TEST_INDEX@] Prebuilt Pipeline Test Starting"
echo "    @test@"

PIPELINE_RUNNER_DEBUG=""
if [ "@CMAKE_BUILD_TYPE@" = "Debug" ]; then
    DEBUG_EXT="@PIPELINE_RUNNER_DEBUG@"
fi

#-----------------------------------------------------------------------------
# Check the input file
#-----------------------------------------------------------------------------
@INPUT_FILE_COMMANDS@

#-----------------------------------------------------------------------------
# Execute nxrunner
#-----------------------------------------------------------------------------
echo "********* Starting @PIPELINE_RUNNER_NAME@${DEBUG_EXT}"
cd "@CMAKE_RUNTIME_OUTPUT_DIRECTORY@"
"./@PIPELINE_RUNNER_NAME@${DEBUG_EXT}" --execute "@ARGS_PIPELINE_PATH@"

#-----------------------------------------------------------------------------
# Check the output file
#-----------------------------------------------------------------------------
@OUTPUT_FILE_COMMANDS@

#-----------------------------------------------------------------------------
# These files need to be deleted after the test has completed
@DELETE_FILE_COMMANDS@

if [ $? -eq 0 ]
then
  echo "[@test@] Success"
  exit 0
else
  echo "[@test@] Failure" >&2
  exit 1
fi

