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
  -D GVS_GIT_HASH:STRING="d7db8a3a5f11b97ca56b9864de710f552d9dccf4" ^
  -D H5Support_DIR:PATH="%LIBRARY_PREFIX%/share/H5Support"
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
  -D CMAKE_BUILD_TYPE:STRING=Release ^
  -D CMAKE_INSTALL_PREFIX:PATH="%LIBRARY_PREFIX%" ^
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="%LIBRARY_PREFIX%" ^
  -D Eigen3_DIR=%lib_prefix%/share/eigen3/cmake ^
  -D BUILD_SHARED_LIBS:STRING=ON ^
  -D CMAKE_OSX_DEPLOYMENT_TARGET:STRING=11.0 ^
  -D CMAKE_SKIP_INSTALL_RPATH:BOOL=OFF ^
  -D CMAKE_SKIP_RPATH:BOOL=OFF ^
  -D CMAKE_CXX_STANDARD:STRING=17 ^
  -D CMAKE_CXX_STANDARD_REQUIRED:BOOL=ON ^
  -D BUILD_DOCUMENTATION:BOOL=OFF ^
  -D BUILD_EXAMPLES:BOOL=OFF ^
  -D BUILD_TESTING:BOOL=OFF ^
  -D KWSYS_USE_MD5:BOOL=ON ^
  -D ITK_LEGACY_REMOVE:BOOL=ON ^
  -D ITK_FUTURE_LEGACY_REMOVE:BOOL=ON ^
  -D ITK_LEGACY_SILENT:BOOL=OFF ^
  -D ITKV4_COMPATIBILITY:BOOL=OFF ^
  -D ITK_USE_SYSTEM_EIGEN:BOOL=ON ^
  -D ITK_USE_SYSTEM_HDF5:BOOL=ON ^
  -D ITKGroup_Core:BOOL=ON ^
  -D ITKGroup_Filtering:BOOL=ON ^
  -D ITKGroup_Registration:BOOL=ON ^
  -D ITKGroup_Segmentation:BOOL=ON ^
  -D ITK_BUILD_DEFAULT_MODULES:BOOL=OFF ^
  -D Module_ITKTestKernel:BOOL=OFF ^
  -D Module_ITKReview:BOOL=OFF ^
  -D Module_SCIFIO=OFF ^
  -D Module_ITKMetricsv4:BOOL=OFF ^
  -D Module_ITKOptimizersv4:BOOL=OFF ^
  -D Module_ITKRegistrationMethodsv4:BOOL=OFF ^
  -D Module_ITKConvolution:BOOL=ON ^
  -D Module_ITKDenoising:BOOL=ON ^
  -D Module_ITKImageNoise:BOOL=ON ^
  -D ITKGroup_IO:BOOL=OFF ^
  -D ITKGroup_Core:BOOL=OFF ^
  -D Module_ITKGDCM:BOOL=OFF ^
  -D Module_ITKIOBioRad:BOOL=ON ^
  -D Module_ITKIOBMP:BOOL=ON ^
  -D Module_ITKIOGE:BOOL=ON ^
  -D Module_ITKIOGIPL:BOOL=ON ^
  -D Module_ITKIOImageBase:BOOL=ON ^
  -D Module_ITKIOIPL:BOOL=ON ^
  -D Module_ITKIOJPEG:BOOL=ON ^
  -D Module_ITKIOMeta:BOOL=ON ^
  -D Module_ITKIOMRC:BOOL=ON ^
  -D Module_ITKIONIFTI:BOOL=ON ^
  -D Module_ITKIONRRD:BOOL=ON ^
  -D Module_ITKIOPNG:BOOL=ON ^
  -D Module_ITKIOStimulate:BOOL=ON ^
  -D Module_ITKIOTIFF:BOOL=ON ^
  -D Module_ITKIOVTK:BOOL=ON ^
  -D Module_ITKIOTransformBase:BOOL=ON ^
  -D ITK_SKIP_PATH_LENGTH_CHECKS:BOOL=ON ^
  -D ITK_CUSTOM_LIBRARY_SUFFIX:STRING="-NX-%ITK_VERSION%"

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
