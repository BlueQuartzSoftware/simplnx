cmake_minimum_required(VERSION 3.19)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Utility.cmake)

option(COMPLEX_BUILD_PYTHON "Enable building Python building" OFF)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_BUILD_PYTHON FEATURE "python")

option(COMPLEX_BUILD_TESTS "Enable building COMPLEX tests" ON)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_BUILD_TESTS FEATURE "tests")

project(complex
  VERSION 0.1.0
  DESCRIPTION "SIMPL Redesign"
  HOMEPAGE_URL "https://github.com/BlueQuartzSoftware/complex"
  LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(expected-lite CONFIG REQUIRED)
find_package(Eigen3 REQUIRED)

add_library(complex SHARED)
add_library(complex::complex ALIAS complex)

target_compile_features(complex
  PUBLIC
    cxx_std_17
)

set_target_properties(complex
  PROPERTIES
    DEBUG_POSTFIX "_d"
)

target_compile_options(complex
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

target_link_libraries(complex
  PUBLIC
    fmt::fmt
    nlohmann_json::nlohmann_json
    nonstd::expected-lite
    Eigen3::Eigen
)

if(UNIX)
  target_link_libraries(complex
    PRIVATE
      ${CMAKE_DL_LIBS}
  )
endif()

set(COMPLEX_GENERATED_DIR ${PROJECT_BINARY_DIR}/generated)
set(COMPLEX_GENERATED_HEADER_DIR ${PROJECT_BINARY_DIR}/generated/complex)
set(COMPLEX_EXPORT_HEADER ${COMPLEX_GENERATED_HEADER_DIR}/complex_export.hpp)

file(READ ${PROJECT_SOURCE_DIR}/cmake/complex_export_extra.hpp COMPLEX_EXPORT_HEADER_EXTRA)

include(GenerateExportHeader)
generate_export_header(complex
  EXPORT_FILE_NAME ${COMPLEX_EXPORT_HEADER}
  CUSTOM_CONTENT_FROM_VARIABLE COMPLEX_EXPORT_HEADER_EXTRA
)

# Specify core filters and parameters here
set(CoreFilters
  ImportTextFilter
)
set(CoreParameters
  ArrayCreationParameter
  BoolParameter
  ChoicesParameter
  InputFileParameter
  NumberParameter
  NumericTypeParameter
  StringParameter
)

# Generate core filter import header
set(CreateCoreFilters "")
set(AddCoreFilters "")

foreach(filter ${CoreFilters})
  list(APPEND CoreFilterIncludes "#include \"complex/Core/Filters/${filter}.hpp\"")
  list(APPEND CreateCoreFilters "IFilter* create${filter}(){ return new ${filter}; }")
  list(APPEND AddCoreFilters "  addCoreFilter(&create${filter});")
endforeach()
configure_file(
  "cmake/ImportCoreFilters.hpp.in"
  "${COMPLEX_GENERATED_HEADER_DIR}/ImportCoreFilters.hpp"
  @ONLY
)
# End generate core filter headers

set(COMPLEX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/complex)

set(COMPLEX_HDRS
  ${COMPLEX_SOURCE_DIR}/Common/Array.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Bit.hpp
  ${COMPLEX_SOURCE_DIR}/Common/BoundingBox.hpp
  ${COMPLEX_SOURCE_DIR}/Common/EulerAngle.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Point2D.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Point3D.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Ray.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Result.hpp
  ${COMPLEX_SOURCE_DIR}/Common/RgbColor.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Types.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Uuid.hpp

  ${COMPLEX_SOURCE_DIR}/Core/Application.hpp
  ${COMPLEX_SOURCE_DIR}/Core/FilterHandle.hpp
  ${COMPLEX_SOURCE_DIR}/Core/FilterList.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/AbstractDataStructureMessage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataAddedMessage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataRemovedMessage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataRenamedMessage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataReparentedMessage.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Observers/AbstractDataStructureObserver.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/AbstractGeometry.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/AbstractGeometry2D.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/AbstractGeometry3D.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/AbstractGeometryGrid.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/EdgeGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/HexahedralGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/ImageGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/QuadGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/RectGridGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/TetrahedralGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/TriangleGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/VertexGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DynamicListArray.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/EmptyDataStore.hpp
  ${COMPLEX_SOURCE_DIR}/Plugin/AbstractPlugin.hpp
  ${COMPLEX_SOURCE_DIR}/Plugin/PluginLoader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Math/GeometryMath.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/GeometryHelpers.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractMontage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractTileIndex.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridMontage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridTileIndex.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/BaseGroup.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataArray.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataGroup.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataMap.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataObject.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataPath.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataStore.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataStructure.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IDataStore.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/LinkedPath.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Metadata.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/ScalarData.hpp

  ${COMPLEX_SOURCE_DIR}/Filter/AbstractParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Arguments.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/ConstDataParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/DataParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/IFilter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/IParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/MutableDataParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Output.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Parameters.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/ValueParameter.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipGenerator.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipRowItem.hpp
)

set(COMPLEX_GENERATED_HEADERS
  ${COMPLEX_EXPORT_HEADER}
  ${COMPLEX_GENERATED_HEADER_DIR}/ImportCoreFilters.hpp
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

  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/AbstractGeometry.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/AbstractGeometry2D.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/AbstractGeometry3D.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/AbstractGeometryGrid.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/EdgeGeom.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/HexahedralGeom.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/ImageGeom.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/QuadGeom.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/RectGridGeom.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/TetrahedralGeom.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/TriangleGeom.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/VertexGeom.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractMontage.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractTileIndex.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridMontage.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridTileIndex.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/BaseGroup.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataGroup.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataMap.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataObject.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataPath.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataStructure.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/LinkedPath.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Metadata.cpp

  ${COMPLEX_SOURCE_DIR}/Filter/AbstractParameter.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Arguments.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/ConstDataParameter.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/DataParameter.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/IFilter.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/IParameter.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/MutableDataParameter.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Output.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Parameters.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/ValueParameter.cpp

  ${COMPLEX_SOURCE_DIR}/Plugin/AbstractPlugin.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Math/GeometryMath.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipGenerator.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipRowItem.cpp
)

# Add Core Filters
foreach(filter ${CoreFilters})
  list(APPEND COMPLEX_HDRS
    "${COMPLEX_SOURCE_DIR}/Core/Filters/${filter}.hpp"
  )
  list(APPEND COMPLEX_SRCS
    "${COMPLEX_SOURCE_DIR}/Core/Filters/${filter}.cpp"
  )
endforeach()

# Add Core FilterParameters
foreach(param ${CoreParameters})
  list(APPEND COMPLEX_HDRS
    "${COMPLEX_SOURCE_DIR}/Core/Parameters/${param}.hpp"
  )
  list(APPEND COMPLEX_SRCS
    "${COMPLEX_SOURCE_DIR}/Core/Parameters/${param}.cpp"
  )
endforeach()

if(WIN32)
  list(APPEND COMPLEX_SRCS
    ${COMPLEX_SOURCE_DIR}/Plugin/PluginLoader_Win.cpp
  )
elseif(UNIX)
  list(APPEND COMPLEX_SRCS
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

if(COMPLEX_BUILD_TESTS)
  include(CTest)
  add_subdirectory(test)
endif()

if(COMPLEX_BUILD_PYTHON)
  add_subdirectory(wrapping/python)
endif()

option(COMPLEX_BUILD_DOCS "Enables building COMPLEX documentation" OFF)
if(COMPLEX_BUILD_DOCS)
  add_subdirectory(docs)
endif()
