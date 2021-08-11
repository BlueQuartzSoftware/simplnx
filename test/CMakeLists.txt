find_package(Catch2 CONFIG REQUIRED)

include(Catch)

add_executable(complex_test
  complex_test_main.cpp
  ArgumentsTest.cpp
  DataStructTest.cpp
  GeometryTest.cpp
  DataStructObserver.hpp
  DataStructObserver.cpp
  MontageTest.cpp
  BitTest.cpp
  UuidTest.cpp
  CoreFilterTest.cpp
)

add_executable(complex_app_test
  AppTest.cpp
)
add_executable(complex_plugin_test
  PluginTest.cpp
)

configure_file( cmake/PluginDir.hpp.in ${PROJECT_BINARY_DIR}/generated/PluginDir.hpp )
include_directories( ${PROJECT_BINARY_DIR}/generated/ ) 

target_include_directories(complex_app_test PRIVATE ${COMPLX_SOURCE_DIR}/test/Plugin)
target_include_directories(complex_plugin_test PRIVATE ${COMPLX_SOURCE_DIR}/test/Plugin)

target_link_libraries(complex_app_test
  PRIVATE
    complex
	TestPlugin
	Test2Plugin
    Catch2::Catch2
)
target_link_libraries(complex_plugin_test
  PRIVATE
    complex
	TestPlugin
	Test2Plugin
    Catch2::Catch2
)

target_link_libraries(complex_test
  PRIVATE
    complex
    Catch2::Catch2
)

set_target_properties(complex_test
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
)

set_target_properties(complex_app_test
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
)
set_target_properties(complex_plugin_test
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
)

target_compile_options(complex_test
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)
target_compile_options(complex_app_test
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)
target_compile_options(complex_plugin_test
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

catch_discover_tests(complex_test)

add_subdirectory(Plugin)
