
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
  ITKReview
  ITKSmoothing
  ITKStatistics
  ITKTestKernel
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

  get_property(COMPLEX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS)
  set_property(GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS ${COMPLEX_EXTRA_LIBRARY_DIRS} ${ITK_LIB_DIR})
endif()

include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ITKImageProcessing")
set(${PLUGIN_NAME}_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

# ------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# ITKImageProcessing/src/ITKImageProcessing/Filters/ directory.
option(ITKImageProcessing_LeanAndMean "Only compile the Reader and Writers, All other filters are disabled" OFF)


set(FilterList
    ITKImageReader
    ITKImageWriter
    ITKImportImageStack
    ITKMedianImage
    ITKMhaFileReader
    )

# ------------------------------------------------------------------------------
# These are all extra filters that are built if we have enabled all the filters
#
if(NOT ITKImageProcessing_LeanAndMean)
  set(FilterList
      ${FilterList}
    ITKAbsImage
    ITKAcosImage
    ITKAdaptiveHistogramEqualizationImage
    ITKAsinImage
    ITKAtanImage
    ITKBinaryContourImage
    ITKBinaryMorphologicalClosingImage
    ITKBinaryThresholdImage
    ITKClosingByReconstructionImage
    ITKCosImage
    ITKGradientMagnitudeImage
    ITKGrayscaleFillholeImage
    ITKInvertIntensityImage
    ITKLog10Image
    ITKLogImage
    ITKMaskImage
    ITKMorphologicalWatershedImage
    ITKNormalizeImage
    ITKOpeningByReconstructionImage
    ITKOtsuMultipleThresholdsImage
    ITKSignedMaurerDistanceMapImage
    ITKSinImage
    ITKSqrtImage
    ITKTanImage


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
    # These filters have a fixed output type
    # -----------------------------------------------------------------------------
    # ITKApproximateSignedDistanceMapImage
    # ITKMinMaxCurvatureFlowImage
    # ITKGradientAnisotropicDiffusionImage
    # ITKCurvatureAnisotropicDiffusionImage
    # ITKBoundedReciprocalImage
    # ITKConnectedComponentImage
    # ITKCurvatureFlowImage
    # ITKDanielssonDistanceMapImage
    # ITKDoubleThresholdImage
    # ITKGradientMagnitudeRecursiveGaussianImage
    # ITKIsoContourDistanceImage
    # ITKLaplacianRecursiveGaussianImage
    # ITKMeanProjectionImage
    # ITKNormalizeToConstantImage
    # ITKRegionalMaximaImage
    # ITKRegionalMinimaImage
    # ITKStandardDeviationProjectionImage
    # ITKSumProjectionImage
    # ITKThresholdMaximumConnectedComponentsImage
    # ITKZeroCrossingImage
    # ITKMorphologicalWatershedFromMarkersImage
    # ITKSignedDanielssonDistanceMapImage
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
    #  ITKImportFijiMontage
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
endif()

# -----------------------------------------------------------------------------
# These are filters that need to be compiled but not exposed to a user
# -----------------------------------------------------------------------------

create_complex_plugin(NAME ${PLUGIN_NAME}
  FILTER_LIST ${FilterList}
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

set (${PLUGIN_NAME}_Common_Srcs
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKArrayHelper.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKArrayHelper.cpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKProgressObserver.hpp
  ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Common/ITKDream3DFilterInterruption.hpp
)

target_sources(${PLUGIN_NAME}
  PRIVATE
  ${${PLUGIN_NAME}_Common_Srcs}
)
source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME} FILES ${${PLUGIN_NAME}_Common_Srcs})


target_include_directories(${PLUGIN_NAME} PUBLIC ${ITK_INCLUDE_DIRS})

if(WIN32)
  # This removes some C++17 Deprecation Warnings inside of ITK 5.1
  target_compile_definitions(${PLUGIN_NAME} PUBLIC "_SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING")
  set_target_properties(${PLUGIN_NAME} PROPERTIES LINK_FLAGS_DEBUG "/INCREMENTAL:NO")
endif()

target_sources(${PLUGIN_NAME}
  PRIVATE ${hidden_filter_HDRS} ${hidden_filter_SRCS}
)
source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME} FILES ${hidden_filter_HDRS} ${hidden_filter_SRCS})

# ------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
enable_testing()
add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)

# ------------------------------------------------------------------------------
# Add in the install rules for ITK Libraries
# ------------------------------------------------------------------------------
get_filename_component(ITKImageProcessing_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY CACHE)
get_filename_component(COMPLEX_PLUGINS_SOURCE_DIR ${ITKImageProcessing_SOURCE_DIR} DIRECTORY CACHE)

# message(STATUS "ITKImageProcessing_SOURCE_DIR: ${ITKImageProcessing_SOURCE_DIR}")
include(${complex_SOURCE_DIR}/cmake/ITKInstall.cmake)
AddItkCopyInstallRules(LIBS ${ItkComponents})

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
  set_target_properties( ${PLUGIN_NAME}
          PROPERTIES
          INSTALL_RPATH \$ORIGIN/../lib)
  target_link_options(${PLUGIN_NAME} PUBLIC "-Wl,--disable-new-dtags")
  target_compile_options(${PLUGIN_NAME} PUBLIC "-no-pie")
endif()


# -----------------------------------------------------------------------
# Install example pipelines
# -----------------------------------------------------------------------
if(EXISTS "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines")
  set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/pipelines/${PLUGIN_NAME}")
  add_custom_target(${PLUGIN_NAME}PipelineFolderCopy ALL
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines"
                    ${PIPELINE_DEST_DIR}
                    COMMENT "Copying Pipeline Folder into Binary Directory")
  set_target_properties(${PLUGIN_NAME}PipelineFolderCopy PROPERTIES FOLDER ZZ_COPY_FILES)

  set(INSTALL_DESTINATION "pipelines/${PLUGIN_NAME}")
  install(DIRECTORY
          "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/"
          DESTINATION "${INSTALL_DESTINATION}"
          COMPONENT Applications
        )
endif()

# -----------------------------------------------------------------------
# Download Example Data Files
# -----------------------------------------------------------------------
include(${complex_SOURCE_DIR}/cmake/Utility.cmake)
set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/Data")

file(TO_CMAKE_PATH "${DREAM3D_DATA_DIR}" DREAM3D_DATA_DIR_NORM)

# -----------------------------------------------------------------------
# These data files are pulled from the 'complex data archive' on github,
# decompressed and then copied to the build folder. Install rules are also
# generated for each of the data sets.
# -----------------------------------------------------------------------
if(EXISTS "${DREAM3D_DATA_DIR}" AND COMPLEX_DOWNLOAD_TEST_FILES) 
  if(NOT EXISTS ${DREAM3D_DATA_DIR}/TestFiles/)
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
                   ARCHIVE_NAME Porosity_Image.tar.gz
                   SHA512 950c71b4f4721f14ea532ed5830b0786f49d2855b950bdf867c335b9fb0eb5dc28727b0899d665af0a2bc56816e1e66a08a008ef06a62b71efea4a6e46bff168
                   INSTALL
                   COPY_DATA)

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
                   ARCHIVE_NAME ITKMhaFileReaderTest.tar.gz
                   SHA512 7b44c28dad8dc70ad79f9dc6247ab2a17e3c0e08cf0400e361acaea965b0881eaa3815af21c1ec9bdba19f70c0bdd67b7c4382e527c844a20da39d6a229dc5fa
                   INSTALL
                   COPY_DATA)
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
    )
  set_target_properties(Copy_${PLUGIN_NAME}_Data PROPERTIES FOLDER ZZ_COPY_FILES)


  set(Installed_Data_Files
  )

  install(FILES 
    ${Installed_Data_Files}
    DESTINATION Data/${PLUGIN_NAME}
    COMPONENT Applications
  )


endif()
