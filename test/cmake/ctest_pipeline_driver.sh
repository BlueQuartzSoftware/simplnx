#!/bin/bash

# This file is auto generated from @CMAKE_CURRENT_LIST_FILE@. 

#set -e

echo "[D3D_Prebuilt_@test_index@] Prebuilt Pipeline Test Starting"
echo "    @test@.d3dpipeline"

DEBUG_EXT=""
if [ "@CMAKE_BUILD_TYPE@" = "Debug" ]; then
    DEBUG_EXT="@DEBUG_EXT@"
fi

cd "@CMAKE_RUNTIME_OUTPUT_DIRECTORY@"

# Put the binary directory on the PATH since one of the filters needs to find PipelineRunner
# in a cross platform way.
export PATH=$PATH:"@CMAKE_RUNTIME_OUTPUT_DIRECTORY@"
"./PipelineRunner${DEBUG_EXT}" "@pipeline_file_path@"

if [ $? -eq 0 ]
then
  echo "[@test@] Success"
  exit 0
else
  echo "[@test@] Failure" >&2
  exit 1
fi

