#include <catch2/catch.hpp>

#include <filesystem>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "complex/unit_test/complex_test_dirs.h"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
const fs::path k_DataDir = "test/data";
const fs::path k_LegacyFilepath = "SmallN100.dream3d";
const fs::path k_Dream3dFilename = "newFile.dream3d";

namespace DataNames
{
constexpr StringLiteral k_Group1Name = "Top-Level";
constexpr StringLiteral k_Group2Name = "Second-Level";
constexpr StringLiteral k_Group3Name = "Third-Level";

constexpr StringLiteral k_CreateDataFilterName = "Create Data Group";
} // namespace DataNames

const inline FilterHandle k_CreateDataGroupHandle(Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
} // namespace

DataStructure CreateTestDataStructure()
{
  DataStructure dataStructure;
  auto group1 = DataGroup::Create(dataStructure, DataNames::k_Group1Name);
  auto group2 = DataGroup::Create(dataStructure, DataNames::k_Group2Name, group1->getId());
  auto group3 = DataGroup::Create(dataStructure, DataNames::k_Group3Name, group2->getId());
  return dataStructure;
}

Pipeline CreatePipeline()
{
  Pipeline pipeline;
  {
    Arguments args;
    args.insert("Data_Object_Path", DataPath({"Group1"}));
    pipeline.push_back(k_CreateDataGroupHandle, args);
  }
  {
    Arguments args;
    args.insert("Data_Object_Path", DataPath({"Group1", "Group2"}));
    pipeline.push_back(k_CreateDataGroupHandle, args);
  }
  return pipeline;
}

DREAM3D::FileData CreateFileData()
{
  return {CreatePipeline(), CreateTestDataStructure()};
}

fs::path GetDataDir(const Application& app)
{
#if __APPLE__
  return app.getCurrentDir().parent_path().parent_path().parent_path() / Constants::k_DataDir;
#else
  return app.getCurrentDir() / k_DataDir;
#endif
}

fs::path GetCurrentDataPath(const Application& app)
{
  return GetDataDir(app) / k_Dream3dFilename;
}

TEST_CASE("DREAM3D File IO Test")
{
  Application app;
  fs::path pluginPath = fmt::format("{}/{}/", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
  app.loadPlugins(pluginPath, false);

  // Write .dream3d file
  {
    auto fileData = CreateFileData();
    H5::FileWriter fileWriter(GetCurrentDataPath(app));
    auto errorCode = DREAM3D::WriteFile(fileWriter, fileData);
    REQUIRE(errorCode >= 0);
  }

  // Read .dream3d file
  {
    H5::ErrorType errorCode;
    H5::FileReader fileReader(GetCurrentDataPath(app));
    auto [pipeline, dataStructure] = DREAM3D::ReadFile(fileReader, errorCode);
    REQUIRE(errorCode >= 0);

    // Test reading the DataStructure
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name, DataNames::k_Group3Name})) != nullptr);

    // Test reading the Pipeline
    REQUIRE(pipeline.size() == 2);
    REQUIRE(pipeline[0]->getName() == DataNames::k_CreateDataFilterName.str());
    REQUIRE(pipeline[1]->getName() == DataNames::k_CreateDataFilterName.str());
  }
}
