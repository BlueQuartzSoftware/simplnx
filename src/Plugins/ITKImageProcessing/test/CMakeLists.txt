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
    ITKCosImageTest.cpp
    ITKDilateObjectMorphologyImageTest.cpp
    ITKErodeObjectMorphologyImageTest.cpp
    ITKExpImageTest.cpp
    ITKExpNegativeImageTest.cpp
    ITKGradientMagnitudeImageTest.cpp
    ITKGrayscaleDilateImageTest.cpp
    ITKGrayscaleErodeImageTest.cpp
    ITKGrayscaleMorphologicalClosingImageTest.cpp
    ITKGrayscaleMorphologicalOpeningImageTest.cpp
    ITKImportFijiMontageTest.cpp
    ITKIntensityWindowingImageTest.cpp
    ITKInvertIntensityImageTest.cpp
    ITKLog10ImageTest.cpp
    ITKLogImageTest.cpp
    ITKMaskImageTest.cpp
    ITKNormalizeImageTest.cpp
    ITKNotImageTest.cpp
    ITKOtsuMultipleThresholdsImageTest.cpp
    ITKRelabelComponentImageTest.cpp
    ITKSigmoidImageTest.cpp
    ITKSinImageTest.cpp
    ITKSqrtImageTest.cpp
    ITKSquareImageTest.cpp
    ITKTanImageTest.cpp
    ITKThresholdImageTest.cpp
    ITKWhiteTopHatImageTest.cpp
  )
endif()

create_simplnx_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
  FILTER_LIST ""
)

target_sources(${PLUGIN_NAME}UnitTest
  PRIVATE
    ${${PLUGIN_NAME}_SOURCE_DIR}/test/ITKTestBase.hpp
    ${${PLUGIN_NAME}_SOURCE_DIR}/test/ITKTestBase.cpp
    ${${PLUGIN_NAME}_SOURCE_DIR}/test/MD5.hpp
    ${${PLUGIN_NAME}_SOURCE_DIR}/test/MD5.cpp
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

endif()

create_pipeline_tests(PLUGIN_NAME ${PLUGIN_NAME} PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})
