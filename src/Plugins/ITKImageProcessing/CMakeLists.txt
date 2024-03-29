set(ItkComponents
  # ITKClassifiers
  ITKCommon
  ITKConvolution
  ITKDenoising

  # ITKImageCompare
  ITKImageFeature
  ITKImageIntensity

  # ITKImageNoise
  ITKIOBioRad
  ITKIOBMP
  ITKIOGE
  ITKIOGIPL
  ITKIOImageBase
  ITKIOIPL
  ITKIOJPEG
  ITKIOMeta
  ITKIOMRC
  ITKIONIFTI
  ITKIONRRD
  ITKIOPNG
  ITKIOStimulate
  ITKIOTIFF
  ITKIOVTK
  ITKMathematicalMorphology
  ITKOptimizers

  # ITKRegistrationCommon
  # ITKReview
  # ITKTestKernel
  ITKSmoothing
  ITKStatistics
  ITKTransform
  ITKWatersheds
)

if(ITK_VERSION_MAJOR EQUAL 5 AND ITK_VERSION_MINOR EQUAL 2)
  list(APPEND ItkComponents ITKMathematicalMorphology)
endif()

find_package(ITK COMPONENTS ${ItkComponents} REQUIRED)

get_property(GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(NOT GENERATOR_IS_MULTI_CONFIG)
  string(TOUPPER "${CMAKE_BUILD_TYPE}" TYPE_UPPER)
  get_target_property(ITKCOMMON_PATH ITKCommon IMPORTED_LOCATION_${TYPE_UPPER})
  get_filename_component(ITK_LIB_DIR ${ITKCOMMON_PATH} DIRECTORY CACHE)

  get_property(SIMPLNX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS)
  set_property(GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS ${SIMPLNX_EXTRA_LIBRARY_DIRS} ${ITK_LIB_DIR})
endif()

message(STATUS "* ITK (${ITK_VERSION}) ${ITK_DIR}")

include("${simplnx_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ITKImageProcessing")
set(${PLUGIN_NAME}_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

# ------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# ITKImageProcessing/src/ITKImageProcessing/Filters/ directory.
option(ITKIMAGEPROCESSING_LEAN_AND_MEAN "Only compile the Reader and Writers, All other filters are disabled" OFF)

set(FilterList
  ITKDiscreteGaussianImage
  ITKImageReader
  ITKImageWriter
  ITKImportImageStack
  ITKMedianImage
  ITKMhaFileReader
  ITKRescaleIntensityImage
  )
    
set(AlgorithmList
)

if(NOT ITKIMAGEPROCESSING_LEAN_AND_MEAN
   # AND NOT SIMPLNX_CONDA_BUILD
)
  list(APPEND FilterList
    ITKBinaryContourImage
    ITKBinaryOpeningByReconstructionImage
    ITKClosingByReconstructionImage
    ITKGrayscaleFillholeImage
    ITKGrayscaleGrindPeakImage
    ITKHConvexImage
    ITKHMaximaImage
    ITKHMinimaImage
    ITKLabelContourImage
    ITKMorphologicalGradientImage
    ITKMorphologicalWatershedImage
    ITKOpeningByReconstructionImage
    ITKSignedMaurerDistanceMapImage
    ITKValuedRegionalMaximaImage
    ITKValuedRegionalMinimaImage
  )
endif()


# ------------------------------------------------------------------------------
# These are all extra filters that are built if we have enabled all the filters
#
if(NOT ITKIMAGEPROCESSING_LEAN_AND_MEAN)
  list(APPEND FilterList
    ITKAbsImage
    ITKAcosImage
    ITKAdaptiveHistogramEqualizationImage
    ITKApproximateSignedDistanceMapImage
    ITKAsinImage
    ITKAtanImage
    ITKBinaryDilateImage
    ITKBinaryErodeImage
    ITKBinaryMorphologicalClosingImage
    ITKBinaryMorphologicalOpeningImage
    ITKBinaryProjectionImage
    ITKBinaryThinningImage
    ITKBinaryThresholdImage
    ITKBlackTopHatImage
    ITKConnectedComponentImage
    ITKCosImage
    ITKCurvatureAnisotropicDiffusionImage
    ITKCurvatureFlowImage
    ITKDanielssonDistanceMapImage
    ITKDilateObjectMorphologyImage
    ITKDoubleThresholdImage
    ITKErodeObjectMorphologyImage
    ITKExpImage
    ITKExpNegativeImage
    ITKGradientAnisotropicDiffusionImage
    ITKGradientMagnitudeImage
    ITKGradientMagnitudeRecursiveGaussianImage
    ITKGrayscaleDilateImage
    ITKGrayscaleErodeImage
    ITKGrayscaleMorphologicalClosingImage
    ITKGrayscaleMorphologicalOpeningImage
    ITKImportFijiMontageFilter
    ITKIntensityWindowingImage
    ITKInvertIntensityImage
    ITKIsoContourDistanceImage
    ITKLaplacianRecursiveGaussianImage
    ITKLog10Image
    ITKLogImage
    ITKMaskImage
    ITKMinMaxCurvatureFlowImage
    ITKNormalizeImage
    ITKNotImage
    ITKOtsuMultipleThresholdsImage
    ITKRegionalMaximaImage
    ITKRegionalMinimaImage
    ITKRelabelComponentImage
    ITKSigmoidImage
    ITKSignedDanielssonDistanceMapImage
    ITKSinImage
    ITKSqrtImage
    ITKSquareImage
    ITKTanImage
    ITKThresholdImage
    ITKThresholdMaximumConnectedComponentsImage
    ITKWhiteTopHatImage
    ITKZeroCrossingImage

    # -----------------------------------------------------------------------------
    # These filters only work on Scalar inputs
    # The unit test for these filters use RGB images. 
    # -----------------------------------------------------------------------------
    # ITKBilateralImage
    # ITKBinaryClosingByReconstructionImage
    # ITKBinomialBlurImage
    # ITKLaplacianSharpeningImage
    # ITKMaximumProjectionImage
    # ITKMedianProjectionImage
    # ITKMinimumProjectionImage
    # ITKMultiScaleHessianBasedObjectnessImage
    # ITKSaltAndPepperNoiseImage
    # ITKShiftScaleImage
    # ITKShotNoiseImage
    # ITKSpeckleNoiseImage
    # ITKBoxMeanImage

    # -----------------------------------------------------------------------------
    # These filters need to have their type or dimension fixed
    # -----------------------------------------------------------------------------
    # ITKBoundedReciprocalImage
    # ITKNormalizeToConstantImage
    # ITKMeanProjectionImage
    # ITKStandardDeviationProjectionImage
    # ITKSumProjectionImage
    # ITKMorphologicalWatershedFromMarkersImage
    # ITKSmoothingRecursiveGaussianImage

    # -----------------------------------------------------------------------------
    # There are parameters from the old version that do not appear in the JSON when generating.
    # -----------------------------------------------------------------------------
    #
    #

    # -----------------------------------------------------------------------------
    # These filters are custom written filters by BQ and do not conform to the
    # standard ITK filter wrapping. These Filters are not even compiling
    # -----------------------------------------------------------------------------
    #  IlluminationCorrection # MISSING 1 or more Parameter Implementations
    #  ImportAxioVisionV4Montage
    #  AxioVisionV4ToTileConfiguration
    #  ImportZenInfoMontage
    #  ApplyDewarpParameters # MISSING 1 or more Parameter Implementations
    #  CalcDewarpParameters # MISSING 1 or more Parameter Implementations
    #  ImportVectorImageStack # MISSING 1 or more Parameter Implementations
    #  ITKRefineTileCoordinates # MISSING 1 or more Parameter Implementations
    #  ITKImportRoboMetMontage
    #  ITKPCMTileRegistration
    #  ITKProxTVImage
    #  ITKStitchMontage # MISSING 1 or more Parameter Implementations
    #  ITKRGBToLuminanceImage # Multiple Different Return types so auto has issues with that

    # -----------------------------------------------------------------------------
    # These filters have a fixed output type
    # -----------------------------------------------------------------------------
    #  ITKVectorConnectedComponentImage # Vector Input, uint32_t output

    # -----------------------------------------------------------------------------
    # These filters use the itk::InPlaceDream3DDataToImageFilter filter
    # -----------------------------------------------------------------------------
    #  ITKPatchBasedDenoisingImage # itk::InPlaceDream3DDataToImageFilter
    #  ITKFFTNormalizedCorrelationImage # itk::InPlaceDream3DDataToImageFilter
    #  ITKHistogramMatchingImage # itk::InPlaceDream3DDataToImageFilter

    # -----------------------------------------------------------------------------
    # These filters use the Dream3DArraySwitchOutputComponentMacro macro
    # -----------------------------------------------------------------------------
    #  ITKCastImage # Dream3DArraySwitchOutputComponentMacro
    #  ITKVectorRescaleIntensityImage # Dream3DArraySwitchOutputComponentMacro
  )

set(AlgorithmList
    ITKImportFijiMontage
  )
endif()

list(SORT FilterList ORDER ASCENDING)

# -----------------------------------------------------------------------------
# These are filters that need to be compiled but not exposed to a user
# -----------------------------------------------------------------------------

create_simplnx_plugin(NAME ${PLUGIN_NAME}
  FILTER_LIST ${FilterList}
  ALGORITHM_LIST ${AlgorithmList}
  DESCRIPTION "${PLUGIN_NAME} Plugin"
  VERSION "0.1.0"
  DOC_CHECK
  ADD_TO_GLOBAL_LIST
)

target_link_libraries(${PLUGIN_NAME} PUBLIC ${ItkComponents})

if(MSVC)
  target_compile_options(${PLUGIN_NAME}
    PUBLIC
      /bigobj
  )
endif()

set(${PLUGIN_NAME}_Common_Srcs
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKArrayHelper.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKArrayHelper.cpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKProgressObserver.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKDream3DFilterInterruption.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ReadImageUtils.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ReadImageUtils.cpp
)

set(${PLUGIN_NAME}_GENERATED_DIR ${simplnx_BINARY_DIR}/Plugins/${ARGS_PLUGIN_NAME}/generated)
set(${PLUGIN_NAME}_GENERATED_HEADER_DIR ${simplnx_BINARY_DIR}/Plugins/${ARGS_PLUGIN_NAME}/generated/ITKImageProcessing)

set(${PLUGIN_NAME}_GENERATED_CONFIG_HEADER ${${PLUGIN_NAME}_GENERATED_HEADER_DIR}/ITKImageProcessingConfig.hpp)

set(${PLUGIN_NAME}_GENERATED_HEADERS ${${PLUGIN_NAME}_GENERATED_CONFIG_HEADER})

configure_file(${${PLUGIN_NAME}_SOURCE_DIR}/cmake/ITKImageProcessingConfig.hpp.in ${${PLUGIN_NAME}_GENERATED_CONFIG_HEADER})

target_sources(${PLUGIN_NAME}
  PRIVATE
    ${${PLUGIN_NAME}_Common_Srcs}
    ${${PLUGIN_NAME}_GENERATED_HEADERS}
)
source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME} FILES ${${PLUGIN_NAME}_Common_Srcs})
source_group("Generated" FILES ${${PLUGIN_NAME}_GENERATED_HEADERS})

if(WIN32)
  # This removes some C++17 Deprecation Warnings inside of ITK 5.1
  target_compile_definitions(${PLUGIN_NAME} PUBLIC "_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING")
  set_target_properties(${PLUGIN_NAME} PROPERTIES LINK_FLAGS_DEBUG "/INCREMENTAL:NO")
endif()

target_sources(${PLUGIN_NAME}
  PRIVATE
    ${hidden_filter_HDRS}
    ${hidden_filter_SRCS}
)
source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME} FILES ${hidden_filter_HDRS} ${hidden_filter_SRCS})

# ------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
if(SIMPLNX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
endif()

# ------------------------------------------------------------------------------
# Add in the install rules for ITK Libraries
# ------------------------------------------------------------------------------
get_filename_component(ITKImageProcessing_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY CACHE)
get_filename_component(SIMPLNX_PLUGINS_SOURCE_DIR ${ITKImageProcessing_SOURCE_DIR} DIRECTORY CACHE)

# message(STATUS "ITKImageProcessing_SOURCE_DIR: ${ITKImageProcessing_SOURCE_DIR}")
include(${simplnx_SOURCE_DIR}/cmake/ITKInstall.cmake)
AddItkCopyInstallRules(LIBS ${ItkComponents})

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
  set_target_properties( ${PLUGIN_NAME}
    PROPERTIES
      INSTALL_RPATH \$ORIGIN/../lib
  )
  target_link_options(${PLUGIN_NAME} PUBLIC "-Wl,--disable-new-dtags")
  target_compile_options(${PLUGIN_NAME} PUBLIC "-no-pie")
endif()

# -----------------------------------------------------------------------
# Install example pipelines
# -----------------------------------------------------------------------
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
  set(CX_CONFIG_DIR "$<CONFIG>")
else()
  set(CX_CONFIG_DIR ".")
endif()

if(EXISTS "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines")
  set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/pipelines/${PLUGIN_NAME}")
  add_custom_target(Copy_${PLUGIN_NAME}_Pipeline_Folder ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines"
    ${PIPELINE_DEST_DIR}
    COMMENT "Copying Pipeline Folder into Binary Directory"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  set_target_properties(Copy_${PLUGIN_NAME}_Pipeline_Folder PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  option(${PLUGIN_NAME}_INSTALL_PIPELINES "Enables install of ${PLUGIN_NAME} pipelines" ON)

  if(${PLUGIN_NAME}_INSTALL_PIPELINES)
    set(INSTALL_DESTINATION "${SIMPLNX_PIPELINE_INSTALL_DIR}/${PLUGIN_NAME}")
    install(DIRECTORY
      "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/"
      DESTINATION "${INSTALL_DESTINATION}"
      COMPONENT Applications
    )
  endif()
endif()

# -----------------------------------------------------------------------
# Download Example Data Files
# -----------------------------------------------------------------------
include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)
set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data")

file(TO_CMAKE_PATH "${DREAM3D_DATA_DIR}" DREAM3D_DATA_DIR_NORM)

# -----------------------------------------------------------------------
# These data files are pulled from the 'simplnx data archive' on github,
# decompressed and then copied to the build folder. Install rules are also
# generated for each of the data sets.
# -----------------------------------------------------------------------
if(EXISTS "${DREAM3D_DATA_DIR}" AND SIMPLNX_DOWNLOAD_TEST_FILES)
  if(NOT EXISTS ${DREAM3D_DATA_DIR}/TestFiles/)
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
    ARCHIVE_NAME ITKMhaFileReaderTest_v3.tar.gz
    SHA512 106bcf9cfb4b77e6feb9355d7df0d1772e59961961201d1efaf2288294b39b3642e25e88caea322be56bb7d1029cfccc5c7c3844e7bd1f25a0c9f6186a29415b
  )
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
    ARCHIVE_NAME Porosity_Image.tar.gz
    SHA512 950c71b4f4721f14ea532ed5830b0786f49d2855b950bdf867c335b9fb0eb5dc28727b0899d665af0a2bc56816e1e66a08a008ef06a62b71efea4a6e46bff168
    INSTALL
  )
  add_custom_target(Copy_${PLUGIN_NAME}_Porosity_Image ALL
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/Porosity_Image.tar.gz"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DREAM3D_DATA_DIR}/TestFiles/Porosity_Image" "${DATA_DEST_DIR}/Porosity_Image"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/Porosity_Image"
    WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
    COMMENT "Copying ${PLUGIN_NAME}/Porosity_Image data into Binary Directory"
    DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  set_target_properties(Copy_${PLUGIN_NAME}_Porosity_Image PROPERTIES FOLDER Plugins/${PLUGIN_NAME})
endif()

# -----------------------------------------------------------------------
# Create build folder copy rules and install rules for the 'data' folder
# for this plugin.
# CURRENTLY DISABLED AS WE ARE NOT INSTALLING ANYTHING FROM DATA. IT IS
# USED FOR TESTING ONLY AND SERVES NO PURPOSE IN A DISTRIBUTION
# -----------------------------------------------------------------------
if(FALSE)
  add_custom_target(Copy_${PLUGIN_NAME}_Data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${${PLUGIN_NAME}_SOURCE_DIR}/data ${DATA_DEST_DIR}/${PLUGIN_NAME}
    COMMENT "Copying ${PLUGIN_NAME} data into Binary Directory"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  set_target_properties(Copy_${PLUGIN_NAME}_Data PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  set(Installed_Data_Files
  )

  install(FILES
    ${Installed_Data_Files}
    DESTINATION Data/${PLUGIN_NAME}
    COMPONENT Applications
  )
endif()

if(CMAKE_GENERATOR STREQUAL "Ninja")
  set(USING_NINJA ON)
else()
  set(USING_NINJA OFF)
endif()

include(CMakeDependentOption)
cmake_dependent_option(ITKImageProcessing_USE_JOB_POOL "Forces ItkImageProcessing to use a ninja job pool. For memory limited builds." OFF "USING_NINJA" OFF)

if(ITKImageProcessing_USE_JOB_POOL)
  if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR "ITKImageProcessing_USE_JOB_POOL requires Ninja as the generator.")
  endif()

  include(ProcessorCount)
  ProcessorCount(thread_count)

  # Guess job count as half the number of threads
  math(EXPR itk_job_count "${thread_count} / 2")

  set(ITKImageProcessing_JOB_POOL ${itk_job_count} CACHE STRING "Number of threads for ITKImageProcessing job pools")
  mark_as_advanced(FORCE ITKImageProcessing_JOB_POOL)

  message(STATUS "ITKImageProcessing_JOB_POOL: ${ITKImageProcessing_JOB_POOL}")

  set_property(GLOBAL APPEND PROPERTY JOB_POOLS itk_pool=${ITKImageProcessing_JOB_POOL})

  set_target_properties(${PLUGIN_NAME}
    PROPERTIES
      JOB_POOL_COMPILE itk_pool
  )
endif()
