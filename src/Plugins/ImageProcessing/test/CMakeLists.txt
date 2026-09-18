set(${PLUGIN_NAME}UnitTest_SRCS
  SmokeTest.cpp
  ExamplePipelineCoverageTest.cpp
  ExamplePipelineExecutionTest.cpp
  ItkGoldenTestUtilsSelfTest.cpp
  ApplyPointwiseTest.cpp
  WorkingMemoryTest.cpp
  StoreElementwiseUtilitiesTest.cpp
  ThresholdImageFilterTest.cpp
  PipelineUuidRedirectTest.cpp
  AbsImageFilterTest.cpp
  AcosImageFilterTest.cpp
  AdaptiveHistogramEqualizationImageFilterTest.cpp
  AsinImageFilterTest.cpp
  AtanImageFilterTest.cpp
  CosImageFilterTest.cpp
  SinImageFilterTest.cpp
  TanImageFilterTest.cpp
  ExpImageFilterTest.cpp
  ExpNegativeImageFilterTest.cpp
  LogImageFilterTest.cpp
  Log10ImageFilterTest.cpp
  SqrtImageFilterTest.cpp
  SquareImageFilterTest.cpp
  NotImageFilterTest.cpp
  SigmoidImageFilterTest.cpp
  InvertIntensityImageFilterTest.cpp
  BoundedReciprocalImageFilterTest.cpp
  BinaryThresholdImageFilterTest.cpp
  IntensityWindowingImageFilterTest.cpp
  RescaleIntensityImageFilterTest.cpp
  NormalizeImageFilterTest.cpp
  NormalizeToConstantImageFilterTest.cpp
  MaskImageFilterTest.cpp
  MaximumProjectionImageFilterTest.cpp
  MeanProjectionImageFilterTest.cpp
  MedianImageFilterTest.cpp
  MedianProjectionImageFilterTest.cpp
  MinimumProjectionImageFilterTest.cpp
  BinaryProjectionImageFilterTest.cpp
  BinaryContourImageFilterTest.cpp
  LabelContourImageFilterTest.cpp
  ContourEngineTest.cpp
  AdaptiveHistogramEqualizationEngineTest.cpp
  BinaryDilateImageFilterTest.cpp
  BinaryErodeImageFilterTest.cpp
  BinaryMorphologicalOpeningImageFilterTest.cpp
  BinaryMorphologicalClosingImageFilterTest.cpp
  StandardDeviationProjectionImageFilterTest.cpp
  SumProjectionImageFilterTest.cpp
  HistogramEngineTest.cpp
  StreamingStatisticsTest.cpp
  OtsuMultipleThresholdsImageFilterTest.cpp
  AxisProjectionEngineTest.cpp
  StructuringElementTest.cpp
  MorphologyEngineTest.cpp
  GrayscaleDilateImageFilterTest.cpp
  GrayscaleErodeImageFilterTest.cpp
  MorphologicalGradientImageFilterTest.cpp
  GrayscaleMorphologicalOpeningImageFilterTest.cpp
  GrayscaleMorphologicalClosingImageFilterTest.cpp
  WhiteTopHatImageFilterTest.cpp
  BlackTopHatImageFilterTest.cpp
  ObjectMorphologyEngineTest.cpp
  DilateObjectMorphologyImageFilterTest.cpp
  ErodeObjectMorphologyImageFilterTest.cpp
  MorphologicalReconstructionEngineTest.cpp
  RegionalExtremaEngineTest.cpp
  MaurerDistanceMapEngineTest.cpp
  ConnectedComponentEngineTest.cpp
  ConnectedComponentImageFilterTest.cpp
  DoubleThresholdImageFilterTest.cpp
  RelabelComponentImageFilterTest.cpp
  ThresholdMaximumConnectedComponentsImageFilterTest.cpp
  DanielssonDistanceMapEngineTest.cpp
  DanielssonDistanceMapImageFilterTest.cpp
  SignedDanielssonDistanceMapImageFilterTest.cpp
  IsoContourDistanceEngineTest.cpp
  IsoContourDistanceImageFilterTest.cpp
  FastChamferDistanceEngineTest.cpp
  RecursiveGaussianEngineTest.cpp
  GradientMagnitudeEngineTest.cpp
  GradientMagnitudeImageFilterTest.cpp
  GradientMagnitudeRecursiveGaussianImageFilterTest.cpp
  DiscreteGaussianEngineTest.cpp
  DiscreteGaussianImageFilterTest.cpp
  FiniteDifferenceEngineTest.cpp
  CurvatureFlowImageFilterTest.cpp
  MinMaxCurvatureFlowImageFilterTest.cpp
  GradientAnisotropicDiffusionImageFilterTest.cpp
  CurvatureAnisotropicDiffusionImageFilterTest.cpp
  LaplacianRecursiveGaussianImageFilterTest.cpp
  SmoothingRecursiveGaussianImageFilterTest.cpp
  ApproximateSignedDistanceMapImageFilterTest.cpp
  ValuedRegionalMaximaImageFilterTest.cpp
  ValuedRegionalMinimaImageFilterTest.cpp
  RegionalMaximaImageFilterTest.cpp
  RegionalMinimaImageFilterTest.cpp
  SignedMaurerDistanceMapImageFilterTest.cpp
  GrayscaleFillholeImageFilterTest.cpp
  GrayscaleGrindPeakImageFilterTest.cpp
  HConvexImageFilterTest.cpp
  OpeningByReconstructionImageFilterTest.cpp
  ClosingByReconstructionImageFilterTest.cpp
  BinaryOpeningByReconstructionImageFilterTest.cpp
  HMaximaImageFilterTest.cpp
  HMinimaImageFilterTest.cpp
  ImportFijiMontageTest.cpp
  ZeroCrossingEngineTest.cpp
  ZeroCrossingImageFilterTest.cpp
  BinaryThinningEngineTest.cpp
  BinaryThinningImageFilterTest.cpp
  WatershedFromMarkersEngineTest.cpp
  MorphologicalWatershedFromMarkersImageFilterTest.cpp
  MorphologicalWatershedImageFilterTest.cpp
  NrrdUtilitiesTest.cpp
  ReadBinaryCTNorthstarTest.cpp
  ReadImageTest.cpp
  ReadImageStackTest.cpp
  ReadMhaFileTest.cpp
  ReadNIfTIFileTest.cpp
  ReadVolumeGraphicsFileTest.cpp
  ReadZeissTxmFileTest.cpp
  WriteImageTest.cpp
)

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

# --- Test-only ITK dependency: direct itk:: oracles for BOTH morphological-watershed filters -- the composite
# --- MorphologicalWatershed (a removal-proof gate independent of the legacy-UUID wrapper) and
# --- MorphologicalWatershedFromMarkers (whose legacy NX wrapper is disabled/broken). The composite oracle,
# --- itk::MorphologicalWatershedImageFilter, internally chains HMinima/RegionalMinima (ITKMathematicalMorphology),
# --- ConnectedComponent (ITKConnectedComponents), and the FromMarkers flood (ITKWatersheds). Removed with the
# --- coexistence layer at ITK removal. The shipped ImageProcessing plugin remains ITK-free.
find_package(ITK COMPONENTS ITKWatersheds ITKCommon ITKMathematicalMorphology ITKConnectedComponents REQUIRED)
target_link_libraries(${PLUGIN_NAME}UnitTest PRIVATE ${ITK_LIBRARIES})
if(MSVC)
  target_compile_options(${PLUGIN_NAME}UnitTest PRIVATE /bigobj)
  target_compile_definitions(${PLUGIN_NAME}UnitTest PRIVATE _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
endif()

# Register each real-world pipeline as an independent nxrunner test.
include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)
set(PREBUILT_PIPELINE_NAMES)
foreach(example_name IN LISTS ${PLUGIN_NAME}_EXAMPLE_PIPELINE_NAMES)
  list(APPEND PREBUILT_PIPELINE_NAMES "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/${example_name}/${example_name}.d3dpipeline")
endforeach()
create_pipeline_tests(PLUGIN_NAME ${PLUGIN_NAME} PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})

if(SIMPLNX_BUILD_PYTHON)
  AddPythonTest(
    NAME "PYTHON::${PLUGIN_NAME}::GenerateAndRunExamples"
    FILE "${${PLUGIN_NAME}_SOURCE_DIR}/test/GenerateAndRunExamplePipelines.py"
    PYTHONPATH "$<TARGET_FILE_DIR:simplnx>"
  )
  set_tests_properties("PYTHON::${PLUGIN_NAME}::GenerateAndRunExamples" PROPERTIES
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:simplnx>"
    RUN_SERIAL TRUE
    LABELS "${PLUGIN_NAME};ExamplePipelines;Python"
  )
endif()

AddPythonTest(
  NAME "PYTHON::${PLUGIN_NAME}::VerifyExampleCitations"
  FILE "${${PLUGIN_NAME}_SOURCE_DIR}/test/VerifyExamplePipelineCitations.py"
  PYTHONPATH "$<TARGET_FILE_DIR:simplnx>"
)
set_tests_properties("PYTHON::${PLUGIN_NAME}::VerifyExampleCitations" PROPERTIES
  RUN_SERIAL TRUE
  LABELS "${PLUGIN_NAME};ExamplePipelines;Citations;Network"
)
