
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"
#include "complex/DataStructure/ScalarData.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"
#include "complex/Utilities/Parsing/Text/CsvParser.hpp"

#include "GeometryTestUtilities.hpp"

// This file is generated into the binary directory
#include "complex/unit_test/complex_test_dirs.hpp"

#include <catch2/catch.hpp>

#include <iostream>
#include <string>
#include <type_traits>

using namespace complex;
namespace fs = std::filesystem;

static_assert(std::is_same_v<hid_t, H5::IdType>, "H5::IdType must be the same type as hid_t");
static_assert(std::is_same_v<herr_t, H5::ErrorType>, "H5::ErrorType must be the same type as herr_t");
static_assert(std::is_same_v<hsize_t, H5::SizeType>, "H5::SizeType must be the same type as hsize_t");

namespace
{
namespace Constants
{
const fs::path k_DataDir = "test/data";
const fs::path k_LegacyFilepath = "SmallN100.dream3d";
const fs::path k_ComplexH5File = "new.h5";
} // namespace Constants

fs::path GetDataDir(const Application& app)
{
#if __APPLE__
  return COMPLEX_BUILD_DIR / Constants::k_DataDir;
#else
  return COMPLEX_BUILD_DIR / Constants::k_DataDir;
#endif
}

fs::path GetLegacyFilepath(const Application& app)
{
  return GetDataDir(app) / Constants::k_LegacyFilepath;
}

fs::path GetComplexH5File(const Application& app)
{
  return GetDataDir(app) / Constants::k_ComplexH5File;
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

TEST_CASE("Read Legacy DREAM.3D Data")
{
#if 0
  Application app;
  DataStructure ds = H5::Reader::DataStructure::readLegacyFile(getLegacyFilepath(app));

  const std::string geomName = "Small IN100";
  auto geom = ds.getData(DataPath({geomName}));
  REQUIRE(geom != nullptr);
  auto image = dynamic_cast<ImageGeom*>(geom);
  REQUIRE(image != nullptr);
  REQUIRE(equalsf(image->getOrigin(), FloatVec3(-47.0f, 0.0f, -29.0f)));
  REQUIRE(image->getDimensions() == SizeVec3(189, 201, 117));
  REQUIRE(equalsf(image->getSpacing(), FloatVec3(0.25f, 0.25f, 0.25f)));

  {
    const std::string scanData = "EBSD Scan Data";
    REQUIRE(ds.getData(DataPath({geomName, scanData})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "Confidence Index"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "EulerAngles"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "FeatureIds"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "Fit"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "IPFColor"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "Image Quality"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "Mask"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "ParentIds"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "Phases"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "Quats"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, scanData, "SEM Signal"})) != nullptr);
  }

  {
    const std::string grainData = "Grain Data";
    REQUIRE(ds.getData(DataPath({geomName, grainData})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "Active"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "AvgEulerAngles"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "AvgQuats"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "EquivalentDiameters"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "NeighborList"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "NeighborList2"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "NumElements"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "NumNeighbors"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "NumNeighbors2"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "ParentIds"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "Phases"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "SharedSurfaceAreaList"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "SharedSurfaceAreaList2"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "Volumes"})) != nullptr);
  }

  {
    const std::string grainData = "NewGrain Data";
    REQUIRE(ds.getData(DataPath({geomName, grainData})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, grainData, "Active"})) != nullptr);
  }

  {
    const std::string phaseData = "Phase Data";
    REQUIRE(ds.getData(DataPath({geomName, phaseData})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, phaseData, "CrystalStructures"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, phaseData, "LatticeConstants"})) != nullptr);
    REQUIRE(ds.getData(DataPath({geomName, phaseData, "MaterialName"})) != nullptr);
  }
#else
  REQUIRE(true);
#endif
}

DataStructure GetTestDataStructure()
{
  DataStructure ds;
  auto group = DataGroup::Create(ds, "Group");
  REQUIRE(group != nullptr);

  auto montage = GridMontage::Create(ds, "Montage", group->getId());
  REQUIRE(montage != nullptr);

  auto geom = ImageGeom::Create(ds, "Geometry", montage->getId());
  REQUIRE(geom != nullptr);

  auto scalar = ScalarData<int32>::Create(ds, "Scalar", 7, geom->getId());
  REQUIRE(scalar != nullptr);

  auto dataStore = new DataStore<uint8>({2}, {3});
  auto dataArray = DataArray<uint8>::Create(ds, "DataArray", dataStore, geom->getId());
  REQUIRE(dataArray != nullptr);

  return ds;
}

template <typename T>
DataArray<T>* CreateTestDataArray(const std::string& name, DataStructure& dataGraph, typename DataStore<T>::ShapeType tupleShape, typename DataStore<T>::ShapeType componentShape,
                                  DataObject::IdType parentId)
{
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  DataStoreType* data_store = new DataStoreType(tupleShape, componentShape);
  ArrayType* dataArray = ArrayType::Create(dataGraph, name, data_store, parentId);

  return dataArray;
}

DataStructure CreateDataStructure()
{
  DataStructure dataGraph;
  DataGroup* group = DataGroup::Create(dataGraph, "Small IN100");
  DataGroup* scanData = DataGroup::Create(dataGraph, "EBSD Scan Data", group->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, "Small IN100 Grid", scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  SizeVec3 imageGeomDims = {100, 100, 100};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  size_t numComponents = 1;
  std::vector<size_t> tupleShape = {imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]};

  Float32Array* ci_data = CreateTestDataArray<float>("Confidence Index", dataGraph, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = CreateTestDataArray<int32>("FeatureIds", dataGraph, tupleShape, {numComponents}, scanData->getId());
  Float32Array* iq_data = CreateTestDataArray<float>("Image Quality", dataGraph, tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = CreateTestDataArray<int32>("Phases", dataGraph, tupleShape, {numComponents}, scanData->getId());

  numComponents = 3;
  UInt8Array* ipf_color_data = CreateTestDataArray<uint8_t>("IPF Colors", dataGraph, tupleShape, {numComponents}, scanData->getId());

  dataGraph.setAdditionalParent(ipf_color_data->getId(), group->getId());

  // Add in another group that holds the phase data such as Laue Class, Lattice Constants, etc.
  DataGroup* phase_group = DataGroup::Create(dataGraph, "Phase Data", group->getId());
  numComponents = 1;
  size_t numTuples = 2;
  Int32Array* laue_data = CreateTestDataArray<int32_t>("Laue Class", dataGraph, {numTuples}, {numComponents}, phase_group->getId());

  return dataGraph;
}

TEST_CASE("Image Geometry IO")
{
  Application app;

  fs::path dataDir = GetDataDir(app);

  if(!fs::exists(dataDir))
  {
    REQUIRE(fs::create_directories(dataDir));
  }

  fs::path filePath = GetDataDir(app) / "image_geometry_io.h5";

  // Write HDF5 file
  try
  {
    DataStructure ds = CreateDataStructure();
    H5::FileWriter fileWriter(filePath);
    REQUIRE(fileWriter.isValid());

    herr_t err;
    err = ds.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }

  // Read HDF5 file
  try
  {
    auto fileReader = H5::FileReader(filePath);
    REQUIRE(fileReader.isValid());

    herr_t err;
    auto ds = DataStructure::readFromHdf5(fileReader, err);
    REQUIRE(err >= 0);

    filePath = GetDataDir(app) / "image_geometry_io_2.h5";

    // Write DataStructure to another file
    try
    {
      auto fileWriter = H5::FileWriter(filePath);
      REQUIRE(fileWriter.isValid());

      err = ds.writeHdf5(fileWriter);
      REQUIRE(err >= 0);
    } catch(const std::exception& e)
    {
      FAIL(e.what());
    }

  } catch(const std::exception& e)
  {
    FAIL(e.what());
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

void CreateVertexGeometry(DataStructure& dataGraph)
{
  DataGroup* geometryGroup = DataGroup::Create(dataGraph, k_VertexGroupName);
  using MeshIndexType = AbstractGeometry::MeshIndexType;
  auto vertexGeometry = VertexGeom::Create(dataGraph, "[Geometry] Vertex", geometryGroup->getId());

  // DataGroup* scanData = DataGroup::Create(dataGraph, "AttributeMatrix", group->getId());
  uint64 skipLines = 1;
  char delimiter = ',';

  // Create the Vertex Array with a component size of 3
  DataPath path = DataPath({k_VertexGroupName, k_VertexListName});
  std::string inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  complex::Result result = complex::CreateArray<float>(dataGraph, {vertexCount}, 3, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto& vertexArray = complex::CsvParser::ArrayFromPath<float>(dataGraph, path);
  CsvParser::ReadFile(inputFile, vertexArray, skipLines, delimiter);
  vertexGeometry->setVertices(&vertexArray);
  REQUIRE(vertexGeometry->getNumberOfVertices() == 144);

  // Now create some "Cell" data for the Vertex Geometry
  std::vector<size_t> tupleShape = {vertexGeometry->getNumberOfVertices()};
  size_t numComponents = 1;
  Int16Array* ci_data = CreateTestDataArray<int16_t>("Area", dataGraph, tupleShape, {numComponents}, geometryGroup->getId());
  Float32Array* power_data = CreateTestDataArray<float>("Power", dataGraph, tupleShape, {numComponents}, geometryGroup->getId());
  UInt32Array* laserTTL_data = CreateTestDataArray<uint32_t>("LaserTTL", dataGraph, tupleShape, {numComponents}, geometryGroup->getId());
  for(size_t i = 0; i < vertexGeometry->getNumberOfVertices(); i++)
  {
    ci_data->getDataStore()->setValue(i, static_cast<int16_t>(i * 10));
    power_data->getDataStore()->setValue(i, static_cast<float>(i * 2.345f));
    laserTTL_data->getDataStore()->setValue(i, static_cast<uint32_t>(i * 3421));
  }
}

void CreateTriangleGeometry(DataStructure& dataGraph)
{
  // Create a Triangle Geometry
  DataGroup* geometryGroup = DataGroup::Create(dataGraph, k_TriangleGroupName);
  using MeshIndexType = AbstractGeometry::MeshIndexType;
  auto triangleGeom = TriangleGeom::Create(dataGraph, "[Geometry] Triangle", geometryGroup->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_TriangleGroupName, k_TriangleFaceName});
  std::string inputFile = fmt::format("{}/test/Data/TriangleConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 242);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  complex::Result result = complex::CreateArray<MeshIndexType>(dataGraph, {faceCount}, 3, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto& dataArray = complex::CsvParser::ArrayFromPath<MeshIndexType>(dataGraph, path);
  CsvParser::ReadFile(inputFile, dataArray, skipLines, delimiter);
  triangleGeom->setTriangles(&dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_TriangleGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = complex::CreateArray<float>(dataGraph, {vertexCount}, 3, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto& vertexArray = complex::CsvParser::ArrayFromPath<float>(dataGraph, path);
  CsvParser::ReadFile(inputFile, vertexArray, skipLines, delimiter);
  triangleGeom->setVertices(&vertexArray);
}

void CreateQuadGeometry(DataStructure& dataGraph)
{
  // Create a Triangle Geometry
  DataGroup* geometryGroup = DataGroup::Create(dataGraph, k_QuadGroupName);
  using MeshIndexType = AbstractGeometry::MeshIndexType;
  auto geometry = QuadGeom::Create(dataGraph, "[Geometry] Quad", geometryGroup->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_QuadGroupName, k_QuadFaceName});
  std::string inputFile = fmt::format("{}/test/Data/QuadConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 121);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  complex::Result result = complex::CreateArray<MeshIndexType>(dataGraph, {faceCount}, 4, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto& dataArray = complex::CsvParser::ArrayFromPath<MeshIndexType>(dataGraph, path);
  CsvParser::ReadFile(inputFile, dataArray, skipLines, delimiter);
  geometry->setQuads(&dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_QuadGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = complex::CreateArray<float>(dataGraph, {vertexCount}, 3, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto& vertexArray = complex::CsvParser::ArrayFromPath<float>(dataGraph, path);
  CsvParser::ReadFile(inputFile, vertexArray, skipLines, delimiter);
  geometry->setVertices(&vertexArray);
}

void CreateEdgeGeometry(DataStructure& dataGraph)
{
  // Create a Triangle Geometry
  DataGroup* geometryGroup = DataGroup::Create(dataGraph, k_EdgeGroupName);
  using MeshIndexType = AbstractGeometry::MeshIndexType;
  auto geometry = EdgeGeom::Create(dataGraph, "[Geometry] Edge", geometryGroup->getId());

  // Create a Path in the DataStructure to place the geometry
  DataPath path = DataPath({k_EdgeGroupName, k_EdgeFaceName});
  std::string inputFile = fmt::format("{}/test/Data/EdgeConnectivity.csv", unit_test::k_SourceDir.view());
  uint64 skipLines = 1;
  char delimiter = ',';
  uint64 faceCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(faceCount == 264);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of typles
  complex::Result result = complex::CreateArray<MeshIndexType>(dataGraph, {faceCount}, 2, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto& dataArray = complex::CsvParser::ArrayFromPath<MeshIndexType>(dataGraph, path);
  CsvParser::ReadFile(inputFile, dataArray, skipLines, delimiter);
  geometry->setEdges(&dataArray);

  // Create the Vertex Array with a component size of 3
  path = DataPath({k_EdgeGroupName, k_VertexListName});
  inputFile = fmt::format("{}/test/Data/VertexCoordinates.csv", unit_test::k_SourceDir.view());
  uint64 vertexCount = CsvParser::LineCount(inputFile) - skipLines;
  REQUIRE(vertexCount == 144);
  result = complex::CreateArray<float>(dataGraph, {vertexCount}, 3, path, IDataAction::Mode::Execute);
  REQUIRE(result.valid());
  auto& vertexArray = complex::CsvParser::ArrayFromPath<float>(dataGraph, path);
  CsvParser::ReadFile(inputFile, vertexArray, skipLines, delimiter);
  geometry->setVertices(&vertexArray);
}

//------------------------------------------------------------------------------
DataStructure CreateNodeBasedGeometries()
{
  DataStructure dataGraph;

  CreateVertexGeometry(dataGraph);
  CreateTriangleGeometry(dataGraph);
  CreateQuadGeometry(dataGraph);
  CreateEdgeGeometry(dataGraph);

  return dataGraph;
}

TEST_CASE("Node Based Geometry IO")
{
  Application app;

  fs::path dataDir = GetDataDir(app);

  if(!fs::exists(dataDir))
  {
    REQUIRE(fs::create_directories(dataDir));
  }

  fs::path filePath = GetDataDir(app) / "NodeGeometryTest.dream3d";

  std::string filePathString = filePath.string();

  // Write HDF5 file
  try
  {
    DataStructure ds = CreateNodeBasedGeometries();
    H5::FileWriter fileWriter(filePathString);
    REQUIRE(fileWriter.isValid());

    herr_t err;
    err = ds.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }

  // Read HDF5 file
  try
  {
    H5::FileReader fileReader(filePathString);
    REQUIRE(fileReader.isValid());

    herr_t err;
    auto ds = DataStructure::readFromHdf5(fileReader, err);
    REQUIRE(err >= 0);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }
}
