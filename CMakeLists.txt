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
find_package(nod CONFIG REQUIRED)

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
    nod::nod
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
  AttributeMatrixSelectionParameter
  NeighborListSelectionParameter
  BoolParameter
  CalculatorParameter
  ChoicesParameter
  DataGroupCreationParameter
  DataGroupSelectionParameter
  DataObjectNameParameter
  DataPathSelectionParameter
  DynamicTableParameter
  EnsembleInfoParameter
  FileSystemPathParameter
  GeneratedFileListParameter
  GeometrySelectionParameter
  Dream3dImportParameter
  MultiArraySelectionParameter
  MultiPathSelectionParameter
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
  ${COMPLEX_SOURCE_DIR}/Common/Constants.hpp
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

  ${COMPLEX_SOURCE_DIR}/Common/Range.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Range2D.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Range3D.hpp

  ${COMPLEX_SOURCE_DIR}/Core/Application.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterHandle.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterList.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/AttributeMatrixFactory.hpp
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

  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/EdgeGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/HexahedralGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/ImageGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/QuadGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/RectGridGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/TetrahedralGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/TriangleGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/VertexGeom.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/LinkedGeometryData.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/IGeometry.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/IGridGeometry.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry0D.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry1D.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry2D.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry3D.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/DynamicListArray.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/EmptyDataStore.hpp

  ${COMPLEX_SOURCE_DIR}/Plugin/AbstractPlugin.hpp
  ${COMPLEX_SOURCE_DIR}/Plugin/PluginLoader.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractMontage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/AbstractTileIndex.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridMontage.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Montage/GridTileIndex.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/AbstractDataStore.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/AttributeMatrix.hpp
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
  ${COMPLEX_SOURCE_DIR}/DataStructure/IArray.hpp
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
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateStringArrayAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CopyArrayInstanceAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CopyDataObjectAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateDataGroupAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateImageGeometryAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateRectGridGeometryAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateGeometry1DAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateGeometry2DAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateGeometry3DAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateNeighborListAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateVertexGeometryAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/DeleteDataAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/ImportH5ObjectPathsAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/ImportObjectAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/EmptyAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/MoveDataAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/RenameDataAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/UpdateImageGeomAction.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateAttributeMatrixAction.hpp

  ${COMPLEX_SOURCE_DIR}/Parameters/util/DynamicTableInfo.hpp
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
  ${COMPLEX_SOURCE_DIR}/Utilities/OStreamUtilities.hpp

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
  ${COMPLEX_SOURCE_DIR}/Common/Range.cpp
  ${COMPLEX_SOURCE_DIR}/Common/Range2D.cpp
  ${COMPLEX_SOURCE_DIR}/Common/Range3D.cpp

  ${COMPLEX_SOURCE_DIR}/Core/Application.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterHandle.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterList.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Factory/AttributeMatrixFactory.cpp
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

  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/IGeometry.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/IGridGeometry.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry0D.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry1D.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry2D.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry3D.cpp
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
  ${COMPLEX_SOURCE_DIR}/DataStructure/AttributeMatrix.cpp
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
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateStringArrayAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CopyArrayInstanceAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CopyDataObjectAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateDataGroupAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateImageGeometryAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateRectGridGeometryAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateNeighborListAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/DeleteDataAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/ImportH5ObjectPathsAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/ImportObjectAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/MoveDataAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/RenameDataAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/UpdateImageGeomAction.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/Actions/CreateAttributeMatrixAction.cpp

  ${COMPLEX_SOURCE_DIR}/Parameters/util/CSVWizardData.cpp
  ${COMPLEX_SOURCE_DIR}/Parameters/util/DynamicTableInfo.cpp

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
  ${COMPLEX_SOURCE_DIR}/Utilities/OStreamUtilities.cpp

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
    $<INSTALL_INTERFACE:include>
)


# -----------------------------------------------------------------------
# Download the Test Data. This needs to be done BEFORE the Plugins and test
# -----------------------------------------------------------------------
file(TO_CMAKE_PATH "${DREAM3D_DATA_DIR}" DREAM3D_DATA_DIR)
# If DREAM3D_DATA_DIR is NOT defined/set then take a look at the same level as complex
if("${DREAM3D_DATA_DIR}" STREQUAL "")
  message(STATUS "DREAM3D_DATA_DIR is empty. Attempting to locate DREAM3D_Data at same level as complex directory.")
  get_filename_component(complex_PARENT ${complex_SOURCE_DIR} DIRECTORY CACHE)
  if(EXISTS "${complex_PARENT}/DREAM3D_Data")
    message(STATUS "DREAM3D_Data directory was *found* at same level as `complex`")
    set(DREAM3D_DATA_DIR "${complex_PARENT}/DREAM3D_Data" CACHE PATH "The directory where to find test data files")
  else()
    message(WARNING "DREAM3D_Data directory was *not* found at the same level as `complex`.")
  endif()
endif()

# -----------------------------------------------------------------------------
# Here we are going to setup to download and decompress the test files. In order
# to add your own test files you will need to tar.gz the test file, compute the
# SHA 512 Hash of the file and then upload the file to 
# https://github.com/BlueQuartzSoftware/complex/releases/tag/Data_Archive.
#
# Go to the web site above, "edit" the release, add the filename and SHA 512 to
# the table and then upload your compressed file.
# Save the release so that the repo is updated
# -----------------------------------------------------------------------------
if(EXISTS "${DREAM3D_DATA_DIR}" AND COMPLEX_BUILD_TESTS) 
  if(NOT EXISTS ${DREAM3D_DATA_DIR}/TestFiles/)
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 12_IN625_GBCD.tar.gz SHA512 f696a8af181505947e6fecfdb1a11fda6c762bba5e85fea8d484b1af00bf18643e1d930d48f092ee238d1c19c9ce7c4fb5a8092d17774bda867961a1400e9cea)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_align_sections_feature_centroid.tar.gz SHA512 e79a4c8e59bc856d40e91daf4cdce8b82c2e5ccfa6de51e23a0b8c6628282b03701bd5b5d7ddde76c4378142e3d9fe7c1cd6db261360c91751cf7c63747b054b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_align_sections_misorientation.tar.gz SHA512 2343654a8bcb344fcc613b6715c1e0b2c780fedbdf06cc8e5306b23f9d7908d5eef8faff7e3f0dd6f7ac734a1c6e2b376832bed38548288cd5e9e0af1b5602a8)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_ebsd_segment_features.tar.gz SHA512 acbb493a0668e0115ac49d4fedbbf7600759b9a66deb5d1004c2749a61d2bad2fcc60344bf72b2aeda5c8c098f458949dd1f8d58cb21682fa1393dfb7d0a1b84)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_erode_dilate_bad_data.tar.gz SHA512 95556a7a09f7ef85cc21b8e5cb78b15b0edd75791b3d205ec57fd626a7933e72f3f3b6dd07707f28df4d8eb7a35be53818974c38b21ef323f655c29805b335d0)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_feature_centroids.tar.gz SHA512 1eecd4eda617b6b13f3321b33d9cc35fd53f01cf19384c8922318639e86b765d381bd0a07d4dae14628c99cc0e1caffcf23aa667683732d3925307409bda9c28)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_shapes.tar.gz SHA512 657bb5015e731afad605274f01f38fa375b423a91159946c691361e69580d282909072fb026a778b09c0f2b6e2359549a4f138072b343a35a2c97170c1af1292)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_min_size_input.tar.gz SHA512 f59d0a5c3b414ed11419a1cf1594f430d76a9b462086592a3c6bbf41b4c62b33d571981b9fc277e4c6ce43e0a870c02ec9e90d91b49720c8005b87ccbdb11a14)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_min_size_output.tar.gz SHA512 2ad1bc5e99eddf9939b52e16fcd1d8ca70ec37bfe9396b7123af3d8f11f83d015cf0bae2b5e903e65cec8e5d7bda328649042253a046d1e6c89b637e0a5faf91)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_stats_test.tar.gz SHA512 cb1d1c004ab5a3cb29cc10f7b6c291dd5819e57e303242b8162cd0b268dea24b1d3e5e3811ec4f5ee216179b6eb6b81c76ee7d37b7c49e83c6ab336147b4b14e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_volume_fraction_feature_count.dream3d.tar.gz SHA512 e4205186dee5b97bef3fb5f265bc12f8379a68860f70de33a483680eba0433a35c9c98db56bc92c949fb215f73f5f7698f83394697e81604c8c4b8a06874943f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ASCII_Data.tar.gz SHA512 4696b919510aa400b9de6c72d0abe66e7c8be2a4effd350bfd07c4fc24d25ccb45c22e7aca5ad162d88db3e04d700b86f7d1d7e954db90ef00b31b5b46955ed4)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Example.tar.gz SHA512 b7ea2b8bb5ecb29cb20d69e5820ebf513433c7740ce5cc9e6c1e44b539941dd3cf1a627442ace2cd430fe64437a479a62a172e6d398edf992ccdfc906ebceeba)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ResampleImageGeom_Exemplar.tar.gz SHA512 464029b7354b96a943d75c495ef02bac0f834032e5a86576dde9afee51febff3fd6ffd7d4f8f1e9f8315d8cda36971df26601c7212e1876151109ca5428b8659)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME SmallIN100.tar.gz SHA512 31a32ba5d1296ef184c2134fe14023c20a5e5c21b95b3f4193c4394db19c86b241cd54d31b0e0128d40c0bfbd6460657531094e588b5d4d224485d17f7c3ed36)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100.tar.gz SHA512 da6c5ea94b90a1877c41fbaf7390bae9230f969aa4b953cf39b6deff8fb529ba1a299e2f13b59ec2835dc1f524514342a21aebc55916e92d91c098723412ea24)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_Exemplar.tar.gz SHA512 c57684c31f73fe1e5871ffd760a8c1af9498c86c4e5bcec09997e89126d1eb6ef85bf96d32a60992de44cba510839f257f5d73585e8413ce34ce65e3b6f433d5)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME align_sections.tar.gz SHA512 b6892e437df86bd79bd2f1d8f48e44d05bfe38b3453058744320bfaf1b1dc461a484edc9e593f6b9de4ad4d04c41b5dbd0a30e6fc605341d046aec4c3062e33e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME bad_data_neighbor_orientation_check.tar.gz SHA512 6452cfb1f2394c10050082256f60a2068cfad78ef742e9e35b1d6e63b3fb7c35c9fe7bbe093bed4dbb4e758c49ec6da7b1f7e2473838a0421f39fbdd9f4a2f76)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME neighbor_orientation_correlation.tar.gz SHA512 122367452174ade2f24dde7a4610bddc4f147a223722d9b30c1df9eaa2cd2bf25e1c7957aba83f3f9de79b4eadd79339b848f9530d1ebf44c69244ea5442cf85)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME so3_cubic_high_ipf_001.tar.gz SHA512 dfe4598cd4406e8b83f244302dc4fe0d4367527835c5ddd6567fe8d8ab3484d5b10ba24a8bb31db269256ec0b5272daa4340eedb5a8b397755541b32dd616b85)  
  
  create_data_copy_rules(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR})

endif()

#------------------------------------------------------------------------------
# Complex Plugins Section
#------------------------------------------------------------------------------
get_property(ComplexPluginTargets GLOBAL PROPERTY ComplexPluginTargets)
if(NOT "${ComplexPluginTargets}" STREQUAL "")
  set_property(GLOBAL PROPERTY ComplexPluginTargets "")
endif()

include(${complex_SOURCE_DIR}/cmake/Plugin.cmake)

# add_subdirectory(${complex_SOURCE_DIR}/src/Plugins/${P_PLUGIN_NAME} ${complex_BINARY_DIR}/Plugins/${P_PLUGIN_NAME})
# get_property(PluginNumFilters GLOBAL PROPERTY ${P_PLUGIN_NAME}_filter_count)
# message(STATUS "${P_PLUGIN_NAME} [ENABLED] ${PluginNumFilters} Filters")
set_property(GLOBAL PROPERTY COMPLEX_PLUGIN_COUNT 0)

#------------------------------------------------------------------------------
# Add user defined Plugin Dirs based on the COMPLEX_PLUGIN_SEARCH_DIRS variable
# and falling back to complex_SOURCE_DIR/../complex_plugins if not defined
# Use 'COMPLEX_PLUGIN_SEARCH_DIRS' hints to look for plugins in specific directories
set(COMPLEX_PLUGIN_LIST ComplexCore;${COMPLEX_EXTRA_PLUGINS})
foreach(plugin ${COMPLEX_PLUGIN_LIST})
  if(NOT DEFINED COMPLEX_PLUGIN_ENABLE_${plugin})
    set(COMPLEX_PLUGIN_ENABLE_${plugin} ON)
  endif()
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
