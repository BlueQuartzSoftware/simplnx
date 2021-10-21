include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ITKImageProcessing")
#------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# ITKImageProcessing/src/ITKImageProcessing/Filters/ directory.
set(FilterList
  #ApplyDewarpParameters # MISSING 1 or more Parameter Implementations
  AxioVisionV4ToTileConfiguration
  #CalcDewarpParameters # MISSING 1 or more Parameter Implementations
  ITKDiscreteGaussianImage
  ITKImageReader
  ITKImageWriter
  #ITKImportFijiMontage # MISSING 1 or more Parameter Implementations
  ITKImportImageStack
  #ITKImportRoboMetMontage # MISSING 1 or more Parameter Implementations
  ITKMedianImage
  ITKPCMTileRegistration
  ITKProxTVImage
  #ITKRefineTileCoordinates # MISSING 1 or more Parameter Implementations
  #ITKStitchMontage # MISSING 1 or more Parameter Implementations
  #IlluminationCorrection # MISSING 1 or more Parameter Implementations
  #ImportAxioVisionV4Montage # MISSING 1 or more Parameter Implementations
  #ImportVectorImageStack # MISSING 1 or more Parameter Implementations
  #ImportZenInfoMontage # MISSING 1 or more Parameter Implementations

)

create_complex_plugin(NAME ${PLUGIN_NAME}
                      FILTER_LIST ${FilterList}
                      DESCRIPTION "Plugin for ${PLUGIN_NAME}"
                      VERSION "0.1.0"
)

#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, there is where
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