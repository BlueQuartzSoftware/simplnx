# Keep the simplnx in-core CI preset options and generated cache in agreement.
if(NOT DEFINED CMAKE_PRESET_NAME)
  message(FATAL_ERROR "CMAKE_PRESET_NAME must be defined for CI.")
endif()

set(ci_configure_options
  "--preset=${CMAKE_PRESET_NAME}"
  "-B${CTEST_BINARY_DIRECTORY}"
  "-DSIMPLNX_TEST_ALGORITHM_PATH:STRING=0"
  "-DSIMPLNX_BUILD_TESTS:BOOL=ON"
)

function(ci_validate_cache)
  set(cache_file "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt")
  if(NOT EXISTS "${cache_file}")
    message(FATAL_ERROR "CI cache is missing: ${cache_file}")
  endif()

  set(expected_SIMPLNX_TEST_ALGORITHM_PATH "0")
  set(expected_SIMPLNX_BUILD_TESTS "ON")
  foreach(key IN ITEMS SIMPLNX_TEST_ALGORITHM_PATH SIMPLNX_BUILD_TESTS)
    file(STRINGS "${cache_file}" entries REGEX "^${key}:[^=]+=")
    list(LENGTH entries entry_count)
    if(NOT entry_count EQUAL 1)
      message(FATAL_ERROR "Expected one ${key} entry in ${cache_file}; found ${entry_count}.")
    endif()
    string(REGEX REPLACE "^[^=]+=" "" actual "${entries}")
    if(NOT "${actual}" STREQUAL "${expected_${key}}")
      message(FATAL_ERROR "${cache_file}: ${key} is '${actual}'; expected '${expected_${key}}'.")
    endif()
    message(STATUS "Verified ${key}=${actual}")
  endforeach()
endfunction()
