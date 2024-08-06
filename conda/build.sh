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
cp -r "simplnx/src/Plugins/ITKImageProcessing/pipelines" "$PREFIX/share/simplnx/pipelines/ITKImageProcessing/"

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
  -D H5Support_DIR:PATH="$PREFIX/share/H5Support/H5Support"

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
echo " BUILDING ITK"
echo "#-------------------------------------------------------------------------------"
mkdir itk_build
cd itk_build

cmake -S "../itk" -B . -G "Ninja" $CMAKE_ARGS \
  -D CMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
  -D CMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX" \
  -D Eigen3_DIR=${PREFIX}/share/eigen3/cmake \
  -D BUILD_SHARED_LIBS:STRING=ON \
  -D CMAKE_OSX_DEPLOYMENT_TARGET:STRING=11.0 \
  -D CMAKE_SKIP_INSTALL_RPATH:BOOL=OFF \
  -D CMAKE_SKIP_RPATH:BOOL=OFF \
  -D CMAKE_CXX_STANDARD:STRING=17 \
  -D CMAKE_CXX_STANDARD_REQUIRED:BOOL=ON \
  -D BUILD_DOCUMENTATION:BOOL=OFF \
  -D BUILD_EXAMPLES:BOOL=OFF \
  -D BUILD_TESTING:BOOL=OFF \
  -D KWSYS_USE_MD5:BOOL=ON \
  -D ITK_LEGACY_REMOVE:BOOL=ON \
  -D ITK_FUTURE_LEGACY_REMOVE:BOOL=ON \
  -D ITK_LEGACY_SILENT:BOOL=OFF \
  -D ITKV4_COMPATIBILITY:BOOL=OFF \
  -D ITK_USE_SYSTEM_EIGEN:BOOL=ON \
  -D ITK_USE_SYSTEM_HDF5:BOOL=ON \
  -D ITKGroup_Core:BOOL=ON \
  -D ITKGroup_Filtering:BOOL=ON \
  -D ITKGroup_Registration:BOOL=ON \
  -D ITKGroup_Segmentation:BOOL=ON \
  -D ITK_BUILD_DEFAULT_MODULES:BOOL=OFF \
  -D Module_ITKTestKernel:BOOL=OFF \
  -D Module_ITKReview:BOOL=OFF \
  -D Module_SCIFIO=OFF \
  -D Module_ITKMetricsv4:BOOL=OFF \
  -D Module_ITKOptimizersv4:BOOL=OFF \
  -D Module_ITKRegistrationMethodsv4:BOOL=OFF \
  -D Module_ITKConvolution:BOOL=ON \
  -D Module_ITKDenoising:BOOL=ON \
  -D Module_ITKImageNoise:BOOL=ON \
  -D ITKGroup_IO:BOOL=OFF \
  -D ITKGroup_Core:BOOL=OFF \
  -D Module_ITKGDCM:BOOL=OFF \
  -D Module_ITKIOBioRad:BOOL=ON \
  -D Module_ITKIOBMP:BOOL=ON \
  -D Module_ITKIOGE:BOOL=ON \
  -D Module_ITKIOGIPL:BOOL=ON \
  -D Module_ITKIOImageBase:BOOL=ON \
  -D Module_ITKIOIPL:BOOL=ON \
  -D Module_ITKIOJPEG:BOOL=ON \
  -D Module_ITKIOMeta:BOOL=ON \
  -D Module_ITKIOMRC:BOOL=ON \
  -D Module_ITKIONIFTI:BOOL=ON \
  -D Module_ITKIONRRD:BOOL=ON \
  -D Module_ITKIOPNG:BOOL=ON \
  -D Module_ITKIOStimulate:BOOL=ON \
  -D Module_ITKIOTIFF:BOOL=ON \
  -D Module_ITKIOVTK:BOOL=ON \
  -D Module_ITKIOTransformBase:BOOL=ON \
  -DITK_CUSTOM_LIBRARY_SUFFIX:STRING="-NX-$ITK_VERSION"

#if errorlevel 1 exit 1

cmake --build . --target all
#if errorlevel 1 exit 1

cmake --build . --target install
#if errorlevel 1 exit 1

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
