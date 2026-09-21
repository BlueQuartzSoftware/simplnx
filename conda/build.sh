#!/bin/bash

mkdir sdk

echo "PREFIX: $PREFIX"
echo "LIBRARY_PREFIX: $LIBRARY_PREFIX"
echo "SRC_DIR: $SRC_DIR"
echo "target_platform: $target_platform"

echo "#-------------------------------------------------------------------------------"
echo " Copying Example Pipelines"
echo "#-------------------------------------------------------------------------------"

mkdir -p "$PREFIX/share/simplnx/pipelines/"
cp -r "simplnx/src/Plugins/SimplnxCore/pipelines" "$PREFIX/share/simplnx/pipelines/SimplnxCore/"
cp -r "simplnx/src/Plugins/OrientationAnalysis/pipelines" "$PREFIX/share/simplnx/pipelines/OrientationAnalysis/"
cp -r "simplnx/src/Plugins/ImageProcessing/pipelines" "$PREFIX/share/simplnx/pipelines/ImageProcessing/"

# *****************************************************************************
echo "#-------------------------------------------------------------------------------"
echo " BUILDING STB"
echo "#-------------------------------------------------------------------------------"

# mkdir stb_build
# cd stb_build
cd stb

cp *.h "$PREFIX/include/"

mkdir -p "$PREFIX/share/stb"
cp "$SRC_DIR/simplnx/conda/FindStb.cmake" "$PREFIX/share/stb/FindStb.cmake"

echo "# This is a blank file to make CMake Happy" > "$PREFIX/share/stb/stb-config.cmake"
cd ..


# *****************************************************************************
echo "#-------------------------------------------------------------------------------"
echo " BUILDING H5Support"
echo "#-------------------------------------------------------------------------------"

mkdir h5support_build
cd h5support_build

cmake -S "../H5Support" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX" \
  -D H5Support_INCLUDE_QT_API:BOOL=OFF \
  -D H5Support_INSTALL_HDF5:BOOL=OFF \
  -D H5Support_INSTALL_QT5:BOOL=OFF \
  -D CMP_HDF5_ENABLE_COPY:BOOL=OFF \
  -D CMP_HDF5_ENABLE_CXX:BOOL=OFF \
  -D CMP_HDF5_ENABLE_INSTALL:BOOL=OFF \
  -D H5Support_BUILD_TESTING:BOOL=OFF \
  -D H5Support_SKIP_INSTALL_FILES:BOOL=OFF \
  -D CMP_HDF5_USE_CONFIG:BOOL=OFF \
  -D DREAM3D_ANACONDA:BOOL=ON \
  -D HDF5_STATUS_PRINTED:BOOL=ON

cmake --build . --target all

cmake --build . --target install

cd ..

# *****************************************************************************
echo "#-------------------------------------------------------------------------------"
echo " BUILDING EbsdLib"
echo "#-------------------------------------------------------------------------------"

mkdir ebsdlib_build
cd ebsdlib_build

cmake -S "../EbsdLib" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX" \
  -D CMAKE_OSX_DEPLOYMENT_TARGET:STRING="11.0" \
  -D DREAM3D_ANACONDA:BOOL=ON \
  -D CMP_TBB_ENABLE_COPY_INSTALL:BOOL=OFF \
  -D EbsdLib_ENABLE_TESTING:BOOL=OFF \
  -D EbsdLib_BUILD_TOOLS:BOOL=OFF \
  -D EbsdLib_ENABLE_HDF5:BOOL=ON \
  -D EbsdLib_BUILD_H5SUPPORT:BOOL=OFF \
  -D TBB_STATUS_PRINTED:BOOL=ON \
  -D CMP_HDF5_USE_CONFIG:BOOL=OFF \
  -D GVS_GIT_HASH:STRING="d7db8a3a5f11b97ca56b9864de710f552d9dccf4" \
  -D H5Support_DIR:PATH="$PREFIX/share/H5Support"

cmake --build . --target all

cmake --build . --target install

cd ..

# *****************************************************************************
echo "#-------------------------------------------------------------------------------"
echo " BUILDING expected-lite"
echo "#-------------------------------------------------------------------------------"

mkdir expected-lite_build
cd expected-lite_build

cmake -S "../expected-lite" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX" \
  -D EXPECTED_LITE_OPT_BUILD_TESTS:BOOL=OFF \
  -D EXPECTED_LITE_OPT_BUILD_EXAMPLES:BOOL=OFF

cmake --build . --target all

cmake --build . --target install

cd ..

# *****************************************************************************
echo "#-------------------------------------------------------------------------------"
echo " BUILDING span-lite"
echo "#-------------------------------------------------------------------------------"

mkdir span-lite_build
cd span-lite_build

cmake -S "../span-lite" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX" \
  -D SPAN_LITE_OPT_BUILD_TESTS:BOOL=OFF \
  -D SPAN_LITE_OPT_BUILD_EXAMPLES:BOOL=OFF

cmake --build . --target all

cmake --build . --target install

cd ..

# *****************************************************************************
echo "#-------------------------------------------------------------------------------"
echo " BUILDING nod"
echo "#-------------------------------------------------------------------------------"

mkdir nod_build
cd nod_build

cmake -S "../nod" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX"

cmake --build . --target all

cmake --build . --target install

cd ..

# *****************************************************************************
echo "#-------------------------------------------------------------------------------"
echo " BUILDING simplnx"
echo "#-------------------------------------------------------------------------------"

mkdir build
cd build

cmake --preset $SIMPLNX_CMAKE_PRESET ../simplnx $CMAKE_ARGS \
  -D CMAKE_OSX_DEPLOYMENT_TARGET:STRING="11.0" \
  -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
  -D Python3_EXECUTABLE:FILEPATH=$PREFIX/bin/python3


cmake --build . --target all

cmake --build . --target install

cd ..


echo "*********************************************************************************"
echo "              BUILD SCRIPT COMPLETE"
echo "*********************************************************************************"
