include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ITKImageProcessing")

#------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# ITKImageProcessing/src/ITKImageProcessing/Filters/ directory.

set(FilterList
  ITKImageReader
  ITKImageWriter

  # ITKAbsImage
  # ITKAcosImage
  # ITKAsinImage
  # ITKAtanImage
  # ITKBilateralImage
  # ITKBinaryClosingByReconstructionImage
  # ITKBinaryContourImage
  # ITKBinaryDilateImage
  # ITKBinaryErodeImage
  # ITKBinaryMorphologicalClosingImage
  # ITKBinaryMorphologicalOpeningImage
  # ITKBinaryOpeningByReconstructionImage
  # ITKBinaryProjectionImage
  # ITKBinaryThinningImage
  # ITKBinomialBlurImage
  # ITKBlackTopHatImage
  # ITKClosingByReconstructionImage
  # ITKCosImage
  # ITKDilateObjectMorphologyImage
  # ITKErodeObjectMorphologyImage
  # ITKExpImage
  # ITKExpNegativeImage
  # ITKGrayscaleDilateImage
  # ITKGrayscaleErodeImage
  ITKGrayscaleFillholeImage
  # ITKGrayscaleGrindPeakImage
  # ITKGrayscaleMorphologicalClosingImage
  # ITKGrayscaleMorphologicalOpeningImage
  # ITKHConvexImage
  # ITKHMaximaImage
  # ITKHMinimaImage
  # ITKIntensityWindowingImage
  # ITKInvertIntensityImage
  # ITKLabelContourImage
  # ITKLaplacianSharpeningImage
  # ITKLog10Image
  # ITKLogImage
  # ITKMaximumProjectionImage
  # ITKMedianProjectionImage
  # ITKMinimumProjectionImage
  # ITKMorphologicalGradientImage
  # ITKMultiScaleHessianBasedObjectnessImage
  # ITKNotImage
  # ITKOpeningByReconstructionImage
  # ITKRelabelComponentImage
  # ITKSaltAndPepperNoiseImage
  # ITKShiftScaleImage
  # ITKShotNoiseImage
  # ITKSigmoidImage
  # ITKSinImage
  # ITKSpeckleNoiseImage
  # ITKSqrtImage
  # ITKSquareImage
  # ITKTanImage
  # ITKThresholdImage
  # ITKValuedRegionalMaximaImage
  # ITKValuedRegionalMinimaImage
  # ITKWhiteTopHatImage

  #-----------------------------------------------------------------------------
  # These filters have a fixed output type
  #-----------------------------------------------------------------------------
  # ITKApproximateSignedDistanceMapImage
  # ITKMinMaxCurvatureFlowImage
  # ITKGradientAnisotropicDiffusionImage
  # ITKCurvatureAnisotropicDiffusionImage
  ITKBinaryThresholdImage
  # ITKBoundedReciprocalImage
  # ITKConnectedComponentImage
  # ITKCurvatureFlowImage
  # ITKDanielssonDistanceMapImage
  # ITKDoubleThresholdImage
  ITKGradientMagnitudeImage
  # ITKGradientMagnitudeRecursiveGaussianImage
  # ITKIsoContourDistanceImage
  # ITKLaplacianRecursiveGaussianImage
  # ITKMeanProjectionImage
  ITKNormalizeImage
  # ITKNormalizeToConstantImage
  ITKOtsuMultipleThresholdsImage
  # ITKRegionalMaximaImage
  # ITKRegionalMinimaImage
  # ITKStandardDeviationProjectionImage
  # ITKSumProjectionImage
  # ITKThresholdMaximumConnectedComponentsImage
  # ITKZeroCrossingImage
  # ITKMorphologicalWatershedFromMarkersImage
  # ITKSignedDanielssonDistanceMapImage
  # ITKSignedMaurerDistanceMapImage
  # ITKSmoothingRecursiveGaussianImage

  #-----------------------------------------------------------------------------
  # There are parameters from the old version that do not appear in the JSON when generating.
  #-----------------------------------------------------------------------------
  # ITKRescaleIntensityImage
  # ITKBoxMeanImage
  # ITKAdaptiveHistogramEqualizationImage
  ITKMedianImage
  # ITKDiscreteGaussianImage

  ITKMaskImage
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
  ITKRGBToLuminanceImage # Multiple Different Return types so auto has issues with that

  #-----------------------------------------------------------------------------
  # These filters have a fixed output type
  #-----------------------------------------------------------------------------
  ITKVectorConnectedComponentImage # Vector Input, uint32_t output

  #-----------------------------------------------------------------------------
  # These filters use the itk::InPlaceDream3DDataToImageFilter filter
  #-----------------------------------------------------------------------------
  ITKPatchBasedDenoisingImage # itk::InPlaceDream3DDataToImageFilter
  ITKMorphologicalWatershedImage # itk::InPlaceDream3DDataToImageFilter
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

set(ItkComponents 
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

target_link_libraries(${PLUGIN_NAME} PUBLIC ${ItkComponents})

if(MSVC)
  target_compile_options(${PLUGIN_NAME}
    PUBLIC
      /bigobj
  )
endif()

target_sources(${PLUGIN_NAME}
  PRIVATE
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKArrayHelper.hpp
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKArrayHelper.cpp
)

target_include_directories(${PLUGIN_NAME} PUBLIC  ${ITK_INCLUDE_DIRS})

if(WIN32)
  # This removes some C++17 Deprecation Warnings inside of ITK 5.1
  target_compile_definitions(${PLUGIN_NAME} PUBLIC "_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING")
  set_target_properties(${PLUGIN_NAME} PROPERTIES LINK_FLAGS_DEBUG "/INCREMENTAL:NO" )
endif()

#------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
enable_testing()
add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)

#------------------------------------------------------------------------------
# Add in the install rules for ITK Libraries
#------------------------------------------------------------------------------
get_filename_component(ITKImageProcessing_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY CACHE)
get_filename_component(COMPLEX_PLUGINS_SOURCE_DIR ${ITKImageProcessing_SOURCE_DIR} DIRECTORY CACHE)
# message(STATUS "ITKImageProcessing_SOURCE_DIR: ${ITKImageProcessing_SOURCE_DIR}")
include(${COMPLEX_PLUGINS_SOURCE_DIR}/cmake/ITKInstall.cmake)
AddItkCopyInstallRules(LIBS ${ItkComponents})
