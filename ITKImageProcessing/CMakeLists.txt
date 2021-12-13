include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ITKImageProcessing")

#------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# ITKImageProcessing/src/ITKImageProcessing/Filters/ directory.
set(FilterList
  #ApplyDewarpParameters # MISSING 1 or more Parameter Implementations
  AxioVisionV4ToTileConfiguration
  #CalcDewarpParameters # MISSING 1 or more Parameter Implementations
  ITKAbsImage
  ITKAcosImage
  ITKAdaptiveHistogramEqualizationImage
  ITKApproximateSignedDistanceMapImage
  ITKAsinImage
  ITKAtanImage
  ITKBilateralImage
  ITKBinaryClosingByReconstructionImage
  ITKBinaryContourImage
  ITKBinaryDilateImage
  ITKBinaryErodeImage
  ITKBinaryMorphologicalClosingImage
  ITKBinaryMorphologicalOpeningImage
  ITKBinaryOpeningByReconstructionImage
  ITKBinaryProjectionImage
  ITKBinaryThinningImage
  ITKBinaryThresholdImage
  ITKBinomialBlurImage
  ITKBlackTopHatImage
  ITKBoundedReciprocalImage
  ITKBoxMeanImage
  ITKCastImage
  ITKClosingByReconstructionImage
  ITKConnectedComponentImage
  ITKCosImage
  ITKCurvatureAnisotropicDiffusionImage
  ITKCurvatureFlowImage
  ITKDanielssonDistanceMapImage
  ITKDilateObjectMorphologyImage
  ITKDiscreteGaussianImage
  ITKDoubleThresholdImage
  ITKErodeObjectMorphologyImage
  ITKExpImage
  ITKExpNegativeImage
  ITKFFTNormalizedCorrelationImage
  ITKGradientAnisotropicDiffusionImage
  ITKGradientMagnitudeImage
  ITKGradientMagnitudeRecursiveGaussianImage
  ITKGrayscaleDilateImage
  ITKGrayscaleErodeImage
  ITKGrayscaleFillholeImage
  ITKGrayscaleGrindPeakImage
  ITKGrayscaleMorphologicalClosingImage
  ITKGrayscaleMorphologicalOpeningImage
  ITKHConvexImage
  ITKHMaximaImage
  ITKHMinimaImage
  ITKHistogramMatchingImage
  ITKImageReader
  ITKImageWriter
  ITKImportFijiMontage
  ITKImportImageStack
  ITKImportRoboMetMontage
  ITKIntensityWindowingImage
  ITKInvertIntensityImage
  ITKIsoContourDistanceImage
  ITKLabelContourImage
  ITKLaplacianRecursiveGaussianImage
  ITKLaplacianSharpeningImage
  ITKLog10Image
  ITKLogImage
  ITKMaskImage
  ITKMaximumProjectionImage
  ITKMeanProjectionImage
  ITKMedianImage
  ITKMedianProjectionImage
  ITKMinMaxCurvatureFlowImage
  ITKMinimumProjectionImage
  ITKMorphologicalGradientImage
  ITKMorphologicalWatershedFromMarkersImage
  ITKMorphologicalWatershedImage
  ITKMultiScaleHessianBasedObjectnessImage
  ITKNormalizeImage
  ITKNormalizeToConstantImage
  ITKNotImage
  ITKOpeningByReconstructionImage
  ITKOtsuMultipleThresholdsImage
  ITKPCMTileRegistration
  ITKPatchBasedDenoisingImage
  ITKProxTVImage
  ITKRGBToLuminanceImage
  #ITKRefineTileCoordinates # MISSING 1 or more Parameter Implementations
  ITKRegionalMaximaImage
  ITKRegionalMinimaImage
  ITKRelabelComponentImage
  ITKRescaleIntensityImage
  ITKSaltAndPepperNoiseImage
  ITKShiftScaleImage
  ITKShotNoiseImage
  ITKSigmoidImage
  ITKSignedDanielssonDistanceMapImage
  ITKSignedMaurerDistanceMapImage
  ITKSinImage
  ITKSmoothingRecursiveGaussianImage
  ITKSpeckleNoiseImage
  ITKSqrtImage
  ITKSquareImage
  ITKStandardDeviationProjectionImage
  #ITKStitchMontage # MISSING 1 or more Parameter Implementations
  ITKSumProjectionImage
  ITKTanImage
  ITKThresholdImage
  ITKThresholdMaximumConnectedComponentsImage
  ITKValuedRegionalMaximaImage
  ITKValuedRegionalMinimaImage
  ITKVectorConnectedComponentImage
  ITKVectorRescaleIntensityImage
  ITKWhiteTopHatImage
  ITKZeroCrossingImage
  #IlluminationCorrection # MISSING 1 or more Parameter Implementations
  ImportAxioVisionV4Montage
  #ImportVectorImageStack # MISSING 1 or more Parameter Implementations
  ImportZenInfoMontage
)

create_complex_plugin(NAME ${PLUGIN_NAME}
  FILTER_LIST ${FilterList}
  DESCRIPTION "Plugin for ${PLUGIN_NAME}"
  VERSION "0.1.0"
)

find_package(ITK REQUIRED)

target_link_libraries(${PLUGIN_NAME}
  PUBLIC
    ITKCommon
    ITKIOImageBase
    ITKIOBMP
    ITKIOBioRad
    ITKIOGE
    ITKIOGIPL
    ITKIOJPEG
    ITKIOMRC
    ITKIOMeta
    ITKIONIFTI
    ITKIONRRD
    ITKIOPNG
    ITKIOStimulate
    ITKIOTIFF
    ITKIOVTK
)

target_compile_options(${PLUGIN_NAME}
  PUBLIC
    ${ITK_REQUIRED_CXX_FLAGS}
)

target_sources(${PLUGIN_NAME}
  PRIVATE
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKArrayHelper.hpp
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKArrayHelper.cpp
)

target_include_directories(${PLUGIN_NAME} PUBLIC  ${ITK_INCLUDE_DIRS})

#------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
enable_testing()
add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
