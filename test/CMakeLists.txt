add_subdirectory(Plugin)

configure_file(cmake/PluginDir.hpp.in PluginDir.hpp @ONLY)

add_executable(DataStructureTest DataStructTest.cpp DataStructObserver.hpp DataStructObserver.cpp)
target_link_libraries(DataStructureTest PUBLIC complex)
set_target_properties(DataStructureTest PROPERTIES
    FOLDER "Tests"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_executable(PluginTest PluginTest.cpp)
target_link_libraries(PluginTest PUBLIC complex)
set_target_properties(PluginTest PROPERTIES
    FOLDER "Tests"
    INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_executable(AppTest AppTest.cpp)
target_link_libraries(AppTest PUBLIC complex)
set_target_properties(AppTest PROPERTIES
    FOLDER "Tests"
    INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(NAME DataStructure COMMAND DataStructureTest)
add_test(NAME Plugins COMMAND PluginTest)
add_test(NAME App COMMAND AppTest)

set_target_properties(complex PROPERTIES
    TEST DataStructure Plugins App
)
