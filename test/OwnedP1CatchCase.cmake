if(NOT DEFINED P1_TEST_EXECUTABLE OR NOT EXISTS "${P1_TEST_EXECUTABLE}")
  message(FATAL_ERROR "P1_TEST_EXECUTABLE must name an existing Catch executable")
endif()
if(NOT DEFINED P1_TEST_NAME OR P1_TEST_NAME STREQUAL "")
  message(FATAL_ERROR "P1_TEST_NAME must select exactly one hidden Catch case")
endif()
if(NOT DEFINED P1_ROOT_BASE)
  message(FATAL_ERROR "P1_ROOT_BASE was not provided")
endif()

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef nonce)
set(case_root "${P1_ROOT_BASE}/p1-${nonce}")
file(MAKE_DIRECTORY "${case_root}/temp" "${case_root}/AppData/Local" "${case_root}/Library/Preferences" "${case_root}/.config")

# This CMake process is the CTest child. Its environment changes do not affect the developer process.
set(ENV{HOME} "${case_root}")
set(ENV{USERPROFILE} "${case_root}")
set(ENV{LOCALAPPDATA} "${case_root}/AppData/Local")
set(ENV{APPDATA} "${case_root}/AppData/Roaming")
set(ENV{TEMP} "${case_root}/temp")
set(ENV{TMP} "${case_root}/temp")
set(ENV{TMPDIR} "${case_root}/temp")
set(ENV{SIMPLNX_P1_OWNED_ROOT} "${case_root}")
unset(ENV{SIMPLNX_OOC_FAULT_INJECT})

execute_process(
  COMMAND "${P1_TEST_EXECUTABLE}" "${P1_TEST_NAME}" --warn NoTests
  WORKING_DIRECTORY "${case_root}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
  TIMEOUT 120
)
file(WRITE "${case_root}/stdout.txt" "${output}")
file(WRITE "${case_root}/stderr.txt" "${error}")
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "P1 owned case '${P1_TEST_NAME}' failed with '${result}'. Evidence: ${case_root}\n${output}\n${error}")
endif()
