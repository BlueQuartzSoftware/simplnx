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

#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)


#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# target_link_libraries(${PLUGIN_NAME} PUBLIC [name of library])

#------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call
# target_sources(${PLUGIN_NAME}
#   PUBLIC
#     additional header files
#   PRIVATE
#     additional source files
# )

#------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
# target_include_directories(${PLUGIN_NAME}
#     PUBLIC
#     additional include directories here
# )

#------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
enable_testing()
add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
