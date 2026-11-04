# -----------------------------------------------------------------------------
# NXImageProcessingBenchmark: a standalone developer tool that times NXImageProcessing filters on large,
# tile-replicated real inputs. It links the NXImageProcessing plugin and resolves its filters at runtime. Four tiny
# registered smoke tests guard the custom benchmark-input preparation paths; normal benchmark sweeps remain manual.
#
# It reuses the golden-test harness (test/ItkGoldenTestUtils.hpp) for input loading + data-dir paths, which pulls in
# Catch2 and simplnx::UnitTestCommon and the generated <Plugin>_test_dirs.hpp. Those are produced by the plugin's
# unit-test setup, so this tool expects SIMPLNX_BUILD_TESTS to be ON (the test-dirs header is configured then).
# -----------------------------------------------------------------------------

# Catch2 is only found in the test scopes (Plugin.cmake / UnitTestCommon), so its imported target is not visible
# here. Re-find it so Catch2::Catch2 resolves (ItkGoldenTestUtils.hpp / UnitTestCommon.hpp include <catch2/catch.hpp>).
find_package(Catch2 CONFIG REQUIRED)

add_executable(NXImageProcessingBenchmark
  main.cpp
  BenchmarkTiling.cpp
  BenchmarkFilterRegistry.cpp
  BenchmarkRunner.cpp
)

add_executable(WriteImageBenchmark
  WriteImageBenchmark.cpp
)

# NOTE: we deliberately do NOT link simplnx::UnitTestCommon. That INTERFACE target compiles its Catch2-using .cpp
# sources into every consumer, whose Catch2 implementation is supplied by a CATCH_CONFIG_MAIN TU that unit-test
# executables have but this standalone tool does not (Catch2 v2 is header/impl-in-main). We only need the golden
# harness's header-inline helpers (ip_golden::ReadInputImage / InputPath), so we add the include dir directly and
# link only Catch2::Catch2 (its include dir; no impl symbols are ODR-used because we never invoke Catch2 macros).
target_link_libraries(NXImageProcessingBenchmark
  PRIVATE
    simplnx
    SimplnxCore
    NXImageProcessing
    fmt::fmt
    Catch2::Catch2
)

target_link_libraries(WriteImageBenchmark
  PRIVATE
    simplnx
    SimplnxCore
    NXImageProcessing
    fmt::fmt
)

target_include_directories(NXImageProcessingBenchmark
  PRIVATE
    ${${PLUGIN_NAME}_SOURCE_DIR}/test                              # ItkGoldenTestUtils.hpp + friends
    ${simplnx_BINARY_DIR}/Plugins/${PLUGIN_NAME}/generated         # generated ${PLUGIN_NAME}_test_dirs.hpp
    ${simplnx_SOURCE_DIR}/test/UnitTestCommon/include              # simplnx/UnitTest/UnitTestCommon.hpp (header only)
)

# Load plugins from this exe's own directory at runtime (that is where the .simplnx plugins are output).
target_compile_definitions(NXImageProcessingBenchmark
  PRIVATE
    SIMPLNX_BUILD_DIR="$<TARGET_FILE_DIR:NXImageProcessingBenchmark>"
)

target_compile_definitions(WriteImageBenchmark
  PRIVATE
    SIMPLNX_BUILD_DIR="$<TARGET_FILE_DIR:WriteImageBenchmark>"
)

# Out-of-core support is opt-in via the SIMPLNX_NXIMAGEPROCESSING_BENCH_HAS_OOC compile definition (see main.cpp).
# An out-of-core-enabled consumer build (e.g. the SimplnxOoc harness, which compiles the OOC backend straight into
# libsimplnx and exports its headers on simplnx's PUBLIC include path) defines that macro on this target AFTER it is
# created; main.cpp then registers the OOC IO manager + resolver so `--mode ooc` yields disk-backed stores. The
# standalone build never defines it, so the OOC column self-skips (no backend registered) exactly as before. Nothing
# is linked here: when the macro is defined, both the OOC symbols and headers already arrive through `simplnx`.

if(MSVC)
  target_compile_options(NXImageProcessingBenchmark PRIVATE /bigobj /MP)
  target_compile_definitions(NXImageProcessingBenchmark PRIVATE _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
endif()

# Output next to simplnx (the bin/<config> dir) so the plugins sit beside the exe.
set_target_properties(NXImageProcessingBenchmark
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:simplnx>
    FOLDER "Plugins/${PLUGIN_NAME}"
)

set_target_properties(WriteImageBenchmark
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:simplnx>
    FOLDER "Plugins/${PLUGIN_NAME}"
)

# Ensure the plugins this tool loads at runtime are built first.
add_dependencies(NXImageProcessingBenchmark NXImageProcessing SimplnxCore)
add_dependencies(WriteImageBenchmark NXImageProcessing SimplnxCore)

if(SIMPLNX_BUILD_TESTS)
  set(image_processing_benchmark_input_smoke_filters
    Mask
    ApproximateSignedDistanceMap
    AdaptiveHistogramEqualization
    MorphologicalWatershedFromMarkers
  )
  foreach(filter_name IN LISTS image_processing_benchmark_input_smoke_filters)
    set(test_name "NXImageProcessingBenchmark::InputPreparation::${filter_name}")
    add_test(
      NAME "${test_name}"
      COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>"
              --mode incore
              --filters "${filter_name}"
              --target-voxels 65536
              --z-slices 1
              --warmup 0
              --repeats 1
              --require-new-result
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "NXImageProcessingBenchmark;Smoke" TIMEOUT 120)
  endforeach()

  add_test(
    NAME NXImageProcessingBenchmark::Selection::RejectDuplicateExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --filters "Mask,Mask"
  )
  set_tests_properties(NXImageProcessingBenchmark::Selection::RejectDuplicateExit
    PROPERTIES
      LABELS "NXImageProcessingBenchmark;Smoke"
      TIMEOUT 120
      WILL_FAIL TRUE
  )

  add_test(
    NAME NXImageProcessingBenchmark::Selection::RejectDuplicateDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --filters "Mask,Mask"
  )
  set_tests_properties(NXImageProcessingBenchmark::Selection::RejectDuplicateDiagnostic
    PROPERTIES
      LABELS "NXImageProcessingBenchmark;Smoke"
      PASS_REGULAR_EXPRESSION "Duplicate benchmark filter name 'Mask' in --filters"
      TIMEOUT 120
  )

  set(image_processing_benchmark_projection_axis_tests)
  foreach(axis IN ITEMS 0 1 2)
    set(test_name "NXImageProcessingBenchmark::ProjectionAxis::Axis${axis}")
    add_test(
      NAME "${test_name}"
      COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>"
              --mode incore
              --filters MaximumProjection
              --target-voxels 65536
              --z-slices 1
              --force-2d
              --projection-axis "${axis}"
              --warmup 0
              --repeats 1
              --require-new-result
    )
    list(APPEND image_processing_benchmark_projection_axis_tests "${test_name}")
  endforeach()

  add_test(
    NAME NXImageProcessingBenchmark::ProjectionAxis::RegistryDefault
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>"
            --mode incore
            --filters MaximumProjection
            --target-voxels 65536
            --z-slices 1
            --force-2d
            --warmup 0
            --repeats 1
            --require-new-result
  )
  list(APPEND image_processing_benchmark_projection_axis_tests NXImageProcessingBenchmark::ProjectionAxis::RegistryDefault)

  add_test(
    NAME NXImageProcessingBenchmark::ProjectionAxis::NonProjectionUnaffected
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>"
            --mode incore
            --filters Mask
            --target-voxels 65536
            --z-slices 1
            --force-2d
            --projection-axis 2
            --warmup 0
            --repeats 1
            --require-new-result
  )
  list(APPEND image_processing_benchmark_projection_axis_tests NXImageProcessingBenchmark::ProjectionAxis::NonProjectionUnaffected)

  add_test(
    NAME NXImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --projection-axis 3
  )
  set_tests_properties(NXImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeExit PROPERTIES WILL_FAIL TRUE)
  list(APPEND image_processing_benchmark_projection_axis_tests NXImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeExit)

  add_test(
    NAME NXImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --projection-axis 3
  )
  set_tests_properties(NXImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--projection-axis must be 0, 1, or 2"
  )
  list(APPEND image_processing_benchmark_projection_axis_tests NXImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeDiagnostic)

  add_test(
    NAME NXImageProcessingBenchmark::ProjectionAxis::RejectMissingExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --projection-axis
  )
  set_tests_properties(NXImageProcessingBenchmark::ProjectionAxis::RejectMissingExit PROPERTIES WILL_FAIL TRUE)
  list(APPEND image_processing_benchmark_projection_axis_tests NXImageProcessingBenchmark::ProjectionAxis::RejectMissingExit)

  add_test(
    NAME NXImageProcessingBenchmark::ProjectionAxis::RejectMissingDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --projection-axis
  )
  set_tests_properties(NXImageProcessingBenchmark::ProjectionAxis::RejectMissingDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--projection-axis requires a value of 0, 1, or 2"
  )
  list(APPEND image_processing_benchmark_projection_axis_tests NXImageProcessingBenchmark::ProjectionAxis::RejectMissingDiagnostic)

  add_test(
    NAME NXImageProcessingBenchmark::ProjectionAxis::RejectNonNumericExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --projection-axis not-a-number
  )
  set_tests_properties(NXImageProcessingBenchmark::ProjectionAxis::RejectNonNumericExit PROPERTIES WILL_FAIL TRUE)
  list(APPEND image_processing_benchmark_projection_axis_tests NXImageProcessingBenchmark::ProjectionAxis::RejectNonNumericExit)

  add_test(
    NAME NXImageProcessingBenchmark::ProjectionAxis::RejectNonNumericDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --projection-axis not-a-number
  )
  set_tests_properties(NXImageProcessingBenchmark::ProjectionAxis::RejectNonNumericDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "Could not parse value for --projection-axis"
  )
  list(APPEND image_processing_benchmark_projection_axis_tests NXImageProcessingBenchmark::ProjectionAxis::RejectNonNumericDiagnostic)

  set_tests_properties(${image_processing_benchmark_projection_axis_tests}
    PROPERTIES
      LABELS "NXImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )

  add_test(
    NAME NXImageProcessingBenchmark::WorkingMemory::RejectMissingExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --working-memory-bytes
  )
  set_tests_properties(NXImageProcessingBenchmark::WorkingMemory::RejectMissingExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME NXImageProcessingBenchmark::WorkingMemory::RejectMissingDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --working-memory-bytes
  )
  set_tests_properties(NXImageProcessingBenchmark::WorkingMemory::RejectMissingDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--working-memory-bytes requires a positive integer byte count"
  )

  add_test(
    NAME NXImageProcessingBenchmark::WorkingMemory::RejectZeroDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --working-memory-bytes 0
  )
  set_tests_properties(NXImageProcessingBenchmark::WorkingMemory::RejectZeroDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--working-memory-bytes requires a positive integer byte count"
  )

  add_test(
    NAME NXImageProcessingBenchmark::WorkingMemory::RejectZeroExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --working-memory-bytes 0
  )
  set_tests_properties(NXImageProcessingBenchmark::WorkingMemory::RejectZeroExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME NXImageProcessingBenchmark::WorkingMemory::RejectNonNumericDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --working-memory-bytes not-a-number
  )
  set_tests_properties(NXImageProcessingBenchmark::WorkingMemory::RejectNonNumericDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "Could not parse value for --working-memory-bytes"
  )

  add_test(
    NAME NXImageProcessingBenchmark::WorkingMemory::RejectNonNumericExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --working-memory-bytes not-a-number
  )
  set_tests_properties(NXImageProcessingBenchmark::WorkingMemory::RejectNonNumericExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME NXImageProcessingBenchmark::WorkingMemory::ValidConfig
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>"
            --mode incore
            --filters Mask
            --target-voxels 65536
            --z-slices 1
            --warmup 0
            --repeats 1
            --working-memory-bytes 1048576
            --require-new-result
  )
  set_tests_properties(NXImageProcessingBenchmark::WorkingMemory::ValidConfig
    PROPERTIES PASS_REGULAR_EXPRESSION "working-memory-bytes=1048576"
  )

  set_tests_properties(
    NXImageProcessingBenchmark::WorkingMemory::RejectMissingExit
    NXImageProcessingBenchmark::WorkingMemory::RejectMissingDiagnostic
    NXImageProcessingBenchmark::WorkingMemory::RejectZeroDiagnostic
    NXImageProcessingBenchmark::WorkingMemory::RejectZeroExit
    NXImageProcessingBenchmark::WorkingMemory::RejectNonNumericDiagnostic
    NXImageProcessingBenchmark::WorkingMemory::RejectNonNumericExit
    NXImageProcessingBenchmark::WorkingMemory::ValidConfig
    PROPERTIES
      LABELS "NXImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )

  add_test(
    NAME NXImageProcessingBenchmark::CacheBudget::RejectMissingExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --cache-memory-budget-bytes
  )
  set_tests_properties(NXImageProcessingBenchmark::CacheBudget::RejectMissingExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME NXImageProcessingBenchmark::CacheBudget::RejectMissingDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --cache-memory-budget-bytes
  )
  set_tests_properties(NXImageProcessingBenchmark::CacheBudget::RejectMissingDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--cache-memory-budget-bytes requires a positive integer byte count"
  )

  add_test(
    NAME NXImageProcessingBenchmark::CacheBudget::RejectZeroExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --cache-memory-budget-bytes 0
  )
  set_tests_properties(NXImageProcessingBenchmark::CacheBudget::RejectZeroExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME NXImageProcessingBenchmark::CacheBudget::RejectZeroDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --cache-memory-budget-bytes 0
  )
  set_tests_properties(NXImageProcessingBenchmark::CacheBudget::RejectZeroDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--cache-memory-budget-bytes requires a positive integer byte count"
  )

  add_test(
    NAME NXImageProcessingBenchmark::CacheBudget::RejectNonNumericExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --cache-memory-budget-bytes not-a-number
  )
  set_tests_properties(NXImageProcessingBenchmark::CacheBudget::RejectNonNumericExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME NXImageProcessingBenchmark::CacheBudget::RejectNonNumericDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --cache-memory-budget-bytes not-a-number
  )
  set_tests_properties(NXImageProcessingBenchmark::CacheBudget::RejectNonNumericDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "Could not parse value for --cache-memory-budget-bytes"
  )

  add_test(
    NAME NXImageProcessingBenchmark::CacheBudget::ValidConfig
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>"
            --mode incore
            --filters Mask
            --target-voxels 65536
            --z-slices 1
            --warmup 0
            --repeats 1
            --cache-memory-budget-bytes 1073741824
            --require-new-result
  )
  set_tests_properties(NXImageProcessingBenchmark::CacheBudget::ValidConfig
    PROPERTIES PASS_REGULAR_EXPRESSION "cache-memory-budget-bytes=1073741824 maximum-working-memory-bytes=268435456"
  )

  set_tests_properties(
    NXImageProcessingBenchmark::CacheBudget::RejectMissingExit
    NXImageProcessingBenchmark::CacheBudget::RejectMissingDiagnostic
    NXImageProcessingBenchmark::CacheBudget::RejectZeroExit
    NXImageProcessingBenchmark::CacheBudget::RejectZeroDiagnostic
    NXImageProcessingBenchmark::CacheBudget::RejectNonNumericExit
    NXImageProcessingBenchmark::CacheBudget::RejectNonNumericDiagnostic
    NXImageProcessingBenchmark::CacheBudget::ValidConfig
    PROPERTIES
      LABELS "NXImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )

  add_test(
    NAME NXImageProcessingBenchmark::OocOnly::RejectInCoreExit
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --mode incore --ooc-only
  )
  set_tests_properties(NXImageProcessingBenchmark::OocOnly::RejectInCoreExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME NXImageProcessingBenchmark::OocOnly::RejectInCoreDiagnostic
    COMMAND "$<TARGET_FILE:NXImageProcessingBenchmark>" --mode incore --ooc-only
  )
  set_tests_properties(NXImageProcessingBenchmark::OocOnly::RejectInCoreDiagnostic
    PROPERTIES
      PASS_REGULAR_EXPRESSION "--ooc-only requires --mode ooc.*received mode 'incore'"
      LABELS "NXImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )
  set_tests_properties(NXImageProcessingBenchmark::OocOnly::RejectInCoreExit
    PROPERTIES
      LABELS "NXImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )
endif()
