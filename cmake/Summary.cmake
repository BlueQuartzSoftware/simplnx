function(OSInformation)
    if(APPLE)
        exec_program(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
        string(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})

        if(DARWIN_VERSION EQUAL 19) # macOS 10.15 Catalina (Xcode 11.x or Xcode 12.x)
            # message(STATUS "Found macOS 10.15 Catalina as the host. Darwin Version:${DARWIN_VERSION}")
            set(CMAKE_MACOS_NAME "Catalina")
            set(CMAKE_MACOS_VERSION "10.15")
        endif()

        if(DARWIN_VERSION EQUAL 20) # macOS 11.00 Big Sur (Xcode 12.x)
            # message(STATUS "Found macOS 11.00 Big Sur as the host. Darwin Version:${DARWIN_VERSION}")
            set(CMAKE_MACOS_NAME "Big Sur")
            set(CMAKE_MACOS_VERSION "11.00")
        endif()

        IF (DARWIN_VERSION EQUAL 21) # macOS 12.00 Monterey (Xcode 12.x/Xcode 13.x)
            set(CMAKE_MACOS_NAME "Monterey")
            set(CMAKE_MACOS_VERSION "12.00")
        ENDIF ()

        IF (DARWIN_VERSION EQUAL 22) # macOS 13.00 Ventura (Xcode 13.x/Xcode 14.x)
            set(CMAKE_MACOS_NAME "Ventura")
            set(CMAKE_MACOS_VERSION "13.00")
        ENDIF ()

        message(STATUS "* System: MacOS ${CMAKE_MACOS_VERSION} ${CMAKE_MACOS_NAME} Running on ${CMAKE_SYSTEM_PROCESSOR}")
    else()
        message(STATUS "* System: ${CMAKE_SYSTEM_NAME}")
        message(STATUS "* Version: ${CMAKE_SYSTEM_VERSION}")
        message(STATUS "* Processor: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

endfunction()

#------------------------------------------------------------------------------------
# Print out a Summary Section:
message(STATUS "* ============= COMPLEX Configuration Summary ===============")
message(STATUS "* BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "* COMPLEX: ${complex_VERSION}")
OSInformation()
message(STATUS "* -------------- Dependent Libraries -------------------------------------------")
message(STATUS "* Eigen (${Eigen3_VERSION}) ${Eigen3_DIR}")
message(STATUS "* HDF5 (${HDF5_VERSION}) ${HDF5_INSTALL}")
message(STATUS "* ITK (${ITK_VERSION}) ${ITK_DIR}")
message(STATUS "* Pybind11 (${pybind11_VERSION}) ${pybind11_DIR}")
message(STATUS "* TBB (${TBB_VERSION}) ${TBB_DIR}")
message(STATUS "* fmt (${fmt_VERSION}) ${fmt_DIR}")
message(STATUS "* nlohmann_json (${nlohmann_json_VERSION}) ${nlohmann_json_DIR}")
message(STATUS "* expected-lite (${expected-lite_VERSION}) ${expected-lite_DIR}")
message(STATUS "* span-lite (${span-lite_VERSION}) ${span-lite_DIR}")
message(STATUS "* boost_mp11 (${boost_mp11_VERSION}) ${boost_mp11_DIR}")
message(STATUS "* nod (${nod_VERSION}) ${nod_DIR}")
message(STATUS "* reproc++ (${reproc_VERSION}) ${reproc++_DIR}")

message(STATUS "* -------------- Complex Configuration Options -------------------------------------")
message(STATUS "* COMPLEX_BUILD_PYTHON: ${COMPLEX_BUILD_PYTHON}")
message(STATUS "* COMPLEX_BUILD_TESTS: ${COMPLEX_BUILD_TESTS}")
message(STATUS "* COMPLEX_ENABLE_MULTICORE: ${COMPLEX_ENABLE_MULTICORE}")
message(STATUS "* COMPLEX_ENABLE_COMPRESSORS: ${COMPLEX_ENABLE_COMPRESSORS}")
message(STATUS "* COMPLEX_DOWNLOAD_TEST_FILES: ${COMPLEX_DOWNLOAD_TEST_FILES}")
message(STATUS "* COMPLEX_WRITE_TEST_OUTPUT: ${COMPLEX_WRITE_TEST_OUTPUT}")
message(STATUS "* COMPLEX_ENABLE_ComplexCore: ${COMPLEX_ENABLE_ComplexCore}")
message(STATUS "* COMPLEX_ENABLE_ITKImageProcessing: ${COMPLEX_ENABLE_ITKImageProcessing}")
message(STATUS "* COMPLEX_ENABLE_OrientationAnalysis: ${COMPLEX_ENABLE_OrientationAnalysis}")
message(STATUS "* COMPLEX_ENABLE_LINK_FILESYSTEM: ${COMPLEX_ENABLE_LINK_FILESYSTEM}")
message(STATUS "* COMPLEX_ENABLE_INSTALL: ${COMPLEX_ENABLE_INSTALL}")
message(STATUS "* COMPLEX_ENABLE_PACKAGING: ${COMPLEX_ENABLE_PACKAGING}")
message(STATUS "* COMPLEX_BUILD_DOCS: ${COMPLEX_BUILD_DOCS}")
message(STATUS "* DREAM3D_DATA_DIR: ${DREAM3D_DATA_DIR}")

set(ALL_PROJECTS complex)
foreach(proj ${ALL_PROJECTS})
    # cmpGitRevisionString(PROJECT_NAME ${proj})
    get_property(githash GLOBAL PROPERTY ${proj}_GIT_HASH)
    message(STATUS "* ${proj}: ${${proj}_SOURCE_DIR}  Git Hash: ${githash}")
endforeach()
message(STATUS "* -------------- Plugins ------------------------------------------------------")

get_property(ComplexPluginTargets GLOBAL PROPERTY ComplexPluginTargets)
foreach(d3dPlugin ${ComplexPluginTargets})
    get_property(PluginNumFilters GLOBAL PROPERTY ${d3dPlugin}_filter_count)
    get_property(PluginGitHash GLOBAL PROPERTY ${d3dPlugin}_GIT_HASH)
    get_property(PluginCommitDate GLOBAL PROPERTY ${d3dPlugin}_GIT_COMMIT_DATE)
    get_property(PluginDocsEnabled GLOBAL PROPERTY ${d3dPlugin}_docs_enabled)
    message(STATUS "* ${d3dPlugin}: [${COMPLEX_PLUGIN_ENABLE_${d3dPlugin}}] ${PluginNumFilters} Filters  Docs:[${PluginDocsEnabled}]")
endforeach()
message(STATUS "* ======================================================================")
