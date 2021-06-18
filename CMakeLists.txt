cmake_minimum_required(VERSION 3.19)

project(complex
  VERSION 0.1.0
  DESCRIPTION "SIMPL Redesign"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(complex SHARED)
add_library(complex::complex ALIAS complex)

target_compile_features(complex
  PUBLIC
    cxx_std_17
)

set_target_properties(complex
  PROPERTIES
    DEBUG_POSTFIX "_d"
    CMAKE_CXX_VISIBILITY_PRESET hidden
    CMAKE_VISIBILITY_INLINES_HIDDEN ON
)

set(COMPLEX_GENERATED_DIR ${PROJECT_BINARY_DIR}/generated)
set(COMPLEX_GENERATED_HEADER_DIR ${PROJECT_BINARY_DIR}/generated/complex)
set(COMPLEX_EXPORT_HEADER ${COMPLEX_GENERATED_HEADER_DIR}/complex_export.hpp)

include(GenerateExportHeader)
generate_export_header(complex
  EXPORT_FILE_NAME ${COMPLEX_EXPORT_HEADER}
)

set(COMPLEX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/complex)

set(COMPLEX_HDRS
    ${COMPLEX_SOURCE_DIR}/Common/BoundingBox.hpp
    ${COMPLEX_SOURCE_DIR}/Common/EulerAngle.hpp
    ${COMPLEX_SOURCE_DIR}/Common/Point2D.hpp
    ${COMPLEX_SOURCE_DIR}/Common/Point3D.hpp
    ${COMPLEX_SOURCE_DIR}/Common/Ray.hpp
    ${COMPLEX_SOURCE_DIR}/Common/RgbColor.hpp
    ${COMPLEX_SOURCE_DIR}/Common/Array.hpp

    ${COMPLEX_SOURCE_DIR}/Core/Application.hpp
    ${COMPLEX_SOURCE_DIR}/Core/FilterHandle.hpp
    ${COMPLEX_SOURCE_DIR}/Core/FilterList.hpp

    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/AbstractDataStructureMessage.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataAddedMessage.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataRemovedMessage.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataRenamedMessage.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataReparentedMessage.hpp

    ${COMPLEX_SOURCE_DIR}/DataStructure/Observers/AbstractDataStructureObserver.hpp

    ${COMPLEX_SOURCE_DIR}/DataStructure/BaseGroup.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataArray.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataGroup.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataMap.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataObject.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataPath.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataStore.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataStructure.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/LinkedPath.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/MetaData.hpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/ScalarData.hpp

    ${COMPLEX_SOURCE_DIR}/Plugin/AbstractPlugin.hpp
    ${COMPLEX_SOURCE_DIR}/Plugin/PluginLoader.hpp

    # ${COMPLEX_SOURCE_DIR}/Rest/PipelineController.hpp
    # ${COMPLEX_SOURCE_DIR}/Rest/PipelineHandler.hpp
    # ${COMPLEX_SOURCE_DIR}/Rest/RestManager.hpp
    # ${COMPLEX_SOURCE_DIR}/Rest/RestServer.hpp

    ${COMPLEX_SOURCE_DIR}/Utilities/Math/GeometryMath.hpp

    ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5.hpp
    
    ${COMPLEX_SOURCE_DIR}/Utilities/TooltipGenerator.hpp
    ${COMPLEX_SOURCE_DIR}/Utilities/TooltipRowItem.hpp
)

set(COMPLEX_GENERATED_HEADERS
  ${COMPLEX_EXPORT_HEADER}
)

set(COMPLEX_ALL_HDRS
  ${COMPLEX_HDRS}
  ${COMPLEX_GENERATED_HEADERS}
)

set(COMPLEX_SRCS
    ${COMPLEX_SOURCE_DIR}/Common/RgbColor.cpp

    ${COMPLEX_SOURCE_DIR}/Core/Application.cpp
    ${COMPLEX_SOURCE_DIR}/Core/FilterHandle.cpp
    ${COMPLEX_SOURCE_DIR}/Core/FilterList.cpp

    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/AbstractDataStructureMessage.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataAddedMessage.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataRemovedMessage.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataRenamedMessage.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataReparentedMessage.cpp

    ${COMPLEX_SOURCE_DIR}/DataStructure/Observers/AbstractDataStructureObserver.cpp

    ${COMPLEX_SOURCE_DIR}/DataStructure/BaseGroup.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataGroup.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataMap.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataObject.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataPath.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/DataStructure.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/LinkedPath.cpp
    ${COMPLEX_SOURCE_DIR}/DataStructure/MetaData.cpp

    ${COMPLEX_SOURCE_DIR}/Plugin/AbstractPlugin.cpp

    # ${COMPLEX_SOURCE_DIR}/Rest/PipelineController.cpp
    # ${COMPLEX_SOURCE_DIR}/Rest/PipelineHandler.cpp
    # ${COMPLEX_SOURCE_DIR}/Rest/RestManager.cpp
    # ${COMPLEX_SOURCE_DIR}/Rest/RestServer.cpp

    ${COMPLEX_SOURCE_DIR}/Utilities/TooltipGenerator.cpp
    ${COMPLEX_SOURCE_DIR}/Utilities/TooltipRowItem.cpp
)

if(WIN32)
    set(COMPLEX_SRCS
        ${COMPLEX_SRCS}
        ${COMPLEX_SOURCE_DIR}/Plugin/PluginLoader_Win.cpp
    )
elseif(UNIX)
    set(COMPLEX_SRCS
        ${COMPLEX_SRCS}
        ${COMPLEX_SOURCE_DIR}/Plugin/PluginLoader_Linux.cpp
    )
endif()

target_sources(complex
  PRIVATE
    ${COMPLEX_ALL_HDRS}
    ${COMPLEX_SRCS}
)

source_group(TREE ${COMPLEX_SOURCE_DIR} PREFIX "complex" FILES ${COMPLEX_HDRS})
source_group(TREE ${COMPLEX_GENERATED_HEADER_DIR} PREFIX "complex/generated" FILES ${COMPLEX_GENERATED_HEADERS})
source_group(TREE ${COMPLEX_SOURCE_DIR} PREFIX "complex" FILES ${COMPLEX_SRCS})

set(COMPLEX_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src)

target_include_directories(complex
  PUBLIC
    $<BUILD_INTERFACE:${COMPLEX_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${COMPLEX_GENERATED_DIR}>
    $<INSTALL_INTERFACE:include>
)

set(COMPLEX_PACKAGE_NAMESPACE "complex::")

install(TARGETS complex
  EXPORT complexTargets
)

export(EXPORT complexTargets NAMESPACE ${COMPLEX_PACKAGE_NAMESPACE})

option(COMPLEX_ENABLE_INSTALL "Enables COMPLEX install rules" ON)
if(COMPLEX_ENABLE_INSTALL)
  include(CMakePackageConfigHelpers)

  set(COMPLEX_INSTALL_CMAKE_PREFIX "lib/cmake")

  set(COMPLEX_CONFIG_FILE ${PROJECT_BINARY_DIR}/complexConfig.cmake)
  set(COMPLEX_VERSION_FILE ${PROJECT_BINARY_DIR}/complexConfigVersion.cmake)

  configure_package_config_file(${PROJECT_SOURCE_DIR}/cmake/complexConfig.cmake.in ${COMPLEX_CONFIG_FILE}
    INSTALL_DESTINATION ${COMPLEX_INSTALL_CMAKE_PREFIX}
  )

  write_basic_package_version_file(${COMPLEX_VERSION_FILE}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES ${COMPLEX_CONFIG_FILE} ${COMPLEX_VERSION_FILE}
    DESTINATION ${COMPLEX_INSTALL_CMAKE_PREFIX}
    COMPONENT develop
  )

  include(GNUInstallDirs)
  install(TARGETS complex
    RUNTIME
      DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT runtime
    LIBRARY 
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT runtime
      NAMELINK_COMPONENT develop
    ARCHIVE
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT develop
  )

  install(EXPORT complexTargets
    NAMESPACE ${COMPLEX_PACKAGE_NAMESPACE}
    DESTINATION ${COMPLEX_INSTALL_CMAKE_PREFIX}
  )

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

  install_with_directory(FILES ${COMPLEX_HDRS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/complex
    COMPONENT develop
    BASE_DIR ${COMPLEX_SOURCE_DIR}
  )

  install_with_directory(FILES ${COMPLEX_GENERATED_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/complex
    COMPONENT develop
    BASE_DIR ${COMPLEX_GENERATED_HEADER_DIR}
  )
endif()

option(COMPLEX_ENABLE_PACKAGING "Enables COMPLEX packaging" ON)
if(COMPLEX_ENABLE_PACKAGING)
  if(WIN32)
    set(CPACK_GENERATOR ZIP)
  else()
    set(CPACK_GENERATOR TGZ)
  endif()
  include(CPack)
endif()

include(CTest)
if(BUILD_TESTING)
  option(COMPLEX_BUILD_TESTS "Enable building COMPLEX tests" ON)
  if(COMPLEX_BUILD_TESTS)
    add_subdirectory(test)
  endif()
endif()
