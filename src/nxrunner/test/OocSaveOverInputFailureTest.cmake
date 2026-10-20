# End-to-end acceptance for saving over the input file from the command line runner.
# EXPECT_SUCCESS selects the unarmed control run.
#
# Every nxrunner process runs with an isolated preferences file that selects Force Out-of-Core storage, so the
# pipeline that reads the input file imports its arrays as read-only stores that read that file.
#
# 1. The generator pipeline writes a fixture with exactly one numeric array, 'Generated/Data': 25,165,824 float32
#    tuples with one component (96 MiB, more than one 64 MiB promotion window), every value 3.
# 2. The fixture is copied to X, and the rename pipeline runs on X: Read X, rename 'Generated/Data' to
#    'Generated/OldData', create a new 'Generated/Data' with every value 7, Write DREAM3D-NX File to X. Before X is
#    replaced, the writer must release the renamed array's store, which copies its data off X window by window.
#
# The control run must exit successfully, and VERIFY_EXE, reading X in a fresh process, must find OldData all 3 and
# Data all 7.
# The armed run sets SIMPLNX_OOC_FAULT_INJECT=fail-promotion-after-first-window, so the release fails after the first
# copied window. nxrunner must survive with its pipeline error status, one output line must carry error -6039, the
# store's dataset 'Generated/Data' and the injected fault's message, and X must still match the fixture byte for byte.
# The fixture's only numeric array is the only store the save can release, so the fault always interrupts that copy.
#
# This script runs only where the fault seam is compiled in; SimplnxOoc's OOC.cmake registers it there.
#
# Inputs: NXRUNNER_EXECUTABLE, VERIFY_EXE, GENERATE_PIPELINE and RENAME_PIPELINE (pipeline templates whose
# @SAVE_OVER_INPUT_FILE@ token names the file each pipeline writes or reads), WORK_DIR (emptied and reused), and the
# optional EXPECT_SUCCESS and VERBOSE. A passing run removes WORK_DIR; a failing run keeps it for diagnosis.

foreach(required_input IN ITEMS NXRUNNER_EXECUTABLE VERIFY_EXE GENERATE_PIPELINE RENAME_PIPELINE WORK_DIR)
  if(NOT DEFINED ${required_input} OR "${${required_input}}" STREQUAL "")
    message(FATAL_ERROR "${required_input} was not provided")
  endif()
endforeach()
foreach(required_file IN ITEMS NXRUNNER_EXECUTABLE VERIFY_EXE GENERATE_PIPELINE RENAME_PIPELINE)
  if(NOT EXISTS "${${required_file}}")
    message(FATAL_ERROR "${required_file} does not exist: ${${required_file}}")
  endif()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/OocProcessExitStatus.cmake")

# Forward slashes keep every path valid inside the pipeline JSON on Windows as well.
file(TO_CMAKE_PATH "${WORK_DIR}" work_dir)
set(test_home "${work_dir}/home")
set(pipeline_dir "${work_dir}/pipelines")
set(fixture_file "${work_dir}/fixture.dream3d")
set(target_file "${work_dir}/save_over_input.dream3d")

file(REMOVE_RECURSE "${work_dir}")
file(MAKE_DIRECTORY "${pipeline_dir}")

# The OOC bootstrap creates the application and loads its preferences before nxrunner's main(), so the home
# directory must be set in the environment each process inherits. Preferences::DefaultFilePath() reads
# USERPROFILE on Windows and HOME elsewhere; the application name is DREAM3DNX. Data storage mode 2 is Force
# Out-of-Core.
if(WIN32)
  set(preferences_dir "${test_home}/AppData/Local/DREAM3DNX")
  set(ENV{USERPROFILE} "${test_home}")
elseif(APPLE)
  set(preferences_dir "${test_home}/Library/Preferences/DREAM3DNX")
  set(ENV{HOME} "${test_home}")
else()
  set(preferences_dir "${test_home}/.config/DREAM3DNX")
  set(ENV{HOME} "${test_home}")
endif()
file(MAKE_DIRECTORY "${preferences_dir}")
file(WRITE "${preferences_dir}/preferences.json" "{\"data_storage_mode\": 2}\n")

# Escapes text for use inside a JSON string literal: backslashes, quotes and every control character U+0001..U+001F.
# A valid path can hold a quote (POSIX) or a backslash, which would otherwise end or corrupt the literal.
function(json_escape_string text out_var)
  string(REPLACE "\\" "\\\\" escaped "${text}")
  string(REPLACE "\"" "\\\"" escaped "${escaped}")
  foreach(code RANGE 1 31)
    string(ASCII ${code} control_character)
    string(HEX "${control_character}" control_hex)
    string(REPLACE "${control_character}" "\\u00${control_hex}" escaped "${escaped}")
  endforeach()
  set(${out_var} "${escaped}" PARENT_SCOPE)
endfunction()

# Runs nxrunner on a pipeline template whose @SAVE_OVER_INPUT_FILE@ token, inside a JSON string literal, is replaced
# by data_file. Sets <prefix>_result to the process status and <prefix>_output to the combined stdout and stderr.
function(run_nxrunner_pipeline template data_file prefix)
  get_filename_component(template_name "${template}" NAME)
  json_escape_string("${data_file}" SAVE_OVER_INPUT_FILE)
  configure_file("${template}" "${pipeline_dir}/${template_name}" @ONLY)
  execute_process(
    COMMAND "${NXRUNNER_EXECUTABLE}" --execute "${pipeline_dir}/${template_name}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_stdout
    ERROR_VARIABLE run_stderr
  )
  set(${prefix}_result "${run_result}" PARENT_SCOPE)
  set(${prefix}_output "${run_stdout}${run_stderr}" PARENT_SCOPE)
endfunction()

# The generator and the control run unarmed, whatever the calling environment holds.
unset(ENV{SIMPLNX_OOC_FAULT_INJECT})

run_nxrunner_pipeline("${GENERATE_PIPELINE}" "${fixture_file}" generate)
simplnx_classify_ooc_process_status("${generate_result}" generate_classification)
if(NOT generate_classification STREQUAL "SUCCESS" OR NOT EXISTS "${fixture_file}")
  message(FATAL_ERROR "The generator pipeline did not write the fixture (status ${generate_result}):\n${generate_output}")
endif()
file(SHA256 "${fixture_file}" fixture_hash)

file(COPY_FILE "${fixture_file}" "${target_file}" RESULT copy_result)
if(NOT copy_result STREQUAL "0")
  message(FATAL_ERROR "Could not copy the fixture to '${target_file}': ${copy_result}")
endif()

if(NOT EXPECT_SUCCESS)
  set(ENV{SIMPLNX_OOC_FAULT_INJECT} "fail-promotion-after-first-window")
endif()
run_nxrunner_pipeline("${RENAME_PIPELINE}" "${target_file}" rename)
unset(ENV{SIMPLNX_OOC_FAULT_INJECT})

simplnx_classify_ooc_process_status("${rename_result}" rename_classification)

# execute_process() reports nxrunner's expected -110 pipeline error as signed -110 on Windows and as truncated status
# 146 on POSIX. Every other nonzero value, textual status, signal, launch failure, or structured-exception status is
# abnormal.
if(rename_classification STREQUAL "ABNORMAL")
  message(FATAL_ERROR "nxrunner did not exit normally (${rename_result}); a failed release must not terminate the process:\n${rename_output}")
endif()

if(EXPECT_SUCCESS)
  if(NOT rename_classification STREQUAL "SUCCESS")
    message(FATAL_ERROR "The unarmed save over the input file failed with ${rename_result}:\n${rename_output}")
  endif()

  # A fresh process reads the saved file, so no store or cache of the pipeline process can supply the values.
  execute_process(
    COMMAND "${VERIFY_EXE}" "${target_file}"
    RESULT_VARIABLE verify_result
    OUTPUT_VARIABLE verify_stdout
    ERROR_VARIABLE verify_stderr
  )
  if(NOT verify_result STREQUAL "0")
    message(FATAL_ERROR "The saved file holds wrong values (verifier status ${verify_result}):\n${verify_stdout}${verify_stderr}\nnxrunner output:\n${rename_output}")
  endif()

  if(VERBOSE)
    message(STATUS "${rename_output}")
  endif()
  file(REMOVE_RECURSE "${work_dir}")
  message(STATUS "The unarmed save over the input file succeeded and the saved values are correct.")
  return()
endif()

if(rename_classification STREQUAL "SUCCESS")
  message(FATAL_ERROR "nxrunner reported success although the release of the renamed array's store was armed to fail:\n${rename_output}")
endif()

# The error code, the store's dataset and the injected message must occur on one output line. Separate matches could
# otherwise combine an unrelated -6039 with a different failure that happens to name the array.
string(REGEX MATCHALL "[^\r\n]+" output_lines "${rename_output}")
set(found_expected_failure_line FALSE)
foreach(output_line IN LISTS output_lines)
  if(output_line MATCHES "(^|[^0-9])-6039([^0-9]|$)"
     AND output_line MATCHES "Generated/Data'"
     AND output_line MATCHES "injected promotion failure after window 1")
    set(found_expected_failure_line TRUE)
    break()
  endif()
endforeach()
if(NOT found_expected_failure_line)
  message(FATAL_ERROR "No nxrunner output line contains error -6039, the dataset 'Generated/Data' and the injected promotion failure:\n${rename_output}")
endif()

file(SHA256 "${target_file}" target_hash)
if(NOT target_hash STREQUAL fixture_hash)
  message(FATAL_ERROR "The failed save changed the input file: SHA-256 ${target_hash}, fixture ${fixture_hash}:\n${rename_output}")
endif()

if(VERBOSE)
  message(STATUS "${rename_output}")
endif()
file(REMOVE_RECURSE "${work_dir}")
message(STATUS "nxrunner exited with ${rename_result}, reported the failed release with -6039, and left the input file unchanged.")
