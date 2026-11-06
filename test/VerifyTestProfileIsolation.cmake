if(NOT DEFINED PROFILE_TEST_EXECUTABLE OR NOT EXISTS "${PROFILE_TEST_EXECUTABLE}")
  message(FATAL_ERROR "PROFILE_TEST_EXECUTABLE must name an existing Catch executable")
endif()
if(NOT DEFINED PROFILE_TEST_ROOT OR NOT IS_ABSOLUTE "${PROFILE_TEST_ROOT}")
  message(FATAL_ERROR "PROFILE_TEST_ROOT must name an absolute test artifact directory")
endif()

# Each invocation owns a new fixture. Failed runs retain it for diagnosis.
string(RANDOM LENGTH 24 ALPHABET 0123456789abcdef nonce)
set(case_root "${PROFILE_TEST_ROOT}/sentinel-${nonce}")
if(EXISTS "${case_root}")
  message(FATAL_ERROR "Refusing to reuse an existing sentinel fixture: ${case_root}")
endif()
set(fake_home "${case_root}/fake-ambient")
if(WIN32)
  set(preferences_dir "${fake_home}/AppData/Local/DREAM3DNX")
  set(home_variable "USERPROFILE=${fake_home}")
elseif(APPLE)
  set(preferences_dir "${fake_home}/Library/Preferences/DREAM3DNX")
  set(home_variable "HOME=${fake_home}")
else()
  set(preferences_dir "${fake_home}/.config/DREAM3DNX")
  set(home_variable "HOME=${fake_home}")
endif()
file(MAKE_DIRECTORY "${preferences_dir}")
set(preferences_file "${preferences_dir}/preferences.json")
set(expected_json "{\"plugins\":{},\"profile_isolation_sentinel\":\"${nonce}\"}")
file(WRITE "${preferences_file}" "${expected_json}")
file(WRITE "${case_root}/expected-preferences.json" "${expected_json}")
file(WRITE "${case_root}/.profile-driver" "${nonce}")

# Redirect only the child. Never read or write the developer's ambient profile.
# Inherited test requests must not select an unrelated caller-owned fixture.
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env
    --unset=SIMPLNX_TEST_PROFILE_DIR
    --unset=SIMPLNX_TEST_PROFILE_TOKEN
    --unset=SIMPLNX_TEST_ACTIVE_PROFILE
    --unset=SIMPLNX_TEST_PROFILE_REPORT
    "${home_variable}"
    "${PROFILE_TEST_EXECUTABLE}" "Application::savePreferences" --reporter compact --warn NoTests
  WORKING_DIRECTORY "${case_root}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
  TIMEOUT 120
)
file(WRITE "${case_root}/stdout.txt" "${output}")
file(WRITE "${case_root}/stderr.txt" "${error}")
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "Application save case failed with '${result}'. Evidence: ${case_root}\n${output}\n${error}")
endif()

if(NOT output MATCHES "Passed 1 test case")
  message(FATAL_ERROR "Expected exactly one passing Application save case. Evidence: ${case_root}\n${output}")
endif()

file(READ "${preferences_file}" actual_json)
if(NOT actual_json STREQUAL expected_json)
  string(JSON written_value ERROR_VARIABLE witness_error GET "${actual_json}" test_key)
  message(FATAL_ERROR
    "Application::savePreferences changed the fake ambient preference bytes. "
    "The existing save case writes test_key=42; observed '${written_value}' (JSON status '${witness_error}'). "
    "Evidence: ${case_root}\nExpected: ${expected_json}\nActual: ${actual_json}")
endif()

# The driver removes only its exact fixture after the child and byte comparison succeed.
file(REAL_PATH "${PROFILE_TEST_ROOT}" resolved_parent)
file(REAL_PATH "${case_root}" resolved_root)
get_filename_component(actual_parent "${resolved_root}" DIRECTORY)
file(READ "${case_root}/.profile-driver" actual_token)
if(NOT actual_parent STREQUAL resolved_parent OR NOT actual_token STREQUAL nonce OR IS_SYMLINK "${case_root}")
  message(FATAL_ERROR "Sentinel fixture ownership changed: ${case_root}")
endif()
file(REMOVE_RECURSE "${resolved_root}")
if(EXISTS "${resolved_root}")
  message(FATAL_ERROR "Cannot remove completed sentinel fixture: ${resolved_root}")
endif()
