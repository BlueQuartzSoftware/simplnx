include("${simplnx_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ImageProcessing")
set(${PLUGIN_NAME}_SOURCE_DIR ${simplnx_SOURCE_DIR}/src/Plugins/${PLUGIN_NAME})

# ------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# ITKImageProcessing/src/ITKImageProcessing/Filters/ directory.
option(ImageProcessing_LEAN_AND_MEAN "Only compile the Reader and Writers, All other filters are disabled" OFF)
set(FilterList
  BinaryContourImageFilter
  BinaryOpeningByReconstructionImageFilter
  ClosingByReconstructionImageFilter
  GrayscaleFillholeImageFilter
  GrayscaleGrindPeakImageFilter
  HConvexImageFilter
  HMaximaImageFilter
  HMinimaImageFilter
  LabelContourImageFilter
  MorphologicalGradientImageFilter
  MorphologicalWatershedImageFilter
  OpeningByReconstructionImageFilter
  SignedMaurerDistanceMapImageFilter
  ValuedRegionalMaximaImageFilter
  ValuedRegionalMinimaImageFilter
  # ------------------------------------
  # Always have the readers and writers
  ImportFijiMontageFilter
  ReadBinaryCTNorthstarFilter
  ReadImageFilter
  ReadImageStackFilter
  ReadMhaFileFilter
  ReadNIfTIFileFilter
  ReadVolumeGraphicsFileFilter
  ReadZeissTxmFileFilter
  WriteImageFilter
)

if(NOT ImageProcessing_LEAN_AND_MEAN)
  list(APPEND FilterList
    AbsImageFilter
    AcosImageFilter
    AdaptiveHistogramEqualizationImageFilter
    ApproximateSignedDistanceMapImageFilter
    AsinImageFilter
    AtanImageFilter
    BinaryDilateImageFilter
    BinaryErodeImageFilter
    BinaryMorphologicalClosingImageFilter
    BinaryMorphologicalOpeningImageFilter
    BinaryProjectionImageFilter
    BinaryThinningImageFilter
    BinaryThresholdImageFilter
    BlackTopHatImageFilter
    BoundedReciprocalImageFilter
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
    GrayscaleMorphologicalClosingImageFilter
    GrayscaleMorphologicalOpeningImageFilter
    IntensityWindowingImageFilter
    InvertIntensityImageFilter
    IsoContourDistanceImageFilter
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
    MorphologicalWatershedFromMarkersImageFilter
    NormalizeImageFilter
    NormalizeToConstantImageFilter
    NotImageFilter
    OtsuMultipleThresholdsImageFilter
    RegionalMaximaImageFilter
    RegionalMinimaImageFilter
    RelabelComponentImageFilter
    RescaleIntensityImageFilter
    SigmoidImageFilter
    SignedDanielssonDistanceMapImageFilter
    SinImageFilter
    SmoothingRecursiveGaussianImageFilter
    SqrtImageFilter
    SquareImageFilter
    StandardDeviationProjectionImageFilter
    SumProjectionImageFilter
    TanImageFilter
    ThresholdImageFilter
    ThresholdMaximumConnectedComponentsImageFilter
    WhiteTopHatImageFilter
    ZeroCrossingImageFilter
  )
endif()

list(LENGTH FilterList image_proc_filter_count)

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
if(MSVC)
  target_compile_options(${PLUGIN_NAME} PRIVATE /bigobj)
endif()

if(ImageProcessing_LEAN_AND_MEAN)
  target_compile_definitions(${PLUGIN_NAME} PRIVATE "-DImageProcessing_LEAN_AND_MEAN")
endif()

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

# The plugin inherited these pipelines from the removed ImageProcessing plugin. They are flat
# .d3dpipeline files, so they have no .yaml or .md companion yet. List them directly until someone
# writes the companions and moves them into the directory layout above.
list(APPEND ${PLUGIN_NAME}_EXAMPLE_PIPELINE_FILES
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(02) Image Segmentation.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(03) Porosity Mesh Export.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/(04) Porosity Analysis.d3dpipeline"
)

if(SIMPLNX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)

  # Standalone developer benchmark tool, plus tiny registered smoke tests for its custom input preparations. Default ON
  # locally. It reuses the golden-test harness and generated test-dirs header, so it is added after the test subdir.
  option(SIMPLNX_BUILD_IMAGEPROCESSING_BENCHMARK "Build the ImageProcessing benchmark tool" ON)
  if(SIMPLNX_BUILD_IMAGEPROCESSING_BENCHMARK AND NOT ImageProcessing_LEAN_AND_MEAN)
    add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/benchmark)
  endif()

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
  set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/pipelines/${PLUGIN_NAME}")
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
      DESTINATION "${SIMPLNX_PIPELINE_INSTALL_DIR}/${PLUGIN_NAME}"
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
    ARCHIVE_NAME Porosity_Image.tar.gz
    SHA512 950c71b4f4721f14ea532ed5830b0786f49d2855b950bdf867c335b9fb0eb5dc28727b0899d665af0a2bc56816e1e66a08a008ef06a62b71efea4a6e46bff168
    INSTALL
    COPY_DATA
  )
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


include(CMakeDependentOption)
cmake_dependent_option(ImageProcessing_USE_JOB_POOL "Forces ImageProcessing to use a ninja job pool. For memory limited builds." OFF "USING_NINJA" OFF)

if(ImageProcessing_USE_JOB_POOL)
  if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR "ImageProcessing_USE_JOB_POOL requires Ninja as the generator.")
  endif()

  include(ProcessorCount)
  ProcessorCount(thread_count)

  # Guess job count as half the number of threads
  math(EXPR ip_job_count "${thread_count} / 2")

  set(ImageProcessing_JOB_POOL ${ip_job_count} CACHE STRING "Number of threads for ImageProcessing job pools")
  mark_as_advanced(FORCE ImageProcessing_JOB_POOL)

  message(STATUS "ImageProcessing_JOB_POOL: ${ImageProcessing_JOB_POOL}")

  set_property(GLOBAL APPEND PROPERTY JOB_POOLS ip_pool=${ImageProcessing_JOB_POOL})

  set_target_properties(${PLUGIN_NAME}
    PROPERTIES
      JOB_POOL_COMPILE ip_pool
  )
endif()
