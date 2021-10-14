include(GenerateExportHeader)
include(GNUInstallDirs)

set(COMPLEX_PLUGIN_NAME "Generic")

PROJECT(${COMPLEX_PLUGIN_NAME}
    VERSION 0.0.1
    DESCRIPTION "Generic"
)

set(PLUGIN_GENERATED_DIR ${PROJECT_BINARY_DIR}/generated)
set(PLUGIN_GENERATED_HEADER_DIR ${PROJECT_BINARY_DIR}/generated/${COMPLEX_PLUGIN_NAME})
set(PLUGIN_EXPORT_HEADER ${PLUGIN_GENERATED_HEADER_DIR}/${COMPLEX_PLUGIN_NAME}_export.hpp)

set(FilterList
  FindBoundaryCells
  FindBoundingBoxFeatures
  FindFeatureCentroids
  FindFeaturePhases
  FindFeaturePhasesBinary
  FindSurfaceFeatures
  GenerateVectorColors

)

list(LENGTH FilterList PluginNumFilters)
set_property(GLOBAL PROPERTY ${COMPLEX_PLUGIN_NAME}_filter_count ${PluginNumFilters})

#------------------------------------------------------------------------------
# Plugin Headers
set(${COMPLEX_PLUGIN_NAME}_Plugin_HDRS
    ${${COMPLEX_PLUGIN_NAME}_SOURCE_DIR}/${COMPLEX_PLUGIN_NAME}Plugin.hpp
)

#------------------------------------------------------------------------------
# Plugin Headers
set(${COMPLEX_PLUGIN_NAME}_Plugin_SRCS
    ${${COMPLEX_PLUGIN_NAME}_SOURCE_DIR}/${COMPLEX_PLUGIN_NAME}Plugin.cpp
)

set(Filter_Registration_Include_String "")
set(Filter_Registration_Code "")
# Add Plugin Filters
foreach(filter ${FilterList})
  list(APPEND ${COMPLEX_PLUGIN_NAME}_Plugin_HDRS
    "${${COMPLEX_PLUGIN_NAME}_SOURCE_DIR}/src/${COMPLEX_PLUGIN_NAME}/Filters/${filter}.hpp"
  )
  list(APPEND ${COMPLEX_PLUGIN_NAME}_Plugin_SRCS
    "${${COMPLEX_PLUGIN_NAME}_SOURCE_DIR}/src/${COMPLEX_PLUGIN_NAME}/Filters/${filter}.cpp"
  )

  string(APPEND Filter_Registration_Include_String
    "#include \"${COMPLEX_PLUGIN_NAME}/Filters/${filter}.hpp\"\n")

  string(APPEND Filter_Registration_Code
    "  addFilter([]() -> IFilter::UniquePointer { return std::make_unique<${filter}>(); });\n")

endforeach()

configure_file( ${complex_SOURCE_DIR}/src/Plugins/Templates/plugin_filter_registration.h.in
                ${PLUGIN_GENERATED_HEADER_DIR}/plugin_filter_registration.h)

add_library(${COMPLEX_PLUGIN_NAME} SHARED)
add_library(complex::${COMPLEX_PLUGIN_NAME} ALIAS ${COMPLEX_PLUGIN_NAME})

set_target_properties(${COMPLEX_PLUGIN_NAME} PROPERTIES
    FOLDER "Plugins/${COMPLEX_PLUGIN_NAME}"
    SUFFIX ".complex"
    LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>/Plugins
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>/Plugins
)

#------------------------------------------------------------------------------
# Where are the plugins going to be placed during the build
get_property(COMPLEX_PLUGIN_OUTPUT_DIR GLOBAL PROPERTY COMPLEX_PLUGIN_OUTPUT_DIR)
if(NOT "${COMPLEX_PLUGIN_OUTPUT_DIR}" STREQUAL "")
  set_target_properties(${COMPLEX_PLUGIN_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${COMPLEX_PLUGIN_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${COMPLEX_PLUGIN_OUTPUT_DIR}
  )
endif()

target_link_libraries(${COMPLEX_PLUGIN_NAME} PUBLIC complex)

generate_export_header(${COMPLEX_PLUGIN_NAME}
  EXPORT_FILE_NAME ${PLUGIN_EXPORT_HEADER}
)
set(${COMPLEX_PLUGIN_NAME}_GENERATED_HEADERS
  ${PLUGIN_EXPORT_HEADER}
)
set(${COMPLEX_PLUGIN_NAME}_ALL_HDRS
  ${${COMPLEX_PLUGIN_NAME}_Plugin_HDRS}
  ${${COMPLEX_PLUGIN_NAME}_GENERATED_HEADERS}
)

target_sources(${COMPLEX_PLUGIN_NAME}
  PUBLIC
    ${${COMPLEX_PLUGIN_NAME}_ALL_HDRS}
  PRIVATE
    ${${COMPLEX_PLUGIN_NAME}_Plugin_SRCS}
)

target_include_directories(${COMPLEX_PLUGIN_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${${COMPLEX_PLUGIN_NAME}_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PLUGIN_GENERATED_DIR}>
    $<BUILD_INTERFACE:${COMPLEX_BINARY_DIR}/Plugins/${COMPLEX_PLUGIN_NAME}>
)

install(TARGETS ${COMPLEX_PLUGIN_NAME}
    PUBLIC_HEADER DESTINATION include/${COMPLEX_PLUGIN_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT   ${COMPLEX_PLUGIN_NAME}_Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   ${COMPLEX_PLUGIN_NAME}_Runtime
        NAMELINK_COMPONENT ${COMPLEX_PLUGIN_NAME}_Development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   ${COMPLEX_PLUGIN_NAME}_Development
)
