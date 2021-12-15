include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ITKImageProcessing")

#------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# ITKImageProcessing/src/ITKImageProcessing/Filters/ directory.
set(FilterList
  ITKAbsImage
  ITKAcosImage
  ITKApproximateSignedDistanceMapImage
  ITKAsinImage
  ITKAtanImage
  ITKBilateralImage
  ITKBinaryContourImage
  ITKBinaryProjectionImage
  ITKBinaryThinningImage
  ITKBinaryThresholdImage
  ITKBinomialBlurImage
  ITKBoundedReciprocalImage
  ITKCosImage
  ITKDoubleThresholdImage
  ITKExpImage
  ITKExpNegativeImage
  ITKGrayscaleFillholeImage
  ITKHConvexImage
  ITKHMaximaImage
  ITKHMinimaImage
  ITKIntensityWindowingImage
  ITKInvertIntensityImage
  ITKLabelContourImage
  ITKLaplacianSharpeningImage
  ITKLog10Image
  ITKLogImage
  ITKMaximumProjectionImage
  ITKMeanProjectionImage
  ITKNormalizeImage
  ITKNormalizeToConstantImage
  ITKNotImage
  ITKOtsuMultipleThresholdsImage
  ITKRegionalMaximaImage
  ITKRegionalMinimaImage
  ITKRelabelComponentImage
  ITKSaltAndPepperNoiseImage
  ITKShiftScaleImage
  ITKShotNoiseImage
  ITKSigmoidImage
  ITKSinImage
  ITKSqrtImage
  ITKSquareImage
  ITKTanImage
  ITKImageReader
  ITKImageWriter
  )

set(Filters_Disabled

  #----------------
    ApplyDewarpParameters # MISSING 1 or more Parameter Implementations
    AxioVisionV4ToTileConfiguration
    CalcDewarpParameters # MISSING 1 or more Parameter Implementations
    IlluminationCorrection # MISSING 1 or more Parameter Implementations
    ImportAxioVisionV4Montage
    ImportVectorImageStack # MISSING 1 or more Parameter Implementations
    ImportZenInfoMontage
    ITKCurvatureAnisotropicDiffusionImage
    ITKCurvatureFlowImage
    ITKDanielssonDistanceMapImage
    ITKDilateObjectMorphologyImage
    ITKErodeObjectMorphologyImage
    ITKFFTNormalizedCorrelationImage
    ITKGradientAnisotropicDiffusionImage
    ITKGradientMagnitudeImage
    ITKGradientMagnitudeRecursiveGaussianImage
    ITKGrayscaleDilateImage
    ITKGrayscaleErodeImage
    ITKGrayscaleGrindPeakImage
    ITKGrayscaleMorphologicalClosingImage
    ITKGrayscaleMorphologicalOpeningImage
    ITKHistogramMatchingImage

    ITKImportFijiMontage
    ITKImportImageStack
    ITKImportRoboMetMontage
    ITKIsoContourDistanceImage
    ITKMedianProjectionImage
    ITKMinimumProjectionImage
    ITKMinMaxCurvatureFlowImage
    ITKMorphologicalGradientImage
    ITKMorphologicalWatershedFromMarkersImage
    ITKMorphologicalWatershedImage
    ITKMultiScaleHessianBasedObjectnessImage
    ITKOpeningByReconstructionImage
    ITKPatchBasedDenoisingImage
    ITKPCMTileRegistration
    ITKProxTVImage
    ITKRefineTileCoordinates # MISSING 1 or more Parameter Implementations
    ITKRGBToLuminanceImage
    ITKSignedDanielssonDistanceMapImage
    ITKSignedMaurerDistanceMapImage
    ITKSmoothingRecursiveGaussianImage
    ITKSpeckleNoiseImage
    ITKStandardDeviationProjectionImage
    ITKStitchMontage # MISSING 1 or more Parameter Implementations
    ITKSumProjectionImage
    ITKThresholdMaximumConnectedComponentsImage
    ITKValuedRegionalMaximaImage
    ITKValuedRegionalMinimaImage
    ITKVectorConnectedComponentImage
    ITKVectorRescaleIntensityImage
    ITKWhiteTopHatImage
    ITKZeroCrossingImage
    ITKAdaptiveHistogramEqualizationImage
    ITKBinaryClosingByReconstructionImage
    ITKBinaryDilateImage
    ITKBinaryErodeImage
    ITKBinaryMorphologicalClosingImage
    ITKBinaryMorphologicalOpeningImage
    ITKBinaryOpeningByReconstructionImage
    ITKBlackTopHatImage
    ITKBoxMeanImage
    ITKCastImage
    ITKClosingByReconstructionImage
    ITKConnectedComponentImage
    ITKDiscreteGaussianImage
    ITKLaplacianRecursiveGaussianImage  # Link error due to missing enums
    ITKMaskImage
    ITKMedianImage
    ITKRescaleIntensityImage
    ITKThresholdImage
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
    ITKStatistics
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
# add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
