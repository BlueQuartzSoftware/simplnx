# Classifies the RESULT_VARIABLE returned by execute_process() for the nxrunner
# out-of-core acceptance tests. The expected -110 pipeline error appears as
# signed -110 on Windows and as the truncated exit status 146 on POSIX.
# Every other nonzero result represents an unexpected failure of the harness.

function(simplnx_classify_ooc_process_status process_status classification_output)
  if("${process_status}" STREQUAL "0")
    set(${classification_output} "SUCCESS" PARENT_SCOPE)
  elseif(WIN32 AND "${process_status}" STREQUAL "-110")
    set(${classification_output} "EXPECTED_FAILURE" PARENT_SCOPE)
  elseif(UNIX AND "${process_status}" STREQUAL "146")
    set(${classification_output} "EXPECTED_FAILURE" PARENT_SCOPE)
  else()
    set(${classification_output} "ABNORMAL" PARENT_SCOPE)
  endif()
endfunction()

if(RUN_OOC_PROCESS_STATUS_CLASSIFIER_TESTS)
  function(require_ooc_process_status process_status expected_classification)
    simplnx_classify_ooc_process_status("${process_status}" actual_classification)
    if(NOT actual_classification STREQUAL expected_classification)
      message(FATAL_ERROR
        "Expected process status '${process_status}' to classify as ${expected_classification}, "
        "but it classified as ${actual_classification}"
      )
    endif()
  endfunction()

  require_ooc_process_status("0" "SUCCESS")
  if(WIN32)
    require_ooc_process_status("-110" "EXPECTED_FAILURE")
    require_ooc_process_status("146" "ABNORMAL")
  elseif(UNIX)
    require_ooc_process_status("146" "EXPECTED_FAILURE")
    require_ooc_process_status("-110" "ABNORMAL")
  else()
    message(FATAL_ERROR "OOC process status classification is unsupported on this platform")
  endif()

  # Unrelated application errors and Windows structured-exception encodings are abnormal.
  require_ooc_process_status("1" "ABNORMAL")
  require_ooc_process_status("3" "ABNORMAL")
  require_ooc_process_status("1073741845" "ABNORMAL") # STATUS_FATAL_APP_EXIT
  require_ooc_process_status("2147483648" "ABNORMAL")
  require_ooc_process_status("3221225477" "ABNORMAL")
  require_ooc_process_status("-1073741819" "ABNORMAL")

  # Alternate zero spellings and textual statuses are not normal exits from execute_process().
  require_ooc_process_status("00" "ABNORMAL")
  require_ooc_process_status("-0" "ABNORMAL")
  require_ooc_process_status("Access violation" "ABNORMAL")
  require_ooc_process_status("Segmentation fault" "ABNORMAL")

  message(STATUS "OOC process status classifier cases passed.")
endif()
