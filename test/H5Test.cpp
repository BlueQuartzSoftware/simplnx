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
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/Montage/GridMontage.hpp"
#include "simplnx/DataStructure/ScalarData.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Readers/FileReader.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"
#include "simplnx/Utilities/Parsing/Text/CsvParser.hpp"

#include "GeometryTestUtilities.hpp"

#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <catch2/catch.hpp>

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
namespace Constants
{
const fs::path k_DataDir = "test/data";
const StringLiteral k_LegacyFilepath = "LegacyData.dream3d";
const fs::path k_ComplexH5File = "new.h5";
} // namespace Constants

fs::path GetDataDir()
{
  return std::filesystem::path(unit_test::k_BinaryTestOutputDir.view());
}

fs::path GetLegacyFilepath()
{
  std::string path = fmt::format("{}/test/Data/{}", unit_test::k_SourceDir.view(), Constants::k_LegacyFilepath);
  return std::filesystem::path(path);
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
} // namespace

#if TEST_LEGACY
TEST_CASE("Read Legacy DREAM.3D Data")
{
  auto app = Application::GetOrCreateInstance();
  std::filesystem::path filepath = GetLegacyFilepath();
  REQUIRE(exists(filepath));
  Result<DataStructure> result = DREAM3D::ImportDataStructureFromFile(filepath, true);
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
}
#endif

DataStructure GetTestDataStructure()
{
  DataStructure dataStructure;
  auto group = DataGroup::Create(dataStructure, "Group");
  REQUIRE(group != nullptr);

  auto montage = GridMontage::Create(dataStructure, "Montage", group->getId());
  REQUIRE(montage != nullptr);

  auto geom = ImageGeom::Create(dataStructure, "Geometry", montage->getId());
  REQUIRE(geom != nullptr);

  auto scalar = ScalarData<int32>::Create(dataStructure, "Scalar", 7, geom->getId());
  REQUIRE(scalar != nullptr);

  auto dataStore = std::make_unique<DataStore<uint8>>(std::vector<usize>{2}, std::vector<usize>{3}, 0);
  auto dataArray = DataArray<uint8>::Create(dataStructure, "DataArray", std::move(dataStore), geom->getId());
  REQUIRE(dataArray != nullptr);

  return dataStructure;
}

template <typename T>
DataArray<T>* CreateTestDataArray(const std::string& name, DataStructure& dataStructure, typename DataStore<T>::ShapeType tupleShape, typename DataStore<T>::ShapeType componentShape,
                                  DataObject::IdType parentId)
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
  std::vector<usize> tupleShape = {imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]};

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
    Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(filePathString);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
    REQUIRE(fileWriter.isValid());

    Result<> writeResult = HDF5::DataStructureWriter::WriteFile(originalDataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  {
    nx::core::HDF5::FileReader fileReader(filePathString);
    REQUIRE(fileReader.isValid());

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
  }
}

const std::string k_TriangleGroupName = "TRIANGLE_GEOMETRY";
const std::string k_TriangleFaceName = "SharedTriList";
const std::string k_VertexListName = "SharedVertexList";
const std::string k_VertexGroupName = "VERTEX_GEOMETRY";
const std::string k_QuadGroupName = "QUAD_GEOMETRY";
const std::string k_QuadFaceName = "SharedQuadList";
const std::string k_EdgeFaceName = "SharedEdgeList";
const std::string k_EdgeGroupName = "EDGE_GEOMETRY";
const std::string k_NeighborGroupName = "NEIGHBORLIST_GROUP";

void CreateVertexGeometry(DataStructure& dataStructure)
{
  DataGroup* geometryGroup = DataGroup::Create(dataStructure, k_VertexGroupName);
  using MeshIndexType = IGeometry::MeshIndexType;
  auto vertexGeometry = VertexGeom::Create(dataStructure, "[Geometry] Vertex", geometryGroup->getId());

  // DataGroup* scanData = DataGroup::Create(dataStructure, "AttributeMatrix", group->getId());
  uint64 skipLines = 1;
  char delimiter = ',';

  // Create the Vertex Array with a component size of 3
  DataPath path = DataPath({k_VertexGroupName, k_VertexListName});
  std::string inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  nx::core::Result result = nx::core::CreateArray<float>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto vertexArray = nx::core::ArrayFromPath<float>(dataStructure, path);
  CsvParser::ReadFile<float, float>(inputFile, *vertexArray, skipLines, delimiter);
  vertexGeometry->setVertices(*vertexArray);
  REQUIRE(vertexGeometry->getNumberOfVertices() == 144);

  // Now create some "Cell" data for the Vertex Geometry
  std::vector<usize> tupleShape = {vertexGeometry->getNumberOfVertices()};
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
  nx::core::Result result = nx::core::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto dataArray = nx::core::ArrayFromPath<MeshIndexType>(dataStructure, path);
  CsvParser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, *dataArray, skipLines, delimiter);
  triangleGeom->setFaceList(*dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_TriangleGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = nx::core::CreateArray<float>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto vertexArray = nx::core::ArrayFromPath<float>(dataStructure, path);
  CsvParser::ReadFile<float, float>(inputFile, *vertexArray, skipLines, delimiter);
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
  nx::core::Result result = nx::core::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {4}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto dataArray = nx::core::ArrayFromPath<MeshIndexType>(dataStructure, path);
  CsvParser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, *dataArray, skipLines, delimiter);
  geometry->setFaceList(*dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_QuadGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = nx::core::CreateArray<float>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto vertexArray = nx::core::ArrayFromPath<float>(dataStructure, path);
  CsvParser::ReadFile<float, float>(inputFile, *vertexArray, skipLines, delimiter);
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
  nx::core::Result result = nx::core::CreateArray<MeshIndexType>(dataStructure, {faceCount}, {2}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto dataArray = nx::core::ArrayFromPath<MeshIndexType>(dataStructure, path);
  CsvParser::ReadFile<MeshIndexType, MeshIndexType>(inputFile, *dataArray, skipLines, delimiter);
  geometry->setEdgeList(*dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_EdgeGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = nx::core::CreateArray<float>(dataStructure, {vertexCount}, {3}, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto vertexArray = nx::core::ArrayFromPath<float>(dataStructure, path);
  CsvParser::ReadFile<float, float>(inputFile, *vertexArray, skipLines, delimiter);
  geometry->setVertices(*vertexArray);
}

void CreateNeighborList(DataStructure& dataStructure)
{
  const usize numItems = 50;

  auto* neighborGroup = DataGroup::Create(dataStructure, k_NeighborGroupName);
  auto* neighborGroup2 = DataGroup::Create(dataStructure, k_NeighborGroupName + "2");
  auto* neighborList = NeighborList<int64>::Create(dataStructure, "NeighborList", numItems, neighborGroup->getId());
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
  const std::vector<usize> tupleShape = {2};
  const std::vector<usize> componentShape = {1};

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

  StringArray::CreateWithValues(dataStructure, "StringArray", {"Foo", "Bar", "Bazz"});
}

//------------------------------------------------------------------------------
DataStructure CreateNodeBasedGeometries()
{
  DataStructure dataStructure;

  CreateVertexGeometry(dataStructure);
  CreateTriangleGeometry(dataStructure);
  CreateQuadGeometry(dataStructure);
  CreateEdgeGeometry(dataStructure);

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

TEST_CASE("Node Based Geometry IO")
{
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
  try
  {
    DataStructure ds = CreateNodeBasedGeometries();
    nodeData = getNodeGeomData(ds);
    Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(filePathString);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
    REQUIRE(fileWriter.isValid());

    auto resultH5 = HDF5::DataStructureWriter::WriteFile(ds, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(resultH5);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }

  // Read HDF5 file
  try
  {
    nx::core::HDF5::FileReader fileReader(filePathString);
    REQUIRE(fileReader.isValid());

    auto readResult = HDF5::DataStructureReader::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    checkNodeGeomData(dataStructure, nodeData);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
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
  try
  {
    DataStructure dataStructure;
    CreateNeighborList(dataStructure);
    Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(filePathString);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
    REQUIRE(fileWriter.isValid());

    Result<> writeResult = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }

  // Read HDF5 file
  try
  {
    nx::core::HDF5::FileReader fileReader(filePathString);
    REQUIRE(fileReader.isValid());

    herr_t err = 0;
    auto readResult = HDF5::DataStructureReader::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    // auto neighborList = dataStructure.getDataAs<NeighborList<int64>>(DataPath({k_NeighborGroupName, "NeighborList"}));
    auto neighborList = dataStructure.getData(DataPath({k_NeighborGroupName, "NeighborList"}));
    REQUIRE(neighborList != nullptr);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
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
  try
  {
    DataStructure dataStructure;
    CreateArrayTypes(dataStructure);
    Result<nx::core::HDF5::FileWriter> result = nx::core::HDF5::FileWriter::CreateFile(filePathString);
    SIMPLNX_RESULT_REQUIRE_VALID(result);

    nx::core::HDF5::FileWriter fileWriter = std::move(result.value());
    REQUIRE(fileWriter.isValid());

    Result<> writeResult = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }

  // Read HDF5 file
  try
  {
    nx::core::HDF5::FileReader fileReader(filePathString);
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
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }

  // Read HDF5 file in preflight mode to make sure StringArrays import correctly
  try
  {
    nx::core::HDF5::FileReader fileReader(filePathString);
    REQUIRE(fileReader.isValid());

    auto readResult = HDF5::DataStructureReader::ReadFile(fileReader, true);
    SIMPLNX_RESULT_REQUIRE_VALID(readResult);
    DataStructure dataStructure = std::move(readResult.value());

    StringArray* stringArray = dataStructure.getDataAs<StringArray>(DataPath({"StringArray"}));
    REQUIRE(stringArray != nullptr);
    REQUIRE(stringArray->size() == 3);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
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
  auto vertsDataStore = std::make_unique<DataStore<float32>>(IDataStore::ShapeType{numVerts}, IDataStore::ShapeType{3}, 0.0f);
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

// #include "hdf5.h"
// #include <stdio.h>

/* Function prototypes */
herr_t list_groups(hid_t loc_id, const char* name, const H5L_info_t* info, void* operator_data);
herr_t list_datasets(hid_t loc_id, const char* name, const H5O_info_t* info, void* operator_data);
herr_t list_attributes(hid_t loc_id, const char* name, const H5A_info_t* info, void* operator_data);

int indent_level = 0;

/* Function to list groups */
herr_t list_groups(hid_t loc_id, const char* name, const H5L_info_t* info, void* operator_data)
{
  indent_level++;
  H5O_info_t obj_info;
  H5Oget_info_by_name(loc_id, name, &obj_info, H5P_DEFAULT);

  if(obj_info.type == H5O_TYPE_GROUP)
  {
  //  printf("Group: %s  \n", name);
    std::cout << fmt::format("\n{: >{}}:Group: {}", "", indent_level*3, name) << std::endl;

    H5Aiterate_by_name(loc_id, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, list_attributes, NULL, H5P_DEFAULT);
    H5Literate_by_name(loc_id, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, list_groups, NULL, H5P_DEFAULT);
  }
  else if(obj_info.type == H5O_TYPE_DATASET)
  {
    //printf("Dataset: %s  \n", name);
    std::cout << fmt::format("\n{: >{}}:Dataset: {}", "", indent_level*3, name) << std::endl;

   // H5Aiterate_by_name(loc_id, name, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, list_attributes, NULL, H5P_DEFAULT);
  }
  indent_level--;

  return 0;
}

/* Function to list attributes */
herr_t list_attributes(hid_t loc_id, const char* name, const H5A_info_t* info, void* operator_data)
{
  hid_t attr_id, type_id, space_id;
  H5T_class_t type_class;
  H5S_class_t space_class;
  size_t type_size;

  //printf("  Attribute: %s  ", name);
  std::cout << fmt::format("{: >{}}:Attribute: {}\n", "", indent_level*3, name) << std::endl;

  /* Open the attribute */
  attr_id = H5Aopen(loc_id, name, H5P_DEFAULT);

  /* Get the datatype */
  type_id = H5Aget_type(attr_id);
  type_class = H5Tget_class(type_id);
  type_size = H5Tget_size(type_id);
  printf("    Type: ");
  switch(type_class)
  {
  case H5T_INTEGER:
    printf("Integer  ");
    break;
  case H5T_FLOAT:
    printf("Float  ");
    break;
  case H5T_STRING:
    printf("String  ");
    break;
  default:
    printf("Other  ");
    break;
  }
  printf("    Size: %zu  ", type_size);

  /* Get the dataspace and display its type (scalar, simple, etc.) */
  space_id = H5Aget_space(attr_id);
  space_class = H5Sget_simple_extent_type(space_id);
  printf("    Dataspace: ");
  switch(space_class)
  {
  case H5S_SCALAR:
    printf("Scalar ");
    break;
  case H5S_SIMPLE:
    printf("Simple ");
    break;
  default:
    printf("Other ");
    break;
  }

  /* Close the attribute, dataspace, and datatype */
  H5Sclose(space_id);
  H5Tclose(type_id);
  H5Aclose(attr_id);
  printf("\n");
  return 0;
}

TEST_CASE("List Contents")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "PoleFigure_Exemplars.tar.gz", "PoleFigure_Exemplars");

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/PoleFigure_Exemplars/fw-ar-IF1-aptr12-corr.dream3d", unit_test::k_TestFilesDir));

  hid_t file_id;

  /* Open the HDF5 file */
  file_id = H5Fopen(baseDataFilePath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  REQUIRE(file_id > 0);
  if(file_id < 0)
  {
    printf("Could not open file: %s  ", baseDataFilePath.c_str());
  }

  /* Recursively list groups and datasets */
  H5Literate(file_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, list_groups, NULL);

  /* Close the file */
  H5Fclose(file_id);
}
