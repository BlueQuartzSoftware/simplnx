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
if(NOT ITKImageProcessing_LeanAndMean)
    set(${PLUGIN_NAME}UnitTest_SRCS
        ${${PLUGIN_NAME}UnitTest_SRCS}

    ITKAbsImageTest.cpp
    ITKAcosImageTest.cpp
    ITKAdaptiveHistogramEqualizationImageTest.cpp
    ITKAsinImageTest.cpp
    ITKAtanImageTest.cpp
    ITKBinaryContourImageTest.cpp
    ITKBinaryDilateImageTest.cpp
    ITKBinaryErodeImageTest.cpp
    ITKBinaryMorphologicalClosingImageTest.cpp
    ITKBinaryMorphologicalOpeningImageTest.cpp
    ITKBinaryOpeningByReconstructionImageTest.cpp
    ITKBinaryProjectionImageTest.cpp
    ITKBinaryThinningImageTest.cpp
    ITKBinaryThresholdImageTest.cpp
    ITKBlackTopHatImageTest.cpp
    ITKClosingByReconstructionImageTest.cpp
    ITKCosImageTest.cpp
    ITKDilateObjectMorphologyImageTest.cpp
    ITKErodeObjectMorphologyImageTest.cpp
    ITKExpImageTest.cpp
    ITKExpNegativeImageTest.cpp
    ITKGradientMagnitudeImageTest.cpp
    ITKGrayscaleDilateImageTest.cpp
    ITKGrayscaleErodeImageTest.cpp
    ITKGrayscaleFillholeImageTest.cpp
    ITKGrayscaleGrindPeakImageTest.cpp
    ITKGrayscaleMorphologicalClosingImageTest.cpp
    ITKGrayscaleMorphologicalOpeningImageTest.cpp
    ITKHConvexImageTest.cpp
    ITKHMaximaImageTest.cpp
    ITKHMinimaImageTest.cpp
    ITKIntensityWindowingImageTest.cpp
    ITKInvertIntensityImageTest.cpp
    ITKLabelContourImageTest.cpp
    ITKLog10ImageTest.cpp
    ITKLogImageTest.cpp
    ITKMaskImageTest.cpp
    ITKMorphologicalGradientImageTest.cpp
    ITKMorphologicalWatershedImageTest.cpp
    ITKNormalizeImageTest.cpp
    ITKNotImageTest.cpp
    ITKOpeningByReconstructionImageTest.cpp
    ITKOtsuMultipleThresholdsImageTest.cpp
    ITKRelabelComponentImageTest.cpp
    ITKSigmoidImageTest.cpp
    ITKSignedMaurerDistanceMapImageTest.cpp
    ITKSinImageTest.cpp
    ITKSqrtImageTest.cpp
    ITKSquareImageTest.cpp
    ITKTanImageTest.cpp
    ITKThresholdImageTest.cpp
    ITKValuedRegionalMaximaImageTest.cpp
    ITKValuedRegionalMinimaImageTest.cpp
    ITKWhiteTopHatImageTest.cpp
)
endif ()

create_complex_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
  FILTER_LIST ""
)

target_sources(${PLUGIN_NAME}UnitTest PUBLIC
  ${${PLUGIN_NAME}_SOURCE_DIR}/test/ITKTestBase.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/test/ITKTestBase.cpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/test/MD5.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/test/MD5.cpp
)

# -----------------------------------------------------------------------------
# Include any example pipelines into the unit testing
# -----------------------------------------------------------------------------
include(${complex_SOURCE_DIR}/cmake/Utility.cmake)

# These should be relative to the 
set(PREBUILT_PIPELINE_NAMES
  # Example or other test pipelines
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(02) Image Segmentation.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(03) Porosity Mesh Export.d3dpipeline"

)

create_pipeline_tests(PLUGIN_NAME ${PLUGIN_NAME} PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})

