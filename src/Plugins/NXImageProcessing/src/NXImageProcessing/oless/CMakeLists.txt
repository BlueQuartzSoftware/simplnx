cmake_minimum_required(VERSION 3.26)

cmake_policy(SET CMP0074 NEW) # ``find_package()`` uses ``<PackageName>_ROOT`` variables.
cmake_policy(SET CMP0077 NEW) # ``option()`` honors normal variables.
cmake_policy(SET CMP0080 NEW) # ``BundleUtilities`` cannot be included at configure time.

# The ``FindPythonInterp`` and ``FindPythonLibs`` modules are removed.
# if(CMAKE_VERSION VERSION_GREATER "3.27.0")
#   cmake_policy(SET CMP0148 NEW)
# endif()

# resolves symlinks before collapsing ../ components.
if(CMAKE_VERSION VERSION_GREATER "3.28.0")
  cmake_policy(SET CMP0152 NEW)
endif()



project(oless
  VERSION 1.5.0
  DESCRIPTION ""
  HOMEPAGE_URL "https://github.com/DBHeise/oless/"
  LANGUAGES CXX
)


set(SIMPLNX_BIN_DIR ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SIMPLNX_BIN_DIR} CACHE PATH "Single Directory for all Libraries")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SIMPLNX_BIN_DIR} CACHE PATH "Single Directory for all Executables.")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SIMPLNX_BIN_DIR} CACHE PATH "Single Directory for all static libraries.")

list(APPEND CMAKE_MODULE_PATH ${simplnx_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)



set(OLESS_SRCS
    ${oless_SOURCE_DIR}/oless/common.hpp
    ${oless_SOURCE_DIR}/oless/oless.h
    #${oless_SOURCE_DIR}/oless/olessoffice.h
    ${oless_SOURCE_DIR}/oless/pole.h
    #{oless_SOURCE_DIR}/oless/vbahelper.h
)

set(OLESS_HDRS
    ${oless_SOURCE_DIR}/oless/pole.cpp
    #${oless_SOURCE_DIR}/oless/olessoffice.cpp
    ${oless_SOURCE_DIR}/oless/oless.cpp
    #${oless_SOURCE_DIR}/oless/vbahelper.cpp


)


add_executable(oless ${OLESS_SRCS} ${OLESS_HDRS} ${oless_SOURCE_DIR}/oless/program.cpp)
target_include_directories(oless PRIVATE ${oless_SOURCE_DIR})

target_compile_features(oless
                        PUBLIC
                        cxx_std_17
)

set_target_properties(oless
                      PROPERTIES
                      DEBUG_POSTFIX "_d"
)

add_executable(txm_reader ${OLESS_SRCS} ${OLESS_HDRS} ${oless_SOURCE_DIR}/txm_reader.cpp)
target_include_directories(txm_reader PRIVATE ${oless_SOURCE_DIR})

target_compile_features(txm_reader
                        PUBLIC
                        cxx_std_17
)

set_target_properties(txm_reader
                      PROPERTIES
                      DEBUG_POSTFIX "_d"
)





