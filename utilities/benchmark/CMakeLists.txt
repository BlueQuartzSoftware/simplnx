find_package(benchmark CONFIG REQUIRED)

add_executable(simplnx_benchmark)

set_target_properties(simplnx_benchmark
  PROPERTIES
    DEBUG_POSTFIX "${SIMPLNX_DEBUG_POSTFIX}"
)

set(SIMPLNX_BENCHMARK_SOURCES
  main.cpp
)

target_sources(simplnx_benchmark
  PRIVATE
    ${SIMPLNX_BENCHMARK_SOURCES}
)

target_link_libraries(simplnx_benchmark
  PRIVATE
    simplnx::simplnx
    benchmark::benchmark
)

simplnx_enable_warnings(TARGET simplnx_benchmark)

if(MSVC)
  target_compile_options(simplnx_benchmark
    PRIVATE
      /MP
  )
endif()

source_group("simplnx_benchmark" FILES ${SIMPLNX_BENCHMARK_SOURCES})
