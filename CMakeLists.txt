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


include(${CMAKE_CURRENT_LIST_DIR}/cmake/Utility.cmake)

# ------------------------------------------------------------------------------
# Is this a commercial or Free build
# ------------------------------------------------------------------------------
option(SIMPLNX_BUILD_PYTHON "Enable building Python building" OFF)
include(CMakeDependentOption)
cmake_dependent_option(SIMPLNX_EMBED_PYTHON "Embeds python interpreter in nxrunner" ON "SIMPLNX_BUILD_PYTHON" OFF)
enable_vcpkg_manifest_feature(TEST_VAR SIMPLNX_BUILD_PYTHON FEATURE "python")

# ------------------------------------------------------------------------------
# is building unit tests enabled
# ------------------------------------------------------------------------------
option(SIMPLNX_BUILD_TESTS "Enable buildingtests" ON)
enable_vcpkg_manifest_feature(TEST_VAR SIMPLNX_BUILD_TESTS FEATURE "tests")

# ------------------------------------------------------------------------------
# are multithreading algorithms enabled
# ------------------------------------------------------------------------------
option(SIMPLNX_ENABLE_MULTICORE "Enable multicore support" ON)
enable_vcpkg_manifest_feature(TEST_VAR SIMPLNX_ENABLE_MULTICORE FEATURE "parallel")

# ------------------------------------------------------------------------------
# Are any out-of-core compressors enabled
# ------------------------------------------------------------------------------
option(SIMPLNX_ENABLE_COMPRESSORS "Install data compressors" OFF)
enable_vcpkg_manifest_feature(TEST_VAR SIMPLNX_ENABLE_COMPRESSORS FEATURE "compressors")

# ------------------------------------------------------------------------------
# Is downloading of the test files enabled
# ------------------------------------------------------------------------------
option(SIMPLNX_DOWNLOAD_TEST_FILES "Download the test files" ON)

# ------------------------------------------------------------------------------
# Will each unit test output the final DataStructure to a file
# ------------------------------------------------------------------------------
option(SIMPLNX_WRITE_TEST_OUTPUT "Write unit test output files" OFF)

# ------------------------------------------------------------------------------
# Is the SimplnxCore Plugin enabled [DEFAULT=ON]
# ------------------------------------------------------------------------------
option(SIMPLNX_ENABLE_SimplnxCore "Enable the SimplnxCore Plugin" ON)

# ------------------------------------------------------------------------------
# Is the ITKImageProcessing Plugin enabled [DEFAULT=ON]
# ------------------------------------------------------------------------------
option(SIMPLNX_ENABLE_ITKImageProcessing "Enable the ITKImageProcessing Plugin" ON)
enable_vcpkg_manifest_feature(TEST_VAR SIMPLNX_ENABLE_ITKImageProcessing FEATURE "itk")

# ------------------------------------------------------------------------------
# Is the OrientationAnalysis Plugin enabled [DEFAULT=ON]
# ------------------------------------------------------------------------------
option(SIMPLNX_ENABLE_OrientationAnalysis "Enable the OrientationAnalysis Plugin" ON)
enable_vcpkg_manifest_feature(TEST_VAR SIMPLNX_ENABLE_OrientationAnalysis FEATURE "ebsd")

# ------------------------------------------------------------------------------
# Simplnx being built for an Anaconda Python distribution [DEFAULT=OFF]
# ------------------------------------------------------------------------------
option(SIMPLNX_CONDA_BUILD "Enables conda-build specific layout" OFF)

# ------------------------------------------------------------------------------
# Enable all installation rules [DEFAULT=ON]
# ------------------------------------------------------------------------------
option(SIMPLNX_ENABLE_INSTALL "Enable install rules" ON)

# ------------------------------------------------------------------------------
# Enable benchmarking utility
# ------------------------------------------------------------------------------
option(SIMPLNX_ENABLE_BENCHMARK_UTILITY "Enables benchmark utility" OFF)

# ------------------------------------------------------------------------------
# Check if a different Data_Archive web site is being used.
#   The alternate would be for the internal build bots that can download from a
#   local webserver
# ------------------------------------------------------------------------------
if("${DATA_ARCHIVE_WEB_SITE}" STREQUAL "")
  set(DATA_ARCHIVE_WEB_SITE "https://github.com/bluequartzsoftware/simplnx/releases/download/Data_Archive/")
endif()

# ------------------------------------------------------------------------------
file(TO_CMAKE_PATH "${CMAKE_COMMAND}" CMAKE_COMMAND_NORM)

project(simplnx
  VERSION 1.3.0
  DESCRIPTION "SIMPL Redesign"
  HOMEPAGE_URL "https://github.com/bluequartzsoftware/simplnx"
  LANGUAGES CXX
)

set(SIMPLNX_BIN_DIR ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SIMPLNX_BIN_DIR} CACHE PATH "Single Directory for all Libraries")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SIMPLNX_BIN_DIR} CACHE PATH "Single Directory for all Executables.")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SIMPLNX_BIN_DIR} CACHE PATH "Single Directory for all static libraries.")

list(APPEND CMAKE_MODULE_PATH ${simplnx_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(SIMPLNX_DEBUG_POSTFIX "_d")
set_property(GLOBAL PROPERTY SIMPLNX_DEBUG_POSTFIX ${SIMPLNX_DEBUG_POSTFIX})
get_property(SIMPLNX_DEBUG_POSTFIX GLOBAL PROPERTY SIMPLNX_DEBUG_POSTFIX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# -----------------------------------------------------------------------
# This section creates a global file that holds the commands to copy data
# files, test data files, pipelines into a single cmake file that gets
# generated. This reduces the many copy targets to a single target.
# -----------------------------------------------------------------------
set(FETCH_FILE_PATH "${simplnx_BINARY_DIR}/simplnx_fetch_remote_files.cmake")
set_property(GLOBAL PROPERTY FETCH_FILE_PATH "${FETCH_FILE_PATH}")
file(WRITE ${FETCH_FILE_PATH} "# -----------------------------------------------------------------------
# This file is programmatically generated from the CMake file ${CMAKE_CURRENT_LIST_FILE}
# This file has the commands to download each of the test files.
# The WORKING_DIRECTORY is set to the following CMake code:
# ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>
# -----------------------------------------------------------------------\n
cmake_policy(SET CMP0012 NEW)\ncmake_policy(SET CMP0054 NEW)\ncmake_policy(SET CMP0152 NEW)\ncmake_policy(SET CMP0148 NEW)")

set(TEST_WORKING_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(TEST_WORKING_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>")
endif()

if(SIMPLNX_DOWNLOAD_TEST_FILES)
  add_custom_target(Fetch_Remote_Data_Files ALL
    COMMAND "${CMAKE_COMMAND}" -DCMAKE_CONFIG=$<CONFIG> -P "${FETCH_FILE_PATH}"
    COMMENT "Downloading Test Data Files"
  #  WORKING_DIRECTORY "${TEST_WORKING_DIR}"
  )
  set_target_properties(Fetch_Remote_Data_Files PROPERTIES FOLDER ZZ_FETCH_TEST_FILES)
endif()

if(SIMPLNX_CONDA_BUILD)
  include(GNUInstallDirs)
  set(SIMPLNX_PIPELINE_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/simplnx/pipelines")
else()
  set(SIMPLNX_PIPELINE_INSTALL_DIR "pipelines")
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
find_package(HDF5 1.14 MODULE REQUIRED)
get_target_property(hdf5_dll_path hdf5::hdf5 IMPORTED_LOCATION_RELEASE)
get_filename_component(hdf5_dll_path "${hdf5_dll_path}" DIRECTORY)
get_property(SIMPLNX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS)
set_property(GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS ${SIMPLNX_EXTRA_LIBRARY_DIRS} ${hdf5_dll_path})

# -----------------------------------------------------------------------
# Find oneTBB and get the path to the DLL libraries and put that into a
# global property for later install, debugging and packaging
# -----------------------------------------------------------------------
if(SIMPLNX_ENABLE_MULTICORE)
  find_package(TBB CONFIG REQUIRED)
  get_target_property(tbb_dll_path TBB::tbb IMPORTED_LOCATION_RELEASE)
  get_filename_component(tbb_dll_path "${tbb_dll_path}" DIRECTORY)
  get_property(SIMPLNX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS)
  set_property(GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS ${SIMPLNX_EXTRA_LIBRARY_DIRS} ${tbb_dll_path})
endif()

# -----------------------------------------------------------------------
# Create the ${DREAM3D_DATA_DIR} if necessary
# -----------------------------------------------------------------------
if("${DREAM3D_DATA_DIR}" STREQUAL "")
  message(WARNING "DREAM3D_DATA_DIR is not defined externally. Defining to ${simplnx_BINARY_DIR}/DREAM3D_Data")
  set(DREAM3D_DATA_DIR "${simplnx_BINARY_DIR}/DREAM3D_Data")
endif()
file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")

add_library(simplnx SHARED)
add_library(simplnx::simplnx ALIAS simplnx)

target_compile_features(simplnx
  PUBLIC
    cxx_std_17
)

set_target_properties(simplnx
  PROPERTIES
    DEBUG_POSTFIX "${SIMPLNX_DEBUG_POSTFIX}"
)

simplnx_enable_warnings(TARGET simplnx)

if(MSVC)
  target_compile_options(simplnx
    PRIVATE
      /MP
  )
endif()

# Force HDF5 1.10 API
target_compile_definitions(simplnx PUBLIC "H5_USE_110_API")

target_compile_options(simplnx
  PUBLIC
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang,Intel>:-ffp-contract=off>
)

if(SIMPLNX_ENABLE_MULTICORE)
  target_compile_definitions(simplnx PUBLIC "SIMPLNX_ENABLE_MULTICORE")
  target_link_libraries(simplnx PUBLIC TBB::tbb)
endif()

target_link_libraries(simplnx
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
  target_link_libraries(simplnx
    PRIVATE
      ${CMAKE_DL_LIBS}
  )
endif()

option(SIMPLNX_ENABLE_LINK_FILESYSTEM "Enables linking to a C++ filesystem library" OFF)
if(SIMPLNX_ENABLE_LINK_FILESYSTEM)
  set(SIMPLNX_FILESYSTEM_LIB "stdc++fs" CACHE STRING "C++ filesystem library to link to")
  set_property(CACHE SIMPLNX_FILESYSTEM_LIB PROPERTY STRINGS "stdc++fs" "c++fs")
  target_link_libraries(simplnx
    PUBLIC
      ${SIMPLNX_FILESYSTEM_LIB}
  )
endif()

set(SIMPLNX_GENERATED_DIR ${simplnx_BINARY_DIR}/generated)
set(SIMPLNX_GENERATED_HEADER_DIR ${simplnx_BINARY_DIR}/generated/simplnx)
set(SIMPLNX_EXPORT_HEADER ${SIMPLNX_GENERATED_HEADER_DIR}/simplnx_export.hpp)

file(READ ${simplnx_SOURCE_DIR}/cmake/simplnx_export_extra.hpp SIMPLNX_EXPORT_HEADER_EXTRA)

include(GenerateExportHeader)
generate_export_header(simplnx
  EXPORT_FILE_NAME ${SIMPLNX_EXPORT_HEADER}
  CUSTOM_CONTENT_FROM_VARIABLE SIMPLNX_EXPORT_HEADER_EXTRA
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
  DataTypeParameter
  DynamicTableParameter
  EnsembleInfoParameter
  FileSystemPathParameter
  CreateColorMapParameter
  GeneratedFileListParameter
  GeometrySelectionParameter
  DataStoreFormatParameter
  Dream3dImportParameter
  MultiArraySelectionParameter
  MultiPathSelectionParameter
  NumberParameter
  NumericTypeParameter
  ReadCSVFileParameter
  ReadHDF5DatasetParameter
  StringParameter
  VectorParameter
)

set(SIMPLNX_SOURCE_DIR ${simplnx_SOURCE_DIR}/src/simplnx)

set(SIMPLNX_HDRS
  ${SIMPLNX_SOURCE_DIR}/Common/Any.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Array.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/AtomicFile.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Bit.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/BoundingBox.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/SimplnxConstants.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Constants.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/DataVector.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/EulerAngle.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/IteratorUtility.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Numbers.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Range.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Range2D.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Range3D.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Ray.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Result.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/RgbColor.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/ScopeGuard.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/StringLiteral.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/TypeTraits.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Types.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/TypesUtility.hpp
  ${SIMPLNX_SOURCE_DIR}/Common/Uuid.hpp

  ${SIMPLNX_SOURCE_DIR}/Core/Application.hpp
  ${SIMPLNX_SOURCE_DIR}/Core/Preferences.hpp

  ${SIMPLNX_SOURCE_DIR}/Filter/FilterHandle.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/FilterList.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/Generic/CoreDataIOManager.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/Generic/DataIOCollection.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/Generic/IDataFactory.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/Generic/IDataIOManager.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/Generic/IOConstants.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataIOManager.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStructureReader.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStructureWriter.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IDataIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IOUtilities.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStoreIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/EmptyDataStoreIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IDataStoreIO.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/AttributeMatrixIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/BaseGroupIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataArrayIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataGroupIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/EdgeGeomIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/GridMontageIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/HexahedralGeomIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/ImageGeomIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IGeometryIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IGridGeometryIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom0dIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom1dIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom2dIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom3dIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/NeighborListIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/QuadGeomIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/RectGridGeomIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/ScalarDataIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/StringArrayIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/TetrahedralGeomIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/TriangleGeomIO.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/VertexGeomIO.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/AbstractDataStructureMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/DataAddedMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/DataRemovedMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/DataRenamedMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/DataReparentedMessage.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/Observers/AbstractDataStructureObserver.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/EdgeGeom.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/HexahedralGeom.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/ImageGeom.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/QuadGeom.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/RectGridGeom.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/TetrahedralGeom.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/TriangleGeom.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/VertexGeom.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/IGeometry.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/IGridGeometry.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry0D.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry1D.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry2D.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry3D.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/DynamicListArray.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/EmptyDataStore.hpp

  ${SIMPLNX_SOURCE_DIR}/Plugin/AbstractPlugin.hpp
  ${SIMPLNX_SOURCE_DIR}/Plugin/PluginLoader.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/Montage/AbstractMontage.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Montage/AbstractTileIndex.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Montage/GridMontage.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Montage/GridTileIndex.hpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/AbstractDataStore.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/AttributeMatrix.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/BaseGroup.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataArray.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataGroup.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataMap.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataObject.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataPath.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataStore.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataStructure.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DynamicListArray.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/EmptyDataStore.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IArray.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IDataArray.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IDataStore.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/INeighborList.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/LinkedPath.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Metadata.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/NeighborList.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/ScalarData.hpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/StringArray.hpp

  ${SIMPLNX_SOURCE_DIR}/Filter/AbstractParameter.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/AnyCloneable.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Arguments.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/ConstDataParameter.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/DataParameter.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/FilterTraits.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/IFilter.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/IParameter.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/MutableDataParameter.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Output.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/ParameterTraits.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Parameters.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/ValueParameter.hpp

  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateArrayAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateStringArrayAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CopyArrayInstanceAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CopyDataObjectAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateDataGroupAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateImageGeometryAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateRectGridGeometryAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateGeometry1DAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateGeometry2DAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateGeometry3DAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateNeighborListAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateVertexGeometryAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/DeleteDataAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/ImportH5ObjectPathsAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/ImportObjectAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/EmptyAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/MoveDataAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/RenameDataAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/UpdateImageGeomAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateAttributeMatrixAction.hpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateGridMontageAction.hpp

  ${SIMPLNX_SOURCE_DIR}/Parameters/util/DynamicTableInfo.hpp
  ${SIMPLNX_SOURCE_DIR}/Parameters/util/ReadCSVData.hpp

  ${SIMPLNX_SOURCE_DIR}/Pipeline/AbstractPipelineFilter.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/AbstractPipelineNode.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Pipeline.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/PipelineFilter.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/PlaceholderFilter.hpp

  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/AbstractPipelineMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/FilterPreflightMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/NodeAddedMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/NodeMovedMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/NodeRemovedMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/NodeStatusMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/OutputRenamedMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/PipelineFilterMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/PipelineNodeMessage.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/PipelineNodeObserver.hpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/RenamedMessage.hpp

  ${SIMPLNX_SOURCE_DIR}/Plugin/AbstractPlugin.hpp
  ${SIMPLNX_SOURCE_DIR}/Plugin/PluginLoader.hpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/AlignSections.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ArrayThreshold.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/DataArrayUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/DataGroupUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/DataObjectUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/FilePathGenerator.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ColorTableUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/FileUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/FilterUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/GeometryUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/GeometryHelpers.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/HistogramUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/MemoryUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/StringUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/IParallelAlgorithm.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ParallelDataAlgorithm.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ParallelData2DAlgorithm.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ParallelData3DAlgorithm.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ParallelTaskAlgorithm.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/SamplingUtils.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/SegmentFeatures.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/TimeUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/TooltipGenerator.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/TooltipRowItem.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/OStreamUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ParallelAlgorithmUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/RTree.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ImageRotationUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/FlyingEdges.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/SampleSurfaceMesh.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ClusteringUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/MontageUtilities.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/SIMPLConversion.hpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Math/GeometryMath.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Math/MatrixMath.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Math/StatisticsCalculations.hpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/DREAM3D/Dream3dIO.hpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5Support.hpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/AttributeIO.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/DatasetIO.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/FileIO.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/GroupIO.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/ObjectIO.hpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/AttributeReader.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/DatasetReader.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/FileReader.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/GroupReader.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/ObjectReader.hpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/DatasetWriter.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/FileWriter.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/GroupWriter.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/ObjectWriter.hpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/Text/CsvParser.hpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/MD5.hpp
)

set(SIMPLNX_GENERATED_HEADERS
  ${SIMPLNX_EXPORT_HEADER}
)

set(SIMPLNX_SRCS
  ${SIMPLNX_SOURCE_DIR}/Common/AtomicFile.cpp
  ${SIMPLNX_SOURCE_DIR}/Common/RgbColor.cpp
  ${SIMPLNX_SOURCE_DIR}/Common/Range.cpp
  ${SIMPLNX_SOURCE_DIR}/Common/Range2D.cpp
  ${SIMPLNX_SOURCE_DIR}/Common/Range3D.cpp

  ${SIMPLNX_SOURCE_DIR}/Core/Application.cpp
  ${SIMPLNX_SOURCE_DIR}/Core/Preferences.cpp

  ${SIMPLNX_SOURCE_DIR}/Filter/FilterHandle.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/FilterList.cpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/AbstractDataStructureMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/DataAddedMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/DataRemovedMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/DataRenamedMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Messaging/DataReparentedMessage.cpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/Observers/AbstractDataStructureObserver.cpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/Generic/DataIOCollection.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/Generic/IDataIOManager.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/Generic/CoreDataIOManager.cpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataIOManager.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStructureReader.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataStructureWriter.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IDataIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IOUtilities.cpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IDataStoreIO.cpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/AttributeMatrixIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/BaseGroupIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/DataGroupIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/EdgeGeomIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/GridMontageIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/HexahedralGeomIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/ImageGeomIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IGeometryIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/IGridGeometryIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom0dIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom1dIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom2dIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/INodeGeom3dIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/QuadGeomIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/RectGridGeomIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/StringArrayIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/TetrahedralGeomIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/TriangleGeomIO.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/IO/HDF5/VertexGeomIO.cpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/IGeometry.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/IGridGeometry.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry0D.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry1D.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry2D.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/INodeGeometry3D.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/EdgeGeom.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/HexahedralGeom.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/ImageGeom.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/QuadGeom.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/RectGridGeom.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/TetrahedralGeom.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/TriangleGeom.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Geometry/VertexGeom.cpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/Montage/AbstractMontage.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Montage/AbstractTileIndex.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Montage/GridMontage.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Montage/GridTileIndex.cpp

  ${SIMPLNX_SOURCE_DIR}/DataStructure/AttributeMatrix.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/BaseGroup.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataGroup.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataMap.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataObject.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataPath.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/DataStructure.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/INeighborList.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/LinkedPath.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/Metadata.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/NeighborList.cpp
  ${SIMPLNX_SOURCE_DIR}/DataStructure/StringArray.cpp

  ${SIMPLNX_SOURCE_DIR}/Filter/AbstractParameter.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Arguments.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/ConstDataParameter.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/DataParameter.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/IFilter.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/IParameter.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/MutableDataParameter.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Output.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Parameters.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/ValueParameter.cpp

  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateArrayAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateStringArrayAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CopyArrayInstanceAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CopyDataObjectAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateDataGroupAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateImageGeometryAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateRectGridGeometryAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateNeighborListAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/DeleteDataAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/ImportH5ObjectPathsAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/ImportObjectAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/MoveDataAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/RenameDataAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/UpdateImageGeomAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateAttributeMatrixAction.cpp
  ${SIMPLNX_SOURCE_DIR}/Filter/Actions/CreateGridMontageAction.cpp

  ${SIMPLNX_SOURCE_DIR}/Parameters/util/ReadCSVData.cpp
  ${SIMPLNX_SOURCE_DIR}/Parameters/util/DynamicTableInfo.cpp

  ${SIMPLNX_SOURCE_DIR}/Pipeline/AbstractPipelineFilter.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/AbstractPipelineNode.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Pipeline.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/PipelineFilter.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/PlaceholderFilter.cpp

  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/AbstractPipelineMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/FilterPreflightMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/NodeAddedMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/NodeMovedMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/NodeRemovedMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/NodeStatusMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/OutputRenamedMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/PipelineFilterMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/PipelineNodeMessage.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/PipelineNodeObserver.cpp
  ${SIMPLNX_SOURCE_DIR}/Pipeline/Messaging/RenamedMessage.cpp

  ${SIMPLNX_SOURCE_DIR}/Plugin/AbstractPlugin.cpp
  ${SIMPLNX_SOURCE_DIR}/Plugin/PluginLoader.cpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/ArrayThreshold.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/FilePathGenerator.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/FilterUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/FileUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/TooltipGenerator.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/TooltipRowItem.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/DataArrayUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/DataGroupUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/MemoryUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/IParallelAlgorithm.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ParallelDataAlgorithm.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ParallelData2DAlgorithm.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ParallelData3DAlgorithm.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ParallelTaskAlgorithm.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/SegmentFeatures.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/AlignSections.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/OStreamUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/GeometryHelpers.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/GeometryUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ColorTableUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/ImageRotationUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Math/GeometryMath.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Math/MatrixMath.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/SampleSurfaceMesh.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/MontageUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/TimeUtilities.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/SIMPLConversion.cpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/DREAM3D/Dream3dIO.cpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/H5Support.cpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/AttributeIO.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/DatasetIO.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/FileIO.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/GroupIO.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/IO/ObjectIO.cpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/AttributeReader.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/DatasetReader.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/FileReader.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/GroupReader.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Readers/ObjectReader.cpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/AttributeWriter.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/DatasetWriter.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/FileWriter.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/GroupWriter.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/HDF5/Writers/ObjectWriter.cpp

  ${SIMPLNX_SOURCE_DIR}/Utilities/Parsing/Text/CsvParser.cpp
  ${SIMPLNX_SOURCE_DIR}/Utilities/MD5.cpp
)

# Add Core FilterParameters
foreach(param ${CoreParameters})
  list(APPEND SIMPLNX_HDRS
    "${SIMPLNX_SOURCE_DIR}/Parameters/${param}.hpp"
  )
  list(APPEND SIMPLNX_SRCS
    "${SIMPLNX_SOURCE_DIR}/Parameters/${param}.cpp"
  )
endforeach()

# Generate Supporting Color Table File
file(READ ${simplnx_SOURCE_DIR}/resources/ColorTablePresets/ColorTablePresets.json COLOR_TABLE_JSON)
string(REGEX REPLACE "[ \t\n\r]+" " " COLOR_TABLE_FLAT ${COLOR_TABLE_JSON})
string(HEX ${COLOR_TABLE_FLAT} COLOR_TABLE_HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\0," COLOR_TABLE_HEX ${COLOR_TABLE_HEX})
string(APPEND COLOR_TABLE_HEX "0x00")

set(SIMPLNX_GENERATED_COLOR_TABLE_HEADER ${SIMPLNX_GENERATED_HEADER_DIR}/util/ColorTable.hpp)
cmpConfigureFileWithMD5Check(CONFIGURED_TEMPLATE_PATH ${SIMPLNX_SOURCE_DIR}/Parameters/util/ColorTable.in.hpp GENERATED_FILE_PATH ${SIMPLNX_GENERATED_COLOR_TABLE_HEADER})
list(APPEND SIMPLNX_GENERATED_HEADERS
  ${SIMPLNX_GENERATED_COLOR_TABLE_HEADER}
)

# Generate Python-Bindings Supporting Files
set(SIMPLNX_GENERATED_PYTHON_HEADER ${SIMPLNX_GENERATED_HEADER_DIR}/SimplnxPython.hpp)
set(SIMPLNX_PYTHON_MODULE "simplnx")
configure_file(${simplnx_SOURCE_DIR}/wrapping/python/CxPybind/SimplnxPython.hpp.in ${SIMPLNX_GENERATED_PYTHON_HEADER})

list(APPEND SIMPLNX_GENERATED_HEADERS
  ${SIMPLNX_GENERATED_PYTHON_HEADER}
)

# set(SIMPLNX_GENERATED_VERSION_HEADER ${SIMPLNX_GENERATED_HEADER_DIR}/SIMPLNXVersion.h)
# configure_file(${simplnx_SOURCE_DIR}/cmake/SIMPLNXVersion.h.in ${SIMPLNX_GENERATED_VERSION_HEADER})


set(CMP_VERSION_HDR_TEMPLATE_FILE ${simplnx_SOURCE_DIR}/cmake/SIMPLNXVersion.hpp)
set(CMP_VERSION_SRC_TEMPLATE_FILE ${simplnx_SOURCE_DIR}/cmake/SIMPLNXVersion.cpp.in)
set(VERSION_GEN_HEADER_FILE_NAME "SIMPLNXVersion.hpp")

cmpBuildDateRevisionString(GENERATED_HEADER_FILE_PATH "generated/simplnx/SIMPLNXVersion.hpp"
      GENERATED_SOURCE_FILE_PATH "generated/simplnx/SIMPLNXVersion.cpp"
      NAMESPACE nx::core
      PROJECT_NAME simplnx
      EXPORT_MACRO ""
      VERSION_MACRO_PATH ""
      STRING_CLASS std::string
      STRING_INCLUDE "#include <string>"
)



list(APPEND SIMPLNX_GENERATED_HEADERS
  ${SIMPLNX_GENERATED_HEADER_DIR}/SIMPLNXVersion.hpp
  ${SIMPLNX_GENERATED_HEADER_DIR}/SIMPLNXVersion.cpp
)

set(SIMPLNX_ALL_HDRS
  ${SIMPLNX_HDRS}
  ${SIMPLNX_GENERATED_HEADERS}
)

target_sources(simplnx
  PRIVATE
    ${SIMPLNX_ALL_HDRS}
    ${SIMPLNX_SRCS}
)

source_group(TREE ${SIMPLNX_SOURCE_DIR} PREFIX "simplnx" FILES ${SIMPLNX_HDRS})
source_group(TREE ${SIMPLNX_GENERATED_HEADER_DIR} PREFIX "simplnx/generated" FILES ${SIMPLNX_GENERATED_HEADERS})
source_group(TREE ${SIMPLNX_SOURCE_DIR} PREFIX "simplnx" FILES ${SIMPLNX_SRCS})

set(SIMPLNX_INCLUDE_DIR ${simplnx_SOURCE_DIR}/src)

target_include_directories(simplnx
  PUBLIC
    $<BUILD_INTERFACE:${SIMPLNX_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${SIMPLNX_GENERATED_DIR}>
    $<INSTALL_INTERFACE:include>
)

cmake_dependent_option(SIMPLNX_DOWNLOAD_TEST_FILES_FIRST "Forces test files to download before simplnx builds" OFF "SIMPLNX_DOWNLOAD_TEST_FILES" OFF)
if(SIMPLNX_DOWNLOAD_TEST_FILES_FIRST)
  add_dependencies(simplnx Fetch_Remote_Data_Files)
endif()

# ------------------------------------------------------------------------------
# CLI Executable
# ------------------------------------------------------------------------------
add_subdirectory(${simplnx_SOURCE_DIR}/src/nxrunner)

#------------------------------------------------------------------------------
# Simplnx Plugins Section
#------------------------------------------------------------------------------
get_property(simplnxPluginTargets GLOBAL PROPERTY simplnxPluginTargets)

if(NOT "${simplnxPluginTargets}" STREQUAL "")
  set_property(GLOBAL PROPERTY simplnxPluginTargets "")
endif()

include(${simplnx_SOURCE_DIR}/cmake/Plugin.cmake)

# add_subdirectory(${simplnx_SOURCE_DIR}/src/Plugins/${P_PLUGIN_NAME} ${simplnx_BINARY_DIR}/Plugins/${P_PLUGIN_NAME})
# get_property(PluginNumFilters GLOBAL PROPERTY ${P_PLUGIN_NAME}_filter_count)
# message(STATUS "${P_PLUGIN_NAME} [ENABLED] ${PluginNumFilters} Filters")
set_property(GLOBAL PROPERTY SIMPLNX_PLUGIN_COUNT 0)

#------------------------------------------------------------------------------
# Add user defined Plugin Dirs based on the SIMPLNX_PLUGIN_SEARCH_DIRS variable
# and falling back to simplnx_SOURCE_DIR/../simplnx_plugins if not defined
# Use 'SIMPLNX_PLUGIN_SEARCH_DIRS' hints to look for plugins in specific directories
set(SIMPLNX_PLUGIN_LIST "")
if(SIMPLNX_ENABLE_SimplnxCore)
  set(SIMPLNX_PLUGIN_LIST ${SIMPLNX_PLUGIN_LIST} SimplnxCore)
endif()
if(SIMPLNX_ENABLE_ITKImageProcessing)
  set(SIMPLNX_PLUGIN_LIST ${SIMPLNX_PLUGIN_LIST} ITKImageProcessing)
endif()
# Ensure ITKImageProcessing comes before OrientationAnalysis as there is currently
# a dependency of ITKImageWriter inside some of the example OrientationAnalysis pipelines
if(SIMPLNX_ENABLE_OrientationAnalysis)
  set(SIMPLNX_PLUGIN_LIST ${SIMPLNX_PLUGIN_LIST} OrientationAnalysis)
endif()

if(SIMPLNX_BUILD_TESTS)
  list(APPEND SIMPLNX_PLUGIN_LIST TestOne TestTwo)
endif()

# Add any plugins to the list
set(SIMPLNX_PLUGIN_LIST ${SIMPLNX_PLUGIN_LIST} ${SIMPLNX_EXTRA_PLUGINS})


foreach(plugin ${SIMPLNX_PLUGIN_LIST})
  if(NOT DEFINED SIMPLNX_PLUGIN_ENABLE_${plugin})
    set(SIMPLNX_PLUGIN_ENABLE_${plugin} ON)
  endif()
  simplnx_add_plugin(PLUGIN_NAME ${plugin})
endforeach()

# Benchmarking utility
if(SIMPLNX_ENABLE_BENCHMARK_UTILITY)
  add_subdirectory(utilities/benchmark)
endif()

set(SIMPLNX_PACKAGE_NAMESPACE "simplnx::")

#------------------------------------------------------------------------------
# Install the appropriate example pipelines based on which plugins are loaded
#------------------------------------------------------------------------------
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
  set(CX_CONFIG_DIR "$<CONFIG>")
else()
  set(CX_CONFIG_DIR ".")
endif()

set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data")
file(TO_CMAKE_PATH "${DREAM3D_DATA_DIR}" DREAM3D_DATA_DIR_NORM)
set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/pipelines")
set(PIPELINE_EXAMPLES_DEST_DIR "${PIPELINE_DEST_DIR}/Examples")

if(SIMPLNX_DOWNLOAD_TEST_FILES AND TARGET SimplnxCore AND TARGET ITKImageProcessing AND TARGET OrientationAnalysis)

  set(PREBUILT_PIPELINE_NAMES
    )
    # Install the pipeline files
    install(FILES
      ${PREBUILT_PIPELINE_NAMES}
      DESTINATION "pipelines/Examples"
      COMPONENT Applications
    )

  create_pipeline_tests(PLUGIN_NAME OrientationAnalysis PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})

endif()

if(SIMPLNX_ENABLE_INSTALL)
  install(TARGETS simplnx
    EXPORT simplnxTargets
  )

  export(EXPORT simplnxTargets NAMESPACE ${SIMPLNX_PACKAGE_NAMESPACE})

  include(CMakePackageConfigHelpers)

  set(SIMPLNX_INSTALL_CMAKE_PREFIX "lib/cmake")

  set(SIMPLNX_CONFIG_FILE ${simplnx_BINARY_DIR}/simplnxConfig.cmake)
  set(SIMPLNX_VERSION_FILE ${simplnx_BINARY_DIR}/simplnxConfigVersion.cmake)

  configure_package_config_file(${simplnx_SOURCE_DIR}/cmake/simplnxConfig.cmake.in ${SIMPLNX_CONFIG_FILE}
    INSTALL_DESTINATION ${SIMPLNX_INSTALL_CMAKE_PREFIX}
  )

  write_basic_package_version_file(${SIMPLNX_VERSION_FILE}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES ${SIMPLNX_CONFIG_FILE} ${SIMPLNX_VERSION_FILE}
    DESTINATION ${SIMPLNX_INSTALL_CMAKE_PREFIX}
    COMPONENT develop
  )

  include(GNUInstallDirs)
  install(TARGETS simplnx
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

  install(EXPORT simplnxTargets
    NAMESPACE ${SIMPLNX_PACKAGE_NAMESPACE}
    DESTINATION ${SIMPLNX_INSTALL_CMAKE_PREFIX}
  )

  install_with_directory(FILES ${SIMPLNX_HDRS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simplnx
    COMPONENT develop
    BASE_DIR ${SIMPLNX_SOURCE_DIR}
  )

  install_with_directory(FILES ${SIMPLNX_GENERATED_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simplnx
    COMPONENT develop
    BASE_DIR ${SIMPLNX_GENERATED_HEADER_DIR}
  )
  
  include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)
endif()

option(SIMPLNX_ENABLE_PACKAGING "Enablespackaging" ON)
if(SIMPLNX_ENABLE_PACKAGING)
  if(WIN32)
    set(CPACK_GENERATOR ZIP)
  else()
    set(CPACK_GENERATOR TGZ)
  endif()
  include(CPack)
endif()

if(SIMPLNX_BUILD_TESTS)
  include(CTest)
  add_subdirectory(test)
endif()

if(SIMPLNX_BUILD_PYTHON)
  add_subdirectory(wrapping/python)
endif()

option(SIMPLNX_BUILD_DOCS "Enables buildingdocumentation" OFF)

if(SIMPLNX_BUILD_DOCS)
  add_subdirectory(docs)
endif()

# ------------------------------------------------------------------------------
# Print Summary Information
# ------------------------------------------------------------------------------
include(${simplnx_SOURCE_DIR}/cmake/Summary.cmake)
