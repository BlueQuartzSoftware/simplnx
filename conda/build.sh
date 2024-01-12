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
  -D GVS_GIT_HASH:STRING="6c0e5ec992472eeae5df9d627de524b59b971fab" \
  -D H5Support_DIR:PATH="$SRC_DIR/sdk/H5Support/share/H5Support"

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
  -DCMAKE_BUILD_TYPE:STRING=Release \
  -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
  -DCMAKE_SYSTEM_PREFIX_PATH:PATH="$PREFIX" \
  -DEigen3_DIR=${PREFIX}/share/eigen3/cmake \
  -DBUILD_SHARED_LIBS:STRING=ON \
  -DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=11.0 \
  -DCMAKE_SKIP_INSTALL_RPATH:BOOL=OFF \
  -DCMAKE_SKIP_RPATH:BOOL=OFF \
  -DCMAKE_CXX_STANDARD:STRING=17 \
  -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=ON \
  -DBUILD_DOCUMENTATION:BOOL=OFF \
  -DBUILD_EXAMPLES:BOOL=OFF \
  -DBUILD_TESTING:BOOL=OFF \
  -DKWSYS_USE_MD5:BOOL=ON \
  -DITK_LEGACY_REMOVE:BOOL=ON \
  -DITK_FUTURE_LEGACY_REMOVE:BOOL=ON \
  -DITK_LEGACY_SILENT:BOOL=OFF \
  -DITKV4_COMPATIBILITY:BOOL=OFF \
  -DITK_USE_SYSTEM_EIGEN:BOOL=ON \
  -DITK_USE_SYSTEM_HDF5:BOOL=ON \
  -DITKGroup_Core:BOOL=ON \
  -DITKGroup_Filtering:BOOL=ON \
  -DITKGroup_Registration:BOOL=ON \
  -DITKGroup_Segmentation:BOOL=ON \
  -DITK_BUILD_DEFAULT_MODULES:BOOL=OFF \
  -DModule_ITKTestKernel:BOOL=OFF \
  -DModule_ITKReview:BOOL=OFF \
  -DModule_SCIFIO=OFF \
  -DModule_ITKMetricsv4:BOOL=OFF \
  -DModule_ITKOptimizersv4:BOOL=OFF \
  -DModule_ITKRegistrationMethodsv4:BOOL=OFF \
  -DModule_ITKConvolution:BOOL=ON \
  -DModule_ITKDenoising:BOOL=ON \
  -DModule_ITKImageNoise:BOOL=ON \
  -DITKGroup_IO:BOOL=OFF \
  -DITKGroup_Core:BOOL=OFF \
  -DModule_ITKGDCM:BOOL=OFF \
  -DModule_ITKIOBioRad:BOOL=ON \
  -DModule_ITKIOBMP:BOOL=ON \
  -DModule_ITKIOGE:BOOL=ON \
  -DModule_ITKIOGIPL:BOOL=ON \
  -DModule_ITKIOImageBase:BOOL=ON \
  -DModule_ITKIOIPL:BOOL=ON \
  -DModule_ITKIOJPEG:BOOL=ON \
  -DModule_ITKIOMeta:BOOL=ON \
  -DModule_ITKIOMRC:BOOL=ON \
  -DModule_ITKIONIFTI:BOOL=ON \
  -DModule_ITKIONRRD:BOOL=ON \
  -DModule_ITKIOPNG:BOOL=ON \
  -DModule_ITKIOStimulate:BOOL=ON \
  -DModule_ITKIOTIFF:BOOL=ON \
  -DModule_ITKIOVTK:BOOL=ON \
  -DModule_ITKIOTransformBase:BOOL=ON \
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
  -D CMAKE_OSX_DEPLOYMENT_TARGET:STRING="11.0"\
  -D CMAKE_INSTALL_PREFIX:PATH="$PREFIX" \
  -D Python3_EXECUTABLE:FILEPATH=$PREFIX/bin/python3


cmake --build . --target all

cmake --build . --target install

cd ..


echo "*********************************************************************************"
echo "              BUILD SCRIPT COMPLETE"
echo "*********************************************************************************"
