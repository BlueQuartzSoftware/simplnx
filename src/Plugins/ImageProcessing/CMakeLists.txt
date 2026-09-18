include("${simplnx_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ImageProcessing")
set(${PLUGIN_NAME}_SOURCE_DIR ${simplnx_SOURCE_DIR}/src/Plugins/${PLUGIN_NAME})

# All filters in the plugin. Keep them in ImageProcessing/src/ImageProcessing/Filters/.
set(FilterList
  AbsImageFilter
  AcosImageFilter
  AdaptiveHistogramEqualizationImageFilter
  ApproximateSignedDistanceMapImageFilter
  AsinImageFilter
  AtanImageFilter
  BinaryContourImageFilter
  BinaryDilateImageFilter
  BinaryErodeImageFilter
  BinaryMorphologicalClosingImageFilter
  BinaryMorphologicalOpeningImageFilter
  BinaryOpeningByReconstructionImageFilter
  BinaryProjectionImageFilter
  BinaryThinningImageFilter
  BinaryThresholdImageFilter
  BlackTopHatImageFilter
  BoundedReciprocalImageFilter
  ClosingByReconstructionImageFilter
  ConnectedComponentImageFilter
  CosImageFilter
  CurvatureAnisotropicDiffusionImageFilter
  CurvatureFlowImageFilter
  DanielssonDistanceMapImageFilter
  DilateObjectMorphologyImageFilter
  DiscreteGaussianImageFilter
  DoubleThresholdImageFilter
  ErodeObjectMorphologyImageFilter
  ExpImageFilter
  ExpNegativeImageFilter
  GradientAnisotropicDiffusionImageFilter
  GradientMagnitudeImageFilter
  GradientMagnitudeRecursiveGaussianImageFilter
  GrayscaleDilateImageFilter
  GrayscaleErodeImageFilter
  GrayscaleFillholeImageFilter
  GrayscaleGrindPeakImageFilter
  GrayscaleMorphologicalClosingImageFilter
  GrayscaleMorphologicalOpeningImageFilter
  HConvexImageFilter
  HMaximaImageFilter
  HMinimaImageFilter
  ImportFijiMontageFilter
  IntensityWindowingImageFilter
  InvertIntensityImageFilter
  IsoContourDistanceImageFilter
  LabelContourImageFilter
  LaplacianRecursiveGaussianImageFilter
  Log10ImageFilter
  LogImageFilter
  MaskImageFilter
  MaximumProjectionImageFilter
  MeanProjectionImageFilter
  MedianImageFilter
  MedianProjectionImageFilter
  MinimumProjectionImageFilter
  MinMaxCurvatureFlowImageFilter
  MorphologicalGradientImageFilter
  MorphologicalWatershedFromMarkersImageFilter
  MorphologicalWatershedImageFilter
  NormalizeImageFilter
  NormalizeToConstantImageFilter
  NotImageFilter
  OpeningByReconstructionImageFilter
  OtsuMultipleThresholdsImageFilter
  RegionalMaximaImageFilter
  RegionalMinimaImageFilter
  ReadBinaryCTNorthstarFilter
  ReadImageFilter
  ReadImageStackFilter
  ReadMhaFileFilter
  ReadNIfTIFileFilter
  ReadVolumeGraphicsFileFilter
  ReadZeissTxmFileFilter
  RelabelComponentImageFilter
  RescaleIntensityImageFilter
  SigmoidImageFilter
  SignedDanielssonDistanceMapImageFilter
  SignedMaurerDistanceMapImageFilter
  SinImageFilter
  SmoothingRecursiveGaussianImageFilter
  SqrtImageFilter
  SquareImageFilter
  StandardDeviationProjectionImageFilter
  SumProjectionImageFilter
  TanImageFilter
  ThresholdImageFilter
  ThresholdMaximumConnectedComponentsImageFilter
  ValuedRegionalMaximaImageFilter
  ValuedRegionalMinimaImageFilter
  WhiteTopHatImageFilter
  WriteImageFilter
  ZeroCrossingImageFilter
)

set(AlgorithmList
  ReadBinaryCTNorthstar
  ReadImage
  ReadImageStack
  ReadMhaFile
  ReadNIfTIFile
  ReadNrrdFile
  ReadVolumeGraphicsFile
  ReadZeissTxmFile
  WriteImage
  WriteImageDirect
  WriteImageScanline
)

create_simplnx_plugin(NAME ${PLUGIN_NAME}
                      FILTER_LIST ${FilterList}
                      ALGORITHM_LIST ${AlgorithmList}
                      DESCRIPTION "ITK-free image processing filters"
                      VERSION "0.1.0"
                      DOC_CHECK
                      ADD_TO_GLOBAL_LIST
)

find_package(ZLIB REQUIRED)
target_link_libraries(${PLUGIN_NAME} PRIVATE ZLIB::ZLIB)

set(PLUGIN_EXTRA_SOURCES
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/FijiMontageUtilities.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/FijiMontageUtilities.cpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/MetaImageUtilities.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/MetaImageUtilities.cpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/NrrdUtilities.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/NrrdUtilities.cpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/NiftiUtilities.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/NiftiUtilities.cpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/nifti1.h"
)

set(OLESS_SOURCES
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/oless_common.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/oless.cpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/oless.h"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/pole.cpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/pole.h"
)

target_sources(${PLUGIN_NAME} PRIVATE ${PLUGIN_EXTRA_SOURCES} ${OLESS_SOURCES})
target_include_directories(${PLUGIN_NAME} PRIVATE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless")
source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME} FILES ${PLUGIN_EXTRA_SOURCES})
source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME} FILES ${OLESS_SOURCES})

set(${PLUGIN_NAME}_EXAMPLE_PIPELINE_NAMES
  "(05) AM XCT Porosity Segmentation"
  "(06) AM Powder-Bed Layer Inspection"
  "(07) AM Melt-Pool and Track Inspection"
  "(08) Powder Particle Watershed Segmentation"
  "(09) Microstructure Watershed Segmentation"
  "(10) Binary Mask Repair and Skeletonization"
  "(11) Grayscale Surface-Defect Morphology"
  "(12) Pore Distance and Wall-Thickness Metrology"
  "(13) Denoising and Edge Detection"
  "(14) Projection-Based Quality Summaries"
  "(15) Radiography Intensity Calibration"
  "(16) Phase and Angle Field Transforms"
  "(17) Scientific Volume Interoperability"
  "(18) Industrial XCT Format Import"
  "(19) Serial-Section Stack Reconstruction"
  "(20) Fiji Microscopy Montage Import"
)

set(${PLUGIN_NAME}_EXAMPLE_PIPELINE_FILES "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/PythonGeneration.yaml")
foreach(example_name IN LISTS ${PLUGIN_NAME}_EXAMPLE_PIPELINE_NAMES)
  list(APPEND ${PLUGIN_NAME}_EXAMPLE_PIPELINE_FILES
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/${example_name}/${example_name}.d3dpipeline"
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/${example_name}/${example_name}.yaml"
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/${example_name}/${example_name}.md"
  )
endforeach()

if(SIMPLNX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
endif()

# Standalone developer benchmark tool, plus tiny registered smoke tests for its custom input preparations. Default ON
# locally. It reuses the golden-test harness and generated test-dirs header, so it is added after the test subdir.
option(SIMPLNX_BUILD_IMAGEPROCESSING_BENCHMARK "Build the ImageProcessing benchmark tool" ON)
if(SIMPLNX_BUILD_IMAGEPROCESSING_BENCHMARK)
  add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/benchmark)
endif()

# -----------------------------------------------------------------------
# Copy and install the real-world example pipelines.
# -----------------------------------------------------------------------
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
  set(CX_CONFIG_DIR "$<CONFIG>")
else()
  set(CX_CONFIG_DIR ".")
endif()

if(EXISTS "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines")
  set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/pipelines/Image_Processing")
  set(OBSOLETE_EXAMPLE_PYTHON_FILES)
  set(OBSOLETE_EXAMPLE_DIRECTORIES)
  foreach(example_name IN LISTS ${PLUGIN_NAME}_EXAMPLE_PIPELINE_NAMES)
    list(APPEND OBSOLETE_EXAMPLE_PYTHON_FILES "${PIPELINE_DEST_DIR}/${example_name}.py")
    list(APPEND OBSOLETE_EXAMPLE_DIRECTORIES "${PIPELINE_DEST_DIR}/${example_name}")
  endforeach()
  add_custom_target(Copy_${PLUGIN_NAME}_Pipeline_Folder ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PIPELINE_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -f ${OBSOLETE_EXAMPLE_PYTHON_FILES}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${OBSOLETE_EXAMPLE_DIRECTORIES}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${${PLUGIN_NAME}_EXAMPLE_PIPELINE_FILES} "${PIPELINE_DEST_DIR}"
    COMMENT "Copying ${PLUGIN_NAME} example pipelines into the runtime directory"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  set_target_properties(Copy_${PLUGIN_NAME}_Pipeline_Folder PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  option(${PLUGIN_NAME}_INSTALL_PIPELINES "Enables install of ${PLUGIN_NAME} pipelines" ON)
  if(${PLUGIN_NAME}_INSTALL_PIPELINES)
    install(FILES
      ${${PLUGIN_NAME}_EXAMPLE_PIPELINE_FILES}
      DESTINATION "${SIMPLNX_PIPELINE_INSTALL_DIR}/Image_Processing"
      COMPONENT Applications
    )
  endif()
endif()

# -----------------------------------------------------------------------
# Download, copy, and install the real-world example inputs.
# -----------------------------------------------------------------------
include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)
set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data")

if(EXISTS "${DREAM3D_DATA_DIR}" AND SIMPLNX_DOWNLOAD_TEST_FILES)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
    ARCHIVE_NAME ImageProcessing_Examples_v1.tar.gz
    SHA512 9bd8174a6c00041b042cd2482ad7a1d2e5df9ffb567a180c93fad34a91b1815c544c3b37481b66f1ffc8f7ad58b7bcbf2446df037d9a5933f534b2761b3df766
  )

  add_custom_target(Copy_${PLUGIN_NAME}_Example_Data ALL
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/ImageProcessing_Examples_v1.tar.gz"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DREAM3D_DATA_DIR}/TestFiles/ImageProcessing_Examples" "${DATA_DEST_DIR}/ImageProcessing_Examples"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/ImageProcessing_Examples"
    WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
    COMMENT "Copying ${PLUGIN_NAME} real-world example data into the runtime directory"
    DEPENDS Fetch_Remote_Data_Files
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  set_target_properties(Copy_${PLUGIN_NAME}_Example_Data PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  install(DIRECTORY
    "${DATA_DEST_DIR}/ImageProcessing_Examples"
    DESTINATION Data
    COMPONENT Applications
  )
endif()
