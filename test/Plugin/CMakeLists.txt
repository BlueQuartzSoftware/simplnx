include(GenerateExportHeader)
include(GNUInstallDirs)

PROJECT(TestPlugin
    VERSION 0.0.1
    DESCRIPTION "Test Plugin"
)

# Plugin 1
set(Plugin_HDRS
    ${TestPlugin_SOURCE_DIR}/TestPlugin.hpp
    ${TestPlugin_SOURCE_DIR}/TestFilter.hpp
)
set(Plugin_SRCS
    ${TestPlugin_SOURCE_DIR}/TestPlugin.cpp
    ${TestPlugin_SOURCE_DIR}/TestFilter.cpp
)

add_library(TestPlugin SHARED ${Plugin_HDRS} ${Plugin_SRCS})
set_target_properties(TestPlugin PROPERTIES
    FOLDER "Tests/Plugins"
)
target_link_libraries(TestPlugin PUBLIC complex)
generate_export_header(TestPlugin)
target_include_directories(TestPlugin
    PUBLIC
    $<BUILD_INTERFACE:${TestPlugin_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/tests/Plugin>
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

add_library(Test2Plugin SHARED ${Plugin2_HDRS} ${Plugin2_SRCS})
set_target_properties(Test2Plugin PROPERTIES
    FOLDER "Tests/Plugins"
)
target_link_libraries(Test2Plugin PUBLIC complex)
generate_export_header(Test2Plugin)
target_include_directories(Test2Plugin
    PUBLIC
    $<BUILD_INTERFACE:${TestPlugin_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/tests/Plugin>
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