# This include also serves test targets configured without SIMPLNX_BUILD_TESTS.
if(NOT TARGET simplnx::TestProfile)
  add_library(SimplnxTestProfile INTERFACE)
  add_library(simplnx::TestProfile ALIAS SimplnxTestProfile)
  target_include_directories(SimplnxTestProfile INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/UnitTestCommon/include")
  target_compile_features(SimplnxTestProfile INTERFACE cxx_std_20)
endif()
