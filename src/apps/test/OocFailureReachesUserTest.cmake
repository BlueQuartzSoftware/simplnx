# End-to-end acceptance for the storage-failure propagation chain in the command line runner.
# EXPECT_SUCCESS selects the matching unarmed control run.
#
# The pipeline creates a 32 MiB int32 array with the "HDF5-OOC" data format and then asks
# Compute Array Statistics to read every tuple of it. SIMPLNX_OOC_FAULT_INJECT tells the
# out-of-core layer to truncate that array's session file once, right after the pipeline flushes
# the data structure at the end of the creating filter. The read in the next filter therefore
# meets a genuinely damaged file.
#
# The armed run must fail loudly and survive with nxrunner's pipeline error status. One output line
# must carry storage error -6032 and the reading filter's bulk-read failure for 'Data/Array'. The
# unarmed control must exit successfully.
# Anything quieter means a storage error was swallowed somewhere between the store and the user.
#
# This script runs only where the fault seam is compiled in; SimplnxOoc's OOC.cmake registers it there.
#
# Set VERBOSE to have a passing run print the captured output.

if(NOT DEFINED NXRUNNER_EXECUTABLE)
  message(FATAL_ERROR "NXRUNNER_EXECUTABLE was not provided")
endif()
if(NOT DEFINED PIPELINE_FILE)
  message(FATAL_ERROR "PIPELINE_FILE was not provided")
endif()
if(NOT EXISTS "${PIPELINE_FILE}")
  message(FATAL_ERROR "PIPELINE_FILE does not exist: ${PIPELINE_FILE}")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/OocProcessExitStatus.cmake")

if(EXPECT_SUCCESS)
  unset(ENV{SIMPLNX_OOC_FAULT_INJECT})
else()
  set(ENV{SIMPLNX_OOC_FAULT_INJECT} "truncate-session-after-first-flush")
endif()

execute_process(
  COMMAND "${NXRUNNER_EXECUTABLE}" --execute "${PIPELINE_FILE}"
  RESULT_VARIABLE run_result
  OUTPUT_VARIABLE run_stdout
  ERROR_VARIABLE run_stderr
)

unset(ENV{SIMPLNX_OOC_FAULT_INJECT})

set(out "${run_stdout}${run_stderr}")
simplnx_classify_ooc_process_status("${run_result}" run_classification)

# execute_process() reports nxrunner's expected -110 pipeline error as signed -110 on Windows and as
# truncated status 146 on POSIX. Every other nonzero value, textual status, signal, launch failure,
# or structured-exception status is abnormal.
if(run_classification STREQUAL "ABNORMAL")
  message(FATAL_ERROR "nxrunner did not exit normally (${run_result}); the storage failure must not terminate the process:\n${out}")
endif()

if(EXPECT_SUCCESS)
  if(NOT run_classification STREQUAL "SUCCESS")
    message(FATAL_ERROR "Unarmed nxrunner control failed with ${run_result}:\n${out}")
  endif()

  if(VERBOSE)
    message(STATUS "${out}")
  endif()
  message(STATUS "Unarmed nxrunner control completed successfully.")
  return()
endif()

if(run_classification STREQUAL "SUCCESS")
  message(FATAL_ERROR "nxrunner reported success although the OOC session file was truncated:\n${out}")
endif()

# The exact storage error and bulk-read context must occur on one output line. Separate matches could
# otherwise combine an unrelated -6032 with a different failure that happens to name the array.
string(REGEX MATCHALL "[^\r\n]+" output_lines "${out}")
set(found_expected_failure_line FALSE)
foreach(output_line IN LISTS output_lines)
  if(output_line MATCHES "(^|[^0-9])-6032([^0-9]|$)" AND output_line MATCHES "bulk read failed for array 'Data/Array'")
    set(found_expected_failure_line TRUE)
    break()
  endif()
endforeach()
if(NOT found_expected_failure_line)
  message(FATAL_ERROR "No nxrunner output line contains both error -6032 and the failed bulk read of 'Data/Array':\n${out}")
endif()

if(VERBOSE)
  message(STATUS "${out}")
endif()
message(STATUS "nxrunner exited with ${run_result} and reported the storage failure to the user.")
