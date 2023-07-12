cmake_minimum_required(VERSION 3.26)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Utility.cmake)

# --------------------------------------------------------------------------------------------------
# Is this a commercial or Free build
# --------------------------------------------------------------------------------------------------
option(COMPLEX_BUILD_PYTHON "Enable building Python building" OFF)
include(CMakeDependentOption)
cmake_dependent_option(COMPLEX_EMBED_PYTHON "Embeds python interpreter in nxrunner" ON "COMPLEX_BUILD_PYTHON" OFF)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_BUILD_PYTHON FEATURE "python")

# --------------------------------------------------------------------------------------------------
# is building unit tests enabled
# --------------------------------------------------------------------------------------------------
option(COMPLEX_BUILD_TESTS "Enable building COMPLEX tests" ON)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_BUILD_TESTS FEATURE "tests")

# --------------------------------------------------------------------------------------------------
# are multithreading algorithms enabled
# --------------------------------------------------------------------------------------------------
option(COMPLEX_ENABLE_MULTICORE "Enable multicore support" ON)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_ENABLE_MULTICORE FEATURE "parallel")

# --------------------------------------------------------------------------------------------------
# Are any out-of-core compressors enabled
# --------------------------------------------------------------------------------------------------
option(COMPLEX_ENABLE_COMPRESSORS "Install data compressors" OFF)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_ENABLE_COMPRESSORS FEATURE "compressors")

# --------------------------------------------------------------------------------------------------
# Is downloading of the test files enabled
# --------------------------------------------------------------------------------------------------
option(COMPLEX_DOWNLOAD_TEST_FILES "Download the test files" ON)

# --------------------------------------------------------------------------------------------------
# Will each unit test output the final DataStructure to a file
# --------------------------------------------------------------------------------------------------
option(COMPLEX_WRITE_TEST_OUTPUT "Write unit test output files" OFF)

# --------------------------------------------------------------------------------------------------
# Is the ComplexCore Plugin enabled [DEFAULT=ON]
# --------------------------------------------------------------------------------------------------
option(COMPLEX_ENABLE_ComplexCore "Enable the ComplexCore Plugin" ON)

# --------------------------------------------------------------------------------------------------
# Is the ITKImageProcessing Plugin enabled [DEFAULT=ON]
# --------------------------------------------------------------------------------------------------
option(COMPLEX_ENABLE_ITKImageProcessing "Enable the ITKImageProcessing Plugin" ON)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_ENABLE_ITKImageProcessing FEATURE "itk")

# --------------------------------------------------------------------------------------------------
# Is the OrientationAnalysis Plugin enabled [DEFAULT=ON]
# --------------------------------------------------------------------------------------------------
option(COMPLEX_ENABLE_OrientationAnalysis "Enable the OrientationAnalysis Plugin" ON)
enable_vcpkg_manifest_feature(TEST_VAR COMPLEX_ENABLE_OrientationAnalysis FEATURE "ebsd")

# --------------------------------------------------------------------------------------------------
file(TO_CMAKE_PATH "${CMAKE_COMMAND}" CMAKE_COMMAND_NORM)

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
set(COMPLEX_DEBUG_POSTFIX "_d")
set_property(GLOBAL PROPERTY COMPLEX_DEBUG_POSTFIX ${COMPLEX_DEBUG_POSTFIX})
get_property(COMPLEX_DEBUG_POSTFIX GLOBAL PROPERTY COMPLEX_DEBUG_POSTFIX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# -----------------------------------------------------------------------
# This section creates a global file that holds the commands to copy data
# files, test data files, pipelines into a single cmake file that gets
# generated. This reduces the many copy targets to a single target.
# -----------------------------------------------------------------------
set(FETCH_FILE_PATH "${complex_BINARY_DIR}/complex_fetch_remote_files.cmake")
set_property(GLOBAL PROPERTY FETCH_FILE_PATH "${FETCH_FILE_PATH}")
file(WRITE ${FETCH_FILE_PATH} "# -----------------------------------------------------------------------
# This file has the commands to download each of the test files.
# The WORKING_DIRECTORY is set to the following CMake code:
# ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>
# -----------------------------------------------------------------------\n
cmake_policy(SET CMP0012 NEW)
cmake_policy(SET CMP0054 NEW)\n")

set(TEST_WORKING_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(TEST_WORKING_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>")
endif()

if(COMPLEX_BUILD_TESTS AND COMPLEX_DOWNLOAD_TEST_FILES)
  add_custom_target(Fetch_Remote_Data_Files ALL
    COMMAND "${CMAKE_COMMAND}" -P "${FETCH_FILE_PATH}"
    COMMENT "Downloading Test Data Files"
  #  WORKING_DIRECTORY "${TEST_WORKING_DIR}"
  )
  set_target_properties(Fetch_Remote_Data_Files PROPERTIES FOLDER ZZ_FETCH_TEST_FILES)
endif()

# -----------------------------------------------------------------------
# Find dependent libraries
# -----------------------------------------------------------------------
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(expected-lite CONFIG REQUIRED)
find_package(span-lite CONFIG REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(boost_mp11 CONFIG REQUIRED)
find_package(nod CONFIG REQUIRED)

# -----------------------------------------------------------------------
# Find HDF5 and get the path to the DLL libraries and put that into a
# global property for later install, debugging and packaging
# -----------------------------------------------------------------------
find_package(HDF5 REQUIRED)
get_target_property(hdf5_dll_path hdf5::hdf5-shared IMPORTED_LOCATION_RELEASE)
get_filename_component(hdf5_dll_path "${hdf5_dll_path}" DIRECTORY)
get_property(COMPLEX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS)
set_property(GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS ${COMPLEX_EXTRA_LIBRARY_DIRS} ${hdf5_dll_path})

# -----------------------------------------------------------------------
# Find oneTBB and get the path to the DLL libraries and put that into a
# global property for later install, debugging and packaging
# -----------------------------------------------------------------------
if(COMPLEX_ENABLE_MULTICORE)
  find_package(TBB CONFIG REQUIRED)
  get_target_property(tbb_dll_path TBB::tbb IMPORTED_LOCATION_RELEASE)
  get_filename_component(tbb_dll_path "${tbb_dll_path}" DIRECTORY)
  get_property(COMPLEX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS)
  set_property(GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS ${COMPLEX_EXTRA_LIBRARY_DIRS} ${tbb_dll_path})
endif()

# -----------------------------------------------------------------------
# Create the ${DREAM3D_DATA_DIR} if necessary
# -----------------------------------------------------------------------
if("${DREAM3D_DATA_DIR}" STREQUAL "")
  message(WARNING "DREAM3D_DATA_DIR is not defined externally. Defining to ${complex_BINARY_DIR}/DREAM3D_Data")
  set(DREAM3D_DATA_DIR "${complex_BINARY_DIR}/DREAM3D_Data")
endif()
file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")

add_library(complex SHARED)
add_library(complex::complex ALIAS complex)

target_compile_features(complex
  PUBLIC
    cxx_std_17
)

set_target_properties(complex
  PROPERTIES
    DEBUG_POSTFIX "${COMPLEX_DEBUG_POSTFIX}"
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
  GenerateColorTableParameter
  GeneratedFileListParameter
  GeometrySelectionParameter
  DataStoreFormatParameter
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
  ${COMPLEX_SOURCE_DIR}/Common/BoundingBox.hpp
  ${COMPLEX_SOURCE_DIR}/Common/ComplexConstants.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Constants.hpp
  ${COMPLEX_SOURCE_DIR}/Common/DataVector.hpp
  ${COMPLEX_SOURCE_DIR}/Common/EulerAngle.hpp
  ${COMPLEX_SOURCE_DIR}/Common/IteratorUtility.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Numbers.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Range.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Range2D.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Range3D.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Ray.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Result.hpp
  ${COMPLEX_SOURCE_DIR}/Common/RgbColor.hpp
  ${COMPLEX_SOURCE_DIR}/Common/ScopeGuard.hpp
  ${COMPLEX_SOURCE_DIR}/Common/StringLiteral.hpp
  ${COMPLEX_SOURCE_DIR}/Common/TypeTraits.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Types.hpp
  ${COMPLEX_SOURCE_DIR}/Common/TypesUtility.hpp
  ${COMPLEX_SOURCE_DIR}/Common/Uuid.hpp

  ${COMPLEX_SOURCE_DIR}/Core/Application.hpp
  ${COMPLEX_SOURCE_DIR}/Core/Preferences.hpp

  ${COMPLEX_SOURCE_DIR}/Filter/FilterHandle.hpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterList.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/Generic/CoreDataIOManager.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/Generic/DataIOCollection.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/Generic/IDataFactory.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/Generic/IDataIOManager.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/Generic/IOConstants.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataIOManager.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStructureReader.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStructureWriter.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IDataIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IOUtilities.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStoreIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/EmptyDataStoreIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IDataStoreIO.hpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/AttributeMatrixIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/BaseGroupIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataArrayIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataGroupIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/EdgeGeomIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/GridMontageIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/HexahedralGeomIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/ImageGeomIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IGeometryIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IGridGeometryIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom0dIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom1dIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom2dIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom3dIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/NeighborListIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/QuadGeomIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/RectGridGeomIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/ScalarDataIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/StringArrayIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/TetrahedralGeomIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/TriangleGeomIO.hpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/VertexGeomIO.hpp

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
  ${COMPLEX_SOURCE_DIR}/Filter/AnyCloneable.hpp
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

  ${COMPLEX_SOURCE_DIR}/Utilities/AlignSections.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ArrayThreshold.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/DataArrayUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/DataGroupUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/DataObjectUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/FilePathGenerator.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ColorPresetsUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/FilterUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/GeometryUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/GeometryHelpers.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/StringUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelDataAlgorithm.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelData2DAlgorithm.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelData3DAlgorithm.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelTaskAlgorithm.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/SamplingUtils.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/SegmentFeatures.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/TimeUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipGenerator.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/TooltipRowItem.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/OStreamUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ParallelAlgorithmUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/RTree.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ImageRotationUtilities.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/FlyingEdges.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/SampleSurfaceMesh.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Math/GeometryMath.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Math/MatrixMath.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Math/StatisticsCalculations.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/DREAM3D/Dream3dIO.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5Support.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/AttributeIO.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/DatasetIO.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/FileIO.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/GroupIO.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/ObjectIO.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/AttributeReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/DatasetReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/FileReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/GroupReader.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/ObjectReader.hpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/DatasetWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/FileWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/GroupWriter.hpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/ObjectWriter.hpp

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
  ${COMPLEX_SOURCE_DIR}/Core/Preferences.cpp

  ${COMPLEX_SOURCE_DIR}/Filter/FilterHandle.cpp
  ${COMPLEX_SOURCE_DIR}/Filter/FilterList.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/AbstractDataStructureMessage.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataAddedMessage.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataRemovedMessage.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataRenamedMessage.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/Messaging/DataReparentedMessage.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/Observers/AbstractDataStructureObserver.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/Generic/DataIOCollection.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/Generic/IDataIOManager.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/Generic/CoreDataIOManager.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataIOManager.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStructureReader.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStructureWriter.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IDataIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IOUtilities.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IDataStoreIO.cpp

  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/AttributeMatrixIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/BaseGroupIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/DataGroupIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/EdgeGeomIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/GridMontageIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/HexahedralGeomIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/ImageGeomIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IGeometryIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/IGridGeometryIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom0dIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom1dIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom2dIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom3dIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/QuadGeomIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/RectGridGeomIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/StringArrayIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/TetrahedralGeomIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/TriangleGeomIO.cpp
  ${COMPLEX_SOURCE_DIR}/DataStructure/IO/HDF5/VertexGeomIO.cpp

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
  ${COMPLEX_SOURCE_DIR}/Utilities/FilterUtilities.cpp
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
  ${COMPLEX_SOURCE_DIR}/Utilities/GeometryHelpers.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/GeometryUtilities.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ColorPresetsUtilities.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/ImageRotationUtilities.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Math/GeometryMath.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Math/MatrixMath.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/SampleSurfaceMesh.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/DREAM3D/Dream3dIO.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5Support.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/AttributeIO.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/DatasetIO.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/FileIO.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/GroupIO.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/ObjectIO.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/AttributeReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/DatasetReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/FileReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/GroupReader.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/ObjectReader.cpp

  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/AttributeWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/DatasetWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/FileWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/GroupWriter.cpp
  ${COMPLEX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/ObjectWriter.cpp

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

set(COMPLEX_GENERATED_PYTHON_HEADER ${COMPLEX_GENERATED_HEADER_DIR}/ComplexPython.hpp)
set(COMPLEX_PYTHON_MODULE "complex")
configure_file(${complex_SOURCE_DIR}/wrapping/python/ComplexPython.hpp.in ${COMPLEX_GENERATED_PYTHON_HEADER})

list(APPEND COMPLEX_GENERATED_HEADERS
  ${COMPLEX_GENERATED_PYTHON_HEADER}
)

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

# ------------------------------------------------------------------------------
# CLI Executable
# ------------------------------------------------------------------------------
add_subdirectory(${complex_SOURCE_DIR}/src/nxrunner)

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
set(COMPLEX_PLUGIN_LIST "")
if(COMPLEX_ENABLE_ComplexCore)
  set(COMPLEX_PLUGIN_LIST ${COMPLEX_PLUGIN_LIST} ComplexCore)
endif()
if(COMPLEX_ENABLE_ITKImageProcessing)
  set(COMPLEX_PLUGIN_LIST ${COMPLEX_PLUGIN_LIST} ITKImageProcessing)
endif()
# Ensure ITKImageProcessing comes before OrientationAnalysis as there is currently
# a dependency of ITKImageWriter inside some of the example OrientationAnalysis pipelines
if(COMPLEX_ENABLE_OrientationAnalysis)
  set(COMPLEX_PLUGIN_LIST ${COMPLEX_PLUGIN_LIST} OrientationAnalysis)
endif()
# Add any plugins to the list
set(COMPLEX_PLUGIN_LIST ${COMPLEX_PLUGIN_LIST} ${COMPLEX_EXTRA_PLUGINS})


foreach(plugin ${COMPLEX_PLUGIN_LIST})
  if(NOT DEFINED COMPLEX_ENABLE_${plugin})
    set(COMPLEX_ENABLE_${plugin} ON)
  endif()
  complex_add_plugin(PLUGIN_NAME ${plugin})
endforeach()

set(COMPLEX_PACKAGE_NAMESPACE "complex::")

#------------------------------------------------------------------------------
# Install the appropriate example pipelines based on which plugins are loaded
#------------------------------------------------------------------------------
set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/Data")
file(TO_CMAKE_PATH "${DREAM3D_DATA_DIR}" DREAM3D_DATA_DIR_NORM)
set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/pipelines")
if(NOT EXISTS "${PIPELINE_DEST_DIR}")
  file(MAKE_DIRECTORY ${PIPELINE_DEST_DIR})
endif()
set(PIPELINE_EXAMPLES_DEST_DIR "${PIPELINE_DEST_DIR}/Examples")
if(NOT EXISTS "${PIPELINE_EXAMPLES_DEST_DIR}")
  file(MAKE_DIRECTORY ${PIPELINE_EXAMPLES_DEST_DIR})
endif()
if(TARGET ComplexCore AND TARGET ITKImageProcessing AND TARGET OrientationAnalysis)
  add_custom_target(Copy_PorosityAnalysis_Pipelines ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${complex_SOURCE_DIR}/pipelines/PorosityAnalysis.d3dpipeline" "${PIPELINE_EXAMPLES_DEST_DIR}/PorosityAnalysis.d3dpipeline"
    COMMENT "Copying Example Pipelines into Binary Directory"
    VERBATIM
  )
  set_target_properties(Copy_PorosityAnalysis_Pipelines PROPERTIES FOLDER ZZ_COPY_FILES)

  add_custom_target(Copy_CAxis_Pipelines ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${complex_SOURCE_DIR}/pipelines/Combo-EBSD-osc_r0c0.d3dpipeline" "${PIPELINE_EXAMPLES_DEST_DIR}/Combo-EBSD-osc_r0c0.d3dpipeline"
    COMMENT "Copying Example Pipelines into Binary Directory"
    VERBATIM
  )
  set_target_properties(Copy_CAxis_Pipelines PROPERTIES FOLDER ZZ_COPY_FILES)
  
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_caxis_data.tar.gz SHA512 6c3eefac62aecf3344de735c0df932dd9a2623653f39174ec72402c54e8911402ad9903ae2122bc23ff2dba8c94973b2882715908e1913973dff604b614a5293)
  add_custom_target(Copy_CAxis_Data ALL
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/6_6_caxis_data.tar.gz" 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DREAM3D_DATA_DIR}/TestFiles/6_6_caxis_data/Combo-EBSD-120130.osc_r0c0.ang" "${DATA_DEST_DIR}/OrientationAnalysis/Combo-EBSD-120130.osc_r0c0.ang"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/6_6_caxis_data"
    WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
    COMMENT "[DATA EXTRACT/COPY] 6_6_caxis_data into Binary Directory"
    DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
  )
  set_target_properties(Copy_CAxis_Data PROPERTIES FOLDER ZZ_COPY_FILES)
endif()

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
  
  include(${complex_SOURCE_DIR}/cmake/Utility.cmake)
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

#------------------------------------------------------------------------------
# Print Summary Information
#------------------------------------------------------------------------------
include(${complex_SOURCE_DIR}/cmake/Summary.cmake)
