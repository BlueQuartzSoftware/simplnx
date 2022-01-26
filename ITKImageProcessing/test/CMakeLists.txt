#------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
  ITKImageReaderTest.cpp
  ITKImageWriterTest.cpp

  # ITKAbsImageTest.cpp
  # ITKAcosImageTest.cpp
  # ITKAsinImageTest.cpp
  # ITKAtanImageTest.cpp
  # ITKBilateralImageTest.cpp
  # ITKBinaryClosingByReconstructionImageTest.cpp
  # ITKBinaryContourImageTest.cpp
  # ITKBinaryDilateImageTest.cpp
  # ITKBinaryErodeImageTest.cpp
  # ITKBinaryMorphologicalClosingImageTest.cpp
  # ITKBinaryMorphologicalOpeningImageTest.cpp
  # ITKBinaryOpeningByReconstructionImageTest.cpp
  # ITKBinaryProjectionImageTest.cpp
  # ITKBinaryThinningImageTest.cpp
  # ITKBinomialBlurImageTest.cpp
  # ITKBlackTopHatImageTest.cpp
  # ITKClosingByReconstructionImageTest.cpp
  # ITKCosImageTest.cpp
  # ITKDilateObjectMorphologyImageTest.cpp
  # ITKErodeObjectMorphologyImageTest.cpp
  # ITKExpImageTest.cpp
  # ITKExpNegativeImageTest.cpp
  # ITKGrayscaleDilateImageTest.cpp
  # ITKGrayscaleErodeImageTest.cpp
  ITKGrayscaleFillholeImageTest.cpp
  # ITKGrayscaleGrindPeakImageTest.cpp
  # ITKGrayscaleMorphologicalClosingImageTest.cpp
  # ITKGrayscaleMorphologicalOpeningImageTest.cpp
  # ITKHConvexImageTest.cpp
  # ITKHMaximaImageTest.cpp
  # ITKHMinimaImageTest.cpp
  # ITKIntensityWindowingImageTest.cpp
  # ITKInvertIntensityImageTest.cpp
  # ITKLabelContourImageTest.cpp
  # ITKLaplacianSharpeningImageTest.cpp
  # ITKLog10ImageTest.cpp
  # ITKLogImageTest.cpp
  # ITKMaximumProjectionImageTest.cpp
  # ITKMedianProjectionImageTest.cpp
  # ITKMinimumProjectionImageTest.cpp
  # ITKMorphologicalGradientImageTest.cpp
  # ITKMultiScaleHessianBasedObjectnessImageTest.cpp
  # ITKNotImageTest.cpp
  # ITKOpeningByReconstructionImageTest.cpp
  # ITKRelabelComponentImageTest.cpp
  # ITKSaltAndPepperNoiseImageTest.cpp
  # ITKShiftScaleImageTest.cpp
  # ITKShotNoiseImageTest.cpp
  # ITKSigmoidImageTest.cpp
  # ITKSinImageTest.cpp
  # ITKSpeckleNoiseImageTest.cpp
  # ITKSqrtImageTest.cpp
  # ITKSquareImageTest.cpp
  # ITKTanImageTest.cpp
  # ITKThresholdImageTest.cpp
  # ITKValuedRegionalMaximaImageTest.cpp
  # ITKValuedRegionalMinimaImageTest.cpp
  # ITKWhiteTopHatImageTest.cpp

  #-----------------------------------------------------------------------------
  # These filters have a fixed output type
  #-----------------------------------------------------------------------------
  # ITKApproximateSignedDistanceMapImageTest.cpp
  # ITKGradientAnisotropicDiffusionImageTest.cpp
  # ITKCurvatureAnisotropicDiffusionImageTest.cpp
  ITKBinaryThresholdImageTest.cpp
  # ITKBoundedReciprocalImageTest.cpp
  # ITKConnectedComponentImageTest.cpp
  # ITKCurvatureFlowImageTest.cpp
  # ITKDanielssonDistanceMapImageTest.cpp
  # ITKDoubleThresholdImageTest.cpp
  ITKGradientMagnitudeImageTest.cpp
  # ITKGradientMagnitudeRecursiveGaussianImageTest.cpp
  # ITKIsoContourDistanceImageTest.cpp
  # ITKLaplacianRecursiveGaussianImageTest.cpp # Link error due to missing enums (?)
  # ITKMeanProjectionImageTest.cpp
  # ITKMinMaxCurvatureFlowImageTest.cpp
  # ITKMorphologicalWatershedFromMarkersImageTest.cpp
  ITKNormalizeImageTest.cpp
  # ITKNormalizeToConstantImageTest.cpp
  ITKOtsuMultipleThresholdsImageTest.cpp
  # ITKRegionalMaximaImageTest.cpp
  # ITKRegionalMinimaImageTest.cpp
  # ITKSignedDanielssonDistanceMapImageTest.cpp
  # ITKSignedMaurerDistanceMapImageTest.cpp
  # ITKSmoothingRecursiveGaussianImageTest.cpp
  # ITKStandardDeviationProjectionImageTest.cpp
  # ITKSumProjectionImageTest.cpp
  # ITKThresholdMaximumConnectedComponentsImageTest.cpp
  # ITKZeroCrossingImageTest.cpp

  #-----------------------------------------------------------------------------
  # There are parameters from the old version that do not appear in the JSON when generating.
  #-----------------------------------------------------------------------------
  # ITKRescaleIntensityImageTest.cpp
  # ITKBoxMeanImageTest.cpp
  # ITKAdaptiveHistogramEqualizationImageTest.cpp
  ITKMedianImageTest.cpp
  # ITKDiscreteGaussianImageTest.cpp

  ITKMaskImageTest.cpp
)

set(${PLUGIN_NAME}UnitTest_SRCS_Disabled
  #-----------------------------------------------------------------------------
  # These filters are custom written filters by BQ and do not conform to the
  # standard ITK filter wrapping
  #-----------------------------------------------------------------------------
  IlluminationCorrectionTest.cpp # MISSING 1 or more Parameter Implementations
  ImportAxioVisionV4MontageTest.cpp
  AxioVisionV4ToTileConfigurationTest.cpp
  ImportZenInfoMontageTest.cpp
  ApplyDewarpParametersTest.cpp # MISSING 1 or more Parameter Implementations
  CalcDewarpParametersTest.cpp # MISSING 1 or more Parameter Implementations
  ITKImportImageStackTest.cpp
  ImportVectorImageStackTest.cpp # MISSING 1 or more Parameter Implementations
  ITKRefineTileCoordinatesTest.cpp # MISSING 1 or more Parameter Implementations
  ITKImportFijiMontageTest.cpp
  ITKImportRoboMetMontageTest.cpp
  ITKPCMTileRegistrationTest.cpp
  ITKProxTVImageTest.cpp
  ITKStitchMontageTest.cpp # MISSING 1 or more Parameter Implementations
  ITKRGBToLuminanceImageTest.cpp # Multiple Different Return types so auto has issues with that

  #-----------------------------------------------------------------------------
  # These filters have a fixed output type
  #-----------------------------------------------------------------------------
  ITKVectorConnectedComponentImageTest.cpp

  #-----------------------------------------------------------------------------
  # These filters use the itk::InPlaceDream3DDataToImageFilter filter
  #-----------------------------------------------------------------------------
  ITKPatchBasedDenoisingImageTest.cpp # itk::InPlaceDream3DDataToImageFilter
  ITKMorphologicalWatershedImageTest.cpp # itk::InPlaceDream3DDataToImageFilter
  ITKMaskImageTest.cpp # itk::InPlaceDream3DDataToImageFilter
  ITKFFTNormalizedCorrelationImageTest.cpp # itk::InPlaceDream3DDataToImageFilter
  ITKHistogramMatchingImageTest.cpp # itk::InPlaceDream3DDataToImageFilter

  #-----------------------------------------------------------------------------
  # These filters use the Dream3DArraySwitchOutputComponentMacro macro
  #-----------------------------------------------------------------------------
  ITKCastImageTest.cpp # Dream3DArraySwitchOutputComponentMacro
  ITKVectorRescaleIntensityImageTest.cpp # Dream3DArraySwitchOutputComponentMacro
)

#set(${PLUGIN_NAME}UnitTest_SRCS "")

create_complex_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
  FILTER_LIST ""
)

target_sources(${PLUGIN_NAME}UnitTest PUBLIC
  ${${PLUGIN_NAME}_SOURCE_DIR}/test/ITKTestBase.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/test/ITKTestBase.cpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/test/MD5.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/test/MD5.cpp
)
