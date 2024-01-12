:: ****************************************************************************
:: This script file builds the necessary dependencies that are not found as 
:: part of a python environment but are needed to ultimately build the 
:: simplnx python bindings.
:: ****************************************************************************

mkdir sdk

set lib_prefix=%LIBRARY_PREFIX:\=/%
echo lib_prefix = %lib_prefix%

:: ****************************************************************************
:: Build the H5Support library
:: ****************************************************************************
echo *************************** h5support ******************************************
cd %SRC_DIR%
mkdir h5support_build
cd h5support_build

cmake -S "%SRC_DIR%/H5Support" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" ^
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

:: ****************************************************************************
:: Install the EbsdLibrary into the python build environment so that stubgen
:: has access to it.
:: ****************************************************************************
echo *************************** EbsdLib ******************************************
cd %SRC_DIR%
mkdir ebsdlib_build
cd ebsdlib_build

cmake -S "%SRC_DIR%/EbsdLib" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" ^
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
  -D H5Support_DIR:PATH="%lib_prefix%/share/H5Support"
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..

:: ****************************************************************************
:: Build the expected-light library
:: ****************************************************************************
echo *************************** expected-light ******************************************
cd %SRC_DIR%
mkdir expected-lite_build
cd expected-lite_build

cmake -S "%SRC_DIR%/expected-lite" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D EXPECTED_LITE_OPT_BUILD_TESTS:BOOL=OFF ^
  -D EXPECTED_LITE_OPT_BUILD_EXAMPLES:BOOL=OFF
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..

:: ****************************************************************************
:: Build the span-light library
:: ****************************************************************************
echo *************************** span-light ******************************************
cd %SRC_DIR%
mkdir span-lite_build
cd span-lite_build

cmake -S "%SRC_DIR%/span-lite" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D SPAN_LITE_OPT_BUILD_TESTS:BOOL=OFF ^
  -D SPAN_LITE_OPT_BUILD_EXAMPLES:BOOL=OFF
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..

:: ****************************************************************************
:: Build the nod library
:: ****************************************************************************
echo *************************** nod ******************************************
cd %SRC_DIR%
mkdir nod_build
cd nod_build

cmake -S "%SRC_DIR%/nod" -B . -G "Ninja" ^
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%"
if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

cd ..


:: ****************************************************************************
:: Build the ITK library
:: ****************************************************************************
echo *************************** ITK ******************************************
cd %SRC_DIR%
mkdir itk_build
cd itk_build

cmake -S "%SRC_DIR%/itk" -B . -G "Ninja" $CMAKE_ARGS ^
  -DCMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D Eigen3_DIR=%lib_prefix%/share/eigen3/cmake ^
  -DBUILD_SHARED_LIBS:STRING=ON ^
  -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=11.0 ^
  -DCMAKE_SKIP_INSTALL_RPATH:BOOL=OFF ^
  -DCMAKE_SKIP_RPATH:BOOL=OFF ^
  -DCMAKE_CXX_STANDARD:STRING=17 ^
  -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=ON ^
  -DBUILD_DOCUMENTATION:BOOL=OFF ^
  -DBUILD_EXAMPLES:BOOL=OFF ^
  -DBUILD_TESTING:BOOL=OFF ^
  -DKWSYS_USE_MD5:BOOL=ON ^
  -DITK_LEGACY_REMOVE:BOOL=ON ^
  -DITK_FUTURE_LEGACY_REMOVE:BOOL=ON ^
  -DITK_LEGACY_SILENT:BOOL=OFF ^
  -DITKV4_COMPATIBILITY:BOOL=OFF ^
  -DITK_USE_SYSTEM_EIGEN:BOOL=ON ^
  -DITK_USE_SYSTEM_HDF5:BOOL=ON ^
  -DITKGroup_Core:BOOL=ON ^
  -DITKGroup_Filtering:BOOL=ON ^
  -DITKGroup_Registration:BOOL=ON ^
  -DITKGroup_Segmentation:BOOL=ON ^
  -DITK_BUILD_DEFAULT_MODULES:BOOL=OFF ^
  -DModule_ITKTestKernel:BOOL=OFF ^
  -DModule_ITKReview:BOOL=OFF ^
  -DModule_SCIFIO=OFF ^
  -DModule_ITKMetricsv4:BOOL=OFF ^
  -DModule_ITKOptimizersv4:BOOL=OFF ^
  -DModule_ITKRegistrationMethodsv4:BOOL=OFF ^
  -DModule_ITKConvolution:BOOL=ON ^
  -DModule_ITKDenoising:BOOL=ON ^
  -DModule_ITKImageNoise:BOOL=ON ^
  -DITKGroup_IO:BOOL=OFF ^
  -DITKGroup_Core:BOOL=OFF ^
  -DModule_ITKGDCM:BOOL=OFF ^
  -DModule_ITKIOBioRad:BOOL=ON ^
  -DModule_ITKIOBMP:BOOL=ON ^
  -DModule_ITKIOGE:BOOL=ON ^
  -DModule_ITKIOGIPL:BOOL=ON ^
  -DModule_ITKIOImageBase:BOOL=ON ^
  -DModule_ITKIOIPL:BOOL=ON ^
  -DModule_ITKIOJPEG:BOOL=ON ^
  -DModule_ITKIOMeta:BOOL=ON ^
  -DModule_ITKIOMRC:BOOL=ON ^
  -DModule_ITKIONIFTI:BOOL=ON ^
  -DModule_ITKIONRRD:BOOL=ON ^
  -DModule_ITKIOPNG:BOOL=ON ^
  -DModule_ITKIOStimulate:BOOL=ON ^
  -DModule_ITKIOTIFF:BOOL=ON ^
  -DModule_ITKIOVTK:BOOL=ON ^
  -DModule_ITKIOTransformBase:BOOL=ON ^
  -DITK_SKIP_PATH_LENGTH_CHECKS:BOOL=ON ^
  -DITK_CUSTOM_LIBRARY_SUFFIX:STRING="-NX-$ITK_VERSION"

if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1

:: ****************************************************************************
:: Build the simplnx library
:: ****************************************************************************
echo *************************** SIMPLNX ******************************************
cd %SRC_DIR%
mkdir build
cd build

cmake --preset conda-win %SRC_DIR%/simplnx ^
  -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D Python3_EXECUTABLE:FILEPATH=%PREFIX%/python.exe

if errorlevel 1 exit 1

cmake --build . --target all
if errorlevel 1 exit 1

cmake --build . --target install
if errorlevel 1 exit 1


echo *********************************************************************
echo                         BUILD COMPLETE
echo *********************************************************************
