
set(${PLUGIN_NAME}UnitTest_SRCS
  BinaryContourImageFilterTest.cpp
  BinaryOpeningByReconstructionImageFilterTest.cpp
  ClosingByReconstructionImageFilterTest.cpp
  GrayscaleFillholeImageFilterTest.cpp
  GrayscaleGrindPeakImageFilterTest.cpp
  HConvexImageFilterTest.cpp
  HMaximaImageFilterTest.cpp
  HMinimaImageFilterTest.cpp
  LabelContourImageFilterTest.cpp
  MorphologicalGradientImageFilterTest.cpp
  MorphologicalWatershedImageFilterTest.cpp
  OpeningByReconstructionImageFilterTest.cpp
  SignedMaurerDistanceMapImageFilterTest.cpp
  ValuedRegionalMaximaImageFilterTest.cpp
  ValuedRegionalMinimaImageFilterTest.cpp

  ReadBinaryCTNorthstarTest.cpp
  ReadImageStackTest.cpp
  ReadImageTest.cpp
  ReadMhaFileTest.cpp
  ReadNIfTIFileTest.cpp
  ReadVolumeGraphicsFileTest.cpp
  ReadZeissTxmFileTest.cpp
  ImportFijiMontageTest.cpp
  WriteImageTest.cpp
)




if(NOT ImageProcessing_LEAN_AND_MEAN)
    list(APPEND ${PLUGIN_NAME}UnitTest_SRCS
      AbsImageFilterTest.cpp
      AcosImageFilterTest.cpp
      AdaptiveHistogramEqualizationImageFilterTest.cpp
      ApplyPointwiseTest.cpp
      ApproximateSignedDistanceMapImageFilterTest.cpp
      AsinImageFilterTest.cpp
      AtanImageFilterTest.cpp
      BinaryDilateImageFilterTest.cpp
      BinaryErodeImageFilterTest.cpp
      BinaryMorphologicalClosingImageFilterTest.cpp
      BinaryMorphologicalOpeningImageFilterTest.cpp
      BinaryProjectionImageFilterTest.cpp
      BinaryThinningImageFilterTest.cpp
      BinaryThresholdImageFilterTest.cpp
      BlackTopHatImageFilterTest.cpp
      BoundedReciprocalImageFilterTest.cpp
      ConnectedComponentImageFilterTest.cpp
      CosImageFilterTest.cpp
      CurvatureAnisotropicDiffusionImageFilterTest.cpp
      CurvatureFlowImageFilterTest.cpp
      DanielssonDistanceMapImageFilterTest.cpp
      DilateObjectMorphologyImageFilterTest.cpp
      DiscreteGaussianImageFilterTest.cpp
      DoubleThresholdImageFilterTest.cpp
      ErodeObjectMorphologyImageFilterTest.cpp
      ExamplePipelineCoverageTest.cpp
      ExamplePipelineExecutionTest.cpp
      ExpImageFilterTest.cpp
      ExpNegativeImageFilterTest.cpp
      GradientAnisotropicDiffusionImageFilterTest.cpp
      GradientMagnitudeImageFilterTest.cpp
      GradientMagnitudeRecursiveGaussianImageFilterTest.cpp
      GrayscaleDilateImageFilterTest.cpp
      GrayscaleErodeImageFilterTest.cpp
      GrayscaleMorphologicalClosingImageFilterTest.cpp
      GrayscaleMorphologicalOpeningImageFilterTest.cpp
      IntensityWindowingImageFilterTest.cpp
      InvertIntensityImageFilterTest.cpp
      IsoContourDistanceImageFilterTest.cpp
      ItkGoldenTestUtilsSelfTest.cpp
      LaplacianRecursiveGaussianImageFilterTest.cpp
      Log10ImageFilterTest.cpp
      LogImageFilterTest.cpp
      MaskImageFilterTest.cpp
      MaximumProjectionImageFilterTest.cpp
      MeanProjectionImageFilterTest.cpp
      MedianImageFilterTest.cpp
      MedianProjectionImageFilterTest.cpp
      MinimumProjectionImageFilterTest.cpp
      MinMaxCurvatureFlowImageFilterTest.cpp
      MorphologicalWatershedFromMarkersImageFilterTest.cpp
      NormalizeImageFilterTest.cpp
      NormalizeToConstantImageFilterTest.cpp
      NotImageFilterTest.cpp
      NrrdUtilitiesTest.cpp
      OtsuMultipleThresholdsImageFilterTest.cpp
      PipelineUuidRedirectTest.cpp
      RegionalMaximaImageFilterTest.cpp
      RegionalMinimaImageFilterTest.cpp
      RelabelComponentImageFilterTest.cpp
      RescaleIntensityImageFilterTest.cpp
      SigmoidImageFilterTest.cpp
      SignedDanielssonDistanceMapImageFilterTest.cpp
      SimplConversionCoverageTest.cpp
      SinImageFilterTest.cpp
      SmokeTest.cpp
      SmoothingRecursiveGaussianImageFilterTest.cpp
      SqrtImageFilterTest.cpp
      SquareImageFilterTest.cpp
      StandardDeviationProjectionImageFilterTest.cpp
      StoreElementwiseUtilitiesTest.cpp
      StreamingStatisticsTest.cpp
      StructuringElementTest.cpp
      SumProjectionImageFilterTest.cpp
      TanImageFilterTest.cpp
      ThresholdImageFilterTest.cpp
      ThresholdMaximumConnectedComponentsImageFilterTest.cpp
      WhiteTopHatImageFilterTest.cpp
      WorkingMemoryTest.cpp
      ZeroCrossingImageFilterTest.cpp


      #---------------------------------------
      # Test All the various Engines
      AxisProjectionEngineTest.cpp
      AdaptiveHistogramEqualizationEngineTest.cpp
      BinaryThinningEngineTest.cpp
      ConnectedComponentEngineTest.cpp
      ContourEngineTest.cpp
      DanielssonDistanceMapEngineTest.cpp
      DiscreteGaussianEngineTest.cpp
      FastChamferDistanceEngineTest.cpp
      FiniteDifferenceEngineTest.cpp
      GradientMagnitudeEngineTest.cpp
      HistogramEngineTest.cpp
      IsoContourDistanceEngineTest.cpp
      MaurerDistanceMapEngineTest.cpp
      MorphologicalReconstructionEngineTest.cpp
      MorphologyEngineTest.cpp
      ObjectMorphologyEngineTest.cpp
      RecursiveGaussianEngineTest.cpp
      RegionalExtremaEngineTest.cpp
      WatershedFromMarkersEngineTest.cpp
      ZeroCrossingEngineTest.cpp
      
  )

endif()

create_simplnx_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
                                FILTER_LIST ${${PLUGIN_NAME}UnitTest_SRCS})

target_include_directories(${PLUGIN_NAME}UnitTest
    PUBLIC
    ${simplnx_SOURCE_DIR}/test
)

# WriteImageTest uses SimplnxCore's CreateColorMapFilter as test setup. The ImageProcessing plugin target remains independent.
target_link_libraries(${PLUGIN_NAME}UnitTest PRIVATE SimplnxCore)

find_package(ZLIB REQUIRED)
target_link_libraries(${PLUGIN_NAME}UnitTest PUBLIC ZLIB::ZLIB)

if(SIMPLNX_DOWNLOAD_TEST_FILES)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ReadZeissTxmFileTest_v2.tar.gz SHA512 2d962655d919480fa7baf0c8720affbcbd1182e8482480f5413b53ead41787d35d0f15487219163ba0dcffad69be90e627730fc780107d80df3d97e14fb15cac)
endif()

if(MSVC)
  target_compile_options(${PLUGIN_NAME}UnitTest PRIVATE /bigobj)
  target_compile_definitions(${PLUGIN_NAME}UnitTest PRIVATE _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
endif()

# Register each real-world pipeline as an independent nxrunner test.
include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)
set(PREBUILT_PIPELINE_NAMES)
# The plugin inherited these three pipelines from the removed ITKImageProcessing plugin. They are flat
# files, so the loop above does not reach them.
list(APPEND PREBUILT_PIPELINE_NAMES
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(02) Image Segmentation.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(03) Porosity Mesh Export.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(04) Porosity Analysis.d3dpipeline"
)

if(NOT ImageProcessing_LEAN_AND_MEAN)
  foreach(example_name IN LISTS ${PLUGIN_NAME}_EXAMPLE_PIPELINE_NAMES)
    list(APPEND PREBUILT_PIPELINE_NAMES "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/${example_name}/${example_name}.d3dpipeline")
  endforeach()
endif()


create_pipeline_tests(PLUGIN_NAME ${PLUGIN_NAME} PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})


# Set the PYTHON_EXECUTABLE variable as that will get used when the stub shell script or batch file is created.
# The `Python3_EXECUTABLE` is usually set in the CMakePresets.json file
set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
AddPythonTest(NAME "PY::ImageProcessing::VerifyExamplePipelineCitations" FILE "${${PLUGIN_NAME}_SOURCE_DIR}/test/VerifyExamplePipelineCitations.py" PYTHONPATH "$<TARGET_FILE_DIR:simplnx>")
