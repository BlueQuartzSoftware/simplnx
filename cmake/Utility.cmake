
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
  set(fetch_data_file "${test_files_dir}/${ARGS_ARCHIVE_NAME}.cmake")
  set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/Data")
  file(MAKE_DIRECTORY "${DATA_DEST_DIR}")
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
    set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/Data")

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
      set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/Data")
      configure_file(${complex_SOURCE_DIR}/cmake/CopyDataFile.cmake.in
                    ${fetch_data_file}
                    @ONLY
                    )
      file(READ "${fetch_data_file}" FETCH_FILE_CONTENTS)
      file(APPEND "${FETCH_FILE_PATH}" "${FETCH_FILE_CONTENTS}")
      file(REMOVE "${fetch_data_file}")
    endif()

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


  set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/Data/")
  if(EXISTS "${ARGS_DREAM3D_DATA_DIR}/Data")
    add_custom_target(DataFolderCopy ALL
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${ARGS_DREAM3D_DATA_DIR}/Data ${DATA_DEST_DIR}
      COMMENT "Copying Data Folder into Binary Directory")
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

#set_tests_properties(dependsTest12 PROPERTIES DEPENDS "baseTest1;baseTest2")
