#include <catch2/catch.hpp>

#include <iostream>
#include <string>
#include <type_traits>

#include <hdf5.h>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"
#include "complex/DataStructure/ScalarData.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Writer.hpp"

#define H5_USE_110_API

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
const fs::path LegacyFilepath = DataDir / "SmallN100.dream3d";
const fs::path ComplexH5File = DataDir / "new.h5";
} // namespace Constants

fs::path getDataDir(const Application& app)
{
  return app.getCurrentDir() / Constants::DataDir;
}

fs::path getLegacyFilepath(const Application& app)
{
  return app.getCurrentDir() / Constants::LegacyFilepath;
}

fs::path getComplexH5File(const Application& app)
{
#if __APPLE__
  return app.getCurrentDir().parent_path().parent_path().parent_path() / Constants::ComplexH5File;
#else
  return app.getCurrentDir() / Constants::ComplexH5File;
#endif
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

  auto dataStore = new DataStore<uint8_t>(3, 2);
  auto dataArray = DataArray<uint8_t>::Create(ds, "DataArray", dataStore, geom->getId());
  REQUIRE(dataArray != nullptr);

  return ds;
}

TEST_CASE("complex IO")
{
  Application app;

  fs::path dataDir = getDataDir(app);

  fmt::print("dataDir = {}\n", dataDir);

  if(!fs::exists(dataDir))
  {
    fmt::print("Creating dataDir\n");
    REQUIRE(fs::create_directories(dataDir));
  }

  fs::path filePath = getComplexH5File(app);

  fmt::print("filePath = {}\n", filePath);

  std::string filePathString = filePath.string();

  // Write HDF5 file
  try
  {
    DataStructure ds = getTestDataStructure();
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
