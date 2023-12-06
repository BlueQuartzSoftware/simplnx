
#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
function(enable_vcpkg_manifest_feature)
  set(optionsArgs)
  set(oneValueArgs TEST_VAR FEATURE)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(${ARGS_TEST_VAR})
    if(NOT ${ARGS_FEATURE} IN_LIST VCPKG_MANIFEST_FEATURES)
      set(VCPKG_MANIFEST_FEATURES ${VCPKG_MANIFEST_FEATURES} ${ARGS_FEATURE} PARENT_SCOPE)
    endif()
  endif()
endfunction()


#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
function(install_with_directory)
  set(optionsArgs)
  set(oneValueArgs DESTINATION COMPONENT BASE_DIR)
  set(multiValueArgs FILES)
  cmake_parse_arguments(ARGS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  foreach(file ${ARGS_FILES})
      file(RELATIVE_PATH relative_file ${ARGS_BASE_DIR} ${file})
      get_filename_component(destination_dir ${relative_file} DIRECTORY)
      install(FILES ${file}
        DESTINATION ${ARGS_DESTINATION}/${destination_dir}
        COMPONENT ${ARGS_COMPONENT}
      )
  endforeach()
endfunction()

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
function(complex_enable_warnings)
  set(optionsArgs)
  set(oneValueArgs TARGET)
  set(multiValueArgs)
  cmake_parse_arguments(ARG "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT TARGET ${ARG_TARGET})
    message(FATAL_ERROR "complex_enable_warnings must be called with the argument TARGET set to a valid target")
  endif()

  if(MSVC)
    target_compile_options(${ARG_TARGET}
      PRIVATE
        # Suppressed warnings
        /wd4275 # C4275: An exported class was derived from a class that wasn't exported.
        /wd4251 # C4251: 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'

        # Warning to error
        /we4706 # C4706: assignment within conditional expression
        /we4715 # C4715: The specified function can potentially not return a value.
        /we4456 # C4456: declaration of 'identifier' hides previous local declaration
        /we4457 # C4457: declaration of 'identifier' hides function parameter
        /we4458 # C4458: declaration of 'identifier' hides class member
        /we4459 # C4459: declaration of 'identifier' hides global declaration

        /permissive- # Standards compliance
    )
  else()

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      set(SHADOW_WARNING "shadow")
    else()
      set(SHADOW_WARNING "shadow-all")
    endif()

    target_compile_options(${ARG_TARGET}
      PRIVATE
        # Warning to error
        -Werror=parentheses # Wparentheses: Warn if parentheses are omitted in certain contexts, such as when there is an assignment in a context where a truth value is expected, or when operators are nested whose precedence people often get confused about
        -Werror=return-type # Wreturn-type: Warn about any "return" statement with no return value in a function whose return type is not "void"
        -Werror=${SHADOW_WARNING} # Wshadow: Warn whenever a local variable or type declaration shadows another variable, parameter, type, class member (in C++), or instance variable (in Objective-C) or whenever a built-in function is shadowed.
    )
  
  endif()
endfunction()


#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
include(FetchContent)
include(ExternalProject)

function(download_test_data)
  set(optionsArgs INSTALL COPY_DATA)
  set(oneValueArgs DREAM3D_DATA_DIR ARCHIVE_NAME SHA512)
  set(multiValueArgs FILES)
  cmake_parse_arguments(ARGS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT COMPLEX_DOWNLOAD_TEST_FILES)
    return()
  endif()

  get_property(FETCH_FILE_PATH GLOBAL PROPERTY FETCH_FILE_PATH)

  get_filename_component(archive_base_name ${ARGS_ARCHIVE_NAME} NAME_WE)
  file(TO_CMAKE_PATH "${complex_BINARY_DIR}/TestFiles" test_files_dir)
  file(TO_CMAKE_PATH "${ARGS_DREAM3D_DATA_DIR}" ARGS_DREAM3D_DATA_DIR)
  #----------------------------------------------------------------------------
  # Create the custom CMake File for this archive file
  #----------------------------------------------------------------------------
  get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(is_multi_config)
    set(CX_CONFIG_DIR "\${CONFIG}")
  else()
    set(CX_CONFIG_DIR ".")
  endif()
  set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data")

  set(fetch_data_file "${test_files_dir}/${ARGS_ARCHIVE_NAME}.cmake")
  # Strip off the .tar.gz extension
  string(REPLACE ".tar.gz" "" ARCHIVE_BASE_NAME "${ARGS_ARCHIVE_NAME}")

  configure_file(${complex_SOURCE_DIR}/cmake/FetchDataFile.cmake.in
                ${fetch_data_file}
                @ONLY
  )
  # Read the file back into a string 
  file(READ "${fetch_data_file}" FETCH_FILE_CONTENTS)
  # Append the string to the master file
  file(APPEND "${FETCH_FILE_PATH}" "${FETCH_FILE_CONTENTS}")
  file(REMOVE "${fetch_data_file}") # Remove the temporary file

  if(ARGS_COPY_DATA)
    configure_file(${complex_SOURCE_DIR}/cmake/CopyDataFile.cmake.in
                   ${fetch_data_file}
                   @ONLY
                   )
    file(READ "${fetch_data_file}" FETCH_FILE_CONTENTS)
    file(APPEND "${FETCH_FILE_PATH}" "${FETCH_FILE_CONTENTS}")
    file(REMOVE "${fetch_data_file}")
  endif()

  #-----
  # If we are installing the data files then we need to create a custom install
  # rule to ensure the archive contents are actually decompressed and available. Since
  # we are possibly making a copy into the binary directory, use that as the source
  # location of the data. Running the unit tests might remove the decompressed data
  # as a by product.
  if(ARGS_INSTALL)
    # If we did NOT already copy the data, then do that now during the build
    if(NOT ARGS_COPY_DATA)
      configure_file(${complex_SOURCE_DIR}/cmake/CopyDataFile.cmake.in
                    ${fetch_data_file}
                    @ONLY
                    )
      file(READ "${fetch_data_file}" FETCH_FILE_CONTENTS)
      file(APPEND "${FETCH_FILE_PATH}" "${FETCH_FILE_CONTENTS}")
      file(REMOVE "${fetch_data_file}")
    endif()

    if(is_multi_config)
      set(CX_CONFIG_DIR "$<CONFIG>")
    else()
      set(CX_CONFIG_DIR ".")
    endif()
    set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data")    
    install(DIRECTORY
            "${DATA_DEST_DIR}/${ARCHIVE_BASE_NAME}"
            DESTINATION Data/
            COMPONENT Applications
            )

  endif()

endfunction()

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
function(create_data_copy_rules)
  set(optionsArgs)
  set(oneValueArgs DREAM3D_DATA_DIR)
  set(multiValueArgs FILES)
  cmake_parse_arguments(ARGS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  message(STATUS "DREAM3D_DATA_DIR: ${ARGS_DREAM3D_DATA_DIR}")
  # If the data directory does not exist then bail out now.
  if(NOT EXISTS "${ARGS_DREAM3D_DATA_DIR}")
    message(STATUS "DREAM3D_Data directory does not exist. Not creating copy and install rules.")
    return()
  endif()

  get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(is_multi_config)
    set(CX_CONFIG_DIR "$<CONFIG>")
  else()
    set(CX_CONFIG_DIR ".")
  endif()

  set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data/")
  if(EXISTS "${ARGS_DREAM3D_DATA_DIR}/Data")
    add_custom_target(DataFolderCopy ALL
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${ARGS_DREAM3D_DATA_DIR}/Data ${DATA_DEST_DIR}
      COMMENT "Copying Data Folder into Binary Directory"
      COMMAND_EXPAND_LISTS
      VERBATIM
    )
    set_target_properties(DataFolderCopy PROPERTIES FOLDER ZZ_COPY_FILES)

    set(DREAM3D_DATA_DIRECTORIES
      ${ARGS_DREAM3D_DATA_DIR}/Data/Image
      ${ARGS_DREAM3D_DATA_DIR}/Data/Models
    )

    set(COMPLEX_DATA_INSTALL_DIR "Data")

    # NOTE: If we are creating an Anaconda install the install directory WILL be different
    foreach(data_dir ${DREAM3D_DATA_DIRECTORIES})
      if(EXISTS ${data_dir})
        install(DIRECTORY
          ${data_dir}
          DESTINATION ${COMPLEX_DATA_INSTALL_DIR}
          COMPONENT Applications
        )
      endif()
    endforeach()
  endif()
endfunction()

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
function(pad_string output str padchar length)
  string(LENGTH "${str}" _strlen)
  math(EXPR _strlen "${length} - ${_strlen}")

  if(_strlen GREATER 0)
    string(REPEAT ${padchar} ${_strlen} _pad)
    string(APPEND ${_pad} str )
  endif()

  set(${output} "${_pad}" PARENT_SCOPE)
endfunction()

#------------------------------------------------------------------------------
# This function creates a CTest for each Pipeline path passed into it.
#------------------------------------------------------------------------------
function(create_pipeline_tests)
  set(optionsArgs)
  set(oneValueArgs PLUGIN_NAME)
  set(multiValueArgs PIPELINE_LIST)
  cmake_parse_arguments(ARGS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  
  set(TEST_PIPELINE_LIST_FILE ${${ARGS_PLUGIN_NAME}_BINARY_DIR}/test/Prebuilt_Pipeline_Tests.txt)
  FILE(WRITE ${TEST_PIPELINE_LIST_FILE} "# ${ARGS_PLUGIN_NAME} Example Pipelines Test List")

  set(TEST_WORKING_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
  if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(TEST_WORKING_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>")
  endif()


  get_target_property(PIPELINE_RUNNER_NAME nxrunner NAME)
  get_target_property(PIPELINE_RUNNER_DEBUG nxrunner DEBUG_POSTFIX)
  
  set(test_index  "0")
  foreach(pipeline_file_path ${ARGS_PIPELINE_LIST} )
    math(EXPR test_index "${test_index} + 1")
    # get the padding string to prefix in front of the test_index
    pad_string(padding "${test_index}" "0" "3")

    FILE(APPEND ${TEST_PIPELINE_LIST_FILE} "[${padding}${test_index}]    ${pipeline_file_path}\n")
    
    get_filename_component(test_file_name ${pipeline_file_path} NAME_WE)
    string(REPLACE "/" "-" test_file_name "${test_file_name}")

    add_test(NAME "${ARGS_PLUGIN_NAME} ${padding}${test_index} ${test_file_name}"
            COMMAND "${PIPELINE_RUNNER_NAME}$<$<CONFIG:Debug>:${PIPELINE_RUNNER_DEBUG}>" --execute ${pipeline_file_path}
            #CONFIGURATIONS Debug
            WORKING_DIRECTORY ${TEST_WORKING_DIR})
  endforeach()

endfunction()

#-------------------------------------------------------------------------------
# This function generates a file ONLY if the MD5 between the "to be" generated file
# and the current file are different. This will help reduce recompiles based on
# the generation of files that are really the same.
#-------------------------------------------------------------------------------
function(cmpConfigureFileWithMD5Check)
    set(options)
    set(oneValueArgs CONFIGURED_TEMPLATE_PATH GENERATED_FILE_PATH )
    cmake_parse_arguments(GVS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

 #   message(STATUS "   GVS_CONFIGURED_TEMPLATE_PATH: ${GVS_CONFIGURED_TEMPLATE_PATH}")
 #   message(STATUS "   GVS_GENERATED_FILE_PATH: ${GVS_GENERATED_FILE_PATH}")

    # Only Generate a file if it is different than what is already there.
    if(EXISTS ${GVS_GENERATED_FILE_PATH} )
        file(MD5 ${GVS_GENERATED_FILE_PATH} VERSION_HDR_MD5)
        configure_file(${GVS_CONFIGURED_TEMPLATE_PATH}   ${GVS_GENERATED_FILE_PATH}_tmp  )

        file(MD5 ${GVS_GENERATED_FILE_PATH}_tmp VERSION_GEN_HDR_MD5)
        #message(STATUS "  File Exists, doing MD5 Comparison")

        # Compare the MD5 checksums. If they are different then configure the file into the proper location
        if(NOT "${VERSION_HDR_MD5}" STREQUAL "${VERSION_GEN_HDR_MD5}")
            #message(STATUS "   ${VERSION_GEN_HDR_MD5}")
            #message(STATUS "   ${VERSION_HDR_MD5}")
            #message(STATUS "  Files differ: Replacing with newly generated file")
            configure_file(${GVS_CONFIGURED_TEMPLATE_PATH}  ${GVS_GENERATED_FILE_PATH} )
        else()
            #message(STATUS "  NO Difference in Files")
        endif()
        file(REMOVE ${GVS_GENERATED_FILE_PATH}_tmp)
    else()
      # message(STATUS "  File does NOT Exist, Generating one...")
      configure_file(${GVS_CONFIGURED_TEMPLATE_PATH} ${GVS_GENERATED_FILE_PATH} )
    endif()

endfunction()



#-------------------------------------------------------------------------------
# We are going to use Git functionality to create a version number for our package
# The MAJOR.MINOR.PATCH is based off of YYYY.MM.DD
# The TWEAK is the git hash of project.
#-------------------------------------------------------------------------------
function(cmpBuildDateRevisionString)
  set(options)
  set(oneValueArgs GENERATED_HEADER_FILE_PATH GENERATED_SOURCE_FILE_PATH
                   NAMESPACE PROJECT_NAME EXPORT_MACRO VERSION_MACRO_PATH STRING_CLASS STRING_INCLUDE)
  cmake_parse_arguments(GVS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if(NOT DEFINED GVS_STRING_CLASS)
    set(GVS_STRING_CLASS "std::string")
  endif()

  if(NOT DEFINED GVS_STRING_INCLUDE)
    set(GVS_STRING_INCLUDE "<string>")
  endif()

  if(0)
    message(STATUS "--------------------------------------------")
    message(STATUS "GVS_NAMESPACE: ${GVS_NAMESPACE}")
    message(STATUS "GVS_PROJECT_NAME: ${GVS_PROJECT_NAME}")
    message(STATUS "GVS_GENERATED_HEADER_FILE_PATH: ${GVS_GENERATED_HEADER_FILE_PATH}")
    message(STATUS "GVS_GENERATED_SOURCE_FILE_PATH: ${GVS_GENERATED_SOURCE_FILE_PATH}")
    message(STATUS "GVS_PROJECT_VERSION_MAJOR: ${GVS_PROJECT_VERSION_MAJOR}")
    message(STATUS "GVS_EXPORT_MACRO: ${GVS_EXPORT_MACRO}")
    message(STATUS "${GVS_PROJECT_NAME}_BUILD_DATE: ${${GVS_PROJECT_NAME}_BUILD_DATE}")
    message(STATUS "${GVS_PROJECT_NAME}_SOURCE_DIR: ${${GVS_PROJECT_NAME}_SOURCE_DIR}")
    message(STATUS "--------------------------------------------")
  endif()

  string(STRIP "${${GVS_PROJECT_NAME}_BUILD_DATE}" DVERS)
  string(REPLACE  "/" "-" DVERS "${DVERS}")
  # Run 'git describe' to get our tag offset
  # execute_process(COMMAND ${GIT_EXECUTABLE} describe --long
  #                 OUTPUT_VARIABLE DVERS
  #                 RESULT_VARIABLE did_run
  #                 ERROR_VARIABLE git_error
  #                 WORKING_DIRECTORY ${${GVS_PROJECT_NAME}_SOURCE_DIR} )

  #message(STATUS "DVERS: ${DVERS}")
  set(PROJECT_PREFIX "${GVS_PROJECT_NAME}")
  set(VERSION_GEN_NAME "${GVS_PROJECT_NAME}")
  set(VERSION_GEN_NAMESPACE "${GVS_NAMESPACE}")
  string(TOLOWER "${VERSION_GEN_NAMESPACE}" VERSION_INCLUDE_GUARD)
  set(VERSION_GEN_NAMESPACE_EXPORT "${GVS_EXPORT_MACRO}")
  set(VERSION_GEN_VER_MAJOR   ${${GVS_PROJECT_NAME}_VERSION_MAJOR})
  set(VERSION_GEN_VER_MINOR   ${${GVS_PROJECT_NAME}_VERSION_MINOR})
  set(VERSION_GEN_VER_PATCH   ${${GVS_PROJECT_NAME}_VERSION_PATCH})
  set(VERSION_GEN_VER_SUFFIX  ${${GVS_PROJECT_NAME}_VERSION_SUFFIX})
  set(VERSION_GEN_GIT_HASH_SHORT "0")

  string(TIMESTAMP VERSION_BUILD_DATE "%Y/%m/%d")

  set(VERSION_GEN_HEADER_FILE_NAME ${GVS_GENERATED_HEADER_FILE_PATH})

  #-- Make sure that actually worked and if not just generate some dummy values
  if(NOT "${DVERS}" STREQUAL "")
    string(STRIP ${DVERS} DVERS)
    string(REPLACE  "-" ";" VERSION_LIST ${DVERS})
    list(LENGTH VERSION_LIST VERSION_LIST_LENGTH)

    set(VERSION_GEN_VER_PATCH "0")
    set(VERSION_GEN_GIT_HASH_SHORT "0")

    list(LENGTH VERSION_LIST LIST_LENGTH)
    if(LIST_LENGTH GREATER 1)
      list(GET VERSION_LIST 0 VERSION_GEN_VER_MAJOR)
      list(GET VERSION_LIST 1 VERSION_GEN_VER_MINOR)
      list(GET VERSION_LIST 2 VERSION_GEN_VER_PATCH)
    endif()

  endif()

  find_package(Git)
  # Run 'git rev-parse --short HEAD' to get our revision
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                  OUTPUT_VARIABLE DVERS
                  RESULT_VARIABLE did_run
                  ERROR_VARIABLE git_error
                  WORKING_DIRECTORY ${${GVS_PROJECT_NAME}_SOURCE_DIR} )
  string(STRIP "${DVERS}" DVERS)
  if(DVERS STREQUAL "")
    message(STATUS "[${GVS_PROJECT_NAME}] 'git rev-parse --short HEAD' did not return anything valid")
    set(VERSION_GEN_GIT_HASH_SHORT "000000000000")
  else()
    set(VERSION_GEN_GIT_HASH_SHORT "${DVERS}")
  endif()

  set(${GVS_PROJECT_NAME}_VERSION_MAJOR "${VERSION_GEN_VER_MAJOR}" PARENT_SCOPE)
  set(${GVS_PROJECT_NAME}_VERSION_MINOR "${VERSION_GEN_VER_MINOR}" PARENT_SCOPE)
  set(${GVS_PROJECT_NAME}_VERSION_PATCH "${VERSION_GEN_VER_PATCH}" PARENT_SCOPE)
  set(${GVS_PROJECT_NAME}_VERSION_TWEAK "${VERSION_GEN_GIT_HASH_SHORT}" PARENT_SCOPE)

  set(CMP_TOP_HEADER_INCLUDE_STATMENT "")
  if(NOT "${CMP_TOP_HEADER_FILE}" STREQUAL "")
    set(CMP_TOP_HEADER_INCLUDE_STATMENT "#include \"${CMP_TOP_HEADER_FILE}\"")
  endif()

  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --verify HEAD
                  OUTPUT_VARIABLE GVS_GIT_HASH
                  RESULT_VARIABLE did_run
                  ERROR_VARIABLE git_error
                  WORKING_DIRECTORY ${${GVS_PROJECT_NAME}_SOURCE_DIR} 
  )
  string(REPLACE "\n" "" GVS_GIT_HASH "${GVS_GIT_HASH}")
  set_property(GLOBAL PROPERTY ${GVS_PROJECT_NAME}_GIT_HASH ${GVS_GIT_HASH})

  execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --pretty='%cd' --date=format:%Y-%m-%d-%H:%M:%S
                  OUTPUT_VARIABLE GVS_GIT_COMMIT_DATE
                  RESULT_VARIABLE did_run
                  ERROR_VARIABLE git_error
                  WORKING_DIRECTORY ${${GVS_PROJECT_NAME}_SOURCE_DIR} 
  )
  string(REPLACE "\n" "" GVS_GIT_COMMIT_DATE "${GVS_GIT_COMMIT_DATE}")
  set_property(GLOBAL PROPERTY ${GVS_PROJECT_NAME}_GIT_COMMIT_DATE ${GVS_GIT_COMMIT_DATE})

  if(NOT "${GVS_GENERATED_HEADER_FILE_PATH}" STREQUAL "")
    # message(STATUS "Generating: ${${GVS_PROJECT_NAME}_BINARY_DIR}/${GVS_GENERATED_HEADER_FILE_PATH}")
    cmpConfigureFileWithMD5Check( GENERATED_FILE_PATH        ${${GVS_PROJECT_NAME}_BINARY_DIR}/${GVS_GENERATED_HEADER_FILE_PATH}
                                  CONFIGURED_TEMPLATE_PATH     ${CMP_VERSION_HDR_TEMPLATE_FILE} )
  endif()
  
  if(NOT "${GVS_GENERATED_SOURCE_FILE_PATH}" STREQUAL "")
    # message(STATUS "Generating: ${${GVS_PROJECT_NAME}_BINARY_DIR}/${GVS_GENERATED_SOURCE_FILE_PATH}")
    cmpConfigureFileWithMD5Check( GENERATED_FILE_PATH        ${${GVS_PROJECT_NAME}_BINARY_DIR}/${GVS_GENERATED_SOURCE_FILE_PATH}
                                  CONFIGURED_TEMPLATE_PATH     ${CMP_VERSION_SRC_TEMPLATE_FILE} )
  endif()
  
  if(NOT "${GVS_VERSION_MACRO_PATH}" STREQUAL "")
    # message(STATUS "Generating: ${${GVS_PROJECT_NAME}_BINARY_DIR}/${GVS_VERSION_MACRO_PATH}")
    cmpConfigureFileWithMD5Check( GENERATED_FILE_PATH        ${${GVS_PROJECT_NAME}_BINARY_DIR}/${GVS_VERSION_MACRO_PATH}
                                  CONFIGURED_TEMPLATE_PATH     ${CMP_CONFIGURED_FILES_SOURCE_DIR}/cmpVersionMacro.h.in )
  endif()
  
endfunction()


#-------------------------------------------------------------------------------
# @Brief function AddPythonTest
# @ NAME
# @ FILE
# @ PYTHONPATH
#-------------------------------------------------------------------------------
function(AddPythonTest)
  set(options )
  set(oneValueArgs NAME FILE)
  set(multiValueArgs PYTHONPATH)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(COMPLEX_BUILD_PYTHON)
    if(WIN32)
      add_test(NAME ${ARGS_NAME}
        COMMAND ${complex_SOURCE_DIR}/wrapping/python/testing/anaconda_test.bat
      )

      set_property(TEST ${ARGS_NAME}
        PROPERTY
          ENVIRONMENT
            "PYTHON_TEST_FILE=${ARGS_FILE}"
      )
      set_property(TEST ${ARGS_NAME}
        PROPERTY
          ENVIRONMENT
            "Python3_EXECUTABLE=${Python3_EXECUTABLE}"
      )
    else()
      add_test(NAME ${ARGS_NAME}
        COMMAND ${complex_SOURCE_DIR}/wrapping/python/testing/anaconda_test.sh
      )

      set_property(TEST ${ARGS_NAME}
        PROPERTY
          ENVIRONMENT
            "PYTHON_TEST_FILE=${ARGS_FILE}"
      )
    endif()
  else()
    add_test(NAME ${ARGS_NAME}
      COMMAND ${PYTHON_EXECUTABLE} ${ARGS_FILE}
    )
  endif()

  if(WIN32)
    string(REPLACE ";" "\\;" ARGS_PYTHONPATH "${ARGS_PYTHONPATH}")
  else()
    string(REPLACE ";" ":" ARGS_PYTHONPATH "${ARGS_PYTHONPATH}")
  endif()

  set_property(TEST ${ARGS_NAME}
    APPEND
    PROPERTY
      ENVIRONMENT
      "PYTHONPATH=${ARGS_PYTHONPATH}"
      "${complex_PYTHON_TEST_ENV}"
  )
endfunction()

#-------------------------------------------------------------------------------
# @Brief function CreatePythonTests
# @ PREFIX
# @ INPUT_DIR
# @ TEST_NAMES
#-------------------------------------------------------------------------------
function(CreatePythonTests)
  set(options)
  set(oneValueArgs PREFIX INPUT_DIR)
  set(multiValueArgs TEST_NAMES)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(TESTS_PYTHONPATH
    "$<TARGET_FILE_DIR:complex>"
  )

  foreach(test ${ARGS_TEST_NAMES})
    string(REPLACE "/" "_" test_name ${test})
    set(PY_TEST_NAME ${ARGS_PREFIX}_${test_name})

    AddPythonTest(NAME ${PY_TEST_NAME}
      FILE ${ARGS_INPUT_DIR}/${test}.py
      PYTHONPATH ${TESTS_PYTHONPATH}
    )
  endforeach()
endfunction()
