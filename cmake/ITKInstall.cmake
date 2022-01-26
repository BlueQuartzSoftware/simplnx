# -------------------------------------------------------------
# This function adds the necessary cmake code to find the Itk
# shared libraries and setup custom copy commands and/or install
# rules for Linux and Windows to use
function(AddItkCopyInstallRules)
  set(options )
  set(oneValueArgs )
  set(multiValueArgs LIBS)
  cmake_parse_arguments(itk "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
  set(INTER_DIR ".")

  if(APPLE)
    return()
  endif()


  if(MSVC_IDE)
    set(itk_TYPES Debug Release)
  else()
    set(itk_TYPES "${CMAKE_BUILD_TYPE}")
    if("${itk_TYPES}" STREQUAL "")
        set(itk_TYPES "Debug")
    endif()
  endif()

  set(itk_INSTALL_DIR "lib")
  if(WIN32)
    set(itk_INSTALL_DIR ".")
  endif()


  set(STACK "")
  list(APPEND STACK ${itk_LIBS})
  # message(STATUS "STACK: ${STACK}")
  # While the depth-first search stack is not empty
  list(LENGTH STACK STACK_LENGTH)
  # message(STATUS "STACK_LENGTH: ${STACK_LENGTH}")

  while(STACK_LENGTH GREATER 0)

    # This pair of commands "pops" the last value off the
    # stack and sets the STACK_LENGTH variable
    list(GET STACK 0 itk_LIBNAME)
    list(REMOVE_AT STACK 0)
    list(LENGTH STACK STACK_LENGTH)

    # See if we have found a Qt5 or System library. All Itk libs start with "Itk"
    string(REGEX MATCH "^ITK" IsItkLib ${itk_LIBNAME})
    # message(STATUS "itk_LIBNAME: ${itk_LIBNAME}: ${IsItkLib}")
    # If we have not seen this library before then find its dependencies
    if(NOT FOUND_${itk_LIBNAME})
      set(FOUND_${itk_LIBNAME} TRUE)
      if("${IsItkLib}" STREQUAL "ITK")
        # message(STATUS "${itk_LIBNAME}: ${FOUND_${itk_LIBNAME}}  IsItkLib: ${IsItkLib}")
        set(itk_LIBVAR ${itk_LIBNAME})
        foreach(BTYPE ${itk_TYPES} )
          # message(STATUS "  BTYPE: ${BTYPE}")
          string(TOUPPER ${BTYPE} UpperBType)
          if(MSVC_IDE)
            set(INTER_DIR "${BTYPE}")
          endif()

          # Find the current library's dependent Itk libraries
          get_target_property(ItkLibDeps ${itk_LIBNAME} IMPORTED_LINK_DEPENDENT_LIBRARIES_${UpperBType})
          #message(STATUS "    ItkLibDeps: ${ItkLibDeps}")
          if(NOT "${ItkLibDeps}" STREQUAL "ItkLibDeps-NOTFOUND" )
            list(APPEND STACK ${ItkLibDeps})
          else()
            # message(STATUS "---->${itk_LIBNAME} IMPORTED_LINK_DEPENDENT_LIBRARIES_${UpperBType} NOT FOUND")
          endif()

          get_target_property(ItkLibDeps ${itk_LIBNAME} INTERFACE_LINK_LIBRARIES)
          if(NOT "${ItkLibDeps}" STREQUAL "ItkLibDeps-NOTFOUND" )
            list(APPEND STACK ${ItkLibDeps})
          else()
            # message(STATUS "---->${itk_LIBNAME} INTERFACE_LINK_LIBRARIES NOT FOUND")
          endif()

          # Get the Actual Library Path and create Install and copy rules
          get_target_property(DllLibPath ${itk_LIBNAME} IMPORTED_LOCATION_${UpperBType})
          # message(STATUS "    DllLibPath: ${DllLibPath}")
          if(NOT "${DllLibPath}" STREQUAL "LibPath-NOTFOUND")
            # message(STATUS "  Creating Install Rule for ${DllLibPath}")
            if(NOT TARGET ZZ_${itk_LIBVAR}_DLL_${UpperBType}-Copy)
              add_custom_target(ZZ_${itk_LIBVAR}_DLL_${UpperBType}-Copy ALL
                                  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DllLibPath}
                                  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${INTER_DIR}/
                                  # COMMENT "  Copy: ${DllLibPath} To: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${INTER_DIR}/"
                                  )
              set_target_properties(ZZ_${itk_LIBVAR}_DLL_${UpperBType}-Copy PROPERTIES FOLDER ZZ_COPY_FILES/${BTYPE}/Itk)
              install(FILES ${DllLibPath} DESTINATION "${itk_INSTALL_DIR}" CONFIGURATIONS ${BTYPE} COMPONENT Applications)
              get_property(COPY_LIBRARY_TARGETS GLOBAL PROPERTY COPY_LIBRARY_TARGETS)
              set_property(GLOBAL PROPERTY COPY_LIBRARY_TARGETS ${COPY_LIBRARY_TARGETS} ZZ_${itk_LIBVAR}_DLL_${UpperBType}-Copy)
            endif()
          endif()

          # Now get the path that the library is in
          get_filename_component(${itk_LIBVAR}_DIR ${DllLibPath} PATH)
          # message(STATUS " ${itk_LIBVAR}_DIR: ${${itk_LIBVAR}_DIR}")

          # Now piece together a complete path for the symlink that Linux Needs to have
          if(WIN32)
            get_target_property(${itk_LIBVAR}_${UpperBType} ${itk_LIBNAME} IMPORTED_IMPLIB_${UpperBType})
          else()
            get_target_property(${itk_LIBVAR}_${UpperBType} ${itk_LIBNAME} IMPORTED_SONAME_${UpperBType})
          endif()

          #----------------------------------------------------------------------
          # This section for Linux only
          # message(STATUS "  ${itk_LIBVAR}_${UpperBType}: ${${itk_LIBVAR}_${UpperBType}}")
          if(NOT "${${itk_LIBVAR}_${UpperBType}}" STREQUAL "${itk_LIBVAR}_${UpperBType}-NOTFOUND" AND NOT WIN32)
            set(SYMLINK_PATH "${${itk_LIBVAR}_DIR}/${${itk_LIBVAR}_${UpperBType}}")
            # message(STATUS "  Creating Install Rule for ${SYMLINK_PATH}")
            if(NOT TARGET ZZ_${itk_LIBVAR}_SYMLINK_${UpperBType}-Copy)
              add_custom_target(ZZ_${itk_LIBVAR}_SYMLINK_${UpperBType}-Copy ALL
                                  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SYMLINK_PATH}
                                  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${INTER_DIR}/
                                  # COMMENT "  Copy: ${SYMLINK_PATH} To: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${INTER_DIR}/"
                                  )
              set_target_properties(ZZ_${itk_LIBVAR}_SYMLINK_${UpperBType}-Copy PROPERTIES FOLDER ZZ_COPY_FILES/${BTYPE}/Itk)
              install(FILES ${SYMLINK_PATH} DESTINATION "${itk_INSTALL_DIR}" CONFIGURATIONS ${BTYPE} COMPONENT Applications)
              get_property(COPY_LIBRARY_TARGETS GLOBAL PROPERTY COPY_LIBRARY_TARGETS)
              set_property(GLOBAL PROPERTY COPY_LIBRARY_TARGETS ${COPY_LIBRARY_TARGETS} ZZ_${itk_LIBVAR}_SYMLINK_${UpperBType}-Copy) 
            endif()
          endif()
          # End Linux Only Section
          #------------------------------------------------------------------------

        endforeach()
      endif()
    else()
      # message(STATUS "----> Already Found ${itk_LIBNAME}")
    endif()

    # Remove duplicates and set the stack_length variable (VERY IMPORTANT)
    list(REMOVE_DUPLICATES STACK)
    list(LENGTH STACK STACK_LENGTH)

  endwhile()

endfunction()