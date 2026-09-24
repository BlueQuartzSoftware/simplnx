foreach(required_variable NXINFO_EXE NXINFO_INVALID_INPUT)
  if(NOT DEFINED ${required_variable})
    message(FATAL_ERROR "Required variable '${required_variable}' is not defined")
  endif()
endforeach()

execute_process(
  COMMAND "${NXINFO_EXE}" -d "${NXINFO_INVALID_INPUT}"
  RESULT_VARIABLE nxinfo_result
  OUTPUT_VARIABLE nxinfo_output
  ERROR_VARIABLE nxinfo_errors
)

if(NOT nxinfo_result EQUAL -103 AND NOT nxinfo_result EQUAL 153 AND NOT nxinfo_result EQUAL 4294967193)
  message(FATAL_ERROR "nxinfo returned '${nxinfo_result}' instead of logical status -103. stderr:\n${nxinfo_errors}")
endif()

if(NOT nxinfo_output STREQUAL "")
  message(FATAL_ERROR "nxinfo wrote unexpected stdout for an invalid file:\n${nxinfo_output}")
endif()

if(NOT nxinfo_errors MATCHES "Error -103: Failed to read the DataStructure")
  message(FATAL_ERROR "nxinfo stderr does not contain its -103 read diagnostic:\n${nxinfo_errors}")
endif()

if(nxinfo_errors MATCHES "HDF5-DIAG")
  message(FATAL_ERROR "nxinfo leaked the native HDF5 diagnostic stack:\n${nxinfo_errors}")
endif()
