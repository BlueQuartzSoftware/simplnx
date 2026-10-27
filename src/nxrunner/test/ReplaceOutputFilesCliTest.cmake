# nxrunner writes its log file and the converted pipeline file through the file-replace helpers
# (OpenFileForReplace and ReplaceFileWithText). This test checks that both files that already exist
# are replaced. The helpers themselves, including a refused replacement, are tested in simplnx_test.
if(NOT DEFINED NXRUNNER_EXECUTABLE OR NOT DEFINED INPUT_PIPELINE OR NOT DEFINED WORK_DIR)
  message(FATAL_ERROR "NXRUNNER_EXECUTABLE, INPUT_PIPELINE and WORK_DIR must be provided")
endif()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")
set(legacy_pipeline "${WORK_DIR}/legacy.json")
set(converted_pipeline "${WORK_DIR}/legacy.d3dpipeline")
set(log_file "${WORK_DIR}/nxrunner.log")
file(COPY_FILE "${INPUT_PIPELINE}" "${legacy_pipeline}")
file(WRITE "${converted_pipeline}" "previous converted pipeline")
file(WRITE "${log_file}" "previous log")

execute_process(
  COMMAND "${NXRUNNER_EXECUTABLE}" --convert "${legacy_pipeline}" --logfile "${log_file}"
  RESULT_VARIABLE convert_result
  OUTPUT_VARIABLE convert_stdout
  ERROR_VARIABLE convert_stderr
)
if(NOT convert_result EQUAL 0)
  message(FATAL_ERROR "nxrunner --convert failed with ${convert_result}: ${convert_stdout}${convert_stderr}")
endif()

file(READ "${converted_pipeline}" converted_text)
if(converted_text MATCHES "previous converted pipeline" OR NOT converted_text MATCHES "\"pipeline\"")
  message(FATAL_ERROR "The converted pipeline file was not replaced: ${converted_text}")
endif()
file(READ "${log_file}" log_text)
if(log_text MATCHES "previous log" OR NOT log_text MATCHES "Converted File")
  message(FATAL_ERROR "The log file was not replaced: ${log_text}")
endif()
file(REMOVE_RECURSE "${WORK_DIR}")
message(STATUS "nxrunner replaced the existing converted pipeline file and log file.")
