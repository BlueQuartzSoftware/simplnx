# -----------------------------------------------------------------------------
# ImageProcessingBenchmark: a standalone developer tool that times ImageProcessing filters on large,
# tile-replicated real inputs. It links the new ImageProcessing plugin and resolves the legacy ITKImageProcessing
# plugin at runtime so a filter can be timed against its legacy counterpart. Four tiny registered smoke tests guard
# the custom benchmark-input preparation paths; normal benchmark sweeps remain manual.
#
# It reuses the golden-test harness (test/ItkGoldenTestUtils.hpp) for input loading + data-dir paths, which pulls in
# Catch2 and simplnx::UnitTestCommon and the generated <Plugin>_test_dirs.hpp. Those are produced by the plugin's
# unit-test setup, so this tool expects SIMPLNX_BUILD_TESTS to be ON (the test-dirs header is configured then).
# -----------------------------------------------------------------------------

# Catch2 is only found in the test scopes (Plugin.cmake / UnitTestCommon), so its imported target is not visible
# here. Re-find it so Catch2::Catch2 resolves (ItkGoldenTestUtils.hpp / UnitTestCommon.hpp include <catch2/catch.hpp>).
find_package(Catch2 CONFIG REQUIRED)

add_executable(ImageProcessingBenchmark
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
target_link_libraries(ImageProcessingBenchmark
  PRIVATE
    simplnx
    SimplnxCore
    ImageProcessing
    fmt::fmt
    Catch2::Catch2
)

target_link_libraries(WriteImageBenchmark
  PRIVATE
    simplnx
    SimplnxCore
    ImageProcessing
    fmt::fmt
)

target_include_directories(ImageProcessingBenchmark
  PRIVATE
    ${${PLUGIN_NAME}_SOURCE_DIR}/test                              # ItkGoldenTestUtils.hpp + friends
    ${simplnx_BINARY_DIR}/Plugins/${PLUGIN_NAME}/generated         # generated ${PLUGIN_NAME}_test_dirs.hpp
    ${simplnx_SOURCE_DIR}/test/UnitTestCommon/include              # simplnx/UnitTest/UnitTestCommon.hpp (header only)
)

# Load plugins from this exe's own directory at runtime (that is where the .simplnx plugins are output).
target_compile_definitions(ImageProcessingBenchmark
  PRIVATE
    SIMPLNX_BUILD_DIR="$<TARGET_FILE_DIR:ImageProcessingBenchmark>"
)

target_compile_definitions(WriteImageBenchmark
  PRIVATE
    SIMPLNX_BUILD_DIR="$<TARGET_FILE_DIR:WriteImageBenchmark>"
)

# Out-of-core support is opt-in via the SIMPLNX_IMAGEPROCESSING_BENCH_HAS_OOC compile definition (see main.cpp).
# An out-of-core-enabled consumer build (e.g. the SimplnxOoc harness, which compiles the OOC backend straight into
# libsimplnx and exports its headers on simplnx's PUBLIC include path) defines that macro on this target AFTER it is
# created; main.cpp then registers the OOC IO manager + resolver so `--mode ooc` yields disk-backed stores. The
# standalone build never defines it, so the OOC column self-skips (no backend registered) exactly as before. Nothing
# is linked here: when the macro is defined, both the OOC symbols and headers already arrive through `simplnx`.

if(MSVC)
  target_compile_options(ImageProcessingBenchmark PRIVATE /bigobj /MP)
  target_compile_definitions(ImageProcessingBenchmark PRIVATE _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
endif()

# Output next to simplnx (the bin/<config> dir) so the plugins sit beside the exe.
set_target_properties(ImageProcessingBenchmark
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
add_dependencies(ImageProcessingBenchmark ImageProcessing SimplnxCore)
add_dependencies(WriteImageBenchmark ImageProcessing SimplnxCore)

if(SIMPLNX_BUILD_TESTS)
  set(image_processing_benchmark_input_smoke_filters
    Mask
    ApproximateSignedDistanceMap
    AdaptiveHistogramEqualization
    MorphologicalWatershedFromMarkers
  )
  foreach(filter_name IN LISTS image_processing_benchmark_input_smoke_filters)
    set(test_name "ImageProcessingBenchmark::InputPreparation::${filter_name}")
    add_test(
      NAME "${test_name}"
      COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>"
              --mode incore
              --filters "${filter_name}"
              --target-voxels 65536
              --z-slices 1
              --warmup 0
              --repeats 1
              --require-new-result
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "ImageProcessingBenchmark;Smoke" TIMEOUT 120)
  endforeach()

  add_test(
    NAME ImageProcessingBenchmark::Selection::RejectDuplicateExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --filters "Mask,Mask"
  )
  set_tests_properties(ImageProcessingBenchmark::Selection::RejectDuplicateExit
    PROPERTIES
      LABELS "ImageProcessingBenchmark;Smoke"
      TIMEOUT 120
      WILL_FAIL TRUE
  )

  add_test(
    NAME ImageProcessingBenchmark::Selection::RejectDuplicateDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --filters "Mask,Mask"
  )
  set_tests_properties(ImageProcessingBenchmark::Selection::RejectDuplicateDiagnostic
    PROPERTIES
      LABELS "ImageProcessingBenchmark;Smoke"
      PASS_REGULAR_EXPRESSION "Duplicate benchmark filter name 'Mask' in --filters"
      TIMEOUT 120
  )

  set(image_processing_benchmark_projection_axis_tests)
  foreach(axis IN ITEMS 0 1 2)
    set(test_name "ImageProcessingBenchmark::ProjectionAxis::Axis${axis}")
    add_test(
      NAME "${test_name}"
      COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>"
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
    NAME ImageProcessingBenchmark::ProjectionAxis::RegistryDefault
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>"
            --mode incore
            --filters MaximumProjection
            --target-voxels 65536
            --z-slices 1
            --force-2d
            --warmup 0
            --repeats 1
            --require-new-result
  )
  list(APPEND image_processing_benchmark_projection_axis_tests ImageProcessingBenchmark::ProjectionAxis::RegistryDefault)

  add_test(
    NAME ImageProcessingBenchmark::ProjectionAxis::NonProjectionUnaffected
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>"
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
  list(APPEND image_processing_benchmark_projection_axis_tests ImageProcessingBenchmark::ProjectionAxis::NonProjectionUnaffected)

  add_test(
    NAME ImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --projection-axis 3
  )
  set_tests_properties(ImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeExit PROPERTIES WILL_FAIL TRUE)
  list(APPEND image_processing_benchmark_projection_axis_tests ImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeExit)

  add_test(
    NAME ImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --projection-axis 3
  )
  set_tests_properties(ImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--projection-axis must be 0, 1, or 2"
  )
  list(APPEND image_processing_benchmark_projection_axis_tests ImageProcessingBenchmark::ProjectionAxis::RejectOutOfRangeDiagnostic)

  add_test(
    NAME ImageProcessingBenchmark::ProjectionAxis::RejectMissingExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --projection-axis
  )
  set_tests_properties(ImageProcessingBenchmark::ProjectionAxis::RejectMissingExit PROPERTIES WILL_FAIL TRUE)
  list(APPEND image_processing_benchmark_projection_axis_tests ImageProcessingBenchmark::ProjectionAxis::RejectMissingExit)

  add_test(
    NAME ImageProcessingBenchmark::ProjectionAxis::RejectMissingDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --projection-axis
  )
  set_tests_properties(ImageProcessingBenchmark::ProjectionAxis::RejectMissingDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--projection-axis requires a value of 0, 1, or 2"
  )
  list(APPEND image_processing_benchmark_projection_axis_tests ImageProcessingBenchmark::ProjectionAxis::RejectMissingDiagnostic)

  add_test(
    NAME ImageProcessingBenchmark::ProjectionAxis::RejectNonNumericExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --projection-axis not-a-number
  )
  set_tests_properties(ImageProcessingBenchmark::ProjectionAxis::RejectNonNumericExit PROPERTIES WILL_FAIL TRUE)
  list(APPEND image_processing_benchmark_projection_axis_tests ImageProcessingBenchmark::ProjectionAxis::RejectNonNumericExit)

  add_test(
    NAME ImageProcessingBenchmark::ProjectionAxis::RejectNonNumericDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --projection-axis not-a-number
  )
  set_tests_properties(ImageProcessingBenchmark::ProjectionAxis::RejectNonNumericDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "Could not parse value for --projection-axis"
  )
  list(APPEND image_processing_benchmark_projection_axis_tests ImageProcessingBenchmark::ProjectionAxis::RejectNonNumericDiagnostic)

  set_tests_properties(${image_processing_benchmark_projection_axis_tests}
    PROPERTIES
      LABELS "ImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )

  add_test(
    NAME ImageProcessingBenchmark::WorkingMemory::RejectMissingExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --working-memory-bytes
  )
  set_tests_properties(ImageProcessingBenchmark::WorkingMemory::RejectMissingExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME ImageProcessingBenchmark::WorkingMemory::RejectMissingDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --working-memory-bytes
  )
  set_tests_properties(ImageProcessingBenchmark::WorkingMemory::RejectMissingDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--working-memory-bytes requires a positive integer byte count"
  )

  add_test(
    NAME ImageProcessingBenchmark::WorkingMemory::RejectZeroDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --working-memory-bytes 0
  )
  set_tests_properties(ImageProcessingBenchmark::WorkingMemory::RejectZeroDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--working-memory-bytes requires a positive integer byte count"
  )

  add_test(
    NAME ImageProcessingBenchmark::WorkingMemory::RejectZeroExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --working-memory-bytes 0
  )
  set_tests_properties(ImageProcessingBenchmark::WorkingMemory::RejectZeroExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME ImageProcessingBenchmark::WorkingMemory::RejectNonNumericDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --working-memory-bytes not-a-number
  )
  set_tests_properties(ImageProcessingBenchmark::WorkingMemory::RejectNonNumericDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "Could not parse value for --working-memory-bytes"
  )

  add_test(
    NAME ImageProcessingBenchmark::WorkingMemory::RejectNonNumericExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --working-memory-bytes not-a-number
  )
  set_tests_properties(ImageProcessingBenchmark::WorkingMemory::RejectNonNumericExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME ImageProcessingBenchmark::WorkingMemory::ValidConfig
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>"
            --mode incore
            --filters Mask
            --target-voxels 65536
            --z-slices 1
            --warmup 0
            --repeats 1
            --working-memory-bytes 1048576
            --require-new-result
  )
  set_tests_properties(ImageProcessingBenchmark::WorkingMemory::ValidConfig
    PROPERTIES PASS_REGULAR_EXPRESSION "working-memory-bytes=1048576"
  )

  set_tests_properties(
    ImageProcessingBenchmark::WorkingMemory::RejectMissingExit
    ImageProcessingBenchmark::WorkingMemory::RejectMissingDiagnostic
    ImageProcessingBenchmark::WorkingMemory::RejectZeroDiagnostic
    ImageProcessingBenchmark::WorkingMemory::RejectZeroExit
    ImageProcessingBenchmark::WorkingMemory::RejectNonNumericDiagnostic
    ImageProcessingBenchmark::WorkingMemory::RejectNonNumericExit
    ImageProcessingBenchmark::WorkingMemory::ValidConfig
    PROPERTIES
      LABELS "ImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )

  add_test(
    NAME ImageProcessingBenchmark::CacheBudget::RejectMissingExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --cache-memory-budget-bytes
  )
  set_tests_properties(ImageProcessingBenchmark::CacheBudget::RejectMissingExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME ImageProcessingBenchmark::CacheBudget::RejectMissingDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --cache-memory-budget-bytes
  )
  set_tests_properties(ImageProcessingBenchmark::CacheBudget::RejectMissingDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--cache-memory-budget-bytes requires a positive integer byte count"
  )

  add_test(
    NAME ImageProcessingBenchmark::CacheBudget::RejectZeroExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --cache-memory-budget-bytes 0
  )
  set_tests_properties(ImageProcessingBenchmark::CacheBudget::RejectZeroExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME ImageProcessingBenchmark::CacheBudget::RejectZeroDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --cache-memory-budget-bytes 0
  )
  set_tests_properties(ImageProcessingBenchmark::CacheBudget::RejectZeroDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "--cache-memory-budget-bytes requires a positive integer byte count"
  )

  add_test(
    NAME ImageProcessingBenchmark::CacheBudget::RejectNonNumericExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --cache-memory-budget-bytes not-a-number
  )
  set_tests_properties(ImageProcessingBenchmark::CacheBudget::RejectNonNumericExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME ImageProcessingBenchmark::CacheBudget::RejectNonNumericDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --cache-memory-budget-bytes not-a-number
  )
  set_tests_properties(ImageProcessingBenchmark::CacheBudget::RejectNonNumericDiagnostic
    PROPERTIES PASS_REGULAR_EXPRESSION "Could not parse value for --cache-memory-budget-bytes"
  )

  add_test(
    NAME ImageProcessingBenchmark::CacheBudget::ValidConfig
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>"
            --mode incore
            --filters Mask
            --target-voxels 65536
            --z-slices 1
            --warmup 0
            --repeats 1
            --cache-memory-budget-bytes 2147483648
            --require-new-result
  )
  set_tests_properties(ImageProcessingBenchmark::CacheBudget::ValidConfig
    PROPERTIES PASS_REGULAR_EXPRESSION "cache-memory-budget-bytes=2147483648 maximum-working-memory-bytes=536870912"
  )

  set_tests_properties(
    ImageProcessingBenchmark::CacheBudget::RejectMissingExit
    ImageProcessingBenchmark::CacheBudget::RejectMissingDiagnostic
    ImageProcessingBenchmark::CacheBudget::RejectZeroExit
    ImageProcessingBenchmark::CacheBudget::RejectZeroDiagnostic
    ImageProcessingBenchmark::CacheBudget::RejectNonNumericExit
    ImageProcessingBenchmark::CacheBudget::RejectNonNumericDiagnostic
    ImageProcessingBenchmark::CacheBudget::ValidConfig
    PROPERTIES
      LABELS "ImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )

  add_test(
    NAME ImageProcessingBenchmark::OocOnly::RejectInCoreExit
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --mode incore --ooc-only
  )
  set_tests_properties(ImageProcessingBenchmark::OocOnly::RejectInCoreExit PROPERTIES WILL_FAIL TRUE)

  add_test(
    NAME ImageProcessingBenchmark::OocOnly::RejectInCoreDiagnostic
    COMMAND "$<TARGET_FILE:ImageProcessingBenchmark>" --mode incore --ooc-only
  )
  set_tests_properties(ImageProcessingBenchmark::OocOnly::RejectInCoreDiagnostic
    PROPERTIES
      PASS_REGULAR_EXPRESSION "--ooc-only requires --mode ooc.*received mode 'incore'"
      LABELS "ImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )
  set_tests_properties(ImageProcessingBenchmark::OocOnly::RejectInCoreExit
    PROPERTIES
      LABELS "ImageProcessingBenchmark;Smoke"
      TIMEOUT 120
  )
endif()
