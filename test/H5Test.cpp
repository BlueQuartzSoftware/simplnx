
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"
#include "complex/DataStructure/ScalarData.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"
#include "complex/Utilities/Parsing/Text/CsvParser.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"

#include "GeometryTestUtilities.hpp"

// This file is generated into the binary directory
#include "complex_test_dirs.h"

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
const fs::path DataDir = "test/data";
const fs::path LegacyFilepath = "SmallN100.dream3d";
const fs::path ComplexH5File = "new.h5";
} // namespace Constants

fs::path getDataDir(const Application& app)
{
#if __APPLE__
  return app.getCurrentDir().parent_path().parent_path().parent_path() / Constants::DataDir;
#else
  return app.getCurrentDir() / Constants::DataDir;
#endif
}

fs::path getLegacyFilepath(const Application& app)
{
  return getDataDir(app) / Constants::LegacyFilepath;
}

fs::path getComplexH5File(const Application& app)
{
  return getDataDir(app) / Constants::ComplexH5File;
}

bool equalsf(const FloatVec3& lhs, const FloatVec3& rhs)
{
  for(size_t i = 0; i < 3; i++)
  {
    float diff = std::abs(lhs[i] - rhs[i]);
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

DataStructure getTestDataStructure()
{
  DataStructure ds;
  auto group = DataGroup::Create(ds, "Group");
  REQUIRE(group != nullptr);

  auto montage = GridMontage::Create(ds, "Montage", group->getId());
  REQUIRE(montage != nullptr);

  auto geom = ImageGeom::Create(ds, "Geometry", montage->getId());
  REQUIRE(geom != nullptr);

  auto scalar = ScalarData<int32_t>::Create(ds, "Scalar", 7, geom->getId());
  REQUIRE(scalar != nullptr);

  auto dataStore = new DataStore<uint8_t>({2}, {3});
  auto dataArray = DataArray<uint8_t>::Create(ds, "DataArray", dataStore, geom->getId());
  REQUIRE(dataArray != nullptr);

  return ds;
}

template <typename T>
DataArray<T>* createTestDataArray(const std::string& name, DataStructure& dataGraph, typename DataStore<T>::ShapeType tupleShape, typename DataStore<T>::ShapeType componentShape,
                                  DataObject::IdType parentId)
{
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  DataStoreType* data_store = new DataStoreType(tupleShape, componentShape);
  ArrayType* dataArray = ArrayType::Create(dataGraph, name, data_store, parentId);

  return dataArray;
}

DataStructure createDataStructure()
{
  DataStructure dataGraph;
  DataGroup* group = complex::DataGroup::Create(dataGraph, "Small IN100");
  DataGroup* scanData = complex::DataGroup::Create(dataGraph, "EBSD Scan Data", group->getId());

  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, "Small IN100 Grid", scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  complex::SizeVec3 imageGeomDims = {100, 100, 100};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  std::cout << "Creating Data Structure" << std::endl;
  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  size_t numComponents = 1;
  std::vector<size_t> tupleShape = {imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]};

  FloatArray* ci_data = createTestDataArray<float>("Confidence Index", dataGraph, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = createTestDataArray<int32_t>("FeatureIds", dataGraph, tupleShape, {numComponents}, scanData->getId());
  FloatArray* iq_data = createTestDataArray<float>("Image Quality", dataGraph, tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = createTestDataArray<int32_t>("Phases", dataGraph, tupleShape, {numComponents}, scanData->getId());

  numComponents = 3;
  UInt8Array* ipf_color_data = createTestDataArray<uint8_t>("IPF Colors", dataGraph, tupleShape, {numComponents}, scanData->getId());

  // Add in another group that holds the phase data such as Laue Class, Lattice Constants, etc.
  DataGroup* phase_group = complex::DataGroup::Create(dataGraph, "Phase Data", group->getId());
  numComponents = 1;
  size_t numTuples = 2;
  Int32Array* laue_data = createTestDataArray<int32_t>("Laue Class", dataGraph, {numTuples}, {numComponents}, phase_group->getId());

  return dataGraph;
}


TEST_CASE("Image Geometry IO")
{
  Application app;

  fs::path dataDir = getDataDir(app);

  if(!fs::exists(dataDir))
  {
    REQUIRE(fs::create_directories(dataDir));
  }

  fs::path filePath = getDataDir(app) / "image_geometry_io.h5";

  std::string filePathString = filePath.string();

  // Write HDF5 file
  try
  {
    DataStructure ds = createDataStructure();
    auto fileId = H5Fcreate(filePathString.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(fileId > 0);

    herr_t err;
    err = ds.writeHdf5(fileId);
    REQUIRE(err <= 0);

    err = H5Fclose(fileId);
    REQUIRE(err <= 0);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }

  // Read HDF5 file
  try
  {
    auto fileId = H5Fopen(filePathString.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(fileId > 0);

    herr_t err;
    auto ds = DataStructure::ReadFromHdf5(fileId, err);
    REQUIRE(err <= 0);

    err = H5Fclose(fileId);
    REQUIRE(err <= 0);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }
}



//------------------------------------------------------------------------------
DataStructure createNodeBasedGeometries()
{
  DataStructure dataGraph;
  DataGroup* group = complex::DataGroup::Create(dataGraph, "AM LPBF Experiment");
  DataGroup* scanData = complex::DataGroup::Create(dataGraph, "Laser Scan Data", group->getId());

  std::string inputFile= fmt::format("{}/test/Data/VertexCoordinates.csv", complex::unit_test::k_ComplexSourceDir);
  std::vector<float> csvVerts = complex::CsvParser::ParseVertices(inputFile, ",", true);


  auto vertexGeom = VertexGeom::Create(dataGraph, "Vertex Geom", scanData->getId());
  vertexGeom->resizeVertexList(csvVerts.size() / 3);
  AbstractGeometry::SharedVertexList* vertices = vertexGeom->getVertices();

  std::copy_n(csvVerts.begin(), csvVerts.size(), vertices->getDataStore()->begin());

#if 0
  // Create an Image Geometry grid for the Scan Data
  ImageGeom* imageGeom = ImageGeom::Create(dataGraph, "Small IN100 Grid", scanData->getId());
  imageGeom->setSpacing({0.25f, 0.25f, 0.25f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  complex::SizeVec3 imageGeomDims = {100, 100, 100};
  imageGeom->setDimensions(imageGeomDims); // Listed from slowest to fastest (Z, Y, X)

  std::cout << "Creating Data Structure" << std::endl;
  // Create some DataArrays; The DataStructure keeps a shared_ptr<> to the DataArray so DO NOT put
  // it into another shared_ptr<>
  size_t numComponents = 1;
  std::vector<size_t> tupleShape = {imageGeomDims[0], imageGeomDims[1], imageGeomDims[2]};

  FloatArray* ci_data = createTestDataArray<float>("Confidence Index", dataGraph, tupleShape, {numComponents}, scanData->getId());
  Int32Array* feature_ids_data = createTestDataArray<int32_t>("FeatureIds", dataGraph, tupleShape, {numComponents}, scanData->getId());
  FloatArray* iq_data = createTestDataArray<float>("Image Quality", dataGraph, tupleShape, {numComponents}, scanData->getId());
  Int32Array* phases_data = createTestDataArray<int32_t>("Phases", dataGraph, tupleShape, {numComponents}, scanData->getId());

  numComponents = 3;
  UInt8Array* ipf_color_data = createTestDataArray<uint8_t>("IPF Colors", dataGraph, tupleShape, {numComponents}, scanData->getId());

  // Add in another group that holds the phase data such as Laue Class, Lattice Constants, etc.
  DataGroup* phase_group = complex::DataGroup::Create(dataGraph, "Phase Data", group->getId());
  numComponents = 1;
  size_t numTuples = 2;
  Int32Array* laue_data = createTestDataArray<int32_t>("Laue Class", dataGraph, {numTuples}, {numComponents}, phase_group->getId());

#endif

  return dataGraph;
}

TEST_CASE("Node Based Geometry IO")
{
  Application app;

  fs::path dataDir = getDataDir(app);

  if(!fs::exists(dataDir))
  {
    REQUIRE(fs::create_directories(dataDir));
  }

  fs::path filePath = getDataDir(app) / "node_geometry_io.h5";

  std::string filePathString = filePath.string();

  // Write HDF5 file
  try
  {
    DataStructure ds = createNodeBasedGeometries();
    auto fileId = H5Fcreate(filePathString.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(fileId > 0);

    herr_t err;
    err = ds.writeHdf5(fileId);
    REQUIRE(err <= 0);

    err = H5Fclose(fileId);
    REQUIRE(err <= 0);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }

  // Read HDF5 file
  try
  {
    auto fileId = H5Fopen(filePathString.c_str(), H5P_DEFAULT, H5P_DEFAULT);
    REQUIRE(fileId > 0);

    herr_t err;
    auto ds = DataStructure::ReadFromHdf5(fileId, err);
    REQUIRE(err <= 0);

    err = H5Fclose(fileId);
    REQUIRE(err <= 0);
  } catch(const std::exception& e)
  {
    FAIL(e.what());
  }
}
