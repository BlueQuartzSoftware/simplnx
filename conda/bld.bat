mkdir sdk

rem "H5Support"

mkdir h5support_build
cd h5support_build

cmake -S "../H5Support" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%SRC_DIR%/sdk/H5Support" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D H5Support_INCLUDE_QT_API:BOOL=OFF ^
  -D H5Support_INSTALL_HDF5:BOOL=OFF ^
  -D H5Support_INSTALL_QT5:BOOL=OFF ^
  -D CMP_HDF5_ENABLE_COPY:BOOL=OFF ^
  -D CMP_HDF5_ENABLE_CXX:BOOL=OFF ^
  -D CMP_HDF5_ENABLE_INSTALL:BOOL=OFF ^
  -D H5Support_BUILD_TESTING:BOOL=OFF ^
  -D H5Support_SKIP_INSTALL_FILES:BOOL=OFF ^
  -D CMP_HDF5_USE_CONFIG:BOOL=OFF ^
  -D DREAM3D_ANACONDA:BOOL=ON ^
  -D HDF5_STATUS_PRINTED:BOOL=ON
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..

rem "EbsdLib"

mkdir ebsdlib_build
cd ebsdlib_build

cmake -S "../EbsdLib" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%SRC_DIR%/sdk/EbsdLib" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D DREAM3D_ANACONDA:BOOL=ON ^
  -D CMP_TBB_ENABLE_COPY_INSTALL:BOOL=OFF ^
  -D EbsdLib_ENABLE_TESTING:BOOL=OFF ^
  -D EbsdLib_BUILD_TOOLS:BOOL=OFF ^
  -D EbsdLib_ENABLE_HDF5:BOOL=ON ^
  -D EbsdLib_BUILD_H5SUPPORT:BOOL=OFF ^
  -D TBB_STATUS_PRINTED:BOOL=ON ^
  -D CMP_HDF5_USE_CONFIG:BOOL=OFF ^
  -D GVS_GIT_HASH:STRING="6c0e5ec992472eeae5df9d627de524b59b971fab" ^
  -D H5Support_DIR:PATH="%SRC_DIR%/sdk/H5Support/share/H5Support"
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..

rem "expected-lite"

mkdir expected-lite_build
cd expected-lite_build

cmake -S "../expected-lite" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%SRC_DIR%/sdk/expected-lite" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D EXPECTED_LITE_OPT_BUILD_TESTS:BOOL=OFF ^
  -D EXPECTED_LITE_OPT_BUILD_EXAMPLES:BOOL=OFF
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..

rem "span-lite"

mkdir span-lite_build
cd span-lite_build

cmake -S "../span-lite" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%SRC_DIR%/sdk/span-lite" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D SPAN_LITE_OPT_BUILD_TESTS:BOOL=OFF ^
  -D SPAN_LITE_OPT_BUILD_EXAMPLES:BOOL=OFF
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..

rem "nod"

mkdir nod_build
cd nod_build

cmake -S "../nod" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%SRC_DIR%/sdk/nod" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%"
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..

rem "complex"

mkdir build
cd build

cmake -S "%SRC_DIR%/complex" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D COMPLEX_BUILD_TESTS:BOOL=OFF ^
  -D COMPLEX_BUILD_DOCS:BOOL=OFF ^
  -D COMPLEX_BUILD_PYTHON:BOOL=ON ^
  -D COMPLEX_DOWNLOAD_TEST_FILES:BOOL=OFF ^
  -D HDF5_NO_FIND_PACKAGE_CONFIG_FILE:BOOL=ON ^
  -D span-lite_DIR:PATH="%SRC_DIR%/sdk/span-lite/lib/cmake/span-lite" ^
  -D expected-lite_DIR:PATH="%SRC_DIR%/sdk/expected-lite/lib/cmake/expected-lite" ^
  -D H5Support_DIR:PATH="%SRC_DIR%/sdk/H5Support/share/H5Support" ^
  -D EbsdLib_DIR:PATH="%SRC_DIR%/sdk/EbsdLib/share/EbsdLib" ^
  -D nod_DIR:PATH="%SRC_DIR%/sdk/nod/share/nod" ^
  -D COMPLEX_CONDA_BUILD:BOOL=ON ^
  -D COMPLEX_ENABLE_INSTALL:BOOL=ON ^
  -D COMPLEX_PY_INSTALL_DIR:PATH="%SP_DIR%" ^
  -D ITKIMAGEPROCESSING_LEAN_AND_MEAN:BOOL=ON ^
  -D ComplexCore_INSTALL_PIPELINES:BOOL=OFF ^
  -D ITKImageProcessing_INSTALL_PIPELINES:BOOL=OFF ^
  -D OrientationAnalysis_INSTALL_PIPELINES:BOOL=OFF ^
  -D OrientationAnalysis_INSTALL_DATA_FILES:BOOL=OFF
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..
cd ebsdlib_build

cmake -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" .
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1
