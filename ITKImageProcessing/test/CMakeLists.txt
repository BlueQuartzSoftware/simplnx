#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

#------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
    #ApplyDewarpParametersTest.cpp    # MISSING 1 or more Parameter Implementations
    AxioVisionV4ToTileConfigurationTest.cpp
    #CalcDewarpParametersTest.cpp    # MISSING 1 or more Parameter Implementations
    ITKAbsImageTest.cpp
    ITKAcosImageTest.cpp
    ITKAdaptiveHistogramEqualizationImageTest.cpp
    ITKApproximateSignedDistanceMapImageTest.cpp
    ITKAsinImageTest.cpp
    ITKAtanImageTest.cpp
    ITKBilateralImageTest.cpp
    ITKBinaryClosingByReconstructionImageTest.cpp
    ITKBinaryContourImageTest.cpp
    ITKBinaryDilateImageTest.cpp
    ITKBinaryErodeImageTest.cpp
    ITKBinaryMorphologicalClosingImageTest.cpp
    ITKBinaryMorphologicalOpeningImageTest.cpp
    ITKBinaryOpeningByReconstructionImageTest.cpp
    ITKBinaryProjectionImageTest.cpp
    ITKBinaryThinningImageTest.cpp
    ITKBinaryThresholdImageTest.cpp
    ITKBinomialBlurImageTest.cpp
    ITKBlackTopHatImageTest.cpp
    ITKBoundedReciprocalImageTest.cpp
    ITKBoxMeanImageTest.cpp
    ITKCastImageTest.cpp
    ITKClosingByReconstructionImageTest.cpp
    ITKConnectedComponentImageTest.cpp
    ITKCosImageTest.cpp
    ITKCurvatureAnisotropicDiffusionImageTest.cpp
    ITKCurvatureFlowImageTest.cpp
    ITKDanielssonDistanceMapImageTest.cpp
    ITKDilateObjectMorphologyImageTest.cpp
    ITKDiscreteGaussianImageTest.cpp
    ITKDoubleThresholdImageTest.cpp
    ITKErodeObjectMorphologyImageTest.cpp
    ITKExpImageTest.cpp
    ITKExpNegativeImageTest.cpp
    ITKFFTNormalizedCorrelationImageTest.cpp
    ITKGradientAnisotropicDiffusionImageTest.cpp
    ITKGradientMagnitudeImageTest.cpp
    ITKGradientMagnitudeRecursiveGaussianImageTest.cpp
    ITKGrayscaleDilateImageTest.cpp
    ITKGrayscaleErodeImageTest.cpp
    ITKGrayscaleFillholeImageTest.cpp
    ITKGrayscaleGrindPeakImageTest.cpp
    ITKGrayscaleMorphologicalClosingImageTest.cpp
    ITKGrayscaleMorphologicalOpeningImageTest.cpp
    ITKHConvexImageTest.cpp
    ITKHMaximaImageTest.cpp
    ITKHMinimaImageTest.cpp
    ITKHistogramMatchingImageTest.cpp
    ITKImageReaderTest.cpp
    ITKImageWriterTest.cpp
    ITKImportFijiMontageTest.cpp
    ITKImportImageStackTest.cpp
    ITKImportRoboMetMontageTest.cpp
    ITKIntensityWindowingImageTest.cpp
    ITKInvertIntensityImageTest.cpp
    ITKIsoContourDistanceImageTest.cpp
    ITKLabelContourImageTest.cpp
    ITKLaplacianRecursiveGaussianImageTest.cpp
    ITKLaplacianSharpeningImageTest.cpp
    ITKLog10ImageTest.cpp
    ITKLogImageTest.cpp
    ITKMaskImageTest.cpp
    ITKMaximumProjectionImageTest.cpp
    ITKMeanProjectionImageTest.cpp
    ITKMedianImageTest.cpp
    ITKMedianProjectionImageTest.cpp
    ITKMinMaxCurvatureFlowImageTest.cpp
    ITKMinimumProjectionImageTest.cpp
    ITKMorphologicalGradientImageTest.cpp
    ITKMorphologicalWatershedFromMarkersImageTest.cpp
    ITKMorphologicalWatershedImageTest.cpp
    ITKMultiScaleHessianBasedObjectnessImageTest.cpp
    ITKNormalizeImageTest.cpp
    ITKNormalizeToConstantImageTest.cpp
    ITKNotImageTest.cpp
    ITKOpeningByReconstructionImageTest.cpp
    ITKOtsuMultipleThresholdsImageTest.cpp
    ITKPCMTileRegistrationTest.cpp
    ITKPatchBasedDenoisingImageTest.cpp
    ITKProxTVImageTest.cpp
    ITKRGBToLuminanceImageTest.cpp
    #ITKRefineTileCoordinatesTest.cpp    # MISSING 1 or more Parameter Implementations
    ITKRegionalMaximaImageTest.cpp
    ITKRegionalMinimaImageTest.cpp
    ITKRelabelComponentImageTest.cpp
    ITKRescaleIntensityImageTest.cpp
    ITKSaltAndPepperNoiseImageTest.cpp
    ITKShiftScaleImageTest.cpp
    ITKShotNoiseImageTest.cpp
    ITKSigmoidImageTest.cpp
    ITKSignedDanielssonDistanceMapImageTest.cpp
    ITKSignedMaurerDistanceMapImageTest.cpp
    ITKSinImageTest.cpp
    ITKSmoothingRecursiveGaussianImageTest.cpp
    ITKSpeckleNoiseImageTest.cpp
    ITKSqrtImageTest.cpp
    ITKSquareImageTest.cpp
    ITKStandardDeviationProjectionImageTest.cpp
    #ITKStitchMontageTest.cpp    # MISSING 1 or more Parameter Implementations
    ITKSumProjectionImageTest.cpp
    ITKTanImageTest.cpp
    ITKThresholdImageTest.cpp
    ITKThresholdMaximumConnectedComponentsImageTest.cpp
    ITKValuedRegionalMaximaImageTest.cpp
    ITKValuedRegionalMinimaImageTest.cpp
    ITKVectorConnectedComponentImageTest.cpp
    ITKVectorRescaleIntensityImageTest.cpp
    ITKWhiteTopHatImageTest.cpp
    ITKZeroCrossingImageTest.cpp
    #IlluminationCorrectionTest.cpp    # MISSING 1 or more Parameter Implementations
    ImportAxioVisionV4MontageTest.cpp
    #ImportVectorImageStackTest.cpp    # MISSING 1 or more Parameter Implementations
    ImportZenInfoMontageTest.cpp

)

create_complex_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
                                FILTER_LIST ${${PLUGIN_NAME}UnitTest_SRCS})

#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# target_link_libraries(${PLUGIN_NAME}UnitTest PUBLIC [name of library])

#------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call
# target_sources(${PLUGIN_NAME}UnitTest
#   PUBLIC
#     additional header files
#   PRIVATE
#     additional source files
# )

#------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
# target_include_directories(${PLUGIN_NAME}UnitTest
#     PUBLIC
#     additional include directories here
# )
