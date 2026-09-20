if(NOT DEFINED NXRUNNER_EXECUTABLE)
  message(FATAL_ERROR "NXRUNNER_EXECUTABLE was not provided")
endif()

execute_process(
  COMMAND "${NXRUNNER_EXECUTABLE}" --help
  RESULT_VARIABLE help_result
  OUTPUT_VARIABLE help_stdout
  ERROR_VARIABLE help_stderr
)
if(NOT help_result EQUAL 0)
  message(FATAL_ERROR "nxrunner --help failed with ${help_result}: ${help_stdout}${help_stderr}")
endif()

set(help_output "${help_stdout}${help_stderr}")
if(NOT help_output MATCHES "--cache-memory-budget")
  message(FATAL_ERROR "nxrunner help does not advertise --cache-memory-budget: ${help_output}")
endif()
if(help_output MATCHES "(^|[ \t\r\n])--memory-budget([ \t\r\n]|$)")
  message(FATAL_ERROR "nxrunner help still advertises --memory-budget: ${help_output}")
endif()

execute_process(
  COMMAND "${NXRUNNER_EXECUTABLE}" --memory-budget 1
  RESULT_VARIABLE legacy_result
  OUTPUT_VARIABLE legacy_stdout
  ERROR_VARIABLE legacy_stderr
)
if(legacy_result EQUAL 0)
  message(FATAL_ERROR "nxrunner still accepts --memory-budget")
endif()

set(legacy_output "${legacy_stdout}${legacy_stderr}")
if(NOT legacy_output MATCHES "Failed to parse argument: --memory-budget")
  message(FATAL_ERROR "nxrunner rejected --memory-budget for the wrong reason: ${legacy_output}")
endif()

execute_process(
  COMMAND "${NXRUNNER_EXECUTABLE}" -b 1
  RESULT_VARIABLE legacy_short_result
  OUTPUT_VARIABLE legacy_short_stdout
  ERROR_VARIABLE legacy_short_stderr
)
if(legacy_short_result EQUAL 0)
  message(FATAL_ERROR "nxrunner still accepts -b")
endif()

set(legacy_short_output "${legacy_short_stdout}${legacy_short_stderr}")
if(NOT legacy_short_output MATCHES "Failed to parse argument: -b")
  message(FATAL_ERROR "nxrunner rejected -b for the wrong reason: ${legacy_short_output}")
endif()
