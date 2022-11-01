
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

function(download_test_data)
  set(optionsArgs)
  set(oneValueArgs DREAM3D_DATA_DIR)
  set(multiValueArgs FILES)
  cmake_parse_arguments(ARGS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  message(STATUS "DREAM3D_DATA_DIR: ${ARGS_DREAM3D_DATA_DIR}")
  # If the data directory does not exist then bail out now.
  if(NOT EXISTS "${ARGS_DREAM3D_DATA_DIR}")
    message(STATUS "DREAM3D_Data directory does not exist. *NOT* downloading test files.")
    return()
  endif()

  message(STATUS "DREAM3D_Data: Found at '${ARGS_DREAM3D_DATA_DIR}'")
  message(STATUS "Downloading DREAM3D_Data/TestFiles")
  FetchContent_Declare(download_DREAM3D_Data
    URL https://github.com/dream3d/DREAM3D_Data/releases/download/v6_10/TestFiles_6_10.tar.gz
    URL_HASH SHA512=340cae8716d9bb3edf64bce893fd584ac08ffcb9f4758ce5347f24c79d2cfbc8ebadd16b6596c47ad8e14a2c9581f1b3ebb8bc267e7f0a774c85d20ddc210586
    SOURCE_DIR "${ARGS_DREAM3D_DATA_DIR}/TestFiles"
    DOWNLOAD_DIR "${ARGS_DREAM3D_DATA_DIR}"
  )
  FetchContent_MakeAvailable(download_DREAM3D_Data)

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
  add_custom_target(DataFolderCopy ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${ARGS_DREAM3D_DATA_DIR}/Data ${DATA_DEST_DIR}
    COMMENT "Copying Data Folder into Binary Directory")
  set_target_properties(DataFolderCopy PROPERTIES FOLDER ZZ_COPY_FILES)

  set(DREAM3D_DATA_DIRECTORIES
    ${ARGS_DREAM3D_DATA_DIR}/Data/Image
    ${ARGS_DREAM3D_DATA_DIR}/Data/Models
  )

  set(INSTALL_DESTINATION "Data")

  # NOTE: If we are creating an Anaconda install the install directory WILL be different
  foreach(data_dir ${DREAM3D_DATA_DIRECTORIES})
    if(EXISTS ${data_dir})
      install(DIRECTORY
        ${data_dir}
        DESTINATION ${INSTALL_DESTINATION}
        COMPONENT Applications
      )
    endif()
  endforeach()
endfunction()
