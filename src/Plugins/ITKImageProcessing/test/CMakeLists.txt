# ------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
  ITKDiscreteGaussianImageTest.cpp
  ITKImageReaderTest.cpp
  ITKImageWriterTest.cpp
  ITKImportImageStackTest.cpp
  ITKMedianImageTest.cpp
  ITKMhaFileReaderTest.cpp
  ITKRescaleIntensityImageTest.cpp
)
if(NOT SIMPLNX_CONDA_BUILD AND NOT ITKIMAGEPROCESSING_LEAN_AND_MEAN)
  list(APPEND ${PLUGIN_NAME}UnitTest_SRCS
    ITKBinaryContourImageTest.cpp
    ITKBinaryOpeningByReconstructionImageTest.cpp
    ITKClosingByReconstructionImageTest.cpp
    ITKGrayscaleFillholeImageTest.cpp
    ITKGrayscaleGrindPeakImageTest.cpp
    ITKHConvexImageTest.cpp
    ITKHMaximaImageTest.cpp
    ITKHMinimaImageTest.cpp
    ITKLabelContourImageTest.cpp
    ITKMorphologicalGradientImageTest.cpp
    ITKMorphologicalWatershedImageTest.cpp
    ITKOpeningByReconstructionImageTest.cpp
    ITKSignedMaurerDistanceMapImageTest.cpp
    ITKValuedRegionalMaximaImageTest.cpp
    ITKValuedRegionalMinimaImageTest.cpp
  )
  endif()

if(NOT ITKIMAGEPROCESSING_LEAN_AND_MEAN)
  list(APPEND ${PLUGIN_NAME}UnitTest_SRCS
    ITKAbsImageTest.cpp
    ITKAcosImageTest.cpp
    ITKAdaptiveHistogramEqualizationImageTest.cpp
    ITKApproximateSignedDistanceMapImageTest.cpp
    ITKAsinImageTest.cpp
    ITKAtanImageTest.cpp
    ITKBinaryDilateImageTest.cpp
    ITKBinaryErodeImageTest.cpp
    ITKBinaryMorphologicalClosingImageTest.cpp
    ITKBinaryMorphologicalOpeningImageTest.cpp
    ITKBinaryProjectionImageTest.cpp
    ITKBinaryThinningImageTest.cpp
    ITKBinaryThresholdImageTest.cpp
    ITKBlackTopHatImageTest.cpp
    ITKConnectedComponentImageTest.cpp
    ITKCosImageTest.cpp
    ITKCurvatureAnisotropicDiffusionImageTest.cpp
    ITKCurvatureFlowImageTest.cpp
    ITKDanielssonDistanceMapImageTest.cpp
    ITKDilateObjectMorphologyImageTest.cpp
    ITKDoubleThresholdImageTest.cpp
    ITKErodeObjectMorphologyImageTest.cpp
    ITKExpImageTest.cpp
    ITKExpNegativeImageTest.cpp
    ITKGradientAnisotropicDiffusionImageTest.cpp
    ITKGradientMagnitudeImageTest.cpp
    ITKGradientMagnitudeRecursiveGaussianImageTest.cpp
    ITKGrayscaleDilateImageTest.cpp
    ITKGrayscaleErodeImageTest.cpp
    ITKGrayscaleMorphologicalClosingImageTest.cpp
    ITKGrayscaleMorphologicalOpeningImageTest.cpp
    ITKImportFijiMontageTest.cpp
    ITKIntensityWindowingImageTest.cpp
    ITKInvertIntensityImageTest.cpp
    ITKIsoContourDistanceImageTest.cpp
    ITKLaplacianRecursiveGaussianImageTest.cpp
    ITKLog10ImageTest.cpp
    ITKLogImageTest.cpp
    ITKMaskImageTest.cpp
    ITKMaximumProjectionImageTest.cpp
    ITKMeanProjectionImageTest.cpp
    ITKMedianProjectionImageTest.cpp
    ITKMinimumProjectionImageTest.cpp
    ITKMinMaxCurvatureFlowImageTest.cpp
    ITKNormalizeImageTest.cpp
    ITKNotImageTest.cpp
    ITKOtsuMultipleThresholdsImageTest.cpp
    ITKRegionalMaximaImageTest.cpp
    ITKRegionalMinimaImageTest.cpp
    ITKRelabelComponentImageTest.cpp
    ITKSigmoidImageTest.cpp
    ITKSignedDanielssonDistanceMapImageTest.cpp
    ITKSinImageTest.cpp
    ITKSqrtImageTest.cpp
    ITKSquareImageTest.cpp
    ITKTanImageTest.cpp
    ITKThresholdImageTest.cpp
    ITKThresholdMaximumConnectedComponentsImageTest.cpp
    ITKWhiteTopHatImageTest.cpp
    ITKZeroCrossingImageTest.cpp
  )
endif()

create_simplnx_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
  FILTER_LIST ""
)

target_sources(${PLUGIN_NAME}UnitTest
  PRIVATE
    ${${PLUGIN_NAME}_SOURCE_DIR}/test/ITKTestBase.hpp
    ${${PLUGIN_NAME}_SOURCE_DIR}/test/ITKTestBase.cpp
)

# -----------------------------------------------------------------------------
# Include any example pipelines into the unit testing
# -----------------------------------------------------------------------------
include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)

# These should be relative to the 
set(PREBUILT_PIPELINE_NAMES
  # Example or other test pipelines
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(02) Image Segmentation.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(03) Porosity Mesh Export.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(04) Porosity Analysis.d3dpipeline"
)

# -----------------------------------------------------------------------------
# Here we are going to setup to download and decompress the test files. In order
# to add your own test files you will need to tar.gz the test file, compute the
# SHA 512 Hash of the file and then upload the file to
# https://github.com/bluequartzsoftware/simplnx/releases/tag/Data_Archive.
#
# Go to the web site above, "edit" the release, add the filename and SHA 512 to
# the table and then upload your compressed file.
# Save the release so that the repo is updated
# -----------------------------------------------------------------------------
if(EXISTS "${DREAM3D_DATA_DIR}" AND SIMPLNX_DOWNLOAD_TEST_FILES) 
  if(NOT EXISTS ${DREAM3D_DATA_DIR}/TestFiles/)
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME fiji_montage.tar.gz SHA512 70139babc838ce3ab1f5adddfddc86dcc51996e614c6c2d757bcb2e59e8ebdc744dac269233494b1ef8d09397aecb4ccca3384f0a91bb017f2cf6309c4ac40fa)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME image_flip_test_images.tar.gz SHA512 4e282a270251133004bf4b979d0d064631b618fc82f503184c602c40d388b725f81faf8e77654d285852acc3217d51534c9a71240be4a87a91dc46da7871e7d2)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME import_image_stack_test.tar.gz SHA512 bec92d655d0d96928f616612d46b52cd51ffa5dbc1d16a64bb79040bbdd3c50f8193350771b177bb64aa68fc0ff7e459745265818c1ea34eb27431426ff15083)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME import_image_stack_test_v2.tar.gz SHA512 3d3e0a627c4bec985a05edf4df3961758bac9ed7d1bbcf1cb5faf9468eef2e0b7183e4d54bbf4080f88364d105b488882d5a98d9eeef65becd14e5ebba9fec03)
endif()

create_pipeline_tests(PLUGIN_NAME ${PLUGIN_NAME} PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})
