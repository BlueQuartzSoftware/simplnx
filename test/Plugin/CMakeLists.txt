include(GenerateExportHeader)
include(GNUInstallDirs)

PROJECT(TestPlugin
    VERSION 0.0.1
    DESCRIPTION "Test Plugin"
)

set(PLUGIN_GENERATED_DIR ${PROJECT_BINARY_DIR}/generated)
set(PLUGIN_GENERATED_HEADER_DIR ${PROJECT_BINARY_DIR}/generated/test)
set(PLUGIN_EXPORT_HEADER ${PLUGIN_GENERATED_HEADER_DIR}/testplugin_export.hpp)
set(PLUGIN2_EXPORT_HEADER ${PLUGIN_GENERATED_HEADER_DIR}/test2plugin_export.hpp)

# Plugin 1
set(Plugin_HDRS
    ${TestPlugin_SOURCE_DIR}/TestPlugin.hpp
    ${TestPlugin_SOURCE_DIR}/TestFilter.hpp
)
set(Plugin_SRCS
    ${TestPlugin_SOURCE_DIR}/TestPlugin.cpp
    ${TestPlugin_SOURCE_DIR}/TestFilter.cpp
)

add_library(TestPlugin SHARED)
set_target_properties(TestPlugin PROPERTIES
    FOLDER "Tests/Plugins"
    SUFFIX ".complex"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/test/Plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/test/Plugins
)
target_link_libraries(TestPlugin PUBLIC complex)

generate_export_header(TestPlugin
  EXPORT_FILE_NAME ${PLUGIN_EXPORT_HEADER}
)
set(TESTPLUGIN_GENERATED_HEADERS
  ${PLUGIN_EXPORT_HEADER}
)
set(TESTPLUGIN_ALL_HDRS
  ${Plugin_HDRS}
  ${TESTPLUGIN_GENERATED_HEADERS}
)

target_sources(TestPlugin
  PUBLIC
    ${TESTPLUGIN_ALL_HDRS}
  PRIVATE
    ${Plugin_SRCS}
)

target_include_directories(TestPlugin
    PUBLIC
    $<BUILD_INTERFACE:${TestPlugin_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${PLUGIN_GENERATED_DIR}>
    $<BUILD_INTERFACE:${COMPLEX_BINARY_DIR}/test/Plugin>
)

install(TARGETS TestPlugin
    PUBLIC_HEADER DESTINATION include/TestPlugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT   TestPlugin_Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   TestPlugin_Runtime
        NAMELINK_COMPONENT TestPlugin_Development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   TestPlugin_Development
)

# Plugin 2
set(Plugin2_HDRS
    ${TestPlugin_SOURCE_DIR}/Test2Plugin.hpp
    ${TestPlugin_SOURCE_DIR}/Test2Filter.hpp
)
set(Plugin2_SRCS
    ${TestPlugin_SOURCE_DIR}/Test2Plugin.cpp
    ${TestPlugin_SOURCE_DIR}/Test2Filter.cpp
)

add_library(Test2Plugin SHARED)
set_target_properties(Test2Plugin PROPERTIES
    FOLDER "Tests/Plugins"
    SUFFIX ".complex"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/test/Plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/test/Plugins
)
target_link_libraries(Test2Plugin PUBLIC complex)

generate_export_header(Test2Plugin
  EXPORT_FILE_NAME ${PLUGIN2_EXPORT_HEADER}
)
set(TEST2PLUGIN_GENERATED_HEADERS
  ${PLUGIN2_EXPORT_HEADER}
)
set(TEST2PLUGIN_ALL_HDRS
  ${Plugin2_HDRS}
  ${TEST2PLUGIN_GENERATED_HEADERS}
)

target_sources(Test2Plugin
  PUBLIC
    ${TEST2PLUGIN_ALL_HDRS}
  PRIVATE
    ${Plugin2_SRCS}
)

target_include_directories(Test2Plugin
    PUBLIC
    $<BUILD_INTERFACE:${TestPlugin_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${PLUGIN_GENERATED_DIR}>
    $<BUILD_INTERFACE:${COMPLEX_BINARY_DIR}/tests/Plugin>
)

install(TARGETS Test2Plugin
    PUBLIC_HEADER DESTINATION include/Test2Plugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT   Test2Plugin_Runtime
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   Test2Plugin_Runtime
        NAMELINK_COMPONENT Test2Plugin_Development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   Test2Plugin_Development
)