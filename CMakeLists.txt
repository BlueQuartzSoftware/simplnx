cmake_minimum_required(VERSION 3.19)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Utility.cmake)

option(COMPLEX_BUILD_PYTHON "Enable building Python building" OFF)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_BUILD_PYTHON FEATURE "python")

option(COMPLEX_BUILD_TESTS "Enable building COMPLEX tests" ON)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_BUILD_TESTS FEATURE "tests")

option(COMPLEX_ENABLE_MULTICORE "Enable multicore support" ON)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_ENABLE_MULTICORE FEATURE "parallel")

project(complex
  VERSION 0.1.0
  DESCRIPTION "SIMPL Redesign"
  HOMEPAGE_URL "https://github.com/BlueQuartzSoftware/complex"
  LANGUAGES CXX
)

set(COMPLEX_BIN_DIR ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${COMPLEX_BIN_DIR} CACHE PATH "Single Directory for all Libraries")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${COMPLEX_BIN_DIR} CACHE PATH "Single Directory for all Executables.")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${COMPLEX_BIN_DIR} CACHE PATH "Single Directory for all static libraries.")

list(APPEND CMAKE_MODULE_PATH ${complex_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(expected-lite CONFIG REQUIRED)
find_package(span-lite CONFIG REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(HDF5 REQUIRED)
find_package(boost_mp11 CONFIG REQUIRED)

if(COMPLEX_ENABLE_MULTICORE)
  find_package(TBB CONFIG REQUIRED)
endif()

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

complex_enable_warnings(TARGET complex)

if(MSVC)
  target_compile_options(complex
    PRIVATE
      /MP
  )
endif()

# Force HDF5 1.10 API
target_compile_definitions(complex PUBLIC "H5_USE_110_API")

if(COMPLEX_ENABLE_MULTICORE)
  target_compile_definitions(complex PUBLIC "COMPLEX_ENABLE_MULTICORE")
  target_link_libraries(complex PUBLIC TBB::tbb)
endif()

target_link_libraries(complex
  PUBLIC
    fmt::fmt
    nlohmann_json::nlohmann_json
    nonstd::expected-lite
    nonstd::span-lite
    Eigen3::Eigen
    HDF5::HDF5
    Boost::mp11
)

if(UNIX)
  target_link_libraries(complex
    PRIVATE
      ${CMAKE_DL_LIBS}
  )
endif()

option(COMPLEX_ENABLE_LINK_FILESYSTEM "Enables linking to a C++ filesystem library" OFF)
if(COMPLEX_ENABLE_LINK_FILESYSTEM)
  set(COMPLEX_FILESYSTEM_LIB "stdc++fs" CACHE STRING "C++ filesystem library to link to")
  set_property(CACHE COMPLEX_FILESYSTEM_LIB PROPERTY STRINGS "stdc++fs" "c++fs")
  target_link_libraries(complex
    PUBLIC
      ${COMPLEX_FILESYSTEM_LIB}
  )
endif()

set(COMPLEX_GENERATED_DIR ${complex_BINARY_DIR}/generated)
set(COMPLEX_GENERATED_HEADER_DIR ${complex_BINARY_DIR}/generated/complex)
set(COMPLEX_EXPORT_HEADER ${COMPLEX_GENERATED_HEADER_DIR}/complex_export.hpp)

file(READ ${complex_SOURCE_DIR}/cmake/complex_export_extra.hpp COMPLEX_EXPORT_HEADER_EXTRA)

include(GenerateExportHeader)
generate_export_header(complex
  EXPORT_FILE_NAME ${COMPLEX_EXPORT_HEADER}
  CUSTOM_CONTENT_FROM_VARIABLE COMPLEX_EXPORT_HEADER_EXTRA
)

# Specify parameters here
set(CoreParameters
  ArrayThresholdsParameter
  ArrayCreationParameter
  ArraySelectionParameter
  NeighborListSelectionParameter
  BoolParameter
  ChoicesParameter
  DataGroupCreationParameter
  DataGroupSelectionParameter
  DataPathSelectionParameter
  DynamicTableParameter
  FileSystemPathParameter
  GeneratedFileListParameter
  GeometrySelectionParameter
  Dream3dImportParameter
  MultiArraySelectionParameter
  NumberParameter
  NumericTypeParameter
  ImportCSVDataParameter
  ImportHDF5DatasetParameter
  StringParameter
  VectorParameter
)

set(COMPLEX_SOURCE_DIR ${complex_SOURCE_DIR}/src/complex)

set(COMPLEX_HDRS
  ${COMPLEX_SOURCE_DIR}/Common/Any.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Array.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Bit.hpp
  ${COMPLEX_SOURCE_DIR}/Common/ComplexConstants.hpp
  ${COMPLEX_SOURCE_DIR}/Common/BoundingBox.hpp
  ${COMPLEX_SOURCE_DIR}/Common/EulerAngle.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Numbers.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Point2D.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Point3D.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Ray.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Result.hpp
  ${COMPLEX_SOURCE_DIR}/Common/RgbColor.hpp
  ${COMPLEX_SOURCE_DIR}/Common/ScopeGuard.hpp
  ${COMPLEX_SOURCE_DIR}/Common/StringLiteral.hpp
  ${COMPLEX_SOURCE_DIR}/Common/TypeTraits.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Types.hpp
  ${COMPLEX_SOURCE_DIR}/Common/TypesUtility.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Uuid.hpp

  ${COMPLEX_SOURCE_DIR}/Common/ComplexRange.hpp
  ${COMPLEX_SOURCE_DIR}/Common/ComplexRange2D.hpp
  ${COMPLEX_SOURCE_DIR}/Common/ComplexRange3D.hpp

  ${COMPLEX_SOURCE_DIR}/Core/Application.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterHandle.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterList.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/DataArrayFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/DataGroupFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/EdgeGeomFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/GridMontageFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/HexahedralGeomFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/ImageGeomFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/NeighborListFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/QuadGeomFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/RectGridGeomFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/ScalarDataFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/StringArrayFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/TetrahedralGeomFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/TriangleGeomFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/VertexGeomFactory.hpp

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
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/LinkedGeometryData.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/DynamicListArray.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/EmptyDataStore.hpp

  ${COMPLEX_SOURCE_DIR}/Plugin/AbstractPlugin.hpp
  ${COMPLEX_SOURCE_DIR}/Plugin/PluginLoader.hpp
  
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractMontage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractTileIndex.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridMontage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridTileIndex.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/AbstractDataStore.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/BaseGroup.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataArray.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataGroup.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataMap.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataObject.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataPath.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataStore.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataStructure.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DynamicListArray.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/EmptyDataStore.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IDataArray.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IDataStore.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/INeighborList.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/LinkedPath.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Metadata.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/NeighborList.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/ScalarData.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/StringArray.hpp

  ${COMPLEX_SOURCE_DIR}/Filter/AbstractParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/AnyParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Arguments.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/ConstDataParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/DataParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterTraits.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/IFilter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/IParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/MutableDataParameter.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Output.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/ParameterTraits.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Parameters.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/ValueParameter.hpp

  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateArrayAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CopyArrayInstanceAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CopyGroupAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateDataGroupAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateImageGeometryAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateGeometry2DAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateNeighborListAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateTriangleGeomAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateVertexGeometryAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/DeleteDataAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/ImportH5ObjectPathsAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/ImportObjectAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/EmptyAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/RenameDataAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/UpdateImageGeomAction.hpp

  ${COMPLEX_SOURCE_DIR}/Parameters/util/DynamicTableData.hpp
  ${COMPLEX_SOURCE_DIR}/Parameters/util/CSVWizardData.hpp

  ${COMPLEX_SOURCE_DIR}/Pipeline/AbstractPipelineNode.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Pipeline.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/PipelineFilter.hpp

  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/AbstractPipelineMessage.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/FilterPreflightMessage.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/NodeAddedMessage.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/NodeMovedMessage.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/NodeRemovedMessage.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/NodeStatusMessage.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/OutputRenamedMessage.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/PipelineFilterMessage.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/PipelineNodeMessage.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/PipelineNodeObserver.hpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/RenamedMessage.hpp

  ${COMPLEX_SOURCE_DIR}/Plugin/AbstractPlugin.hpp
  ${COMPLEX_SOURCE_DIR}/Plugin/PluginLoader.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/ArrayThreshold.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/FilePathGenerator.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/FilterUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/GeometryHelpers.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/StringUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipGenerator.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipRowItem.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/DataArrayUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/DataGroupUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelDataAlgorithm.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelData2DAlgorithm.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelData3DAlgorithm.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelTaskAlgorithm.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/SamplingUtils.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/SegmentFeatures.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/AlignSections.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Math/GeometryMath.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Math/MatrixMath.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Math/StatisticsCalculations.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/DREAM3D/Dream3dIO.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5AttributeReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5AttributeWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DataFactoryManager.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DatasetReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DatasetWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DataStructureReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DataStructureWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5FileReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5FileWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5GroupReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5GroupWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5IDataFactory.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5ObjectReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5ObjectWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5Support.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5Constants.hpp


  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/Text/CsvParser.hpp
)

set(COMPLEX_GENERATED_HEADERS
  ${COMPLEX_EXPORT_HEADER}
)

set(COMPLEX_SRCS
  ${COMPLEX_SOURCE_DIR}/Common/RgbColor.cpp
  ${COMPLEX_SOURCE_DIR}/Common/ComplexRange.cpp
  ${COMPLEX_SOURCE_DIR}/Common/ComplexRange2D.cpp
  ${COMPLEX_SOURCE_DIR}/Common/ComplexRange3D.cpp

  ${COMPLEX_SOURCE_DIR}/Core/Application.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterHandle.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterList.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/DataGroupFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/EdgeGeomFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/GridMontageFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/HexahedralGeomFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/ImageGeomFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/NeighborListFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/QuadGeomFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/RectGridGeomFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/ScalarDataFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/StringArrayFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/TetrahedralGeomFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/TriangleGeomFactory.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/VertexGeomFactory.cpp

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
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/LinkedGeometryData.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractMontage.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractTileIndex.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridMontage.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridTileIndex.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/AbstractDataStore.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/BaseGroup.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataGroup.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataMap.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataObject.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataPath.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/DataStructure.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/INeighborList.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/LinkedPath.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Metadata.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/NeighborList.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/StringArray.cpp

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

  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateArrayAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CopyArrayInstanceAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CopyGroupAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateDataGroupAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateImageGeometryAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateNeighborListAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateTriangleGeomAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/DeleteDataAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/ImportH5ObjectPathsAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/ImportObjectAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/RenameDataAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/UpdateImageGeomAction.cpp

  ${COMPLEX_SOURCE_DIR}/Parameters/util/CSVWizardData.cpp
  ${COMPLEX_SOURCE_DIR}/Parameters/util/DynamicTableData.cpp

  ${COMPLEX_SOURCE_DIR}/Pipeline/AbstractPipelineNode.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Pipeline.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/PipelineFilter.cpp

  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/AbstractPipelineMessage.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/FilterPreflightMessage.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/NodeAddedMessage.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/NodeMovedMessage.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/NodeRemovedMessage.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/NodeStatusMessage.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/OutputRenamedMessage.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/PipelineFilterMessage.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/PipelineNodeMessage.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/PipelineNodeObserver.cpp
  ${COMPLEX_SOURCE_DIR}/Pipeline/Messaging/RenamedMessage.cpp

  ${COMPLEX_SOURCE_DIR}/Plugin/AbstractPlugin.cpp
  ${COMPLEX_SOURCE_DIR}/Plugin/PluginLoader.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/ArrayThreshold.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/FilePathGenerator.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipGenerator.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipRowItem.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/DataArrayUtilities.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/DataGroupUtilities.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelDataAlgorithm.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelData2DAlgorithm.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelData3DAlgorithm.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelTaskAlgorithm.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/SegmentFeatures.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/AlignSections.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Math/GeometryMath.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Math/MatrixMath.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/DREAM3D/Dream3dIO.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5AttributeReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5AttributeWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DataFactoryManager.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DatasetReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DatasetWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DataStructureReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5DataStructureWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5FileReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5FileWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5GroupReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5GroupWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5IDataFactory.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5ObjectReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5ObjectWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5Support.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/Text/CsvParser.cpp
  )


# Add Core FilterParameters
foreach(param ${CoreParameters})
  list(APPEND COMPLEX_HDRS
    "${COMPLEX_SOURCE_DIR}/Parameters/${param}.hpp"
  )
  list(APPEND COMPLEX_SRCS
    "${COMPLEX_SOURCE_DIR}/Parameters/${param}.cpp"
  )
endforeach()

set(COMPLEX_ALL_HDRS
  ${COMPLEX_HDRS}
  ${COMPLEX_GENERATED_HEADERS}
)

target_sources(complex
  PRIVATE
    ${COMPLEX_ALL_HDRS}
    ${COMPLEX_SRCS}
)

source_group(TREE ${COMPLEX_SOURCE_DIR} PREFIX "complex" FILES ${COMPLEX_HDRS})
source_group(TREE ${COMPLEX_GENERATED_HEADER_DIR} PREFIX "complex/generated" FILES ${COMPLEX_GENERATED_HEADERS})
source_group(TREE ${COMPLEX_SOURCE_DIR} PREFIX "complex" FILES ${COMPLEX_SRCS})

set(COMPLEX_INCLUDE_DIR ${complex_SOURCE_DIR}/src)

target_include_directories(complex
  PUBLIC
    $<BUILD_INTERFACE:${COMPLEX_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${COMPLEX_GENERATED_DIR}>
    $<BUILD_INTERFACE:${complex_SOURCE_DIR}/3rdparty/nod/include>
    $<INSTALL_INTERFACE:include>
)

get_property(ComplexPluginTargets GLOBAL PROPERTY ComplexPluginTargets)
if("${ComplexPluginTargets}" STREQUAL "")
  set_property(GLOBAL PROPERTY ComplexPluginTargets "")
endif()

#------------------------------------------------------------------------------
# Add in the ComplexCore Plugin
set(P_PLUGIN_NAME "ComplexCore")
add_subdirectory(${complex_SOURCE_DIR}/src/Plugins/${P_PLUGIN_NAME} ${complex_BINARY_DIR}/Plugins/${P_PLUGIN_NAME})
get_property(PluginNumFilters GLOBAL PROPERTY ${P_PLUGIN_NAME}_filter_count)
message(STATUS "${P_PLUGIN_NAME} [ENABLED] ${PluginNumFilters} Filters")
set_property(GLOBAL PROPERTY COMPLEX_PLUGIN_COUNT 1)

#------------------------------------------------------------------------------
# Add user defined Plugin Dirs based on the COMPLEX_PLUGIN_SEARCH_DIRS variable
# and falling back to complex_SOURCE_DIR/../complex_plugins if not defined
# Use 'COMPLEX_PLUGIN_SEARCH_DIRS' hints to look for plugins in specific directories
foreach(plugin ${COMPLEX_EXTRA_PLUGINS})
  complex_add_plugin(PLUGIN_NAME ${plugin})
endforeach()

set(COMPLEX_PACKAGE_NAMESPACE "complex::")

option(COMPLEX_ENABLE_INSTALL "Enables COMPLEX install rules" ON)
if(COMPLEX_ENABLE_INSTALL)

  install(TARGETS complex
    EXPORT complexTargets
  )

  export(EXPORT complexTargets NAMESPACE ${COMPLEX_PACKAGE_NAMESPACE})

  include(CMakePackageConfigHelpers)

  set(COMPLEX_INSTALL_CMAKE_PREFIX "lib/cmake")

  set(COMPLEX_CONFIG_FILE ${complex_BINARY_DIR}/complexConfig.cmake)
  set(COMPLEX_VERSION_FILE ${complex_BINARY_DIR}/complexConfigVersion.cmake)

  configure_package_config_file(${complex_SOURCE_DIR}/cmake/complexConfig.cmake.in ${COMPLEX_CONFIG_FILE}
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

# -----------------------------
# Pipeline Runner
add_subdirectory(${complex_SOURCE_DIR}/src/PipelineRunner)
