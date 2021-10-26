#include <catch2/catch.hpp>

#include <filesystem>
#include <iostream>
#include <mutex>
#include <vector>

#include "nlohmann/json.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
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
const fs::path k_ExportFilename1 = "export.dream3d";
const fs::path k_ExportFilename2 = "export2.dream3d";

std::mutex m_DataMutex;

namespace DataNames
{
constexpr StringLiteral k_Group1Name = "Top-Level";
constexpr StringLiteral k_Group2Name = "Second-Level";
constexpr StringLiteral k_Group3Name = "Third-Level";

constexpr StringLiteral k_CreateDataFilterName = "Create Data Group";
constexpr StringLiteral k_ExportD3DFilterName = "Write DREAM.3D File";
} // namespace DataNames

const inline FilterHandle k_CreateDataGroupHandle(Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const inline FilterHandle k_ExportD3DHandle(Uuid::FromString("b3a95784-2ced-11ec-8d3d-0242ac130003").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const inline FilterHandle k_ImportD3DHandle(Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
} // namespace

fs::path GetDataDir(const Application& app)
{
#if __APPLE__
  return app.getCurrentDir().parent_path().parent_path().parent_path() / k_DataDir;
#else
  return app.getCurrentDir() / k_DataDir;
#endif
}

fs::path GetIODataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("complex::Application instance not found");
  }

  return GetDataDir(*app) / k_Dream3dFilename;
}

fs::path GetExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("complex::Application instance not found");
  }

  return GetDataDir(*app) / k_ExportFilename1;
}

fs::path GetReExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("complex::Application instance not found");
  }

  return GetDataDir(*app) / k_ExportFilename2;
}

DataStructure CreateTestDataStructure()
{
  DataStructure dataStructure;
  auto group1 = DataGroup::Create(dataStructure, DataNames::k_Group1Name);
  auto group2 = DataGroup::Create(dataStructure, DataNames::k_Group2Name, group1->getId());
  auto group3 = DataGroup::Create(dataStructure, DataNames::k_Group3Name, group2->getId());
  return dataStructure;
}

Pipeline CreateExportPipeline()
{
  Pipeline pipeline("Export DREAM3D Pipeline 1");
  {
    Arguments args;
    args.insert("Data_Object_Path", DataPath({DataNames::k_Group1Name}));
    pipeline.push_back(k_CreateDataGroupHandle, args);
  }
  {
    Arguments args;
    args.insert("Export_File_Path", GetExportDataPath());
    pipeline.push_back(k_ExportD3DHandle, args);
  }
  return pipeline;
}

Pipeline CreateImportPipeline()
{
  Pipeline pipeline("Import DREAM3D Pipeline");
  {
    Arguments args;
    Dream3dImportParameter::ImportData importData;
    importData.FilePath = GetExportDataPath();
    importData.DataPaths = std::vector<DataPath>{DataPath({DataNames::k_Group1Name})};
    args.insert("Import_File_Data", importData);
    pipeline.push_back(k_ImportD3DHandle, args);
  }
  {
    Arguments args;
    args.insert("Export_File_Path", GetReExportDataPath());
    pipeline.push_back(k_ExportD3DHandle, args);
  }
  return pipeline;
}

DREAM3D::FileData CreateFileData()
{
  return {CreateExportPipeline(), CreateTestDataStructure()};
}

TEST_CASE("DREAM3D File IO Test")
{
  Application app;
  fs::path pluginPath = fmt::format("{}/{}/", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
  app.loadPlugins(pluginPath, false);

  std::lock_guard<std::mutex> lock(m_DataMutex);
  // Write .dream3d file
  {
    auto fileData = CreateFileData();
    H5::FileWriter fileWriter(GetIODataPath());
    auto errorCode = DREAM3D::WriteFile(fileWriter, fileData);
    REQUIRE(errorCode >= 0);
  }

  // Read .dream3d file
  {
    H5::ErrorType errorCode;
    H5::FileReader fileReader(GetIODataPath());
    auto [pipeline, dataStructure] = DREAM3D::ReadFile(fileReader, errorCode);
    REQUIRE(errorCode >= 0);

    // Test reading the DataStructure
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name, DataNames::k_Group3Name})) != nullptr);

    // Test reading the Pipeline
    REQUIRE(pipeline.size() == 2);
    REQUIRE(pipeline[0]->getName() == DataNames::k_CreateDataFilterName.str());
    REQUIRE(pipeline[1]->getName() == DataNames::k_ExportD3DFilterName.str());
  }
}

TEST_CASE("Import/Export DREAM3D Filter Test")
{
  Application app;
  fs::path pluginPath = fmt::format("{}/{}/", complex::unit_test::k_BuildDir, complex::unit_test::k_BuildTypeDir);
  app.loadPlugins(pluginPath, false);

  std::lock_guard<std::mutex> lock(m_DataMutex);

  auto exportPipeline = CreateExportPipeline();
  REQUIRE(exportPipeline.execute());

  auto importPipeline = CreateImportPipeline();
  REQUIRE(importPipeline.execute());
  auto importDataStructure = importPipeline[0]->getDataStructure();
  auto group1Obj = importDataStructure.getData(DataPath({DataNames::k_Group1Name}));
  auto size = importDataStructure.getSize();
  REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
}
