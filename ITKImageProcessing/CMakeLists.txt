include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ITKImageProcessing")

#------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# ITKImageProcessing/src/ITKImageProcessing/Filters/ directory.
set(FilterList
  ITKImageReader
  ITKImageWriter

  ITKAbsImage
  ITKAcosImage
  ITKAdaptiveHistogramEqualizationImage
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
  ITKBinomialBlurImage
  ITKBlackTopHatImage
  ITKBoxMeanImage
  ITKClosingByReconstructionImage
  ITKCosImage
  ITKDilateObjectMorphologyImage
  ITKDiscreteGaussianImage
  ITKErodeObjectMorphologyImage
  ITKExpImage
  ITKExpNegativeImage
  ITKGrayscaleDilateImage
  ITKGrayscaleErodeImage
  ITKGrayscaleFillholeImage
  ITKGrayscaleGrindPeakImage
  ITKGrayscaleMorphologicalClosingImage
  ITKGrayscaleMorphologicalOpeningImage
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
  ITKMedianImage
  ITKMedianProjectionImage
  ITKMinimumProjectionImage
  ITKMorphologicalGradientImage
  ITKMultiScaleHessianBasedObjectnessImage
  ITKNotImage
  ITKOpeningByReconstructionImage
  ITKRelabelComponentImage
  ITKRescaleIntensityImage
  ITKSaltAndPepperNoiseImage
  ITKShiftScaleImage
  ITKShotNoiseImage
  ITKSigmoidImage
  ITKSinImage
  ITKSpeckleNoiseImage
  ITKSqrtImage
  ITKSquareImage
  ITKTanImage
  ITKThresholdImage
  ITKValuedRegionalMaximaImage
  ITKValuedRegionalMinimaImage
  ITKWhiteTopHatImage
  )

set(Filters_Disabled
  #-----------------------------------------------------------------------------
  # These filters are custom written filters by BQ and do not conform to the
  # standard ITK filter wrapping
  #-----------------------------------------------------------------------------
  IlluminationCorrection # MISSING 1 or more Parameter Implementations
  ImportAxioVisionV4Montage
  AxioVisionV4ToTileConfiguration
  ImportZenInfoMontage
  ApplyDewarpParameters # MISSING 1 or more Parameter Implementations
  CalcDewarpParameters # MISSING 1 or more Parameter Implementations
  ITKImportImageStack
  ImportVectorImageStack # MISSING 1 or more Parameter Implementations
  ITKRefineTileCoordinates # MISSING 1 or more Parameter Implementations
  ITKImportFijiMontage
  ITKImportRoboMetMontage
  ITKPCMTileRegistration
  ITKProxTVImage
  ITKStitchMontage # MISSING 1 or more Parameter Implementations

  #-----------------------------------------------------------------------------
  # These filters have a fixed output type of 'double'
  #-----------------------------------------------------------------------------
  ITKRGBToLuminanceImage # Multiple Different Return types so auto has issues with that
  ITKMinMaxCurvatureFlowImage
  ITKGradientAnisotropicDiffusionImage
  ITKCurvatureAnisotropicDiffusionImage
  ITKApproximateSignedDistanceMapImage
  ITKBinaryThresholdImage
  ITKBoundedReciprocalImage
  ITKConnectedComponentImage
  ITKCurvatureFlowImage
  ITKDanielssonDistanceMapImage
  ITKDoubleThresholdImage
  ITKGradientMagnitudeImage
  ITKGradientMagnitudeRecursiveGaussianImage
  ITKIsoContourDistanceImage
  ITKLaplacianRecursiveGaussianImage # Link error due to missing enums (?)
  ITKMeanProjectionImage
  ITKMorphologicalWatershedFromMarkersImage
  ITKNormalizeImage
  ITKNormalizeToConstantImage
  ITKOtsuMultipleThresholdsImage
  ITKRegionalMaximaImage
  ITKRegionalMinimaImage
  ITKSignedDanielssonDistanceMapImage
  ITKSignedMaurerDistanceMapImage
  ITKSmoothingRecursiveGaussianImage
  ITKStandardDeviationProjectionImage
  ITKSumProjectionImage
  ITKThresholdMaximumConnectedComponentsImage
  ITKVectorConnectedComponentImage
  ITKZeroCrossingImage

  #-----------------------------------------------------------------------------
  # These filters use the itk::InPlaceDream3DDataToImageFilter filter
  #-----------------------------------------------------------------------------
  ITKPatchBasedDenoisingImage # itk::InPlaceDream3DDataToImageFilter
  ITKMorphologicalWatershedImage # itk::InPlaceDream3DDataToImageFilter
  ITKMaskImage # itk::InPlaceDream3DDataToImageFilter
  ITKFFTNormalizedCorrelationImage # itk::InPlaceDream3DDataToImageFilter
  ITKHistogramMatchingImage # itk::InPlaceDream3DDataToImageFilter

  #-----------------------------------------------------------------------------
  # These filters use the Dream3DArraySwitchOutputComponentMacro macro
  #-----------------------------------------------------------------------------
  ITKCastImage # Dream3DArraySwitchOutputComponentMacro
  ITKVectorRescaleIntensityImage # Dream3DArraySwitchOutputComponentMacro
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
  ITKMathematicalMorphology
  ITKImageFeature
  ITKTransform
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
