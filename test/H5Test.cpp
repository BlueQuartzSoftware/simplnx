#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"
#include "simplnx/Utilities/Parsing/Text/CsvParser.hpp"

#include "simplnx/UnitTest/DataObjectComparison.hpp"
#include "simplnx/UnitTest/FailCloseVfd.hpp"
#include "simplnx/UnitTest/HDF5DatasetProbe.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "GeometryTestUtilities.hpp"

#include <thread>
#include <vector>

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <hdf5.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <mutex>
#include <numeric>
#include <string>
#include <type_traits>

// This file is generated into the binary directory

#define TEST_LEGACY 1

namespace fs = std::filesystem;
using namespace nx::core;

static_assert(std::is_same_v<hid_t, nx::core::HDF5::IdType>, "H5::IdType must be the same type as hid_t");
static_assert(std::is_same_v<herr_t, nx::core::HDF5::ErrorType>, "H5::ErrorType must be the same type as herr_t");
static_assert(std::is_same_v<hsize_t, nx::core::HDF5::SizeType>, "H5::SizeType must be the same type as hsize_t");

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
    std::lock_guard<std::mutex> lock(HDF5::Support::ApiLock());
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
  std::lock_guard<std::mutex> lock(HDF5::Support::ApiLock());
  return static_cast<int64>(H5Fget_obj_count(H5F_OBJ_ALL, H5F_OBJ_DATATYPE));
}

/**
 * @brief Returns the number of open HDF5 dataset identifiers.
 * @return Open dataset count, or a negative HDF5 error status.
 */
int64 CountHdf5DatasetIds()
{
  std::lock_guard<std::mutex> lock(HDF5::Support::ApiLock());
  return static_cast<int64>(H5Fget_obj_count(H5F_OBJ_ALL, H5F_OBJ_DATASET));
}

/**
 * @brief Tests whether an HDF5 identifier is valid.
 * @param id Identifies the HDF5 object to test.
 * @return True if HDF5 reports a valid identifier.
 */
bool IsHdf5IdValid(hid_t id)
{
  std::lock_guard<std::mutex> lock(HDF5::Support::ApiLock());
  return H5Iis_valid(id) > 0;
}

namespace Constants
{
const fs::path k_DataDir = "test/data";
const StringLiteral k_LegacyFilepath = "LegacyData.dream3d";
const fs::path k_ComplexH5File = "new.h5";
} // namespace Constants

fs::path GetDataDir()
{
  return fs::path(unit_test::k_BinaryTestOutputDir.view());
}

fs::path GetLegacyFilepath()
{
  std::string path = fmt::format("{}/test/Data/{}", unit_test::k_SourceDir.view(), Constants::k_LegacyFilepath);
  return fs::path(path);
}

fs::path GetComplexH5File()
{
  return GetDataDir() / Constants::k_ComplexH5File;
}

bool equalsf(const FloatVec3& lhs, const FloatVec3& rhs)
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
  auto group = DataGroup::Create(dataStructure, "Group");
  REQUIRE(group != nullptr);

  auto geom = ImageGeom::Create(dataStructure, "Geometry");
  REQUIRE(geom != nullptr);

  auto scalar = ScalarData<int32>::Create(dataStructure, "Scalar", 7, geom->getId());
  REQUIRE(scalar != nullptr);

  auto dataStore = std::make_unique<DataStore<uint8>>(std::vector<usize>{2}, std::vector<usize>{3}, 0);
  auto dataArray = DataArray<uint8>::Create(dataStructure, "DataArray", std::move(dataStore), geom->getId());
  REQUIRE(dataArray != nullptr);

  return dataStructure;
}

template <typename T>
DataArray<T>* CreateTestDataArray(const std::string& name, DataStructure& dataStructure, ShapeType tupleShape, ShapeType componentShape, DataObject::IdType parentId)
{
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  auto data_store = std::make_unique<DataStoreType>(tupleShape, componentShape, static_cast<T>(0));
  ArrayType* dataArray = ArrayType::Create(dataStructure, name, std::move(data_store), parentId);

  return dataArray;
}

DataStructure CreateDataStructure()
{
  DataStructure dataStructure;
  DataGroup* group = DataGroup::Create(dataStructure, "Small IN100");
  DataGroup* scanData = DataGroup::Create(dataStructure, "EBSD Scan Data", group->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, "Small IN100 Grid", scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  SizeVec3 imageGeomDims = {100, 100, 100};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  usize numComponents = 1;
  ShapeType tupleShape = {imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]};

  Float32Array* ci_data = CreateTestDataArray<float>("Confidence Index", dataStructure, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = CreateTestDataArray<int32>("FeatureIds", dataStructure, tupleShape, {numComponents}, scanData->getId());
  Float32Array* iq_data = CreateTestDataArray<float>("Image Quality", dataStructure, tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = CreateTestDataArray<int32>("Phases", dataStructure, tupleShape, {numComponents}, scanData->getId());

  numComponents = 3;
  UInt8Array* ipf_color_data = CreateTestDataArray<uint8_t>("IPF Colors", dataStructure, tupleShape, {numComponents}, scanData->getId());

  dataStructure.setAdditionalParent(ipf_color_data->getId(), group->getId());

  // Add in another group that holds the phase data such as Laue Class, Lattice Constants, etc.
  DataGroup* phase_group = DataGroup::Create(dataStructure, "Phase Data", group->getId());
  numComponents = 1;
  usize numTuples = 2;
  Int32Array* laue_data = CreateTestDataArray<int32>("Laue Class", dataStructure, {numTuples}, {numComponents}, phase_group->getId());

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
  DataGroup* geometryGroup = DataGroup::Create(dataStructure, k_VertexGroupName);
  auto vertexGeometry = VertexGeom::Create(dataStructure, "[Geometry] Vertex", geometryGroup->getId());

  // DataGroup* scanData = DataGroup::Create(dataStructure, "AttributeMatrix", group->getId());
  uint64 skipLines = 1;
  char delimiter = ',';

  // Create the Vertex Array with a component size of 3
  DataPath path = DataPath({k_VertexGroupName, k_VertexListName});
  std::string inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  nx::core::Result result = ArrayCreationUtilities::CreateArray<float>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArray = dataStructure.getDataAs<Float32Array>(path);
  if(vertexArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<float, float>(inputFile, vertexArray->getDataStoreRef(), skipLines, delimiter);
  vertexGeometry->setVertices(*vertexArray);
  REQUIRE(vertexGeometry->getNumberOfVertices() == 144);

  // Create cell-level arrays for the Vertex Geometry.
  ShapeType tupleShape = {vertexGeometry->getNumberOfVertices()};
  usize numComponents = 1;
  Int16Array* ci_data = CreateTestDataArray<int16_t>("Area", dataStructure, tupleShape, {numComponents}, geometryGroup->getId());
  Float32Array* power_data = CreateTestDataArray<float>("Power", dataStructure, tupleShape, {numComponents}, geometryGroup->getId());
  UInt32Array* laserTTL_data = CreateTestDataArray<uint32>("LaserTTL", dataStructure, tupleShape, {numComponents}, geometryGroup->getId());
  for(usize i = 0; i < vertexGeometry->getNumberOfVertices(); i++)
  {
    ci_data->getDataStore()->setValue(i, static_cast<int16_t>(i * 10));
    power_data->getDataStore()->setValue(i, static_cast<float>(i * 2.345f));
    laserTTL_data->getDataStore()->setValue(i, static_cast<uint32>(i * 3421));
  }
}

void CreateTriangleGeometry(DataStructure& dataStructure)
{
  // Create a Triangle Geometry
  DataGroup* geometryGroup = DataGroup::Create(dataStructure, k_TriangleGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto triangleGeom = TriangleGeom::Create(dataStructure, "[Geometry] Triangle", geometryGroup->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_TriangleGroupName, k_TriangleFaceName});
  std::string inputFile = fmt::format("{}/test/Data/TriangleConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 242);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = ArrayCreationUtilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArray = dataStructure.getDataAs<IGeometry::SharedFaceList>(path);
  if(dataArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArray->getDataStoreRef(), skipLines, delimiter);
  triangleGeom->setFaceList(*dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_TriangleGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = ArrayCreationUtilities::CreateArray<float>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArray = dataStructure.getDataAs<Float32Array>(path);
  if(vertexArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<float, float>(inputFile, vertexArray->getDataStoreRef(), skipLines, delimiter);
  triangleGeom->setVertices(*vertexArray);
}

void CreateQuadGeometry(DataStructure& dataStructure)
{
  // Create a Triangle Geometry
  DataGroup* geometryGroup = DataGroup::Create(dataStructure, k_QuadGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto geometry = QuadGeom::Create(dataStructure, "[Geometry] Quad", geometryGroup->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_QuadGroupName, k_QuadFaceName});
  std::string inputFile = fmt::format("{}/test/Data/QuadConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 121);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = ArrayCreationUtilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {4}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArray = dataStructure.getDataAs<IGeometry::SharedFaceList>(path);
  if(dataArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArray->getDataStoreRef(), skipLines, delimiter);
  geometry->setFaceList(*dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_QuadGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArray = dataStructure.getDataAs<Float32Array>(path);
  if(vertexArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<float, float>(inputFile, vertexArray->getDataStoreRef(), skipLines, delimiter);
  geometry->setVertices(*vertexArray);
}

void CreateEdgeGeometry(DataStructure& dataStructure)
{
  // Create a Triangle Geometry
  DataGroup* geometryGroup = DataGroup::Create(dataStructure, k_EdgeGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto geometry = EdgeGeom::Create(dataStructure, "[Geometry] Edge", geometryGroup->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_EdgeGroupName, k_EdgeFaceName});
  std::string inputFile = fmt::format("{}/test/Data/EdgeConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 264);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = ArrayCreationUtilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {2}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArray = dataStructure.getDataAs<IGeometry::SharedEdgeList>(path);
  if(dataArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArray->getDataStoreRef(), skipLines, delimiter);
  geometry->setEdgeList(*dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_EdgeGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArray = dataStructure.getDataAs<Float32Array>(path);
  if(vertexArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<float, float>(inputFile, vertexArray->getDataStoreRef(), skipLines, delimiter);
  geometry->setVertices(*vertexArray);
}

void CreateTetrahedralGeometry(DataStructure& dataStructure)
{
  // Create a Tetrahedral Geometry
  DataGroup* geometryGroup = DataGroup::Create(dataStructure, k_TetraGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto geometry = TetrahedralGeom::Create(dataStructure, "[Geometry] Tetrahedral", geometryGroup->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_TetraGroupName, k_TetraFaceName});
  std::string inputFile = fmt::format("{}/test/Data/TetraConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 3);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = ArrayCreationUtilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {4}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArray = dataStructure.getDataAs<IGeometry::SharedTetList>(path);
  if(dataArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArray->getDataStoreRef(), skipLines, delimiter);
  geometry->setPolyhedraList(*dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_TetraGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/TetraVertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 9);
  result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArray = dataStructure.getDataAs<Float32Array>(path);
  if(vertexArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<float, float>(inputFile, vertexArray->getDataStoreRef(), skipLines, delimiter);
  geometry->setVertices(*vertexArray);
}

void CreateHexahedralGeometry(DataStructure& dataStructure)
{
  // Create a Hexahedral Geometry
  DataGroup* geometryGroup = DataGroup::Create(dataStructure, k_HexaGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto geometry = TetrahedralGeom::Create(dataStructure, "[Geometry] Hexahedral", geometryGroup->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_HexaGroupName, k_HexaFaceName});
  std::string inputFile = fmt::format("{}/test/Data/HexaConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 3);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  nx::core::Result result = ArrayCreationUtilities::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {8}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* dataArray = dataStructure.getDataAs<IGeometry::SharedHexList>(path);
  if(dataArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, dataArray->getDataStoreRef(), skipLines, delimiter);
  geometry->setPolyhedraList(*dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_HexaGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/HexaVertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 20);
  result = ArrayCreationUtilities::CreateArray<float32>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto* vertexArray = dataStructure.getDataAs<Float32Array>(path);
  if(vertexArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  CsvParser::ReadFile<float, float>(inputFile, vertexArray->getDataStoreRef(), skipLines, delimiter);
  geometry->setVertices(*vertexArray);
}

void CreateNeighborList(DataStructure& dataStructure)
{
  const usize numItems = 50;
  const ShapeType tupleShape = {numItems};

  auto* neighborGroup = DataGroup::Create(dataStructure, k_NeighborGroupName);
  auto* neighborGroup2 = DataGroup::Create(dataStructure, k_NeighborGroupName + "2");
  auto* neighborList = NeighborList<int64>::Create(dataStructure, "NeighborList", tupleShape, neighborGroup->getId());
  for(usize i = 0; i < numItems; i++)
  {
    for(usize j = 0; j < i + 1; j++)
    {
      neighborList->addEntry(j, j);
    }
  }
  dataStructure.setAdditionalParent(neighborList->getId(), neighborGroup2->getId());
}

void CreateArrayTypes(DataStructure& dataStructure)
{
  const ShapeType tupleShape = {2};
  const ShapeType componentShape = {1};

  auto* boolArray = DataArray<bool>::CreateWithStore<DataStore<bool>>(dataStructure, "BoolArray", tupleShape, componentShape);
  AbstractDataStore<bool>& boolStore = boolArray->getDataStoreRef();
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
  std::optional<DataObject::IdType> verticesId;

  bool operator==(const VertexGeomData& rhs) const
  {
    return verticesId == rhs.verticesId;
  }
};

struct EdgeGeomData
{
  std::optional<DataObject::IdType> verticesId;
  std::optional<DataObject::IdType> edgesId;

  bool operator==(const EdgeGeomData& rhs) const
  {
    return (verticesId == rhs.verticesId) && (edgesId == rhs.edgesId);
  }
};

struct TriangleGeomData
{
  std::optional<DataObject::IdType> verticesId;
  std::optional<DataObject::IdType> edgesId;
  std::optional<DataObject::IdType> trianglesId;

  bool operator==(const TriangleGeomData& rhs) const
  {
    return (verticesId == rhs.verticesId) && (edgesId == rhs.edgesId) && (trianglesId == rhs.trianglesId);
  }
};

struct QuadGeomData
{
  std::optional<DataObject::IdType> verticesId;
  std::optional<DataObject::IdType> edgesId;
  std::optional<DataObject::IdType> quadsId;

  bool operator==(const QuadGeomData& rhs) const
  {
    return (verticesId == rhs.verticesId) && (edgesId == rhs.edgesId) && (quadsId == rhs.quadsId);
  }
};

struct NodeBasedGeomData
{
  VertexGeomData vertexData;
  EdgeGeomData edgeData;
  TriangleGeomData triangleData;
  QuadGeomData quadData;

  bool operator==(const NodeBasedGeomData& rhs) const
  {
    bool vertexCheck = (vertexData == rhs.vertexData);
    bool edgeCheck = (edgeData == rhs.edgeData);
    bool triangleCheck = (triangleData == rhs.triangleData);
    bool quadCheck = (quadData == rhs.quadData);
    return vertexCheck && edgeCheck && triangleCheck && quadCheck;
  }
};

NodeBasedGeomData getNodeGeomData(const DataStructure& dataStructure)
{
  NodeBasedGeomData nodeData;

  DataPath vertexPath({k_VertexGroupName, "[Geometry] Vertex"});
  auto* vertexGeom = dataStructure.getDataAs<VertexGeom>(vertexPath);
  REQUIRE(vertexGeom != nullptr);
  nodeData.vertexData.verticesId = vertexGeom->getSharedVertexDataArrayId();

  DataPath edgePath({k_EdgeGroupName, "[Geometry] Edge"});
  auto* edgeGeom = dataStructure.getDataAs<EdgeGeom>(edgePath);
  REQUIRE(edgeGeom != nullptr);
  nodeData.edgeData.verticesId = edgeGeom->getSharedVertexDataArrayId();
  nodeData.edgeData.edgesId = edgeGeom->getEdgeListDataArrayId();

  DataPath trianglePath({k_TriangleGroupName, "[Geometry] Triangle"});
  auto* triangleGeom = dataStructure.getDataAs<TriangleGeom>(trianglePath);
  REQUIRE(triangleGeom != nullptr);
  nodeData.triangleData.verticesId = triangleGeom->getSharedVertexDataArrayId();
  nodeData.triangleData.edgesId = triangleGeom->getEdgeListDataArrayId();
  nodeData.triangleData.trianglesId = triangleGeom->getFaceListDataArrayId();

  DataPath quadPath({k_QuadGroupName, "[Geometry] Quad"});
  auto* quadGeom = dataStructure.getDataAs<QuadGeom>(quadPath);
  REQUIRE(quadGeom != nullptr);
  nodeData.quadData.verticesId = quadGeom->getSharedVertexDataArrayId();
  nodeData.quadData.edgesId = quadGeom->getEdgeListDataArrayId();
  nodeData.quadData.quadsId = quadGeom->getFaceListDataArrayId();

  return nodeData;
}

void checkNodeGeomData(const DataStructure& dataStructure, const NodeBasedGeomData& nodeData)
{
  NodeBasedGeomData newNodeData = getNodeGeomData(dataStructure);
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
  if constexpr(std::is_same_v<H5ClassT, HDF5::FileIO>)
  {
    return HDF5::FileIO();
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

  H5ClassT moveConstructorObject(std::move(originalObject));

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
    Result<DataStructure> result = DREAM3D::LoadDataStructureMetadata(filepath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    DataStructure dataStructure = result.value();

    const std::string geomName = "Small IN100";
    const auto* image = dataStructure.getDataAs<ImageGeom>(DataPath({geomName}));
    REQUIRE(image != nullptr);
    REQUIRE(equalsf(image->getOrigin(), FloatVec3(-47.0f, 0.0f, -29.0f)));
    REQUIRE(image->getDimensions() == SizeVec3(189, 201, 117));
    REQUIRE(equalsf(image->getSpacing(), FloatVec3(0.25f, 0.25f, 0.25f)));

    {
      const std::string testDCName = "DataContainer";
      DataPath testDCPath({testDCName});
      auto* testDC = dataStructure.getDataAs<DataGroup>(testDCPath);
      REQUIRE(testDC != nullptr);

      DataPath testAMPath = testDCPath.createChildPath("AttributeMatrix");
      REQUIRE(dataStructure.getDataAs<AttributeMatrix>(DataPath({testAMPath})) != nullptr);

      REQUIRE(dataStructure.getDataAs<Int8Array>(testAMPath.createChildPath("Int8")) != nullptr);
      REQUIRE(dataStructure.getDataAs<UInt8Array>(testAMPath.createChildPath("UInt8")) != nullptr);
      REQUIRE(dataStructure.getDataAs<Float32Array>(testAMPath.createChildPath("Float32")) != nullptr);
      REQUIRE(dataStructure.getDataAs<Float64Array>(testAMPath.createChildPath("Float64")) != nullptr);
      REQUIRE(dataStructure.getDataAs<BoolArray>(testAMPath.createChildPath("Bool")) != nullptr);
    }

    {
      DataPath grainDataPath({geomName, "Grain Data"});
      REQUIRE(dataStructure.getData(grainDataPath) != nullptr);
      REQUIRE(dataStructure.getDataAs<NeighborList<int32_t>>(grainDataPath.createChildPath("NeighborList")) != nullptr);
      REQUIRE(dataStructure.getDataAs<Int32Array>(grainDataPath.createChildPath("NumElements")) != nullptr);
      REQUIRE(dataStructure.getDataAs<Int32Array>(grainDataPath.createChildPath("NumNeighbors")) != nullptr);
    }

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    auto fileWriter = nx::core::HDF5::FileIO::WriteFile(filePath);
    REQUIRE(fileWriter.isValid());

    Result<> writeResult = HDF5::DataStructureWriter::WriteFile(originalDataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  {
    auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
    auto readResult = HDF5::DataStructureReader::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure newDataStructure = std::move(readResult.value());

    auto* imageGeom = newDataStructure.getDataAs<ImageGeom>(imageGeomPath);
    REQUIRE(imageGeom != nullptr);

    REQUIRE(imageGeom->getDimensions() == SizeVec3(dims));
    REQUIRE(imageGeom->getOrigin() == FloatVec3(origin));
    REQUIRE(imageGeom->getSpacing() == FloatVec3(spacing));

    auto* cellData = imageGeom->getCellData();
    REQUIRE(cellData != nullptr);

    REQUIRE(cellData->getName() == cellDataName);
    REQUIRE(imageGeom->getCellDataPath() == cellDataPath);

    auto cellDataShape = std::vector<usize>(dims.crbegin(), dims.crend());

    REQUIRE(cellData->getShape() == cellDataShape);

    UnitTest::CheckArraysInheritTupleDims(newDataStructure);
  }
}

// Invalid HDF5 objects must return recoverable attribute errors instead of throwing.
// This test exercises the empty-name formatting path.
TEST_CASE("HDF5 ObjectIO: Invalid object reads recover gracefully")
{
  auto app = Application::GetOrCreateInstance();

  nx::core::HDF5::GroupIO invalidGroup;
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
  UnitTest::LoadPlugins();

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
    DataStructure ds = CreateNodeBasedGeometries();
    nodeData = getNodeGeomData(ds);
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(filePath);
    REQUIRE(fileWriter.isValid());

    auto resultH5 = HDF5::DataStructureWriter::WriteFile(ds, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  }

  // Read HDF5 file
  {
    auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());

    auto readResult = HDF5::DataStructureReader::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    checkNodeGeomData(dataStructure, nodeData);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    auto fileWriter = nx::core::HDF5::FileIO::WriteFile(filePath);
    REQUIRE(fileWriter.isValid());

    Result<> writeResult = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  // Read HDF5 file
  {
    auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());

    herr_t err = 0;
    auto readResult = HDF5::DataStructureReader::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    // auto neighborList = dataStructure.getDataAs<NeighborList<int64>>(DataPath({k_NeighborGroupName, "NeighborList"}));
    auto neighborList = dataStructure.getData(DataPath({k_NeighborGroupName, "NeighborList"}));
    REQUIRE(neighborList != nullptr);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
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
    nx::core::HDF5::FileIO fileWriter = nx::core::HDF5::FileIO::WriteFile(filePath);
    REQUIRE(fileWriter.isValid());

    Result<> writeResult = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  // Read HDF5 file
  {
    auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());

    auto readResult = HDF5::DataStructureReader::ReadFile(fileReader);
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

    BoolArray* boolArray = dataStructure.getDataAs<BoolArray>(DataPath({"BoolArray"}));
    REQUIRE(boolArray != nullptr);
    AbstractDataStore<bool>& boolStore = boolArray->getDataStoreRef();
    REQUIRE(boolStore[0] == false);
    REQUIRE(boolStore[1] == true);

    StringArray* stringArray = dataStructure.getDataAs<StringArray>(DataPath({"StringArray"}));
    REQUIRE(stringArray != nullptr);
    REQUIRE(stringArray->values() == std::vector<std::string>{"Foo", "Bar", "Bazz"});
  }

  // Read HDF5 file in preflight mode to make sure StringArrays import correctly
  {
    auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());

    auto readResult = HDF5::DataStructureReader::ReadFile(fileReader, true);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    StringArray* stringArray = dataStructure.getDataAs<StringArray>(DataPath({"StringArray"}));
    REQUIRE(stringArray != nullptr);
    REQUIRE(stringArray->size() == 3);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("xdmf")
{
  DataStructure dataStructure;
  auto* vertexGeom = VertexGeom::Create(dataStructure, "VertexGeom");
  DataObject::IdType geomId = vertexGeom->getId();
  constexpr usize numVerts = 100;
  std::random_device randomDevice;
  std::mt19937 generator(randomDevice());
  std::uniform_int_distribution<> intDistribution(0, 100);
  std::uniform_real_distribution<> realDistribution(-10.0f, 10.0f);
  auto vertsDataStore = std::make_unique<DataStore<float32>>(ShapeType{numVerts}, ShapeType{3}, 0.0f);
  for(auto& item : vertsDataStore->createSpan())
  {
    item = realDistribution(generator);
  }
  auto* verts = DataArray<float32>::Create(dataStructure, "Verts", std::move(vertsDataStore), geomId);
  auto* vertexData = AttributeMatrix::Create(dataStructure, "VertexData", {numVerts}, geomId);
  vertexGeom->setVertexAttributeMatrix(*vertexData);
  vertexGeom->setVertices(*verts);
  auto vertexAssociatedData = std::make_unique<DataStore<int32>>(numVerts, 0);
  for(auto& item : vertexAssociatedData->createSpan())
  {
    item = intDistribution(generator);
  }
  auto* data = DataArray<int32>::Create(dataStructure, "Data", std::move(vertexAssociatedData), vertexData->getId());
  Result<> result = DREAM3D::WriteFile(GetDataDir() / "xdmfTest.dream3d", dataStructure, {}, true);
  SIMPLNX_RESULT_REQUIRE_VALID(result);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("H5 Utilities")
{
  REQUIRE_THROWS(nx::core::HDF5::GetNameFromBuffer(""));
  REQUIRE_THROWS(nx::core::HDF5::GetNameFromBuffer("/Group/"));
  std::string objectName = nx::core::HDF5::GetNameFromBuffer("/");
  REQUIRE(objectName == "/");
  objectName = nx::core::HDF5::GetNameFromBuffer("/Group/Geometry/Data");
  REQUIRE(objectName == "Data");
  objectName = nx::core::HDF5::GetNameFromBuffer("Data");
  REQUIRE(objectName == "Data");
  objectName = nx::core::HDF5::GetNameFromBuffer("/Data");
  REQUIRE(objectName == "Data");
}

TEST_CASE("HDF5ImplicitCopyReaderTest")
{
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<HDF5::FileIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<HDF5::GroupIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<HDF5::DatasetIO>);
  fs::path filePath = GetDataDir() / "HDF5ImplicitCopyReaderTest.dream3d";
  {
    DataStructure dataStructure = GetTestDataStructure();

    Result<> writeFileResult = DREAM3D::WriteFile(filePath, dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(writeFileResult);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
  auto fileReader = HDF5::FileIO::ReadFile(filePath);
  HDF5::FileIO newFileIO = TestH5ImplicitCopy(std::move(fileReader), "HDF5::FileIO");

  HDF5::GroupIO groupReader = newFileIO.openGroup("DataStructure");
  HDF5::GroupIO newGroupIO = TestH5ImplicitCopy(std::move(groupReader), "HDF5::GroupIO");

  HDF5::GroupIO groupReaderIntermediate = newGroupIO.openGroup("Geometry");
  REQUIRE(groupReaderIntermediate.isValid());

  HDF5::DatasetIO datasetReader = groupReaderIntermediate.openDataset("DataArray");
  REQUIRE(datasetReader.getId() > 0);
  TestH5ImplicitCopy(std::move(datasetReader), "HDF5::DatasetIO");
}

TEST_CASE("HDF5ImplicitCopyWriterTest")
{
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<HDF5::FileIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<HDF5::GroupIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<HDF5::DatasetIO>);

  fs::path outputPath = GetDataDir() / "HDF5ImplicitCopyWriterTest.h5";
  fs::remove(outputPath);

  auto fileWriter = HDF5::FileIO::WriteFile(outputPath);
  HDF5::FileIO newFileIO = TestH5ImplicitCopy(std::move(fileWriter), "HDF5::FileIO");

  HDF5::GroupIO groupWriter = newFileIO.createGroup("foo");
  TestH5ImplicitCopy(std::move(groupWriter), "HDF5::GroupIO");

  HDF5::DatasetIO datasetWriter = newFileIO.createDataset("bar");
  std::array<int32, 5> data = {1, 2, 3, 4, 5};
  HDF5::DatasetIO::DimsType dims = {data.size()};
  Result<> datasetResult = datasetWriter.writeSpan<int32>(dims, nonstd::span<int32>(data));
  SIMPLNX_RESULT_REQUIRE_VALID(datasetResult);
  TestH5ImplicitCopy(std::move(datasetWriter), "HDF5::DatasetIO");
}

TEST_CASE("HDF5ImplicitCopyIOTest")
{
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<HDF5::FileIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<HDF5::GroupIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<HDF5::DatasetIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<HDF5::FileIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<HDF5::GroupIO>);
  STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<HDF5::DatasetIO>);

  fs::path filePath = GetDataDir() / "HDF5ImplicitCopyIOTest.dream3d";
  {
    DataStructure dataStructure = GetTestDataStructure();

    Result<> writeFileResult = DREAM3D::WriteFile(filePath, dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(writeFileResult);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }

  auto file1 = HDF5::FileIO::ReadFile(filePath);
  HDF5::FileIO fileIO = std::move(file1);
  HDF5::FileIO newFileIO = TestH5ImplicitCopy(std::move(fileIO), "HDF5::FileIO");

  HDF5::GroupIO groupIO = newFileIO.openGroup("DataStructure");
  HDF5::GroupIO newGroupIO = TestH5ImplicitCopy(std::move(groupIO), "HDF5::GroupIO");

  HDF5::GroupIO groupIOIntermediate = newGroupIO.openGroup("Geometry");
  REQUIRE(groupIOIntermediate.isValid());

  HDF5::DatasetIO datasetIO = groupIOIntermediate.openDataset("DataArray");
  REQUIRE(datasetIO.getId() > 0);
  TestH5ImplicitCopy(std::move(datasetIO), "HDF5::DatasetIO");
}

TEST_CASE("DataStructureWriter: WriteOptions round-trip", "[DataStructureWriter][WriteOptions]")
{
  nx::core::HDF5::DataStructureWriter writer;
  // Default compression level is 0 (off).
  REQUIRE(writer.getWriteOptions().compressionLevel == 0);

  nx::core::HDF5::DataStructureWriter::WriteOptions options;
  options.compressionLevel = 7;
  writer.setWriteOptions(options);
  REQUIRE(writer.getWriteOptions().compressionLevel == 7);
}

TEST_CASE("DataStructureAppend")
{
  const fs::path inputFilePath = fs::path(unit_test::k_SourceDir.view()) / "test/Data/geoms.dream3d";
  const fs::path outputFilePath = GetDataDir() / "DataStructureAppend.dream3d";
  const DataPath originalArrayPath({"foo"});

  DataStructure baseDataStructure;

  auto createArrayResult = ArrayCreationUtilities::CreateArray<int32>(baseDataStructure, {10, 10, 10}, {3}, originalArrayPath, IDataAction::Mode::Execute);
  SIMPLNX_RESULT_REQUIRE_VALID(createArrayResult);

  const auto& originalArray = baseDataStructure.getDataRefAs<Int32Array>(originalArrayPath);

  Result<> writeResult = DREAM3D::WriteFile(outputFilePath, baseDataStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  DataStructure exemplarDataStructure = UnitTest::LoadDataStructure(inputFilePath);

  usize currentTopLevelSize = baseDataStructure.getTopLevelData().size();
  for(const DataObject* object : exemplarDataStructure.getTopLevelData())
  {
    REQUIRE(object != nullptr);
    DataPath path({object->getName()});
    auto appendResult = DREAM3D::AppendFile(outputFilePath, exemplarDataStructure, path);
    SIMPLNX_RESULT_REQUIRE_VALID(appendResult);

    DataStructure appendedDataStructure = UnitTest::LoadDataStructure(outputFilePath);

    currentTopLevelSize++;

    REQUIRE(currentTopLevelSize == appendedDataStructure.getTopLevelData().size());

    REQUIRE(appendedDataStructure.containsData(originalArrayPath));
    REQUIRE(appendedDataStructure.containsData(path));

    const DataObject& appendedOriginalArray = appendedDataStructure.getDataRef(originalArrayPath);
    REQUIRE(UnitTest::Comparison::CompareDataObject(originalArray, appendedOriginalArray));

    const DataObject& appendedObject = appendedDataStructure.getDataRef(path);
    const DataObject& exemplarObject = exemplarDataStructure.getDataRef(path);

    REQUIRE(UnitTest::Comparison::CompareDataObject(exemplarObject, appendedObject));

    UnitTest::CheckArraysInheritTupleDims(appendedDataStructure);
  }

  // Failure states

  // empty path
  auto appendFailureResult1 = DREAM3D::AppendFile(outputFilePath, baseDataStructure, DataPath());
  SIMPLNX_RESULT_REQUIRE_INVALID(appendFailureResult1);

  // not top level
  auto appendFailureResult2 = DREAM3D::AppendFile(outputFilePath, baseDataStructure, DataPath({"foo", "bar"}));
  SIMPLNX_RESULT_REQUIRE_INVALID(appendFailureResult2);

  // target already exists
  auto appendFailureResult3 = DREAM3D::AppendFile(outputFilePath, baseDataStructure, originalArrayPath);
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
    auto fileWriter = nx::core::HDF5::FileIO::WriteFile(outPath);
    REQUIRE(fileWriter.isValid());
    auto group = fileWriter.createGroup("g");
    auto dataset = group.createDataset(datasetName);
    dataset.setCompressionLevel(5);
    auto wr = dataset.writeSpan<float>({payload.size()}, nonstd::span<const float>(payload.data(), payload.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(wr);
  }

  auto info = nx::core::UnitTest::ProbeHdf5Dataset(outPath, "g/" + datasetName);
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::UnitTest::DatasetLayout::Chunked);
  REQUIRE(info->hasDeflate);
  REQUIRE(info->deflateLevel == 5);
}

TEST_CASE("DatasetIO: writeSpan stays contiguous when compression level is 0", "[DatasetIO][Compression]")
{
  const fs::path outPath = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.view()) / "dataset_no_compression.h5";
  fs::remove(outPath);

  std::vector<float> payload(1'000'000, 3.14f);

  {
    auto fileWriter = nx::core::HDF5::FileIO::WriteFile(outPath);
    REQUIRE(fileWriter.isValid());
    auto group = fileWriter.createGroup("g");
    auto dataset = group.createDataset("d");
    // Leave m_CompressionLevel at its 0 default — the opt-in path is not triggered.
    auto wr = dataset.writeSpan<float>({payload.size()}, nonstd::span<const float>(payload.data(), payload.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(wr);
  }

  auto info = nx::core::UnitTest::ProbeHdf5Dataset(outPath, "g/d");
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::UnitTest::DatasetLayout::Contiguous);
  REQUIRE(info->hasDeflate == false);
}

TEST_CASE("DatasetIO: writeSpan bypasses chunking for small arrays even with compression on", "[DatasetIO][Compression]")
{
  const fs::path outPath = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.view()) / "dataset_small_bypass.h5";
  fs::remove(outPath);

  // 100 floats = 400 bytes, below the 16 KiB bypass — DCPL helper returns H5P_DEFAULT.
  std::vector<float> payload(100, 1.0f);

  {
    auto fileWriter = nx::core::HDF5::FileIO::WriteFile(outPath);
    REQUIRE(fileWriter.isValid());
    auto group = fileWriter.createGroup("g");
    auto dataset = group.createDataset("d");
    dataset.setCompressionLevel(9);
    auto wr = dataset.writeSpan<float>({payload.size()}, nonstd::span<const float>(payload.data(), payload.size()));
    SIMPLNX_RESULT_REQUIRE_VALID(wr);
  }

  auto info = nx::core::UnitTest::ProbeHdf5Dataset(outPath, "g/d");
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::UnitTest::DatasetLayout::Contiguous);
  REQUIRE(info->hasDeflate == false);
}

TEST_CASE("DatasetIO: value getters release temporary datatype handles", "[simplnx][HDF5][DatasetIO][Lifetime]")
{
  const fs::path path = GetDataDir() / "dataset_type_getter_lifetime.h5";
  fs::create_directories(path.parent_path());
  DatatypeFixtureFile cleanup{path};
  auto file = HDF5::FileIO::WriteFile(path);
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
      std::lock_guard<std::mutex> lock(HDF5::Support::ApiLock());
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
    auto file = HDF5::FileIO::WriteFile(path);
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
      HDF5::DatasetIO destination;
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
      HDF5::DatasetIO destination;
      HDF5::DatasetIO source;

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
      auto* alias = &dataset;

      dataset = std::move(*alias);

      CHECK(dataset.getId() == originalId);
      CHECK(dataset.getCompressionLevel() == 4);
      CHECK(IsHdf5IdValid(originalId));
      CHECK(CountHdf5DatasetIds() == initialDatasetCount + 1);
    }
    REQUIRE(CountHdf5DatasetIds() == initialDatasetCount);

    {
      auto dataset = file.openDataset("second");
      auto* alias = &dataset;

      dataset = std::move(*alias);

      CHECK_FALSE(dataset.isValid());
      CHECK(dataset.getNamePath() == "second");
      CHECK(CountHdf5DatasetIds() == initialDatasetCount);
    }
  }

  REQUIRE(CountHdf5DatasetIds() == initialDatasetCount);
  auto replacement = HDF5::FileIO::WriteFile(path);
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
constexpr int32 k_DuplicateRegistrationCode = -6055;

/**
 * @brief Reports whether two paths in an open HDF5 file name the same object, as hard links to one object do.
 * @param fileId Open HDF5 file identifier.
 * @param firstPath Absolute path of the first link.
 * @param secondPath Absolute path of the second link.
 * @return True when both links resolve to the same object token; false when they differ or either lookup fails.
 */
bool LinksNameSameObject(hid_t fileId, const std::string& firstPath, const std::string& secondPath)
{
  std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
  H5O_info2_t firstInfo{};
  H5O_info2_t secondInfo{};
  if(H5Oget_info_by_name3(fileId, firstPath.c_str(), &firstInfo, H5O_INFO_BASIC, H5P_DEFAULT) < 0)
  {
    return false;
  }
  if(H5Oget_info_by_name3(fileId, secondPath.c_str(), &secondInfo, H5O_INFO_BASIC, H5P_DEFAULT) < 0)
  {
    return false;
  }
  int tokenOrder = 1;
  if(H5Otoken_cmp(fileId, &firstInfo.token, &secondInfo.token, &tokenOrder) < 0)
  {
    return false;
  }
  return tokenOrder == 0;
}
} // namespace

TEST_CASE("DataStructureWriter: an object written after a NeighborList keeps its own dataset", "[DataStructureWriter][NeighborList]")
{
  auto app = Application::GetOrCreateInstance();
  constexpr usize k_TupleCount = 5;
  const std::vector<int32> sharedValues = {1001, 1002, 1003, 1004, 1005};

  DataStructure dataStructure;
  // Created first, so it has object id 1: the same id that NeighborListIO gives its temporary counts array.
  auto* sharedArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Shared", ShapeType{k_TupleCount}, ShapeType{1});
  REQUIRE(sharedArray != nullptr);
  REQUIRE(sharedArray->getId() == 1);
  for(usize index = 0; index < k_TupleCount; ++index)
  {
    sharedArray->getDataStoreRef()[index] = sharedValues[index];
  }

  auto* listGroup = DataGroup::Create(dataStructure, "A_Lists");
  REQUIRE(listGroup != nullptr);
  auto* neighborList = NeighborList<int32>::Create(dataStructure, "NeighborList", ShapeType{k_TupleCount}, listGroup->getId());
  REQUIRE(neighborList != nullptr);
  neighborList->setLists({{1}, {2, 2}, {3}, {4, 4, 4}, {5}});

  // The writer reaches "Shared" a second time after the NeighborList and writes it as a hard link.
  auto* copyGroup = DataGroup::Create(dataStructure, "Z_Copy");
  REQUIRE(copyGroup != nullptr);
  REQUIRE(dataStructure.setAdditionalParent(sharedArray->getId(), copyGroup->getId()));

  const fs::path filePath = GetDataDir() / "WriterIdMapNeighborList.dream3d";
  fs::create_directories(filePath.parent_path());
  {
    auto writeResult = DREAM3D::WriteFile(filePath, dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  // Read the dataset at the second path directly, as any HDF5 reader of the file does.
  auto fileReader = HDF5::FileIO::ReadFile(filePath);
  REQUIRE(fileReader.isValid());
  auto dataStructureGroup = fileReader.openGroup("DataStructure");
  REQUIRE(dataStructureGroup.isValid());
  auto copyGroupReader = dataStructureGroup.openGroup("Z_Copy");
  REQUIRE(copyGroupReader.isValid());
  auto linkedDataset = copyGroupReader.openDataset("Shared");
  // The returned DatasetIO opens lazily; resolving its identifier lets isValid() report whether the dataset opened.
  (void)linkedDataset.getId();
  REQUIRE(linkedDataset.isValid());
  const std::vector<int32> linkedValues = linkedDataset.readAsVector<int32>();
  CHECK(linkedValues == sharedValues);

  // The second path is a hard link to the first-written dataset, and no counts link was written beside it.
  CHECK(LinksNameSameObject(fileReader.getId(), "/DataStructure/Shared", "/DataStructure/Z_Copy/Shared"));
  CHECK_FALSE(copyGroupReader.exists(neighborList->getNumNeighborsArrayName()));
}

TEST_CASE("WriterFix: a NeighborList with two parents keeps its counts link at both locations", "[DataStructureWriter][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  constexpr usize k_TupleCount = 4;
  const std::vector<std::vector<int32>> expectedLists = {{1}, {2, 2}, {}, {4, 4, 4}};
  const std::vector<std::string> parentNames = {"A_First", "B_Second"};
  const std::string listName = "Neighbors";

  DataStructure dataStructure;
  auto* firstParent = DataGroup::Create(dataStructure, parentNames[0]);
  REQUIRE(firstParent != nullptr);
  auto* secondParent = DataGroup::Create(dataStructure, parentNames[1]);
  REQUIRE(secondParent != nullptr);
  auto* neighborList = NeighborList<int32>::Create(dataStructure, listName, ShapeType{k_TupleCount}, firstParent->getId());
  REQUIRE(neighborList != nullptr);
  neighborList->setLists(expectedLists);
  REQUIRE(dataStructure.setAdditionalParent(neighborList->getId(), secondParent->getId()));
  const std::string countsName = neighborList->getNumNeighborsArrayName();

  const fs::path filePath = GetDataDir() / "WriterFixNeighborListTwoParents.dream3d";
  fs::create_directories(filePath.parent_path());
  {
    const Result<> writeResult = DREAM3D::WriteFile(filePath, dataStructure);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  // Each parent group lists the NeighborList and its counts link, as an h5py key listing of the group does.
  {
    auto fileReader = HDF5::FileIO::ReadFile(filePath);
    REQUIRE(fileReader.isValid());
    auto dataStructureGroup = fileReader.openGroup("DataStructure");
    REQUIRE(dataStructureGroup.isValid());
    for(const auto& parentName : parentNames)
    {
      INFO(fmt::format("Parent group '{}'", parentName));
      auto parentGroup = dataStructureGroup.openGroup(parentName);
      REQUIRE(parentGroup.isValid());
      const std::vector<std::string> childNames = parentGroup.getChildNames();
      CHECK(std::find(childNames.begin(), childNames.end(), listName) != childNames.end());
      CHECK(std::find(childNames.begin(), childNames.end(), countsName) != childNames.end());
    }
  }

  auto loadResult = DREAM3D::LoadDataStructure(filePath);
  SIMPLNX_RESULT_REQUIRE_VALID(loadResult);
  const DataStructure& loadedStructure = loadResult.value();
  for(const auto& parentName : parentNames)
  {
    INFO(fmt::format("Parent group '{}'", parentName));
    const auto* loadedList = loadedStructure.getDataAs<NeighborList<int32>>(DataPath({parentName, listName}));
    REQUIRE(loadedList != nullptr);
    CHECK(loadedList->getVectors() == expectedLists);
  }
}

TEST_CASE("WriterFix: registering one object at two paths is an error", "[DataStructureWriter][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  constexpr usize k_TupleCount = 4;
  const std::vector<int32> expectedValues = {11, 12, 13, 14};
  const std::string firstPath = "/DataStructure/A_First/Values";
  const std::string secondPath = "/DataStructure/B_Second/Values";

  DataStructure dataStructure;
  auto* firstParent = DataGroup::Create(dataStructure, "A_First");
  REQUIRE(firstParent != nullptr);
  auto* secondParent = DataGroup::Create(dataStructure, "B_Second");
  REQUIRE(secondParent != nullptr);
  auto* valuesArray = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, "Values", ShapeType{k_TupleCount}, ShapeType{1}, firstParent->getId());
  REQUIRE(valuesArray != nullptr);
  for(usize index = 0; index < k_TupleCount; ++index)
  {
    valuesArray->getDataStoreRef()[index] = expectedValues[index];
  }

  const fs::path filePath = GetDataDir() / "WriterFixDuplicateRegistration.h5";
  fs::create_directories(filePath.parent_path());
  {
    auto fileWriter = HDF5::FileIO::WriteFile(filePath);
    REQUIRE(fileWriter.isValid());
    auto dataStructureGroup = fileWriter.createGroup("DataStructure");
    HDF5::DataStructureWriter writer;
    const Result<> writeResult = writer.writeDataStructure(dataStructure, dataStructureGroup);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

    // Registering the written object again at its own path is accepted.
    {
      auto firstParentGroup = dataStructureGroup.openGroup("A_First");
      auto writtenDataset = firstParentGroup.openDataset("Values");
      // Resolve the lazily opened dataset so its object path is the HDF5 path, not only its name.
      REQUIRE(writtenDataset.getId() >= 0);
      REQUIRE(writtenDataset.getObjectPath() == firstPath);
      const Result<> samePathResult = HDF5::WriteObjectAttributes(writer, writtenDataset, valuesArray, true);
      SIMPLNX_RESULT_REQUIRE_VALID(samePathResult);
    }

    // Registering it through another object writer is refused and names the path it was written at.
    {
      auto otherGroup = dataStructureGroup.createGroup("C_Other");
      REQUIRE(otherGroup.getId() >= 0);
      const Result<> otherPathResult = HDF5::WriteObjectAttributes(writer, otherGroup, valuesArray, true);
      REQUIRE(otherPathResult.invalid());
      REQUIRE(otherPathResult.errors().size() == 1);
      CHECK(otherPathResult.errors().front().code == k_DuplicateRegistrationCode);
      CHECK(otherPathResult.errors().front().message.find(fmt::format("'{}'", firstPath)) != std::string::npos);
    }

    // The second parent has no link yet; the link the writer creates there still points at the first path.
    {
      auto secondParentGroup = dataStructureGroup.openGroup("B_Second");
      const Result<> linkResult = writer.writeDataObject(valuesArray, secondParentGroup);
      SIMPLNX_RESULT_REQUIRE_VALID(linkResult);
    }
  }

  auto fileReader = HDF5::FileIO::ReadFile(filePath);
  REQUIRE(fileReader.isValid());
  CHECK(LinksNameSameObject(fileReader.getId(), firstPath, secondPath));
  auto dataStructureGroup = fileReader.openGroup("DataStructure");
  REQUIRE(dataStructureGroup.isValid());
  auto secondParentGroup = dataStructureGroup.openGroup("B_Second");
  REQUIRE(secondParentGroup.isValid());
  auto linkedDataset = secondParentGroup.openDataset("Values");
  // The returned DatasetIO opens lazily; resolving its identifier lets isValid() report whether the dataset opened.
  (void)linkedDataset.getId();
  REQUIRE(linkedDataset.isValid());
  CHECK(linkedDataset.readAsVector<int32>() == expectedValues);
}

namespace
{
constexpr int32 k_FailedCloseCode = -6056;

/**
 * @brief Builds a small in-memory DataStructure with one top-level array, so a writer has something to write.
 * @param arrayName Name of the top-level array.
 * @return DataStructure holding one 4-tuple int32 array.
 */
DataStructure MakeCloseTestStructure(const std::string& arrayName)
{
  DataStructure dataStructure;
  auto* array = Int32Array::CreateWithStore<Int32DataStore>(dataStructure, arrayName, ShapeType{4}, ShapeType{1});
  REQUIRE(array != nullptr);
  return dataStructure;
}

/**
 * @brief Requires the result to be exactly the checked-close error for filePath.
 * @param result Writer or closeChecked() result.
 * @param filePath File the error must name.
 */
void RequireFailedCloseError(const Result<>& result, const fs::path& filePath)
{
  REQUIRE(result.invalid());
  REQUIRE(result.errors().size() == 1);
  CHECK(result.errors().front().code == k_FailedCloseCode);
  CHECK(result.errors().front().message.find(filePath.string()) != std::string::npos);
}

/**
 * @brief Runs one writer call with the failing-close driver armed and requires it to report the failed close.
 * @param filePath File the writer writes; the error must name it.
 * @param writerCall Writer call under test. Any fixture it appends to must already exist and be closed.
 *
 * The close-failure counter must increase, so the -6056 comes from a close that really failed.
 */
void RequireWriterReportsFailedClose(const fs::path& filePath, const std::function<Result<>()>& writerCall)
{
  const hid_t driverId = UnitTest::FailCloseVfd::RegisterFailCloseVfd();
  REQUIRE(driverId >= 0);
  const int32 failuresBefore = UnitTest::FailCloseVfd::CloseFailureCount();
  Result<> writeResult;
  {
    const UnitTest::FailCloseVfd::FailCloseArmGuard armGuard(driverId);
    writeResult = writerCall();
  }
  REQUIRE(UnitTest::FailCloseVfd::CloseFailureCount() > failuresBefore);
  RequireFailedCloseError(writeResult, filePath);
}

/**
 * @brief Writes a closed .dream3d fixture holding the top-level array "Existing", for append tests.
 * @param filePath Fixture path; any existing file is replaced.
 */
void WriteAppendFixture(const fs::path& filePath)
{
  fs::create_directories(filePath.parent_path());
  const DataStructure fixtureStructure = MakeCloseTestStructure("Existing");
  const Result<> fixtureResult = DREAM3D::WriteFile(filePath, fixtureStructure);
  SIMPLNX_RESULT_REQUIRE_VALID(fixtureResult);
}

/**
 * @brief Closes a raw HDF5 identifier under the HDF5 API lock and marks it closed.
 * @param objectId Identifier to close; left as H5I_INVALID_HID afterwards.
 * @param closeFunction H5Gclose or H5Fclose, matching the identifier's type.
 */
void CloseRawId(hid_t& objectId, herr_t (*closeFunction)(hid_t))
{
  if(objectId < 0)
  {
    return;
  }
  std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
  closeFunction(objectId);
  objectId = H5I_INVALID_HID;
}
} // namespace

TEST_CASE("DREAM3D::WriteFile reports a failure while the file closes", "[DataStructureWriter][CloseFailure][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  const DataStructure dataStructure = MakeCloseTestStructure("Values");
  const fs::path filePath = GetDataDir() / "WriteFileCloseFailure.dream3d";
  fs::create_directories(filePath.parent_path());

  RequireWriterReportsFailedClose(filePath, [&]() { return DREAM3D::WriteFile(filePath, dataStructure); });
}

TEST_CASE("WriterFix: DataStructureWriter::WriteFile reports a failed close", "[DataStructureWriter][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  const DataStructure dataStructure = MakeCloseTestStructure("Values");
  const fs::path filePath = GetDataDir() / "WriterFixStructureWriteCloseFailure.h5";
  fs::create_directories(filePath.parent_path());

  RequireWriterReportsFailedClose(filePath, [&]() { return HDF5::DataStructureWriter::WriteFile(dataStructure, filePath); });
}

TEST_CASE("WriterFix: DREAM3D::AppendFile reports a failed close", "[DataStructureWriter][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  const fs::path filePath = GetDataDir() / "WriterFixDream3dAppendCloseFailure.dream3d";
  WriteAppendFixture(filePath);
  const DataStructure appendStructure = MakeCloseTestStructure("Appended");

  RequireWriterReportsFailedClose(filePath, [&]() { return DREAM3D::AppendFile(filePath, appendStructure, DataPath({"Appended"})); });
}

TEST_CASE("WriterFix: DataStructureWriter::AppendFile reports a failed close", "[DataStructureWriter][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  const fs::path filePath = GetDataDir() / "WriterFixStructureAppendCloseFailure.dream3d";
  WriteAppendFixture(filePath);
  const DataStructure appendStructure = MakeCloseTestStructure("Appended");

  RequireWriterReportsFailedClose(filePath, [&]() { return HDF5::DataStructureWriter::AppendFile(filePath, appendStructure, DataPath({"Appended"})); });
}

TEST_CASE("WriterFix: full recovery reports a failed close", "[DataStructureWriter][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  const DataStructure dataStructure = MakeCloseTestStructure("Values");
  const fs::path filePath = GetDataDir() / "WriterFixFullRecoveryCloseFailure.dream3d";
  fs::create_directories(filePath.parent_path());

  RequireWriterReportsFailedClose(filePath, [&]() { return DREAM3D::WriteRecoveryFile(filePath, dataStructure, Pipeline{}); });
}

TEST_CASE("WriterFix: redirect recovery reports a failed close", "[DataStructureWriter][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  const DataStructure dataStructure = MakeCloseTestStructure("Values");
  const fs::path filePath = GetDataDir() / "WriterFixRedirectRecoveryCloseFailure.dream3d";
  const fs::path userDataFilePath = GetDataDir() / "WriterFixRedirectRecoveryUserData.dream3d";
  fs::create_directories(filePath.parent_path());

  RequireWriterReportsFailedClose(filePath, [&]() { return DREAM3D::WriteRecoveryFile(filePath, dataStructure, Pipeline{}, userDataFilePath); });
}

namespace
{
/**
 * @brief Counts the open file identifiers of every file in this process.
 * @return H5Fget_obj_count(H5F_OBJ_ALL, H5F_OBJ_FILE), read under the HDF5 API lock.
 */
ssize_t CountOpenFileIds()
{
  std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
  return H5Fget_obj_count(H5F_OBJ_ALL, H5F_OBJ_FILE);
}
} // namespace

TEST_CASE("WriterFix: a failed close still releases the file identifier", "[FileIO][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  const fs::path filePath = GetDataDir() / "WriterFixFailedCloseReleasesId.h5";
  fs::create_directories(filePath.parent_path());
  const hid_t driverId = UnitTest::FailCloseVfd::RegisterFailCloseVfd();
  REQUIRE(driverId >= 0);
  const ssize_t openFileIdsBefore = CountOpenFileIds();
  REQUIRE(openFileIdsBefore >= 0);
  const int32 failuresBefore = UnitTest::FailCloseVfd::CloseFailureCount();

  hid_t rawFileId = H5I_INVALID_HID;
  {
    const UnitTest::FailCloseVfd::FailCloseArmGuard armGuard(driverId);
    {
      auto file = HDF5::FileIO::WriteFile(filePath);
      REQUIRE(file.isValid());
      rawFileId = file.getId();

      RequireFailedCloseError(file.closeChecked(), filePath);
      REQUIRE(UnitTest::FailCloseVfd::CloseFailureCount() == failuresBefore + 1);

      // HDF5 keeps the identifier after its close callback failed, so the wrapper must keep owning it.
      REQUIRE(IsHdf5IdValid(rawFileId));
      REQUIRE(file.isValid());
    }
    // The wrapper's destructor released the identifier without running the failing driver close again.
    CHECK(UnitTest::FailCloseVfd::CloseFailureCount() == failuresBefore + 1);
  }

  CHECK_FALSE(IsHdf5IdValid(rawFileId));
  CHECK(CountOpenFileIds() == openFileIdsBefore);
}

TEST_CASE("WriterFix: closeChecked refuses while another holder is open", "[FileIO][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  const fs::path filePath = GetDataDir() / "WriterFixCloseCheckedHolders.h5";
  fs::create_directories(filePath.parent_path());

  auto file = HDF5::FileIO::WriteFile(filePath);
  REQUIRE(file.isValid());
  const hid_t fileId = file.getId();

  // Every raw identifier a section opens is closed here even when a REQUIRE fails, before the wrapper closes the file.
  hid_t groupId = H5I_INVALID_HID;
  hid_t reopenedId = H5I_INVALID_HID;
  auto cleanup = MakeScopeGuard([&]() noexcept {
    CloseRawId(groupId, H5Gclose);
    CloseRawId(reopenedId, H5Fclose);
  });

  SECTION("a group still open")
  {
    {
      std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
      groupId = H5Gcreate2(fileId, "Held", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    }
    REQUIRE(groupId >= 0);

    RequireFailedCloseError(file.closeChecked(), filePath);
    REQUIRE(file.isValid());

    CloseRawId(groupId, H5Gclose);
  }

  SECTION("a second identifier from H5Freopen")
  {
    {
      std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
      reopenedId = H5Freopen(fileId);
    }
    REQUIRE(reopenedId >= 0);

    RequireFailedCloseError(file.closeChecked(), filePath);
    REQUIRE(file.isValid());

    CloseRawId(reopenedId, H5Fclose);
  }

  SECTION("a child opened through the second identifier")
  {
    {
      std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
      reopenedId = H5Freopen(fileId);
      if(reopenedId >= 0)
      {
        groupId = H5Gcreate2(reopenedId, "HeldThroughReopen", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      }
    }
    REQUIRE(reopenedId >= 0);
    REQUIRE(groupId >= 0);

    RequireFailedCloseError(file.closeChecked(), filePath);

    // With the reopened identifier gone, only the child opened through it still holds the file.
    CloseRawId(reopenedId, H5Fclose);
    RequireFailedCloseError(file.closeChecked(), filePath);
    REQUIRE(file.isValid());

    CloseRawId(groupId, H5Gclose);
  }

  // Every holder is closed, so the checked close succeeds and releases the wrapper's identifier.
  const Result<> closeResult = file.closeChecked();
  SIMPLNX_RESULT_REQUIRE_VALID(closeResult);
  CHECK_FALSE(file.isValid());
}

TEST_CASE("WriterFix: FileIO::WriteFile refuses a directory target and leaves it intact", "[FileIO][WriterFix]")
{
  auto app = Application::GetOrCreateInstance();
  const fs::path testDirectory = GetDataDir() / "WriterFixDirectoryTarget";
  fs::remove_all(testDirectory);
  fs::create_directories(testDirectory);
  const auto removeTestDirectory = MakeScopeGuard([&testDirectory]() noexcept {
    std::error_code ignored;
    fs::remove_all(testDirectory, ignored);
  });
  const fs::path emptyDirectory = testDirectory / "empty";
  const fs::path filledDirectory = testDirectory / "filled";
  fs::create_directories(emptyDirectory);
  fs::create_directories(filledDirectory);
  const fs::path entry = filledDirectory / "entry.txt";
  {
    std::ofstream stream(entry, std::ios::binary);
    stream << "directory entry";
  }

  std::vector<fs::path> targets = {emptyDirectory, filledDirectory};
#ifndef _WIN32
  // The status follows links, so a link to a directory is refused like the directory.
  const fs::path directoryLink = testDirectory / "linked";
  fs::create_directory_symlink(emptyDirectory, directoryLink);
  targets.push_back(directoryLink);
#endif

  const DataStructure dataStructure = MakeCloseTestStructure("Values");
  for(const fs::path& target : targets)
  {
    INFO("target: " << target.string());
    {
      auto file = HDF5::FileIO::WriteFile(target);
      CHECK_FALSE(file.isValid());
    }
    const Result<> writeResult = DREAM3D::WriteFile(target, dataStructure);
    REQUIRE(writeResult.invalid());
    CHECK(writeResult.errors().front().code == -9045);
    const Result<> structureResult = HDF5::DataStructureWriter::WriteFile(dataStructure, target);
    REQUIRE(structureResult.invalid());
    CHECK(structureResult.errors().front().code == -8054);
    CHECK(fs::is_directory(target));
  }
  CHECK(fs::is_directory(emptyDirectory));
  CHECK(fs::is_empty(emptyDirectory));
  CHECK(fs::is_regular_file(entry));
}
