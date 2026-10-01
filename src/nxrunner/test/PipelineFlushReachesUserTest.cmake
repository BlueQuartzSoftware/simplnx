# Execute the real CLI with a registered test-only filter and, when applicable,
# the real WriteDREAM3DFilter. All writable paths belong to this test's scratch directory.
foreach(required IN ITEMS NXRUNNER_EXECUTABLE TEST_MODE SCRATCH_ROOT)
  if(NOT DEFINED ${required})
    message(FATAL_ERROR "${required} was not provided")
  endif()
endforeach()
if(NOT EXISTS "${NXRUNNER_EXECUTABLE}")
  message(FATAL_ERROR "nxrunner executable is missing: ${NXRUNNER_EXECUTABLE}")
endif()
set(modes unarmed flush execute_and_flush allocation snapshot root_snapshot snapshot_allocation)
list(FIND modes "${TEST_MODE}" mode_index)
if(mode_index LESS 0)
  message(FATAL_ERROR "Unknown completion test mode: ${TEST_MODE}")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/OocProcessExitStatus.cmake")
file(MAKE_DIRECTORY
  "${SCRATCH_ROOT}/home"
  "${SCRATCH_ROOT}/temp"
  "${SCRATCH_ROOT}/home/AppData/Local"
  "${SCRATCH_ROOT}/home/AppData/Roaming"
  "${SCRATCH_ROOT}/home/.config"
  "${SCRATCH_ROOT}/home/.cache"
  "${SCRATCH_ROOT}/home/.local/share"
  "${SCRATCH_ROOT}/home/.local/state"
)
file(TO_CMAKE_PATH "${SCRATCH_ROOT}/export.dream3d" export_path)
file(TO_CMAKE_PATH "${SCRATCH_ROOT}/pipeline.d3dpipeline" pipeline_path)
file(WRITE "${export_path}" "C2-owned-export-sentinel")
file(SHA256 "${export_path}" old_export_hash)
set(fixture_uuid "2a98fc79-c485-45d5-8fa9-1f298f3d6729")
set(first_filter [=[
{
  "filter": {"name": "nx::core::FlushFailureFilter", "uuid": "@fixture_uuid@"},
  "args": {"parameters_version": 1, "completion_test_mode": {"version": 1, "value": @mode_index@}},
  "comments": "",
  "isDisabled": false
}
]=])
string(CONFIGURE "${first_filter}" first_filter @ONLY)
set(writer_filter [=[
{
  "filter": {"name": "nx::core::WriteDREAM3DFilter", "uuid": "b3a95784-2ced-41ec-8d3d-0242ac130003"},
  "args": {
    "parameters_version": 2,
    "export_file_path": {"version": 1, "value": "@export_path@"},
    "write_xdmf_file": {"version": 1, "value": false},
    "use_compression": {"version": 1, "value": false},
    "compression_level": {"version": 1, "value": 5}
  },
  "comments": "",
  "isDisabled": false
}
]=])
string(CONFIGURE "${writer_filter}" writer_filter @ONLY)
# Root-only failure happens after all filters; no test can promise rollback of a prior writer.
if(TEST_MODE STREQUAL "root_snapshot")
  set(pipeline_filters "${first_filter}")
else()
  set(pipeline_filters "${first_filter},${writer_filter}")
endif()
file(WRITE "${pipeline_path}" "{\"version\":1,\"name\":\"C2 CLI completion\",\"isDisabled\":false,\"pipeline\":[${pipeline_filters}]}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env --unset=SIMPLNX_OOC_FAULT_INJECT
    "HOME=${SCRATCH_ROOT}/home" "USERPROFILE=${SCRATCH_ROOT}/home"
    "LOCALAPPDATA=${SCRATCH_ROOT}/home/AppData/Local" "APPDATA=${SCRATCH_ROOT}/home/AppData/Roaming"
    "TEMP=${SCRATCH_ROOT}/temp" "TMP=${SCRATCH_ROOT}/temp" "TMPDIR=${SCRATCH_ROOT}/temp"
    "XDG_CONFIG_HOME=${SCRATCH_ROOT}/home/.config" "XDG_CACHE_HOME=${SCRATCH_ROOT}/home/.cache"
    "XDG_DATA_HOME=${SCRATCH_ROOT}/home/.local/share" "XDG_STATE_HOME=${SCRATCH_ROOT}/home/.local/state"
    "${NXRUNNER_EXECUTABLE}" --execute "${pipeline_path}"
  RESULT_VARIABLE run_result
  OUTPUT_VARIABLE run_stdout
  ERROR_VARIABLE run_stderr
  TIMEOUT 90
)
set(output "${run_stdout}${run_stderr}")
simplnx_classify_ooc_process_status("${run_result}" classification)
if(classification STREQUAL "ABNORMAL")
  message(FATAL_ERROR "Completion run ended abnormally (${run_result}); unknown-filter, exception, crash, and timeout exits are not expected failure:\n${output}")
endif()
string(FIND "${output}" "C2 fixture UUID: ${fixture_uuid}" fixture_witness)
if(fixture_witness LESS 0)
  message(FATAL_ERROR "The actual registered fixture did not execute. An unknown-filter failure is not completion evidence:\n${output}")
endif()
if(TEST_MODE STREQUAL "unarmed")
  if(NOT classification STREQUAL "SUCCESS")
    message(FATAL_ERROR "Unarmed control failed with ${run_result}:\n${output}")
  endif()
  file(READ "${export_path}" signature OFFSET 0 LIMIT 8 HEX)
  if(NOT signature STREQUAL "894844460d0a1a0a")
    message(FATAL_ERROR "The real writer did not publish an HDF5 file: ${signature}\n${output}")
  endif()
  message(STATUS "Unarmed completion fixture and real DREAM3D writer succeeded.")
  return()
endif()
if(NOT classification STREQUAL "EXPECTED_FAILURE")
  message(FATAL_ERROR "Armed completion run returned success:\n${output}")
endif()
if(NOT TEST_MODE STREQUAL "root_snapshot")
  file(SHA256 "${export_path}" export_hash)
  if(NOT export_hash STREQUAL old_export_hash)
    message(FATAL_ERROR "A node completion failure reached the later writer and replaced the owned export sentinel:\n${output}")
  endif()
endif()

if(TEST_MODE STREQUAL "flush" OR TEST_MODE STREQUAL "execute_and_flush" OR TEST_MODE STREQUAL "allocation")
  if(TEST_MODE STREQUAL "allocation")
    set(expected_code -272)
  else()
    set(expected_code -76402)
  endif()
  string(REGEX MATCH "Code: +${expected_code}[^\r\n]*C2-final-flush-error: cli-backing.h5:/Values" flush_line "${output}")
  if(flush_line STREQUAL "")
    message(FATAL_ERROR "Missing final checked error with exact backing context:\n${output}")
  endif()
  string(FIND "${output}" "C2-final-flush-warning" flush_warning)
  if(flush_warning LESS 0)
    message(FATAL_ERROR "The final flush warning was lost:\n${output}")
  endif()
  if(TEST_MODE STREQUAL "execute_and_flush")
    string(FIND "${output}" "C2-original-execute-error" original_position)
    string(FIND "${output}" "C2-final-flush-error" flush_position)
    if(original_position LESS 0 OR original_position GREATER_EQUAL flush_position)
      message(FATAL_ERROR "Original execute error did not precede the completion error:\n${output}")
    endif()
  endif()
else()
  if(TEST_MODE STREQUAL "snapshot_allocation")
    set(expected_code -272)
  else()
    set(expected_code -6071)
  endif()
  string(REGEX MATCHALL "Code: +${expected_code}([^0-9]|$)" snapshot_errors "${output}")
  list(LENGTH snapshot_errors error_count)
  if(TEST_MODE STREQUAL "root_snapshot")
    set(expected_count 1)
  else()
    set(expected_count 2)
  endif()
  if(NOT error_count EQUAL expected_count)
    message(FATAL_ERROR "Expected ${expected_count} snapshot diagnostics, got ${error_count}:\n${output}")
  endif()
  string(REGEX MATCHALL "Pipeline completion errors" root_headings "${output}")
  list(LENGTH root_headings root_heading_count)
  if(NOT root_heading_count EQUAL 1)
    message(FATAL_ERROR "The root completion detail was missing or duplicated:\n${output}")
  endif()
endif()
message(STATUS "Completion mode ${TEST_MODE} returned the expected pipeline error and diagnostic evidence.")
