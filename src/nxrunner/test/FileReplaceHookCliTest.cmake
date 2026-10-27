# Checks the file-replace hook results at the nxrunner boundary for one destination: the converted pipeline file
# (DESTINATION=convert) or the log file (DESTINATION=log). A test build of nxrunner registers the handler that
# SIMPLNX_TEST_FILE_REPLACE_HANDLER selects ("<action>|<target path>"):
#   warn         -> the warning is printed and the destination is replaced.
#   refuse       -> the error code is printed, the exit status is nonzero and the destination is unchanged.
#   refuse-empty -> the -6066 diagnostic is printed, the exit status is nonzero and the destination is unchanged.
# SIMPLNX_TEST_FILE_REPLACE_LOG makes the handler write one observation line per call for the destination. The test
# requires one line with the action, result validity, warning and error codes, and original destination bytes at callback time.
# Every case runs in an ASCII folder and in a folder with a non-ASCII name.
foreach(required_variable NXRUNNER_EXECUTABLE INPUT_PIPELINE WORK_DIR DESTINATION)
  if(NOT DEFINED ${required_variable})
    message(FATAL_ERROR "${required_variable} must be provided")
  endif()
endforeach()

if(DESTINATION STREQUAL "convert")
  set(destination_name "legacy.d3dpipeline")
  set(original_content "previous converted pipeline")
  set(original_hash "cc718c0b535d4eec")
elseif(DESTINATION STREQUAL "log")
  set(destination_name "nxrunner.log")
  set(original_content "previous log")
  set(original_hash "f7ec7845f4691bea")
else()
  message(FATAL_ERROR "Unknown DESTINATION '${DESTINATION}'")
endif()
string(LENGTH "${original_content}" original_size)

# Runs nxrunner with one handler action in case_dir. Leaves the exit status, the output, the destination bytes before and
# after, and the observation lines in run_result, run_output, bytes_before, bytes_after and observation_lines.
macro(run_nxrunner case_dir action)
  set(legacy_pipeline "${case_dir}/legacy.json")
  set(destination "${case_dir}/${destination_name}")
  set(observation_log "${WORK_DIR}/observations.txt")
  file(REMOVE_RECURSE "${WORK_DIR}")
  file(MAKE_DIRECTORY "${case_dir}")
  file(COPY_FILE "${INPUT_PIPELINE}" "${legacy_pipeline}")
  file(WRITE "${case_dir}/legacy.d3dpipeline" "previous converted pipeline")
  file(WRITE "${case_dir}/nxrunner.log" "previous log")
  file(READ "${destination}" bytes_before HEX)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "SIMPLNX_TEST_FILE_REPLACE_HANDLER=${action}|${destination}" "SIMPLNX_TEST_FILE_REPLACE_LOG=${observation_log}"
            "${NXRUNNER_EXECUTABLE}" --convert "${legacy_pipeline}" --logfile "${case_dir}/nxrunner.log"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_stdout
    ERROR_VARIABLE run_stderr
  )
  set(run_output "${run_stdout}${run_stderr}")
  file(READ "${destination}" bytes_after HEX)
  set(observation_lines "")
  if(EXISTS "${observation_log}")
    # UTF-8 keeps non-ASCII path characters; file(STRINGS) otherwise splits on them.
    file(STRINGS "${observation_log}" observation_lines ENCODING UTF-8)
  endif()
endmacro()

# Requires one observation with the expected result and the original bytes at callback time.
macro(require_one_observation label action valid warnings errors)
  list(LENGTH observation_lines observation_count)
  if(NOT observation_count EQUAL 1)
    message(FATAL_ERROR "[${label}] expected one hook call for '${destination}', found ${observation_count}: ${observation_lines}")
  endif()
  set(expected_line "action=${action} valid=${valid} warnings=${warnings} errors=${errors} size=${original_size} fnv1a64=${original_hash} target=${destination}")
  if(NOT observation_lines STREQUAL expected_line)
    message(FATAL_ERROR "[${label}] unexpected observation:\n  ${observation_lines}\nexpected:\n  ${expected_line}")
  endif()
endmacro()

foreach(case_dir "${WORK_DIR}/ascii" "${WORK_DIR}/ünïcødé_测试")
  run_nxrunner("${case_dir}" "warn")
  if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "[warn ${case_dir}] nxrunner failed with ${run_result}: ${run_output}")
  endif()
  # nxrunner and its embedded Python module both ask for the test handler; one registration prints the warning once.
  string(REGEX MATCHALL "Warning 99701" warning_lines "${run_output}")
  list(LENGTH warning_lines warning_count)
  if(NOT warning_count EQUAL 1)
    message(FATAL_ERROR "[warn ${case_dir}] the hook warning was printed ${warning_count} times instead of once: ${run_output}")
  endif()
  if(bytes_after STREQUAL bytes_before)
    message(FATAL_ERROR "[warn ${case_dir}] the destination '${destination}' was not replaced")
  endif()
  require_one_observation("warn ${case_dir}" "warn" "1" "99701" "")

  run_nxrunner("${case_dir}" "refuse")
  if(run_result EQUAL 0)
    message(FATAL_ERROR "[refuse ${case_dir}] nxrunner succeeded although the hook refused: ${run_output}")
  endif()
  if(NOT run_output MATCHES "Error -99702")
    message(FATAL_ERROR "[refuse ${case_dir}] the refusal code was not printed: ${run_output}")
  endif()
  if(NOT bytes_after STREQUAL bytes_before)
    message(FATAL_ERROR "[refuse ${case_dir}] the destination '${destination}' changed")
  endif()
  require_one_observation("refuse ${case_dir}" "refuse" "0" "" "-99702")

  run_nxrunner("${case_dir}" "refuse-empty")
  if(run_result EQUAL 0)
    message(FATAL_ERROR "[refuse-empty ${case_dir}] nxrunner succeeded although the hook refused: ${run_output}")
  endif()
  if(NOT run_output MATCHES "Error -6066: The operation failed, but no error message was reported")
    message(FATAL_ERROR "[refuse-empty ${case_dir}] the -6066 diagnostic was not printed: ${run_output}")
  endif()
  if(NOT bytes_after STREQUAL bytes_before)
    message(FATAL_ERROR "[refuse-empty ${case_dir}] the destination '${destination}' changed")
  endif()
  require_one_observation("refuse-empty ${case_dir}" "refuse-empty" "0" "" "")
endforeach()

file(REMOVE_RECURSE "${WORK_DIR}")
message(STATUS "nxrunner reports the file-replace hook results for the ${DESTINATION} destination.")
