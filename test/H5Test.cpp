#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/DynamicListArray.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/IOConstants.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/ImportH5ObjectPathsAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"
#include "simplnx/Utilities/Parsing/Text/CsvParser.hpp"

#include "simplnx/UnitTest/DataObjectComparison.hpp"
#include "simplnx/UnitTest/HDF5DatasetProbe.hpp"

#include "GeometryTestUtilities.hpp"

#include <thread>
#include <vector>

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <hdf5.h>

#include <algorithm>
#include <mutex>
#include <numeric>
#include <optional>
#include <string>
#include <type_traits>

// This file is generated into the binary directory

#define TEST_LEGACY 1

namespace fs = std::filesystem;
using namespace nx::core;

static_assert(std::is_same_v<hid_t, nx::core::hdf5::IdType>, "H5::IdType must be the same type as hid_t");
static_assert(std::is_same_v<herr_t, nx::core::hdf5::ErrorType>, "H5::ErrorType must be the same type as herr_t");
static_assert(std::is_same_v<hsize_t, nx::core::hdf5::SizeType>, "H5::SizeType must be the same type as hsize_t");

namespace
{
/**
 * @struct DatatypeFixtureFile
 * @brief Removes the datatype test file after its HDF5 wrappers have closed.
 */
struct DatatypeFixtureFile
{
  fs::path path;

  ~DatatypeFixtureFile()
  {
    std::error_code error;
    fs::remove(path, error);
  }
};

/**
 * @class CallerOwnedDatatype
 * @brief Closes only the datatype identifier returned to this test by getTypeId().
 */
class CallerOwnedDatatype
{
public:
  /**
   * @brief Adopts the identifier that the getter transfers to its caller.
   * @param identifier Supplies the owned datatype identifier.
   */
  explicit CallerOwnedDatatype(hid_t identifier)
  : m_Id(identifier)
  {
  }

  /**
   * @brief Releases the test's own handle if an assertion skips its explicit close.
   */
  ~CallerOwnedDatatype()
  {
    if(m_Id >= 0)
    {
      static_cast<void>(close());
    }
  }

  CallerOwnedDatatype(const CallerOwnedDatatype&) = delete;
  CallerOwnedDatatype& operator=(const CallerOwnedDatatype&) = delete;

  hid_t getId() const
  {
    return m_Id;
  }

  /**
   * @brief Explicitly releases the caller-owned identifier under the existing HDF5 API lock.
   * @return HDF5 close status.
   */
  herr_t close()
  {
    std::lock_guard<std::mutex> lock(hdf5::Support::ApiLock());
    const herr_t status = H5Tclose(m_Id);
    if(status >= 0)
    {
      m_Id = -1;
    }
    return status;
  }

private:
  hid_t m_Id = -1;
};

/**
 * @brief Counts datatype identifiers, including transient types without an associated file.
 * @return Open datatype count, or a negative HDF5 error status.
 */
int64 CountHdf5DatatypeIds()
{
  std::lock_guard<std::mutex> lock(hdf5::Support::ApiLock());
  return static_cast<int64>(H5Fget_obj_count(H5F_OBJ_ALL, H5F_OBJ_DATATYPE));
}

/**
 * @brief Returns the number of open HDF5 dataset identifiers.
 * @return Open dataset count, or a negative HDF5 error status.
 */
int64 CountHdf5DatasetIds()
{
  std::lock_guard<std::mutex> lock(hdf5::Support::ApiLock());
  return static_cast<int64>(H5Fget_obj_count(H5F_OBJ_ALL, H5F_OBJ_DATASET));
}

/**
 * @brief Tests whether an HDF5 identifier is valid.
 * @param id Identifies the HDF5 object to test.
 * @return True if HDF5 reports a valid identifier.
 */
bool IsHdf5IdValid(hid_t id)
{
  std::lock_guard<std::mutex> lock(hdf5::Support::ApiLock());
  return H5Iis_valid(id) > 0;
}

namespace constants
{
const fs::path k_DataDir = "test/data";
const StringLiteralType k_LegacyFilepath = "LegacyData.dream3d";
const fs::path k_ComplexH5File = "new.h5";
} // namespace constants

fs::path GetDataDir()
{
  return fs::path(unit_test::k_BinaryTestOutputDir.view());
}

fs::path GetLegacyFilepath()
{
  std::string path = fmt::format("{}/test/Data/{}", unit_test::k_SourceDir.view(), constants::k_LegacyFilepath);
  return fs::path(path);
}

fs::path GetComplexH5File()
{
  return GetDataDir() / constants::k_ComplexH5File;
}

bool Equalsf(const FloatVec3Type& lhs, const FloatVec3Type& rhs)
{
  for(usize i = 0; i < 3; i++)
  {
    float32 diff = std::abs(lhs[i] - rhs[i]);
    if(diff >= 0.0001f)
    {
      return false;
    }
  }
  return true;
}

DataStructure GetTestDataStructure()
{
  DataStructure dataStructure;
  auto* groupPtr = DataGroup::Create(dataStructure, "Group");
  REQUIRE(groupPtr != nullptr);

  auto* geomPtr = ImageGeom::Create(dataStructure, "Geometry");
  REQUIRE(geomPtr != nullptr);

  auto* scalarPtr = ScalarData<int32>::Create(dataStructure, "Scalar", 7, geomPtr->getId());
  REQUIRE(scalarPtr != nullptr);

  auto dataStore = std::make_unique<DataStore<uint8>>(std::vector<usize>{2}, std::vector<usize>{3}, 0);
  auto* dataArrayPtr = DataArray<uint8>::Create(dataStructure, "DataArray", std::move(dataStore), geomPtr->getId());
  REQUIRE(dataArrayPtr != nullptr);

  return dataStructure;
}

template <typename T>
DataArray<T>* CreateTestDataArray(const std::string& name, DataStructure& dataStructure, const ShapeType& tupleShape, const ShapeType& componentShape, DataObject::IdType parentId)
{
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  auto dataStore = std::make_unique<DataStoreType>(tupleShape, componentShape, static_cast<T>(0));
  ArrayType* dataArrayPtr = ArrayType::Create(dataStructure, name, std::move(dataStore), parentId);

  return dataArrayPtr;
}

DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  DataGroup* groupPtr = DataGroup::Create(dataStructure, "Small IN100");
  DataGroup* scanDataPtr = DataGroup::Create(dataStructure, "EBSD Scan Data", groupPtr->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeomPtr = ImageGeom::Create(dataStructure, "Small IN100 Grid", scanDataPtr->getId());
  REQUIRE(imageGeomPtr->setSpacing({0.25f, 0.25f, 0.25f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());
  SizeVec3Type imageGeomDims = {100, 100, 100};
  imageGeomPtr->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  usize numComponents = 1;
  ShapeType tupleShape = {imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]};

  Float32ArrayType* ciDataPtr = CreateTestDataArray<float>("Confidence Index", dataStructure, tupleShape, {numComponents}, scanDataPtr->getId());
  Int32ArrayType* featureIdsDataPtr = CreateTestDataArray<int32>("FeatureIds", dataStructure, tupleShape, {numComponents}, scanDataPtr->getId());
  Float32ArrayType* iqDataPtr = CreateTestDataArray<float>("Image Quality", dataStructure, tupleShape, {numComponents}, scanDataPtr->getId());
  Int32ArrayType* phasesDataPtr = CreateTestDataArray<int32>("Phases", dataStructure, tupleShape, {numComponents}, scanDataPtr->getId());

  numComponents = 3;
  UInt8ArrayType* ipfColorDataPtr = CreateTestDataArray<uint8_t>("IPF Colors", dataStructure, tupleShape, {numComponents}, scanDataPtr->getId());

  dataStructure.setAdditionalParent(ipfColorDataPtr->getId(), groupPtr->getId());

  // Add in another group that holds the phase data such as Laue Class, Lattice Constants, etc.
  DataGroup* phaseGroupPtr = DataGroup::Create(dataStructure, "Phase Data", groupPtr->getId());
  numComponents = 1;
  usize numTuples = 2;
  Int32ArrayType* laueDataPtr = CreateTestDataArray<int32>("Laue Class", dataStructure, {numTuples}, {numComponents}, phaseGroupPtr->getId());

  return dataStructure;
}

const std::string k_TriangleGroupName = "TRIANGLE_GEOMETRY";
const std::string k_TriangleFaceName = "SharedTriList";
const std::string k_VertexListName = "SharedVertexList";
const std::string k_VertexGroupName = "VERTEX_GEOMETRY";
const std::string k_QuadGroupName = "QUAD_GEOMETRY";
const std::string k_QuadFaceName = "SharedQuadList";
const std::string k_EdgeFaceName = "SharedEdgeList";
const std::string k_EdgeGroupName = "EDGE_GEOMETRY";
const std::string k_TetraGroupName = "TETRA_GEOMETRY";
const std::string k_TetraFaceName = "SharedCellList";
const std::string k_HexaGroupName = "HEXA_GEOMETRY";
const std::string k_HexaFaceName = "SharedCellList";
const std::string k_NeighborGroupName = "NEIGHBORLIST_GROUP";

void CreateVertexGeometry(DataStructure& dataStructure)
{
  DataGroup* geometryGroupPtr = DataGroup::Create(dataStructure, k_VertexGroupName);
  auto* vertexGeometryPtr = VertexGeom::Create(dataStructure, "[Geometry] Vertex", geometryGroupPtr->getId());

  // DataGroup* scanData = DataGroup::Create(dataStructure, "AttributeMatrix", group->getId());
  uint64 skipLines = 1;
  char delimiter = ',';

  // Create the Vertex Array with a component size of 3
  DataPath path = DataPath({k_VertexGroupName, k_VertexListName});
  std::string inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  nx::core::Result result = array_creation_utilities::CreateArray<float>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArrayPtr = dataStructure.getDataAs<Float32ArrayType>(path);
  if(vertexArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<float, float>(inputFile, vertexArrayPtr->getDataStoreRef(), skipLines, delimiter);
  vertexGeometryPtr->setVertices(*vertexArrayPtr);
  REQUIRE(vertexGeometryPtr->getNumberOfVertices() == 144);

  // Create cell-level arrays for the Vertex Geometry.
  ShapeType tupleShape = {vertexGeometryPtr->getNumberOfVertices()};
  usize numComponents = 1;
  Int16ArrayType* ciDataPtr = CreateTestDataArray<int16_t>("Area", dataStructure, tupleShape, {numComponents}, geometryGroupPtr->getId());
  Float32ArrayType* powerDataPtr = CreateTestDataArray<float>("Power", dataStructure, tupleShape, {numComponents}, geometryGroupPtr->getId());
  UInt32ArrayType* laserTtlDataPtr = CreateTestDataArray<uint32>("LaserTTL", dataStructure, tupleShape, {numComponents}, geometryGroupPtr->getId());
  for(usize i = 0; i < vertexGeometryPtr->getNumberOfVertices(); i++)
  {
    ciDataPtr->getDataStore()->setValue(i, static_cast<int16_t>(i * 10));
    powerDataPtr->getDataStore()->setValue(i, static_cast<float>(i * 2.345f));
    laserTtlDataPtr->getDataStore()->setValue(i, static_cast<uint32>(i * 3421));
  }
}

void CreateTriangleGeometry(DataStructure& dataStructure)
{
  // Create a Triangle Geometry
  DataGroup* geometryGroupPtr = DataGroup::Create(dataStructure, k_TriangleGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, "[Geometry] Triangle", geometryGroupPtr->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_TriangleGroupName, k_TriangleFaceName});
  std::string inputFile = fmt::format("{}/test/Data/TriangleConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 242);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = array_creation_utilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArrayPtr = dataStructure.getDataAs<IGeometry::SharedFaceListType>(path);
  if(dataArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArrayPtr->getDataStoreRef(), skipLines, delimiter);
  triangleGeomPtr->setFaceList(*dataArrayPtr);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_TriangleGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = array_creation_utilities::CreateArray<float>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArrayPtr = dataStructure.getDataAs<Float32ArrayType>(path);
  if(vertexArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<float, float>(inputFile, vertexArrayPtr->getDataStoreRef(), skipLines, delimiter);
  triangleGeomPtr->setVertices(*vertexArrayPtr);
}

void CreateQuadGeometry(DataStructure& dataStructure)
{
  // Create a Triangle Geometry
  DataGroup* geometryGroupPtr = DataGroup::Create(dataStructure, k_QuadGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto* geometryPtr = QuadGeom::Create(dataStructure, "[Geometry] Quad", geometryGroupPtr->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_QuadGroupName, k_QuadFaceName});
  std::string inputFile = fmt::format("{}/test/Data/QuadConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 121);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = array_creation_utilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {4}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArrayPtr = dataStructure.getDataAs<IGeometry::SharedFaceListType>(path);
  if(dataArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArrayPtr->getDataStoreRef(), skipLines, delimiter);
  geometryPtr->setFaceList(*dataArrayPtr);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_QuadGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = array_creation_utilities::CreateArray<float32>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArrayPtr = dataStructure.getDataAs<Float32ArrayType>(path);
  if(vertexArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<float, float>(inputFile, vertexArrayPtr->getDataStoreRef(), skipLines, delimiter);
  geometryPtr->setVertices(*vertexArrayPtr);
}

void CreateEdgeGeometry(DataStructure& dataStructure)
{
  // Create a Triangle Geometry
  DataGroup* geometryGroupPtr = DataGroup::Create(dataStructure, k_EdgeGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto* geometryPtr = EdgeGeom::Create(dataStructure, "[Geometry] Edge", geometryGroupPtr->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_EdgeGroupName, k_EdgeFaceName});
  std::string inputFile = fmt::format("{}/test/Data/EdgeConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 264);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = array_creation_utilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {2}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArrayPtr = dataStructure.getDataAs<IGeometry::SharedEdgeListType>(path);
  if(dataArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArrayPtr->getDataStoreRef(), skipLines, delimiter);
  geometryPtr->setEdgeList(*dataArrayPtr);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_EdgeGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = array_creation_utilities::CreateArray<float32>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArrayPtr = dataStructure.getDataAs<Float32ArrayType>(path);
  if(vertexArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<float, float>(inputFile, vertexArrayPtr->getDataStoreRef(), skipLines, delimiter);
  geometryPtr->setVertices(*vertexArrayPtr);
}

void CreateTetrahedralGeometry(DataStructure& dataStructure)
{
  // Create a Tetrahedral Geometry
  DataGroup* geometryGroupPtr = DataGroup::Create(dataStructure, k_TetraGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto* geometryPtr = TetrahedralGeom::Create(dataStructure, "[Geometry] Tetrahedral", geometryGroupPtr->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_TetraGroupName, k_TetraFaceName});
  std::string inputFile = fmt::format("{}/test/Data/TetraConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 3);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = array_creation_utilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {4}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArrayPtr = dataStructure.getDataAs<IGeometry::SharedTetListType>(path);
  if(dataArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArrayPtr->getDataStoreRef(), skipLines, delimiter);
  geometryPtr->setPolyhedraList(*dataArrayPtr);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_TetraGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/TetraVertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 9);
  result = array_creation_utilities::CreateArray<float32>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArrayPtr = dataStructure.getDataAs<Float32ArrayType>(path);
  if(vertexArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<float, float>(inputFile, vertexArrayPtr->getDataStoreRef(), skipLines, delimiter);
  geometryPtr->setVertices(*vertexArrayPtr);
}

void CreateHexahedralGeometry(DataStructure& dataStructure)
{
  // Create a Hexahedral Geometry
  DataGroup* geometryGroupPtr = DataGroup::Create(dataStructure, k_HexaGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto* geometryPtr = TetrahedralGeom::Create(dataStructure, "[Geometry] Hexahedral", geometryGroupPtr->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_HexaGroupName, k_HexaFaceName});
  std::string inputFile = fmt::format("{}/test/Data/HexaConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 3);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = array_creation_utilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {8}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArrayPtr = dataStructure.getDataAs<IGeometry::SharedHexListType>(path);
  if(dataArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArrayPtr->getDataStoreRef(), skipLines, delimiter);
  geometryPtr->setPolyhedraList(*dataArrayPtr);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_HexaGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/HexaVertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = csv_parser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 20);
  result = array_creation_utilities::CreateArray<float32>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArrayPtr = dataStructure.getDataAs<Float32ArrayType>(path);
  if(vertexArrayPtr == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  csv_parser::ReadFile<float, float>(inputFile, vertexArrayPtr->getDataStoreRef(), skipLines, delimiter);
  geometryPtr->setVertices(*vertexArrayPtr);
}

void CreateNeighborList(DataStructure& dataStructure)
{
  const usize numItems = 50;
  const ShapeType tupleShape = {numItems};

  auto* neighborGroupPtr = DataGroup::Create(dataStructure, k_NeighborGroupName);
  auto* neighborGroup2Ptr = DataGroup::Create(dataStructure, k_NeighborGroupName + "2");
  auto* neighborListPtr = NeighborList<int64>::Create(dataStructure, "NeighborList", tupleShape, neighborGroupPtr->getId());
  for(usize i = 0; i < numItems; i++)
  {
    for(usize j = 0; j < i + 1; j++)
    {
      neighborListPtr->addEntry(j, j);
    }
  }
  dataStructure.setAdditionalParent(neighborListPtr->getId(), neighborGroup2Ptr->getId());
}

void CreateArrayTypes(DataStructure& dataStructure)
{
  const ShapeType tupleShape = {2};
  const ShapeType componentShape = {1};

  auto* boolArrayPtr = DataArray<bool>::CreateWithStore<DataStore<bool>>(dataStructure, "BoolArray", tupleShape, componentShape);
  AbstractDataStore<bool>& boolStore = boolArrayPtr->getDataStoreRef();
  boolStore[0] = false;
  boolStore[1] = true;

  DataArray<int8>::CreateWithStore<DataStore<int8>>(dataStructure, "Int8Array", tupleShape, componentShape);
  DataArray<int16>::CreateWithStore<DataStore<int16>>(dataStructure, "Int16Array", tupleShape, componentShape);
  DataArray<int32>::CreateWithStore<DataStore<int32>>(dataStructure, "Int32Array", tupleShape, componentShape);
  DataArray<int64>::CreateWithStore<DataStore<int64>>(dataStructure, "Int64Array", tupleShape, componentShape);

  DataArray<uint8>::CreateWithStore<DataStore<uint8>>(dataStructure, "UInt8Array", tupleShape, componentShape);
  DataArray<uint16>::CreateWithStore<DataStore<uint16>>(dataStructure, "UInt16Array", tupleShape, componentShape);
  DataArray<uint32>::CreateWithStore<DataStore<uint32>>(dataStructure, "UInt32Array", tupleShape, componentShape);
  DataArray<uint64>::CreateWithStore<DataStore<uint64>>(dataStructure, "UInt64Array", tupleShape, componentShape);

  DataArray<float32>::CreateWithStore<DataStore<float32>>(dataStructure, "Float32Array", tupleShape, componentShape);
  DataArray<float64>::CreateWithStore<DataStore<float64>>(dataStructure, "Float64Array", tupleShape, componentShape);

  StringArray::CreateWithValues(dataStructure, "StringArray", ShapeType{3}, {"Foo", "Bar", "Bazz"});
}

//------------------------------------------------------------------------------
DataStructure CreateNodeBasedGeometries()
{
  DataStructure dataStructure;

  CreateVertexGeometry(dataStructure);
  CreateTriangleGeometry(dataStructure);
  CreateQuadGeometry(dataStructure);
  CreateEdgeGeometry(dataStructure);
  CreateTetrahedralGeometry(dataStructure);
  CreateHexahedralGeometry(dataStructure);

  return dataStructure;
}

struct VertexGeomData
{
  std::optional<DataObject::IdType> vertices_id;

  bool operator==(const VertexGeomData& rhs) const
  {
    return vertices_id == rhs.vertices_id;
  }
};

struct EdgeGeomData
{
  std::optional<DataObject::IdType> vertices_id;
  std::optional<DataObject::IdType> edges_id;

  bool operator==(const EdgeGeomData& rhs) const
  {
    return (vertices_id == rhs.vertices_id) && (edges_id == rhs.edges_id);
  }
};

struct TriangleGeomData
{
  std::optional<DataObject::IdType> vertices_id;
  std::optional<DataObject::IdType> edges_id;
  std::optional<DataObject::IdType> triangles_id;

  bool operator==(const TriangleGeomData& rhs) const
  {
    return (vertices_id == rhs.vertices_id) && (edges_id == rhs.edges_id) && (triangles_id == rhs.triangles_id);
  }
};

struct QuadGeomData
{
  std::optional<DataObject::IdType> vertices_id;
  std::optional<DataObject::IdType> edges_id;
  std::optional<DataObject::IdType> quads_id;

  bool operator==(const QuadGeomData& rhs) const
  {
    return (vertices_id == rhs.vertices_id) && (edges_id == rhs.edges_id) && (quads_id == rhs.quads_id);
  }
};

struct NodeBasedGeomData
{
  VertexGeomData vertex_data;
  EdgeGeomData edge_data;
  TriangleGeomData triangle_data;
  QuadGeomData quad_data;

  bool operator==(const NodeBasedGeomData& rhs) const
  {
    bool vertexCheck = (vertex_data == rhs.vertex_data);
    bool edgeCheck = (edge_data == rhs.edge_data);
    bool triangleCheck = (triangle_data == rhs.triangle_data);
    bool quadCheck = (quad_data == rhs.quad_data);
    return vertexCheck && edgeCheck && triangleCheck && quadCheck;
  }
};

NodeBasedGeomData GetNodeGeomData(const DataStructure& dataStructure)
{
  NodeBasedGeomData nodeData;

  DataPath vertexPath({k_VertexGroupName, "[Geometry] Vertex"});
  const auto* vertexGeomPtr = dataStructure.getDataAs<VertexGeom>(vertexPath);
  REQUIRE(vertexGeomPtr != nullptr);
  nodeData.vertex_data.vertices_id = vertexGeomPtr->getSharedVertexDataArrayId();

  DataPath edgePath({k_EdgeGroupName, "[Geometry] Edge"});
  const auto* edgeGeomPtr = dataStructure.getDataAs<EdgeGeom>(edgePath);
  REQUIRE(edgeGeomPtr != nullptr);
  nodeData.edge_data.vertices_id = edgeGeomPtr->getSharedVertexDataArrayId();
  nodeData.edge_data.edges_id = edgeGeomPtr->getEdgeListDataArrayId();

  DataPath trianglePath({k_TriangleGroupName, "[Geometry] Triangle"});
  const auto* triangleGeomPtr = dataStructure.getDataAs<TriangleGeom>(trianglePath);
  REQUIRE(triangleGeomPtr != nullptr);
  nodeData.triangle_data.vertices_id = triangleGeomPtr->getSharedVertexDataArrayId();
  nodeData.triangle_data.edges_id = triangleGeomPtr->getEdgeListDataArrayId();
  nodeData.triangle_data.triangles_id = triangleGeomPtr->getFaceListDataArrayId();

  DataPath quadPath({k_QuadGroupName, "[Geometry] Quad"});
  const auto* quadGeomPtr = dataStructure.getDataAs<QuadGeom>(quadPath);
  REQUIRE(quadGeomPtr != nullptr);
  nodeData.quad_data.vertices_id = quadGeomPtr->getSharedVertexDataArrayId();
  nodeData.quad_data.edges_id = quadGeomPtr->getEdgeListDataArrayId();
  nodeData.quad_data.quads_id = quadGeomPtr->getFaceListDataArrayId();

  return nodeData;
}

void CheckNodeGeomData(const DataStructure& dataStructure, const NodeBasedGeomData& nodeData)
{
  NodeBasedGeomData newNodeData = GetNodeGeomData(dataStructure);
  REQUIRE(newNodeData == nodeData);
}

// template <class T>
// HDF5::IdType GetId(const T& object)
//{
//   return object.getId();
// }

template <class H5ClassT>
H5ClassT CreateTempObject()
{
  if constexpr(std::is_same_v<H5ClassT, hdf5::FileIO>)
  {
    return hdf5::FileIO();
  }
  else
  {
    return H5ClassT();
  }
}

template <class H5ClassT>
H5ClassT TestH5ImplicitCopy(H5ClassT&& originalObject, std::string_view testedClassName)
{
  INFO(testedClassName)

  REQUIRE(originalObject.isValid());

  /*HDF5::IdType originalId = GetId(originalObject);*/

  H5ClassT moveConstructorObject(std::forward<H5ClassT>(originalObject));

  // REQUIRE_FALSE(originalObject.isValid());
  // REQUIRE(GetId(originalObject) != originalId);
  // REQUIRE(moveConstructorObject.isValid());
  // REQUIRE(GetId(moveConstructorObject) == originalId);

  H5ClassT moveOperatorObject = CreateTempObject<H5ClassT>();
  moveOperatorObject = std::move(moveConstructorObject);

  // REQUIRE_FALSE(moveConstructorObject.isValid());
  // REQUIRE(GetId(moveConstructorObject) != originalId);
  // REQUIRE(moveOperatorObject.isValid());
  // REQUIRE(GetId(moveOperatorObject) == originalId);

  return moveOperatorObject;
}
} // namespace

#if TEST_LEGACY
TEST_CASE("Read Legacy DREAM3D-NX Data")
{
  auto app = Application::GetOrCreateInstance();
  fs::path filepath = GetLegacyFilepath();
  REQUIRE(exists(filepath));
  {
    Result<DataStructure> result = dream3d::LoadDataStructureMetadata(filepath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    DataStructure dataStructure = result.value();

    const std::string geomName = "Small IN100";
    const auto* imagePtr = dataStructure.getDataAs<ImageGeom>(DataPath({geomName}));
    REQUIRE(imagePtr != nullptr);
    REQUIRE(Equalsf(imagePtr->getOrigin(), FloatVec3Type(-47.0f, 0.0f, -29.0f)));
    REQUIRE(imagePtr->getDimensions() == SizeVec3Type(189, 201, 117));
    REQUIRE(Equalsf(imagePtr->getSpacing(), FloatVec3Type(0.25f, 0.25f, 0.25f)));

    {
      const std::string testDCName = "DataContainer";
      DataPath testDCPath({testDCName});
      auto* testDcPtr = dataStructure.getDataAs<DataGroup>(testDCPath);
      REQUIRE(testDcPtr != nullptr);

      DataPath testAMPath = testDCPath.createChildPath("AttributeMatrix");
      REQUIRE(dataStructure.getDataAs<AttributeMatrix>(DataPath({testAMPath})) != nullptr);

      REQUIRE(dataStructure.getDataAs<Int8ArrayType>(testAMPath.createChildPath("Int8")) != nullptr);
      REQUIRE(dataStructure.getDataAs<UInt8ArrayType>(testAMPath.createChildPath("UInt8")) != nullptr);
      REQUIRE(dataStructure.getDataAs<Float32ArrayType>(testAMPath.createChildPath("Float32")) != nullptr);
      REQUIRE(dataStructure.getDataAs<Float64ArrayType>(testAMPath.createChildPath("Float64")) != nullptr);
      REQUIRE(dataStructure.getDataAs<BoolArrayType>(testAMPath.createChildPath("Bool")) != nullptr);
    }

    {
      DataPath grainDataPath({geomName, "Grain Data"});
      REQUIRE(dataStructure.getData(grainDataPath) != nullptr);
      REQUIRE(dataStructure.getDataAs<NeighborList<int32_t>>(grainDataPath.createChildPath("NeighborList")) != nullptr);
      REQUIRE(dataStructure.getDataAs<Int32ArrayType>(grainDataPath.createChildPath("NumElements")) != nullptr);
      REQUIRE(dataStructure.getDataAs<Int32ArrayType>(grainDataPath.createChildPath("NumNeighbors")) != nullptr);
    }

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}
#endif

TEST_CASE("ImageGeometryIO")
{
  fs::path dataDir = GetDataDir();

  if(!fs::exists(dataDir))
  {
    REQUIRE(fs::create_directories(dataDir));
  }

  const DataPath imageGeomPath({"ImageGeom"});
  const std::string cellDataName = "CellData";
  const DataPath cellDataPath = imageGeomPath.createChildPath(cellDataName);
  const CreateImageGeometryAction::DimensionType dims = {10, 10, 10};
  const CreateImageGeometryAction::OriginType origin = {0.0f, 0.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType spacing = {1.0f, 1.0f, 1.0f};

  DataStructure originalDataStructure;
  auto action = CreateImageGeometryAction(imageGeomPath, dims, origin, spacing, cellDataName, IGeometry::LengthUnit::Micrometer);
  Result<> actionResult = action.apply(originalDataStructure, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(actionResult);

  fs::path filePath = dataDir / "ImageGeometryIO.dream3d";

  std::string filePathString = filePath.string();

  {
    auto fileWriter = nx::core::hdf5::FileIO::WriteFile(filePath);
    REQUIRE(fileWriter.isValid());

    Result<> writeResult = hdf5::DataStructureWriter::WriteFile(originalDataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  {
    auto fileReader = nx::core::hdf5::FileIO::ReadFile(filePath);
    auto readResult = hdf5::DataStructureReader::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure newDataStructure = std::move(readResult.value());

    auto* imageGeomPtr = newDataStructure.getDataAs<ImageGeom>(imageGeomPath);
    REQUIRE(imageGeomPtr != nullptr);

    REQUIRE(imageGeomPtr->getDimensions() == SizeVec3Type(dims));
    REQUIRE(imageGeomPtr->getOrigin() == FloatVec3Type(origin));
    REQUIRE(imageGeomPtr->getSpacing() == FloatVec3Type(spacing));

    auto* cellDataPtr = imageGeomPtr->getCellData();
    REQUIRE(cellDataPtr != nullptr);

    REQUIRE(cellDataPtr->getName() == cellDataName);
    REQUIRE(imageGeomPtr->getCellDataPath() == cellDataPath);

    auto cellDataShape = std::vector<usize>(dims.crbegin(), dims.crend());

    REQUIRE(cellDataPtr->getShape() == cellDataShape);

    unit_test::CheckArraysInheritTupleDims(newDataStructure);
  }
}

// Invalid HDF5 objects must return recoverable attribute errors instead of throwing.
// This test exercises the empty-name formatting path.
TEST_CASE("HDF5 ObjectIO: Invalid object reads recover gracefully")
{
  auto app = Application::GetOrCreateInstance();

  nx::core::hdf5::GroupIO invalidGroup;
  REQUIRE_FALSE(invalidGroup.isValid());

  // getName() must not throw on an empty/invalid object.
  REQUIRE_NOTHROW(invalidGroup.getName());
  REQUIRE(invalidGroup.getName().empty());

  // Reading an attribute returns an invalid Result instead of throwing.
  Result<uint32> scalarResult;
  REQUIRE_NOTHROW(scalarResult = invalidGroup.readScalarAttribute<uint32>("SomeAttribute"));
  REQUIRE(scalarResult.invalid());

  Result<std::vector<uint32>> vectorResult;
  REQUIRE_NOTHROW(vectorResult = invalidGroup.readVectorAttribute<uint32>("SomeAttribute"));
  REQUIRE(vectorResult.invalid());
}

TEST_CASE("Node Based Geometry IO")
{
  unit_test::LoadPlugins();

  auto app = Application::GetOrCreateInstance();

  fs::path dataDir = GetDataDir();

  if(!fs::exists(dataDir))
  {
    REQUIRE(fs::create_directories(dataDir));
  }

  fs::path filePath = GetDataDir() / "NodeGeometryTest.dream3d";

  std::string filePathString = filePath.string();

  NodeBasedGeomData nodeData;

  // Write HDF5 file
  {
    DataStructure dataStructure = CreateNodeBasedGeometries();
    nodeData = GetNodeGeomData(dataStructure);
    nx::core::hdf5::FileIO fileWriter = nx::core::hdf5::FileIO::WriteFile(filePath);
    REQUIRE(fileWriter.isValid());

    auto resultH5 = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  // Read HDF5 file
  {
    auto fileReader = nx::core::hdf5::FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());

    auto readResult = hdf5::DataStructureReader::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    CheckNodeGeomData(dataStructure, nodeData);

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("NeighborList IO")
{
  auto app = Application::GetOrCreateInstance();

  fs::path dataDir = GetDataDir();

  if(!fs::exists(dataDir))
  {
    REQUIRE(fs::create_directories(dataDir));
  }

  fs::path filePath = GetDataDir() / "NeighborListTest.dream3d";

  std::string filePathString = filePath.string();

  // Write HDF5 file
  {
    DataStructure dataStructure;
    CreateNeighborList(dataStructure);
    auto fileWriter = nx::core::hdf5::FileIO::WriteFile(filePath);
    REQUIRE(fileWriter.isValid());

    Result<> writeResult = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  // Read HDF5 file
  {
    auto fileReader = nx::core::hdf5::FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());

    herr_t err = 0;
    auto readResult = hdf5::DataStructureReader::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    // auto neighborList = dataStructure.getDataAs<NeighborList<int64>>(DataPath({k_NeighborGroupName, "NeighborList"}));
    auto* neighborListPtr = dataStructure.getData(DataPath({k_NeighborGroupName, "NeighborList"}));
    REQUIRE(neighborListPtr != nullptr);

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("DataArray<bool> IO")
{
  auto app = Application::GetOrCreateInstance();

  fs::path dataDir = GetDataDir();

  if(!fs::exists(dataDir))
  {
    REQUIRE(fs::create_directories(dataDir));
  }

  fs::path filePath = GetDataDir() / "BoolArrayTest.dream3d";

  std::string filePathString = filePath.string();

  // Write HDF5 file
  {
    DataStructure dataStructure;
    CreateArrayTypes(dataStructure);
    nx::core::hdf5::FileIO fileWriter = nx::core::hdf5::FileIO::WriteFile(filePath);
    REQUIRE(fileWriter.isValid());

    Result<> writeResult = hdf5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  // Read HDF5 file
  {
    auto fileReader = nx::core::hdf5::FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());

    auto readResult = hdf5::DataStructureReader::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    REQUIRE(dataStructure.getDataAs<DataArray<int8>>(DataPath({"Int8Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<DataArray<int16>>(DataPath({"Int16Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<DataArray<int32>>(DataPath({"Int32Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<DataArray<int64>>(DataPath({"Int64Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<DataArray<uint8>>(DataPath({"UInt8Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<DataArray<uint16>>(DataPath({"UInt16Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<DataArray<uint32>>(DataPath({"UInt32Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<DataArray<uint64>>(DataPath({"UInt64Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<DataArray<float32>>(DataPath({"Float32Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<DataArray<float64>>(DataPath({"Float64Array"})) != nullptr);
    REQUIRE(dataStructure.getDataAs<StringArray>(DataPath({"StringArray"})) != nullptr);

    BoolArrayType* boolArrayPtr = dataStructure.getDataAs<BoolArrayType>(DataPath({"BoolArray"}));
    REQUIRE(boolArrayPtr != nullptr);
    AbstractDataStore<bool>& boolStore = boolArrayPtr->getDataStoreRef();
    REQUIRE(boolStore[0] == false);
    REQUIRE(boolStore[1] == true);

    StringArray* stringArrayPtr = dataStructure.getDataAs<StringArray>(DataPath({"StringArray"}));
    REQUIRE(stringArrayPtr != nullptr);
    REQUIRE(stringArrayPtr->values() == std::vector<std::string>{"Foo", "Bar", "Bazz"});
  }

  // Read HDF5 file in preflight mode to make sure StringArrays import correctly
  {
    auto fileReader = nx::core::hdf5::FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());

    auto readResult = hdf5::DataStructureReader::ReadFile(fileReader, true);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    StringArray* stringArrayPtr = dataStructure.getDataAs<StringArray>(DataPath({"StringArray"}));
    REQUIRE(stringArrayPtr != nullptr);
    REQUIRE(stringArrayPtr->size() == 3);

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("xdmf")
{
  DataStructure dataStructure;
  auto* vertexGeomPtr = VertexGeom::Create(dataStructure, "VertexGeom");
  DataObject::IdType geomId = vertexGeomPtr->getId();
  constexpr usize k_NumVerts = 100;
  std::random_device randomDevice;
  std::mt19937 generator(randomDevice());
  std::uniform_int_distribution<> intDistribution(0, 100);
  std::uniform_real_distribution<> realDistribution(-10.0f, 10.0f);
  auto vertsDataStore = std::make_unique<DataStore<float32>>(ShapeType{k_NumVerts}, ShapeType{3}, 0.0f);
  for(auto& item : vertsDataStore->createSpan())
  {
    item = realDistribution(generator);
  }
  auto* vertsPtr = DataArray<float32>::Create(dataStructure, "Verts", std::move(vertsDataStore), geomId);
  auto* vertexDataPtr = AttributeMatrix::Create(dataStructure, "VertexData", {k_NumVerts}, geomId);
  vertexGeomPtr->setVertexAttributeMatrix(*vertexDataPtr);
  vertexGeomPtr->setVertices(*vertsPtr);
  auto vertexAssociatedData = std::make_unique<DataStore<int32>>(k_NumVerts, 0);
  for(auto& item : vertexAssociatedData->createSpan())
  {
    item = intDistribution(generator);
  }
  auto* dataPtr = DataArray<int32>::Create(dataStructure, "Data", std::move(vertexAssociatedData), vertexDataPtr->getId());
  Result<> result = dream3d::WriteFile(GetDataDir() / "xdmfTest.dream3d", dataStructure, {}, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("H5 Utilities")
{
  REQUIRE_THROWS(nx::core::hdf5::GetNameFromBuffer(""));
  REQUIRE_THROWS(nx::core::hdf5::GetNameFromBuffer("/Group/"));
  std::string objectName = nx::core::hdf5::GetNameFromBuffer("/");
  REQUIRE(objectName == "/");
  objectName = nx::core::hdf5::GetNameFromBuffer("/Group/Geometry/Data");
  REQUIRE(objectName == "Data");
  objectName = nx::core::hdf5::GetNameFromBuffer("Data");
  REQUIRE(objectName == "Data");
  objectName = nx::core::hdf5::GetNameFromBuffer("/Data");
  REQUIRE(objectName == "Data");
}

TEST_CASE("HDF5ImplicitCopyReaderTest")
{
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<hdf5::FileIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<hdf5::GroupIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<hdf5::DatasetIO>);
  fs::path filePath = GetDataDir() / "HDF5ImplicitCopyReaderTest.dream3d";
  {
    DataStructure dataStructure = GetTestDataStructure();

    Result<> writeFileResult = dream3d::WriteFile(filePath, dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(writeFileResult);

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }
  auto fileReader = hdf5::FileIO::ReadFile(filePath);
  hdf5::FileIO newFileIO = TestH5ImplicitCopy(std::move(fileReader), "HDF5::FileIO");

  hdf5::GroupIO groupReader = newFileIO.openGroup("DataStructure");
  hdf5::GroupIO newGroupIO = TestH5ImplicitCopy(std::move(groupReader), "HDF5::GroupIO");

  hdf5::GroupIO groupReaderIntermediate = newGroupIO.openGroup("Geometry");
  REQUIRE(groupReaderIntermediate.isValid());

  hdf5::DatasetIO datasetReader = groupReaderIntermediate.openDataset("DataArray");
  REQUIRE(datasetReader.getId() > 0);
  TestH5ImplicitCopy(std::move(datasetReader), "HDF5::DatasetIO");
}

TEST_CASE("HDF5ImplicitCopyWriterTest")
{
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<hdf5::FileIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<hdf5::GroupIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<hdf5::DatasetIO>);

  fs::path outputPath = GetDataDir() / "HDF5ImplicitCopyWriterTest.h5";
  fs::remove(outputPath);

  auto fileWriter = hdf5::FileIO::WriteFile(outputPath);
  hdf5::FileIO newFileIO = TestH5ImplicitCopy(std::move(fileWriter), "HDF5::FileIO");

  hdf5::GroupIO groupWriter = newFileIO.createGroup("foo");
  TestH5ImplicitCopy(std::move(groupWriter), "HDF5::GroupIO");

  hdf5::DatasetIO datasetWriter = newFileIO.createDataset("bar");
  std::array<int32, 5> data = {1, 2, 3, 4, 5};
  hdf5::DatasetIO::DimsType dims = {data.size()};
  Result<> datasetResult = datasetWriter.writeSpan<int32>(dims, nonstd::span<int32>(data));
  SIMPLNX_RESULT_REQUIRE_VALID(datasetResult);
  TestH5ImplicitCopy(std::move(datasetWriter), "HDF5::DatasetIO");
}

TEST_CASE("HDF5ImplicitCopyIOTest")
{
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<hdf5::FileIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<hdf5::GroupIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<hdf5::DatasetIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<hdf5::FileIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<hdf5::GroupIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<hdf5::DatasetIO>);

  fs::path filePath = GetDataDir() / "HDF5ImplicitCopyIOTest.dream3d";
  {
    DataStructure dataStructure = GetTestDataStructure();

    Result<> writeFileResult = dream3d::WriteFile(filePath, dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(writeFileResult);

    unit_test::CheckArraysInheritTupleDims(dataStructure);
  }

  auto file1 = hdf5::FileIO::ReadFile(filePath);
  hdf5::FileIO fileIO = std::move(file1);
  hdf5::FileIO newFileIO = TestH5ImplicitCopy(std::move(fileIO), "HDF5::FileIO");

  hdf5::GroupIO groupIO = newFileIO.openGroup("DataStructure");
  hdf5::GroupIO newGroupIO = TestH5ImplicitCopy(std::move(groupIO), "HDF5::GroupIO");

  hdf5::GroupIO groupIOIntermediate = newGroupIO.openGroup("Geometry");
  REQUIRE(groupIOIntermediate.isValid());

  hdf5::DatasetIO datasetIO = groupIOIntermediate.openDataset("DataArray");
  REQUIRE(datasetIO.getId() > 0);
  TestH5ImplicitCopy(std::move(datasetIO), "HDF5::DatasetIO");
}

TEST_CASE("DataStructureWriter: WriteOptions round-trip", "[DataStructureWriter][WriteOptions]")
{
  nx::core::hdf5::DataStructureWriter writer;
  // Default compression level is 0 (off).
  REQUIRE(writer.getWriteOptions().compression_level == 0);

  nx::core::hdf5::DataStructureWriter::WriteOptions options;
  options.compression_level = 7;
  writer.setWriteOptions(options);
  REQUIRE(writer.getWriteOptions().compression_level == 7);
}

TEST_CASE("DataStructureAppend")
{
  const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / "test/Data/geoms.dream3d";
  const fs::path outputFilePath = GetDataDir() / "DataStructureAppend.dream3d";
  const DataPath originalArrayPath({"foo"});

  DataStructure baseDataStructure;

  auto createArrayResult = array_creation_utilities::CreateArray<int32>(baseDataStructure, {10, 10, 10}, {3}, originalArrayPath, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(createArrayResult);

  const auto& originalArray = baseDataStructure.getDataRefAs<Int32ArrayType>(originalArrayPath);

  Result<> writeResult = dream3d::WriteFile(outputFilePath, baseDataStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  DataStructure exemplarDataStructure = unit_test::LoadDataStructure(inputFilePath);

  usize currentTopLevelSize = baseDataStructure.getTopLevelData().size();
  for(const DataObject* objectPtr : exemplarDataStructure.getTopLevelData())
  {
    REQUIRE(objectPtr != nullptr);
    DataPath path({objectPtr->getName()});
    auto appendResult = dream3d::AppendFile(outputFilePath, exemplarDataStructure, path);
    SIMPLNX_RESULT_REQUIRE_VALID(appendResult);

    DataStructure appendedDataStructure = unit_test::LoadDataStructure(outputFilePath);

    currentTopLevelSize++;

    REQUIRE(currentTopLevelSize == appendedDataStructure.getTopLevelData().size());

    REQUIRE(appendedDataStructure.containsData(originalArrayPath));
    REQUIRE(appendedDataStructure.containsData(path));

    const DataObject& appendedOriginalArray = appendedDataStructure.getDataRef(originalArrayPath);
    REQUIRE(unit_test::comparison::CompareDataObject(originalArray, appendedOriginalArray));

    const DataObject& appendedObject = appendedDataStructure.getDataRef(path);
    const DataObject& exemplarObject = exemplarDataStructure.getDataRef(path);

    REQUIRE(unit_test::comparison::CompareDataObject(exemplarObject, appendedObject));

    unit_test::CheckArraysInheritTupleDims(appendedDataStructure);
  }

  // Failure states

  // empty path
  auto appendFailureResult1 = dream3d::AppendFile(outputFilePath, baseDataStructure, DataPath());
  SIMPLNX_RESULT_REQUIRE_INVALID(appendFailureResult1);

  // not top level
  auto appendFailureResult2 = dream3d::AppendFile(outputFilePath, baseDataStructure, DataPath({"foo", "bar"}));
  SIMPLNX_RESULT_REQUIRE_INVALID(appendFailureResult2);

  // target already exists
  auto appendFailureResult3 = dream3d::AppendFile(outputFilePath, baseDataStructure, originalArrayPath);
  SIMPLNX_RESULT_REQUIRE_INVALID(appendFailureResult3);
}

TEST_CASE("DatasetIO: writeSpan uses chunked+deflate when compression level > 0", "[DatasetIO][Compression]")
{
  const fs::path outPath = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.view()) / "dataset_compression.h5";
  fs::remove(outPath);

  const std::string datasetName = "compressed_data";
  std::vector<float> payload(1'000'000);
  std::iota(payload.begin(), payload.end(), 0.0f);

  {
    auto fileWriter = nx::core::hdf5::FileIO::WriteFile(outPath);
    REQUIRE(fileWriter.isValid());
    auto group = fileWriter.createGroup("g");
    auto dataset = group.createDataset(datasetName);
    dataset.setCompressionLevel(5);
    auto wr = dataset.writeSpan<float>({payload.size()}, nonstd::span<const float>(payload.data(), payload.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(wr);
  }

  auto info = nx::core::unit_test::ProbeHdf5Dataset(outPath, "g/" + datasetName);
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::unit_test::DatasetLayout::Chunked);
  REQUIRE(info->has_deflate);
  REQUIRE(info->deflate_level == 5);
}

TEST_CASE("DatasetIO: writeSpan stays contiguous when compression level is 0", "[DatasetIO][Compression]")
{
  const fs::path outPath = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.view()) / "dataset_no_compression.h5";
  fs::remove(outPath);

  std::vector<float> payload(1'000'000, 3.14f);

  {
    auto fileWriter = nx::core::hdf5::FileIO::WriteFile(outPath);
    REQUIRE(fileWriter.isValid());
    auto group = fileWriter.createGroup("g");
    auto dataset = group.createDataset("d");
    // Leave m_CompressionLevel at its 0 default — the opt-in path is not triggered.
    auto wr = dataset.writeSpan<float>({payload.size()}, nonstd::span<const float>(payload.data(), payload.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(wr);
  }

  auto info = nx::core::unit_test::ProbeHdf5Dataset(outPath, "g/d");
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::unit_test::DatasetLayout::Contiguous);
  REQUIRE(info->has_deflate == false);
}

TEST_CASE("DatasetIO: writeSpan bypasses chunking for small arrays even with compression on", "[DatasetIO][Compression]")
{
  const fs::path outPath = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.view()) / "dataset_small_bypass.h5";
  fs::remove(outPath);

  // 100 floats = 400 bytes, below the 16 KiB bypass — DCPL helper returns H5P_DEFAULT.
  std::vector<float> payload(100, 1.0f);

  {
    auto fileWriter = nx::core::hdf5::FileIO::WriteFile(outPath);
    REQUIRE(fileWriter.isValid());
    auto group = fileWriter.createGroup("g");
    auto dataset = group.createDataset("d");
    dataset.setCompressionLevel(9);
    auto wr = dataset.writeSpan<float>({payload.size()}, nonstd::span<const float>(payload.data(), payload.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(wr);
  }

  auto info = nx::core::unit_test::ProbeHdf5Dataset(outPath, "g/d");
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::unit_test::DatasetLayout::Contiguous);
  REQUIRE(info->has_deflate == false);
}

TEST_CASE("DatasetIO: value getters release temporary datatype handles", "[simplnx][HDF5][DatasetIO][Lifetime]")
{
  const fs::path path = GetDataDir() / "dataset_type_getter_lifetime.h5";
  fs::create_directories(path.parent_path());
  DatatypeFixtureFile cleanup{path};
  auto file = hdf5::FileIO::WriteFile(path);
  REQUIRE(file.isValid());
  auto dataset = file.createDataset("values");
  const std::vector<int32> values{7, 11, 19, 23};
  const auto written = dataset.writeSpan<int32>({values.size()}, nonstd::span<const int32>(values.data(), values.size()));
  REQUIRE(written.valid());
  REQUIRE(dataset.getId() >= 0);
  const int64 baseline = CountHdf5DatatypeIds();
  REQUIRE(baseline >= 0);
  constexpr usize k_Repetitions = 32;

  SECTION("getClassType returns the class without retaining temporary identifiers")
  {
    for(usize repetition = 0; repetition < k_Repetitions; ++repetition)
    {
      CHECK(dataset.getClassType() == H5T_INTEGER);
    }
    CHECK(CountHdf5DatatypeIds() == baseline);
  }
  SECTION("getTypeSize returns the scalar size without retaining temporary identifiers")
  {
    for(usize repetition = 0; repetition < k_Repetitions; ++repetition)
    {
      CHECK(dataset.getTypeSize() == sizeof(int32));
    }
    CHECK(CountHdf5DatatypeIds() == baseline);
  }
  SECTION("getTypeId preserves caller ownership until explicit close")
  {
    CallerOwnedDatatype type(dataset.getTypeId());
    REQUIRE(type.getId() >= 0);
    {
      // Getter calls lock themselves. Only direct HDF5 queries belong inside this leaf scope.
      std::lock_guard<std::mutex> lock(hdf5::Support::ApiLock());
      CHECK(H5Iis_valid(type.getId()) > 0);
      CHECK(H5Tget_class(type.getId()) == H5T_INTEGER);
      CHECK(H5Tget_size(type.getId()) == sizeof(int32));
    }
    CHECK(CountHdf5DatatypeIds() == baseline + 1);
    REQUIRE(type.close() >= 0);
    CHECK(CountHdf5DatatypeIds() == baseline);
  }
}

TEST_CASE("DatasetIO: move assignment preserves single dataset ownership", "[simplnx][HDF5][DatasetIO][Lifetime]")
{
  const fs::path path = GetDataDir() / "dataset_move_assignment_lifetime.h5";
  fs::create_directories(path.parent_path());
  DatatypeFixtureFile cleanup{path};
  const std::vector<int32> firstValues{11};
  const std::vector<int32> secondValues{22};
  const int64 initialDatasetCount = CountHdf5DatasetIds();
  REQUIRE(initialDatasetCount >= 0);

  {
    auto file = hdf5::FileIO::WriteFile(path);
    REQUIRE(file.isValid());
    {
      auto first = file.createDataset("first");
      REQUIRE(first.writeSpan<int32>({firstValues.size()}, nonstd::span<const int32>(firstValues.data(), firstValues.size())).valid());
      auto second = file.createDataset("second");
      REQUIRE(second.writeSpan<int32>({secondValues.size()}, nonstd::span<const int32>(secondValues.data(), secondValues.size())).valid());
    }
    REQUIRE(CountHdf5DatasetIds() == initialDatasetCount);

    {
      auto destination = file.openDataset("first");
      auto source = file.openDataset("second");
      const hid_t displacedId = destination.getId();
      const hid_t transferredId = source.getId();
      REQUIRE(displacedId > 0);
      REQUIRE(transferredId > 0);
      REQUIRE(CountHdf5DatasetIds() == initialDatasetCount + 2);
      source.setCompressionLevel(6);

      destination = std::move(source);

      CHECK_FALSE(source.isValid());
      CHECK(destination.getId() == transferredId);
      CHECK(destination.getCompressionLevel() == 6);
      CHECK_FALSE(IsHdf5IdValid(displacedId));
      CHECK(IsHdf5IdValid(transferredId));
      CHECK(CountHdf5DatasetIds() == initialDatasetCount + 1);
    }
    REQUIRE(CountHdf5DatasetIds() == initialDatasetCount);

    {
      hdf5::DatasetIO destination;
      auto source = file.openDataset("first");
      const hid_t transferredId = source.getId();
      REQUIRE(transferredId > 0);
      REQUIRE(CountHdf5DatasetIds() == initialDatasetCount + 1);

      destination = std::move(source);

      CHECK_FALSE(source.isValid());
      CHECK(destination.getId() == transferredId);
      CHECK(IsHdf5IdValid(transferredId));
      CHECK(CountHdf5DatasetIds() == initialDatasetCount + 1);
    }
    REQUIRE(CountHdf5DatasetIds() == initialDatasetCount);

    {
      auto destination = file.openDataset("first");
      auto source = file.openDataset("second");
      const hid_t displacedId = destination.getId();
      REQUIRE(displacedId > 0);
      REQUIRE(CountHdf5DatasetIds() == initialDatasetCount + 1);

      destination = std::move(source);

      CHECK_FALSE(source.isValid());
      CHECK_FALSE(destination.isValid());
      CHECK(destination.getNamePath() == "second");
      CHECK_FALSE(IsHdf5IdValid(displacedId));
      CHECK(CountHdf5DatasetIds() == initialDatasetCount);
      CHECK(destination.getId() > 0);
      CHECK(CountHdf5DatasetIds() == initialDatasetCount + 1);
    }
    REQUIRE(CountHdf5DatasetIds() == initialDatasetCount);

    {
      hdf5::DatasetIO destination;
      hdf5::DatasetIO source;

      destination = std::move(source);

      CHECK_FALSE(source.isValid());
      CHECK_FALSE(destination.isValid());
      CHECK(CountHdf5DatasetIds() == initialDatasetCount);
    }

    {
      auto dataset = file.openDataset("first");
      const hid_t originalId = dataset.getId();
      REQUIRE(originalId > 0);
      dataset.setCompressionLevel(4);
      auto* aliasPtr = &dataset;

      dataset = std::move(*aliasPtr);

      CHECK(dataset.getId() == originalId);
      CHECK(dataset.getCompressionLevel() == 4);
      CHECK(IsHdf5IdValid(originalId));
      CHECK(CountHdf5DatasetIds() == initialDatasetCount + 1);
    }
    REQUIRE(CountHdf5DatasetIds() == initialDatasetCount);

    {
      auto dataset = file.openDataset("second");
      auto* aliasPtr = &dataset;

      dataset = std::move(*aliasPtr);

      CHECK_FALSE(dataset.isValid());
      CHECK(dataset.getNamePath() == "second");
      CHECK(CountHdf5DatasetIds() == initialDatasetCount);
    }
  }

  REQUIRE(CountHdf5DatasetIds() == initialDatasetCount);
  auto replacement = hdf5::FileIO::WriteFile(path);
  REQUIRE(replacement.isValid());
}

TEST_CASE("HDF5 ApiLock serializes access via H5SUPPORT_MUTEX_LOCK", "[simplnx][HDF5]")
{
#ifdef H5Support_USE_MUTEX
  // Each thread increments one shared counter under H5SUPPORT_MUTEX_LOCK().
  // The exact total proves that one shared ApiLock protects every update.
  constexpr int k_Threads = 8;
  constexpr int k_Iters = 20000;
  int counter = 0;
  std::vector<std::thread> workers;
  workers.reserve(k_Threads);
  for(int t = 0; t < k_Threads; ++t)
  {
    workers.emplace_back([&counter]() {
      for(int i = 0; i < k_Iters; ++i)
      {
        H5SUPPORT_MUTEX_LOCK();
        ++counter;
      }
    });
  }
  for(auto& worker : workers)
  {
    worker.join();
  }
  REQUIRE(counter == k_Threads * k_Iters);
#else
  SUCCEED("H5Support_USE_MUTEX disabled; H5SUPPORT_MUTEX_LOCK is intentionally a no-op");
#endif
}

namespace
{
using ConnectivityListsType = std::vector<std::vector<IGeometry::MeshIndexType>>;

const std::string k_ConnectivityTriangleName = "ConnectivityTriangles";
const std::string k_ConnectivityEdgeName = "ConnectivityEdges";
// Two triangles sharing the diagonal 0-2 of a unit square, and a two-edge path 0-1-2.
const ConnectivityListsType k_TriangleIncidence = {{0, 1}, {0}, {0, 1}, {1}};
const ConnectivityListsType k_TriangleNeighbors = {{1}, {0}};
const ConnectivityListsType k_EdgeIncidence = {{0}, {0, 1}, {1}};
const ConnectivityListsType k_EdgeNeighbors = {{1}, {0}};

template <typename T>
DataArray<T>& CreateConnectivityFixtureArray(DataStructure& dataStructure, const DataPath& path, usize tuples, usize components, const std::vector<T>& values)
{
  const auto created = array_creation_utilities::CreateArray<T>(dataStructure, {tuples}, {components}, path, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(created);
  auto* arrayPtr = dataStructure.getDataAs<DataArray<T>>(path);
  REQUIRE(arrayPtr != nullptr);
  REQUIRE(arrayPtr->getSize() == values.size());
  for(usize index = 0; index < values.size(); ++index)
  {
    (*arrayPtr)[index] = values[index];
  }
  return *arrayPtr;
}

TriangleGeom& CreateConnectivityTriangles(DataStructure& dataStructure)
{
  auto* geometryPtr = TriangleGeom::Create(dataStructure, k_ConnectivityTriangleName);
  REQUIRE(geometryPtr != nullptr);
  const DataPath geometryPath({k_ConnectivityTriangleName});
  geometryPtr->setVertices(CreateConnectivityFixtureArray<float32>(dataStructure, geometryPath.createChildPath("Vertices"), 4, 3, {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0}));
  geometryPtr->setFaceList(CreateConnectivityFixtureArray<IGeometry::MeshIndexType>(dataStructure, geometryPath.createChildPath("Faces"), 2, 3, {0, 1, 2, 0, 2, 3}));
  return *geometryPtr;
}

EdgeGeom& CreateConnectivityEdges(DataStructure& dataStructure)
{
  auto* geometryPtr = EdgeGeom::Create(dataStructure, k_ConnectivityEdgeName);
  REQUIRE(geometryPtr != nullptr);
  const DataPath geometryPath({k_ConnectivityEdgeName});
  geometryPtr->setVertices(CreateConnectivityFixtureArray<float32>(dataStructure, geometryPath.createChildPath("Vertices"), 3, 3, {0, 0, 0, 1, 0, 0, 2, 0, 0}));
  geometryPtr->setEdgeList(CreateConnectivityFixtureArray<IGeometry::MeshIndexType>(dataStructure, geometryPath.createChildPath("Edges"), 2, 2, {0, 1, 1, 2}));
  return *geometryPtr;
}

ConnectivityListsType ReadConnectivity(const DataStructure& dataStructure, const std::optional<DataObject::IdType>& id)
{
  REQUIRE(id.has_value());
  const auto* listPtr = dataStructure.getDataAs<IGeometry::ElementDynamicListType>(id.value());
  REQUIRE(listPtr != nullptr);
  ConnectivityListsType lists(listPtr->size());
  for(usize index = 0; index < listPtr->size(); ++index)
  {
    const auto* membersPtr = listPtr->getElementListPointer(index);
    lists[index].assign(membersPtr, membersPtr + listPtr->getNumberOfElements(index));
    std::sort(lists[index].begin(), lists[index].end());
  }
  return lists;
}

/**
 * @brief Requires that a DataStructure read from a file holds no connectivity cache anywhere.
 * @param dataStructure Reloaded structure to inspect.
 * @param geometryPath Geometry whose connectivity associations must be unset.
 */
void RequireNoConnectivity(const DataStructure& dataStructure, const DataPath& geometryPath)
{
  const auto* geometryPtr = dataStructure.getDataAs<INodeGeometry1D>(geometryPath);
  REQUIRE(geometryPtr != nullptr);
  CHECK_FALSE(geometryPtr->getElementContainingVertId().has_value());
  CHECK_FALSE(geometryPtr->getElementNeighborsId().has_value());
  for(const DataObject::IdType id : dataStructure.getAllDataObjectIds())
  {
    const DataObject* objectPtr = dataStructure.getData(id);
    REQUIRE(objectPtr != nullptr);
    CAPTURE(objectPtr->getName());
    CHECK(objectPtr->getDataObjectType() != DataObject::Type::DynamicListArray);
  }
}

/**
 * @struct LiveConnectivity
 * @brief Captures the identity and contents of a geometry's live connectivity.
 */
struct LiveConnectivity
{
  LiveConnectivity() = default;
  /**
   * @brief Captures both connectivity lists of a geometry.
   * @param dataStructure Structure that owns the geometry and its lists.
   * @param geometry Geometry to capture.
   * @pre Both of the geometry's connectivity associations name existing ElementDynamicList objects.
   */
  LiveConnectivity(const DataStructure& dataStructure, const INodeGeometry1D& geometry)
  : incidence_id(geometry.getElementContainingVertId())
  , neighbors_id(geometry.getElementNeighborsId())
  , incidence_object(dataStructure.getData(incidence_id.value()))
  , neighbors_object(dataStructure.getData(neighbors_id.value()))
  , incidence(ReadConnectivity(dataStructure, incidence_id))
  , neighbors(ReadConnectivity(dataStructure, neighbors_id))
  {
  }
  ~LiveConnectivity() noexcept = default;

  LiveConnectivity(const LiveConnectivity&) = default;
  LiveConnectivity(LiveConnectivity&&) noexcept = default;
  LiveConnectivity& operator=(const LiveConnectivity&) = default;
  LiveConnectivity& operator=(LiveConnectivity&&) noexcept = default;

  std::optional<DataObject::IdType> incidence_id;
  std::optional<DataObject::IdType> neighbors_id;
  const DataObject* incidence_object = nullptr;
  const DataObject* neighbors_object = nullptr;
  ConnectivityListsType incidence;
  ConnectivityListsType neighbors;
};

void RequireLiveConnectivity(const DataStructure& dataStructure, const INodeGeometry1D& geometry, const LiveConnectivity& expected)
{
  REQUIRE(geometry.getElementContainingVertId() == expected.incidence_id);
  REQUIRE(geometry.getElementNeighborsId() == expected.neighbors_id);
  CHECK(dataStructure.getData(expected.incidence_id.value()) == expected.incidence_object);
  CHECK(dataStructure.getData(expected.neighbors_id.value()) == expected.neighbors_object);
  CHECK(ReadConnectivity(dataStructure, expected.incidence_id) == expected.incidence);
  CHECK(ReadConnectivity(dataStructure, expected.neighbors_id) == expected.neighbors);
}

std::vector<std::string> TopLevelNamesInFile(const fs::path& filePath, DataObject::IdType& nextId)
{
  auto file = hdf5::FileIO::ReadFile(filePath);
  REQUIRE(file.isValid());
  auto group = file.openGroup(nx::core::constants::k_DataStructureTag);
  REQUIRE(group.isValid());
  auto nextIdResult = group.readScalarAttribute<DataObject::IdType>(nx::core::constants::k_NextIdTag);
  SIMPLNX_RESULT_REQUIRE_VALID(nextIdResult);
  nextId = nextIdResult.value();
  auto names = group.getChildNames();
  std::sort(names.begin(), names.end());
  return names;
}
} // namespace

TEST_CASE("DataStructureWriter: geometry connectivity is omitted and rebuilt after reload", "[simplnx][HDF5][Connectivity]")
{
  const fs::path filePath = GetDataDir() / "ConnectivityRoundTrip.dream3d";
  DataStructure dataStructure;
  auto& triangles = CreateConnectivityTriangles(dataStructure);
  auto& edges = CreateConnectivityEdges(dataStructure);
  const auto triangleSupport = triangles.findElementNeighbors(false);
  SIMPLNX_RESULT_REQUIRE_VALID(triangleSupport);
  const auto edgeSupport = edges.findElementNeighbors(false);
  SIMPLNX_RESULT_REQUIRE_VALID(edgeSupport);
  const LiveConnectivity triangleBefore(dataStructure, triangles);
  const LiveConnectivity edgeBefore(dataStructure, edges);
  REQUIRE(triangleBefore.incidence == k_TriangleIncidence);
  REQUIRE(triangleBefore.neighbors == k_TriangleNeighbors);
  REQUIRE(edgeBefore.incidence == k_EdgeIncidence);
  REQUIRE(edgeBefore.neighbors == k_EdgeNeighbors);

  const auto saveResult = hdf5::DataStructureWriter::WriteFile(dataStructure, filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(saveResult);
  RequireLiveConnectivity(dataStructure, triangles, triangleBefore);
  RequireLiveConnectivity(dataStructure, edges, edgeBefore);

  auto readResult = hdf5::DataStructureReader::ReadFile(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  DataStructure& reloaded = readResult.value();
  RequireNoConnectivity(reloaded, DataPath({k_ConnectivityTriangleName}));
  RequireNoConnectivity(reloaded, DataPath({k_ConnectivityEdgeName}));
  for(const auto& path : {DataPath({k_ConnectivityTriangleName, "Vertices"}), DataPath({k_ConnectivityTriangleName, "Faces"}), DataPath({k_ConnectivityEdgeName, "Vertices"}),
                          DataPath({k_ConnectivityEdgeName, "Edges"})})
  {
    CAPTURE(path.toString());
    REQUIRE(unit_test::comparison::CompareDataObject(dataStructure.getDataRef(path), reloaded.getDataRef(path)));
  }

  auto& reloadedTriangles = reloaded.getDataRefAs<TriangleGeom>(DataPath({k_ConnectivityTriangleName}));
  auto& reloadedEdges = reloaded.getDataRefAs<EdgeGeom>(DataPath({k_ConnectivityEdgeName}));
  const auto reloadedTriangleSupport = reloadedTriangles.findElementNeighbors(false);
  SIMPLNX_RESULT_REQUIRE_VALID(reloadedTriangleSupport);
  const auto reloadedEdgeSupport = reloadedEdges.findElementNeighbors(false);
  SIMPLNX_RESULT_REQUIRE_VALID(reloadedEdgeSupport);
  CHECK(ReadConnectivity(reloaded, reloadedTriangles.getElementContainingVertId()) == k_TriangleIncidence);
  CHECK(ReadConnectivity(reloaded, reloadedTriangles.getElementNeighborsId()) == k_TriangleNeighbors);
  CHECK(ReadConnectivity(reloaded, reloadedEdges.getElementContainingVertId()) == k_EdgeIncidence);
  CHECK(ReadConnectivity(reloaded, reloadedEdges.getElementNeighborsId()) == k_EdgeNeighbors);
  unit_test::CheckArraysInheritTupleDims(reloaded);
}

TEST_CASE("DataStructureWriter: shared connectivity is omitted at every parent", "[simplnx][HDF5][Connectivity]")
{
  const fs::path filePath = GetDataDir() / "ConnectivityShared.dream3d";
  DataStructure dataStructure;
  auto& triangles = CreateConnectivityTriangles(dataStructure);
  const auto triangleSupport = triangles.findElementNeighbors(false);
  SIMPLNX_RESULT_REQUIRE_VALID(triangleSupport);
  auto* groupPtr = DataGroup::Create(dataStructure, "SharedGroup");
  REQUIRE(groupPtr != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(triangles.getElementNeighborsId().value(), groupPtr->getId()));
  const LiveConnectivity before(dataStructure, triangles);

  const auto saveResult = hdf5::DataStructureWriter::WriteFile(dataStructure, filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(saveResult);
  RequireLiveConnectivity(dataStructure, triangles, before);

  auto readResult = hdf5::DataStructureReader::ReadFile(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  RequireNoConnectivity(readResult.value(), DataPath({k_ConnectivityTriangleName}));
  const auto* reloadedGroupPtr = readResult.value().getDataAs<DataGroup>(DataPath({"SharedGroup"}));
  REQUIRE(reloadedGroupPtr != nullptr);
  CHECK(reloadedGroupPtr->getSize() == 0);
}

TEST_CASE("DataStructureWriter: appended geometry drops its connectivity", "[simplnx][HDF5][Connectivity]")
{
  const fs::path filePath = GetDataDir() / "ConnectivityAppend.dream3d";
  DataStructure baseDataStructure;
  CreateConnectivityFixtureArray<int32>(baseDataStructure, DataPath({"foo"}), 2, 1, {7, 8});
  const auto baseSaveResult = hdf5::DataStructureWriter::WriteFile(baseDataStructure, filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(baseSaveResult);

  DataStructure dataStructure;
  auto& triangles = CreateConnectivityTriangles(dataStructure);
  const auto triangleSupport = triangles.findElementNeighbors(false);
  SIMPLNX_RESULT_REQUIRE_VALID(triangleSupport);
  const LiveConnectivity before(dataStructure, triangles);

  const auto appendResult = hdf5::DataStructureWriter::AppendFile(filePath, dataStructure, DataPath({k_ConnectivityTriangleName}));
  SIMPLNX_RESULT_REQUIRE_VALID(appendResult);
  RequireLiveConnectivity(dataStructure, triangles, before);

  auto readResult = hdf5::DataStructureReader::ReadFile(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  const DataStructure& reloaded = readResult.value();
  RequireNoConnectivity(reloaded, DataPath({k_ConnectivityTriangleName}));
  const auto& reloadedTriangles = reloaded.getDataRefAs<TriangleGeom>(DataPath({k_ConnectivityTriangleName}));
  CHECK(reloadedTriangles.getVertices() == reloaded.getDataAs<Float32ArrayType>(DataPath({k_ConnectivityTriangleName, "Vertices"})));
  CHECK(reloadedTriangles.getFaces() == reloaded.getDataAs<IGeometry::SharedFaceListType>(DataPath({k_ConnectivityTriangleName, "Faces"})));
  REQUIRE(unit_test::comparison::CompareDataObject(dataStructure.getDataRef(DataPath({k_ConnectivityTriangleName, "Faces"})), reloaded.getDataRef(DataPath({k_ConnectivityTriangleName, "Faces"}))));
  REQUIRE(reloaded.containsData(DataPath({"foo"})));
}

TEST_CASE("DataStructureWriter: appended geometry drops an external connectivity cache", "[simplnx][HDF5][Connectivity]")
{
  const fs::path filePath = GetDataDir() / "ConnectivityAppendExternal.dream3d";
  DataStructure baseDataStructure;
  CreateConnectivityFixtureArray<int32>(baseDataStructure, DataPath({"foo"}), 2, 1, {7, 8});
  const auto baseSaveResult = hdf5::DataStructureWriter::WriteFile(baseDataStructure, filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(baseSaveResult);

  // The geometry, its two arrays and then the external cache take consecutive IDs. Renumbering the appended
  // geometry from the file's next ID then assigns the cache's old ID to a retained array.
  DataStructure dataStructure;
  auto& triangles = CreateConnectivityTriangles(dataStructure);
  auto* externalCachePtr = IGeometry::ElementDynamicListType::Create(dataStructure, "ExternalIncidence", std::nullopt);
  REQUIRE(externalCachePtr != nullptr);
  triangles.setElementContainingVertId(externalCachePtr->getId());
  const auto incidenceResult = triangles.findElementsContainingVert(false);
  SIMPLNX_RESULT_REQUIRE_VALID(incidenceResult);
  REQUIRE(triangles.getElementContainingVertId() == externalCachePtr->getId());
  const DataObject::IdType cacheId = externalCachePtr->getId();
  DataObject::IdType nextIdBefore = 0;
  const auto namesBefore = TopLevelNamesInFile(filePath, nextIdBefore);
  REQUIRE(nextIdBefore <= cacheId);
  REQUIRE(cacheId < nextIdBefore + 3);

  SECTION("Appending the geometry")
  {
    const auto appendResult = hdf5::DataStructureWriter::AppendFile(filePath, dataStructure, DataPath({k_ConnectivityTriangleName}));
    SIMPLNX_RESULT_REQUIRE_VALID(appendResult);
    CHECK(triangles.getElementContainingVertId() == cacheId);
    CHECK(dataStructure.getData(cacheId) == externalCachePtr);

    auto readResult = hdf5::DataStructureReader::ReadFile(filePath);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    RequireNoConnectivity(readResult.value(), DataPath({k_ConnectivityTriangleName}));
  }
  SECTION("Appending the cache itself")
  {
    const auto appendResult = hdf5::DataStructureWriter::AppendFile(filePath, dataStructure, DataPath({"ExternalIncidence"}));
    SIMPLNX_RESULT_REQUIRE_VALID(appendResult);
    DataObject::IdType nextIdAfter = 0;
    CHECK(TopLevelNamesInFile(filePath, nextIdAfter) == namesBefore);
    CHECK(nextIdAfter == nextIdBefore);
  }
}

TEST_CASE("DataStructureWriter: stale connectivity attributes are removed", "[simplnx][HDF5][Connectivity]")
{
  const bool incidence = GENERATE(false, true);
  const bool neighbors = GENERATE(false, true);
  const bool plainArray = GENERATE(false, true);
  if(plainArray && (incidence || neighbors))
  {
    return;
  }
  CAPTURE(incidence, neighbors, plainArray);
  const fs::path filePath = GetDataDir() / "ConnectivityStaleAttributes.dream3d";
  constexpr DataObject::IdType k_StaleId = 9999;

  DataStructure dataStructure;
  auto& triangles = CreateConnectivityTriangles(dataStructure);
  if(neighbors)
  {
    const auto neighborsResult = triangles.findElementNeighbors(false);
    SIMPLNX_RESULT_REQUIRE_VALID(neighborsResult);
    if(!incidence)
    {
      REQUIRE(dataStructure.removeData(triangles.getElementContainingVertId().value()));
      triangles.setElementContainingVertId(std::nullopt);
    }
  }
  else if(incidence)
  {
    const auto incidenceResult = triangles.findElementsContainingVert(false);
    SIMPLNX_RESULT_REQUIRE_VALID(incidenceResult);
  }
  std::optional<DataObject::IdType> plainId;
  if(plainArray)
  {
    // An ordinary array in the neighbors field is not a cache, so its ID is still written.
    plainId = CreateConnectivityFixtureArray<uint64>(dataStructure, DataPath({k_ConnectivityTriangleName, "PlainNeighbors"}), 2, 1, {1, 0}).getId();
    triangles.setElementNeighborsId(plainId);
  }

  {
    auto file = hdf5::FileIO::WriteFile(filePath);
    REQUIRE(file.isValid());
    {
      auto dataStructureGroup = file.createGroup(nx::core::constants::k_DataStructureTag);
      auto geometryGroup = dataStructureGroup.createGroup(k_ConnectivityTriangleName);
      const auto staleIncidenceResult = geometryGroup.writeScalarAttribute(io_constants::k_ElementContainingVertTag, k_StaleId);
      SIMPLNX_RESULT_REQUIRE_VALID(staleIncidenceResult);
      const auto staleNeighborsResult = geometryGroup.writeScalarAttribute(io_constants::k_ElementNeighborsTag, k_StaleId);
      SIMPLNX_RESULT_REQUIRE_VALID(staleNeighborsResult);
    }
    const auto saveResult = hdf5::DataStructureWriter::WriteFile(dataStructure, file);
    SIMPLNX_RESULT_REQUIRE_VALID(saveResult);
  }

  {
    auto file = hdf5::FileIO::ReadFile(filePath);
    REQUIRE(file.isValid());
    auto geometryGroup = file.openGroup(nx::core::constants::k_DataStructureTag).openGroup(k_ConnectivityTriangleName);
    REQUIRE(geometryGroup.isValid());
    CHECK_FALSE(geometryGroup.hasAttribute(io_constants::k_ElementContainingVertTag));
    CHECK(geometryGroup.hasAttribute(io_constants::k_ElementNeighborsTag) == plainArray);
  }

  auto readResult = hdf5::DataStructureReader::ReadFile(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);
  const DataStructure& reloaded = readResult.value();
  const auto& reloadedTriangles = reloaded.getDataRefAs<TriangleGeom>(DataPath({k_ConnectivityTriangleName}));
  CHECK_FALSE(reloadedTriangles.getElementContainingVertId().has_value());
  if(plainArray)
  {
    const auto* plainPtr = reloaded.getData(DataPath({k_ConnectivityTriangleName, "PlainNeighbors"}));
    REQUIRE(plainPtr != nullptr);
    CHECK(reloadedTriangles.getElementNeighborsId() == plainPtr->getId());
  }
  else
  {
    RequireNoConnectivity(reloaded, DataPath({k_ConnectivityTriangleName}));
  }
}

TEST_CASE("GroupIO: removeAttribute reports every HDF5 failure", "[simplnx][HDF5][Connectivity]")
{
  const fs::path filePath = GetDataDir() / "GroupIORemoveAttribute.dream3d";
  {
    auto file = hdf5::FileIO::WriteFile(filePath);
    REQUIRE(file.isValid());
    auto group = file.createGroup("Group");
    const auto removedWrite = group.writeScalarAttribute("Removed", int32{1});
    SIMPLNX_RESULT_REQUIRE_VALID(removedWrite);
    const auto keptWrite = group.writeScalarAttribute("Kept", int32{2});
    SIMPLNX_RESULT_REQUIRE_VALID(keptWrite);
    const auto missingRemoval = group.removeAttribute("Missing");
    SIMPLNX_RESULT_REQUIRE_VALID(missingRemoval);
    const auto removal = group.removeAttribute("Removed");
    SIMPLNX_RESULT_REQUIRE_VALID(removal);
    CHECK_FALSE(group.hasAttribute("Removed"));
    CHECK(group.hasAttribute("Kept"));
  }
  {
    auto file = hdf5::FileIO::ReadFile(filePath);
    REQUIRE(file.isValid());
    auto group = file.openGroup("Group");
    REQUIRE(group.isValid());
    const auto result = group.removeAttribute("Kept");
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -18802);
    CHECK(result.errors()[0].message == "Cannot remove attribute 'Kept' from HDF5 object '/Group'.");
    CHECK(group.hasAttribute("Kept"));
  }
  {
    hdf5::GroupIO invalidGroup;
    const auto result = invalidGroup.removeAttribute("Kept");
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    CHECK(result.errors()[0].code == -18801);
    CHECK(result.errors()[0].message.starts_with("Cannot check whether attribute 'Kept' exists on HDF5 object '"));
  }
}

TEST_CASE("DataStructureWriter: a dynamic list that is not geometry connectivity still fails", "[simplnx][HDF5][Connectivity]")
{
  const fs::path filePath = GetDataDir() / "ConnectivityStandalone.dream3d";
  DataStructure dataStructure;
  REQUIRE(IGeometry::ElementDynamicListType::Create(dataStructure, "Standalone", std::nullopt) != nullptr);
  const auto result = hdf5::DataStructureWriter::WriteFile(dataStructure, filePath);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -5);
  CHECK(result.errors()[0].message == "Could not find IO factory for datatype: DynamicListArray");
}

TEST_CASE("DataStructureWriter: an edge geometry reports a failed base write", "[simplnx][HDF5][Connectivity]")
{
  const fs::path filePath = GetDataDir() / "ConnectivityEdgeBaseFailure.dream3d";
  DataStructure dataStructure;
  auto& edges = CreateConnectivityEdges(dataStructure);
  // A child that no geometry association names is not a cache, so writing the geometry's children fails.
  REQUIRE(IGeometry::ElementDynamicListType::Create(dataStructure, "Unassociated", edges.getId()) != nullptr);
  const auto result = hdf5::DataStructureWriter::WriteFile(dataStructure, filePath);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors()[0].code == -5);
  CHECK(result.errors()[0].message == "Could not find IO factory for datatype: DynamicListArray");
}

TEST_CASE("ImageGeomIO rejects stored invalid spacing", "[simplnx][HDF5][ImageGeom]")
{
  const fs::path filePath = GetDataDir() / "ImageGeomInvalidSpacing.dream3d";
  {
    DataStructure dataStructure;
    const CreateImageGeometryAction action(DataPath({"Stored Image"}), {2, 3, 1}, {0.0f, 0.0f, 0.0f}, {0.5f, 2.0f, 1.0f}, "Cell Data");
    const auto createResult = action.apply(dataStructure, IDataAction::Mode::Execute);
    SIMPLNX_RESULT_REQUIRE_VALID(createResult);
    auto file = hdf5::FileIO::WriteFile(filePath);
    REQUIRE(file.isValid());
    const auto writeResult = hdf5::DataStructureWriter::WriteFile(dataStructure, file);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

    auto group = file.openGroup(nx::core::constants::k_DataStructureTag).openGroup("Stored Image");
    REQUIRE(group.isValid());
    const auto removeResult = group.removeAttribute(io_constants::k_H5Spacing);
    SIMPLNX_RESULT_REQUIRE_VALID(removeResult);
    const auto spacingResult = group.writeVectorAttribute(io_constants::k_H5Spacing, std::vector<float32>{0.5f, 2.0f, 0.0f});
    SIMPLNX_RESULT_REQUIRE_VALID(spacingResult);
  }

  const bool metadataOnly = GENERATE(false, true);
  const auto result = hdf5::DataStructureReader::ReadFile(filePath, metadataOnly);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  CHECK(result.errors().front().code == ImageGeom::k_InvalidSpacing);
  CHECK(result.errors().front().message.find("Stored Image") != std::string::npos);
  CHECK(result.errors().front().message.find(filePath.filename().string()) != std::string::npos);
  CHECK(result.errors().front().message.find('Z') != std::string::npos);
}

TEST_CASE("RectGridGeomIO rejects invalid external bounds on full and selective reads", "[simplnx][HDF5][RectGridGeom]")
{
  const fs::path filePath = GetDataDir() / "RectGridInvalidBounds.dream3d";
  const DataPath geometryPath({"Stored Grid"});
  const DataPath boundsPath({"Z Bound Coordinates"});
  {
    DataStructure dataStructure;
    auto* geomPtr = RectGridGeom::Create(dataStructure, geometryPath.getTargetName());
    REQUIRE(geomPtr != nullptr);
    geomPtr->setDimensions({1, 1, 1});
    // This array lives outside the geometry's HDF5 group and is visited after it.
    auto* boundsPtr = Float32ArrayType::CreateWithStore<DataStore<float32>>(dataStructure, boundsPath.getTargetName(), ShapeType{2}, ShapeType{1});
    REQUIRE(boundsPtr != nullptr);
    boundsPtr->setValue(0, 0.0f);
    boundsPtr->setValue(1, 1.0f);
    const auto boundResult = geomPtr->setBounds(nullptr, nullptr, boundsPtr);
    SIMPLNX_RESULT_REQUIRE_VALID(boundResult);
    const auto writeResult = dream3d::WriteFile(filePath, dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

    // Modify the stored dataset directly to model an invalid file from an older producer.
    auto file = hdf5::FileIO::AppendFile(filePath);
    auto dataGroup = file.openGroup(nx::core::constants::k_DataStructureTag);
    auto dataset = dataGroup.openDataset(boundsPath.getTargetName());
    REQUIRE(dataset.getId() > 0);
    const std::array<float32, 2> duplicateBounds{0.0F, 0.0F};
    const auto editResult = dataset.writeSpan<float32>(dataset.getDimensions(), duplicateBounds);
    SIMPLNX_RESULT_REQUIRE_VALID(editResult);
  }

  const auto metadata = hdf5::DataStructureReader::ReadFile(filePath, true);
  SIMPLNX_RESULT_REQUIRE_VALID(metadata);
  const auto fullResult = hdf5::DataStructureReader::ReadFile(filePath);
  SIMPLNX_RESULT_REQUIRE_INVALID(fullResult);
  CHECK(fullResult.errors().front().code == -4007);
  CHECK(fullResult.errors().front().message.find("Stored Grid") != std::string::npos);
  CHECK(fullResult.errors().front().message.find(filePath.filename().string()) != std::string::npos);

  DataStructure selected;
  const ImportH5ObjectPathsAction action(filePath, {geometryPath, boundsPath});
  const auto selectiveResult = action.apply(selected, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_INVALID(selectiveResult);
  CHECK(selectiveResult.errors().front().code == -4007);
  CHECK(selectiveResult.errors().front().message.find("Stored Grid") != std::string::npos);
  CHECK_FALSE(selected.containsData(geometryPath));
}

TEST_CASE("RectGridGeomIO rejects invalid bounds on write across a validation block boundary", "[simplnx][HDF5][RectGridGeom]")
{
  DataStructure dataStructure;
  auto* gridPtr = RectGridGeom::Create(dataStructure, "Grid");
  REQUIRE(gridPtr != nullptr);
  constexpr usize k_Count = 4097;
  auto* boundsPtr = Float32ArrayType::CreateWithStore<Float32DataStoreType>(dataStructure, "Y Bounds", ShapeType{k_Count}, ShapeType{1});
  REQUIRE(boundsPtr != nullptr);
  for(usize i = 0; i < k_Count; ++i)
  {
    boundsPtr->setValue(i, static_cast<float32>(i));
  }
  gridPtr->setDimensions({1, k_Count - 1, 1});
  const auto setResult = gridPtr->setBounds(nullptr, boundsPtr, nullptr);
  SIMPLNX_RESULT_REQUIRE_VALID(setResult);
  boundsPtr->setValue(k_Count - 1, static_cast<float32>(k_Count - 2));

  const fs::path filePath = GetDataDir() / "RectGridInvalidWrite.dream3d";
  const auto writeResult = dream3d::WriteFile(filePath, dataStructure);
  SIMPLNX_RESULT_REQUIRE_INVALID(writeResult);
  CHECK(writeResult.errors().front().code == -4007);
  CHECK(writeResult.errors().front().message.find("Y bounds array is not strictly monotonically increasing at index 4096") != std::string::npos);
  CHECK(writeResult.errors().front().message.find(filePath.filename().string()) != std::string::npos);
}
