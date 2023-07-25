#!/bin/bash

mkdir sdk

# "H5Support"

mkdir h5support_build
cd h5support_build

cmake -S "../H5Support" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$SRC_DIR/sdk/H5Support" \
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

# "EbsdLib"

mkdir ebsdlib_build
cd ebsdlib_build

cmake -S "../EbsdLib" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$SRC_DIR/sdk/EbsdLib" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX" \
  -D DREAM3D_ANACONDA:BOOL=ON \
  -D CMP_TBB_ENABLE_COPY_INSTALL:BOOL=OFF \
  -D EbsdLib_ENABLE_TESTING:BOOL=OFF \
  -D EbsdLib_BUILD_TOOLS:BOOL=OFF \
  -D EbsdLib_ENABLE_HDF5:BOOL=ON \
  -D EbsdLib_BUILD_H5SUPPORT:BOOL=OFF \
  -D TBB_STATUS_PRINTED:BOOL=ON \
  -D CMP_HDF5_USE_CONFIG:BOOL=OFF \
  -D GVS_GIT_HASH:STRING="6c0e5ec992472eeae5df9d627de524b59b971fab" \
  -D H5Support_DIR:PATH="$SRC_DIR/sdk/H5Support/share/H5Support"

cmake --build . --target all

cmake --build . --target install

cd ..

# "expected-lite"

mkdir expected-lite_build
cd expected-lite_build

cmake -S "../expected-lite" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$SRC_DIR/sdk/expected-lite" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX" \
  -D EXPECTED_LITE_OPT_BUILD_TESTS:BOOL=OFF \
  -D EXPECTED_LITE_OPT_BUILD_EXAMPLES:BOOL=OFF

cmake --build . --target all

cmake --build . --target install

cd ..

# "span-lite"

mkdir span-lite_build
cd span-lite_build

cmake -S "../span-lite" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$SRC_DIR/sdk/span-lite" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX" \
  -D SPAN_LITE_OPT_BUILD_TESTS:BOOL=OFF \
  -D SPAN_LITE_OPT_BUILD_EXAMPLES:BOOL=OFF

cmake --build . --target all

cmake --build . --target install

cd ..

# "nod"

mkdir nod_build
cd nod_build

cmake -S "../nod" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$SRC_DIR/sdk/nod" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX"

cmake --build . --target all

cmake --build . --target install

cd ..

# "complex"

mkdir build
cd build

cmake --preset $COMPLEX_CMAKE_PRESET ../complex $CMAKE_ARGS

cmake --build . --target all

cmake --build . --target install

cd ..
cd ebsdlib_build

cmake -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" .

cmake --build . --target install
