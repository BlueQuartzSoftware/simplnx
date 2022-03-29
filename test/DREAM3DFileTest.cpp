#include <catch2/catch.hpp>

#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Parameters/Dream3dImportParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "complex/unit_test/complex_test_dirs.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
namespace Constants
{
const fs::path k_DataDir = "test/data";
const fs::path k_LegacyFilepath = "SmallN100.dream3d";
const fs::path k_Dream3dFilename = "newFile.dream3d";
const fs::path k_ExportFilename1 = "export.dream3d";
const fs::path k_ExportFilename2 = "export2.dream3d";
const fs::path k_MultiExportFilename1 = "multi_export1.dream3d";
const fs::path k_MultiExportFilename2 = "multi_export2.dream3d";
const fs::path k_MultiExportFilename3 = "multi_export3.dream3d";
} // namespace Constants

std::mutex m_DataMutex;

namespace DataNames
{
constexpr StringLiteral k_Group1Name = "Top-Level";
constexpr StringLiteral k_Group2Name = "Second-Level";
constexpr StringLiteral k_Group3Name = "Third-Level";
constexpr StringLiteral k_ArrayName = "Test-Array";

constexpr StringLiteral k_CreateDataFilterName = "Create Data Group";
constexpr StringLiteral k_ExportD3DFilterName = "Write DREAM3D NX File (V8)";
} // namespace DataNames

const FilterHandle k_CreateDataGroupHandle(Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_CreateDataArrayHandle(Uuid::FromString("67041f9b-bdc6-4122-acc6-c9fe9280e90d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ExportD3DHandle(Uuid::FromString("b3a95784-2ced-11ec-8d3d-0242ac130003").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ImportD3DHandle(Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());

fs::path GetDataDir(const Application& app)
{
  return COMPLEX_BUILD_DIR / Constants::k_DataDir;
}

fs::path GetIODataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("complex::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_Dream3dFilename;
}

fs::path GetExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("complex::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_ExportFilename1;
}

fs::path GetReExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("complex::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_ExportFilename2;
}

fs::path GetMultiExportDataPath1()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("complex::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename1;
}

fs::path GetMultiExportDataPath2()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("complex::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename2;
}

fs::path GetReMultiExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("complex::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename3;
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
    args.insert("numeric_type", std::make_any<NumericType>(NumericType::int8));
    args.insert("component_count", std::make_any<uint64>(3));

    DynamicTableData::TableDataType tableData{{1.0}};
    DynamicTableData::HeadersListType headers{"Headers"};
    DynamicTableData::HeadersListType columns{"Columns"};
    auto tupleDimsTable = DynamicTableData::Create(tableData, headers, columns);
    args.insert("tuple_dimensions", tupleDimsTable);
    args.insert("initialization_value", std::make_any<std::string>("7"));
    args.insert("output_data_array", DataPath({DataNames::k_ArrayName}));
    pipeline.push_back(k_CreateDataArrayHandle, args);
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
    importData.DataPaths = std::vector<DataPath>{DataPath({DataNames::k_Group1Name}), DataPath({DataNames::k_ArrayName})};
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

void CreateMultiExportFiles()
{
  // Pipeline 1
  {
    Pipeline pipeline("Export Multi DREAM3D Pipeline 1");
    {
      Arguments args;
      args.insert("Data_Object_Path", DataPath({DataNames::k_Group1Name}));
      pipeline.push_back(k_CreateDataGroupHandle, args);
    }
    {
      Arguments args;
      args.insert("Export_File_Path", GetMultiExportDataPath1());
      pipeline.push_back(k_ExportD3DHandle, args);
    }
    REQUIRE(pipeline.execute());
  }
  // Pipeline 2
  {
    Pipeline pipeline("Export Multi DREAM3D Pipeline 2");
    {
      Arguments args;
      args.insert("Data_Object_Path", DataPath({DataNames::k_Group2Name}));
      pipeline.push_back(k_CreateDataGroupHandle, args);
    }
    {
      Arguments args;
      args.insert("Export_File_Path", GetMultiExportDataPath2());
      pipeline.push_back(k_ExportD3DHandle, args);
    }
    REQUIRE(pipeline.execute());
  }
}

Pipeline CreateMultiImportPipeline()
{
  Pipeline pipeline("Import DREAM3D Pipeline");
  {
    Arguments args;
    Dream3dImportParameter::ImportData importData;
    importData.FilePath = GetMultiExportDataPath1();
    importData.DataPaths = std::vector<DataPath>{DataPath({DataNames::k_Group1Name})};
    args.insert("Import_File_Data", importData);
    pipeline.push_back(k_ImportD3DHandle, args);
  }
  {
    Arguments args;
    Dream3dImportParameter::ImportData importData;
    importData.FilePath = GetMultiExportDataPath2();
    importData.DataPaths = std::vector<DataPath>{DataPath({DataNames::k_Group2Name})};
    args.insert("Import_File_Data", importData);
    pipeline.push_back(k_ImportD3DHandle, args);
  }
  {
    Arguments args;
    args.insert("Export_File_Path", GetReMultiExportDataPath());
    pipeline.push_back(k_ExportD3DHandle, args);
  }
  return pipeline;
}

DREAM3D::FileData CreateFileData()
{
  return {CreateExportPipeline(), CreateTestDataStructure()};
}

} // End Namespace

TEST_CASE("DREAM3D File IO Test")
{
  Application app;
  fs::path pluginPath = complex::unit_test::k_BuildDir.str();
  app.loadPlugins(pluginPath, false);

  std::lock_guard<std::mutex> lock(m_DataMutex);
  // Write .dream3d file
  {
    auto fileData = CreateFileData();
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(GetIODataPath());
    REQUIRE(result.valid());

    auto errorCode = DREAM3D::WriteFile(result.value(), fileData);
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
    REQUIRE(pipeline.size() == 3);
    REQUIRE(pipeline[0]->getName() == DataNames::k_CreateDataFilterName.str());
    REQUIRE(pipeline[2]->getName() == DataNames::k_ExportD3DFilterName.str());
  }
}

TEST_CASE("Import/Export DREAM3D Filter Test")
{
  Application app;
  fs::path pluginPath = complex::unit_test::k_BuildDir.str();
  app.loadPlugins(pluginPath, false);

  std::lock_guard<std::mutex> lock(m_DataMutex);

  auto exportPipeline = CreateExportPipeline();
  REQUIRE(exportPipeline.execute());

  {
    auto importPipeline = CreateImportPipeline();
    REQUIRE(importPipeline.execute());
    auto importDataStructure = importPipeline[0]->getDataStructure();
    auto group1Obj = importDataStructure.getData(DataPath({DataNames::k_Group1Name}));
    auto size = importDataStructure.getSize();
    REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
    auto* dataArray = importDataStructure.getDataAs<DataArray<int8>>(DataPath({DataNames::k_ArrayName}));
    REQUIRE(dataArray != nullptr);
    REQUIRE(dataArray->getIDataStoreAs<DataStore<int8>>() != nullptr);
  }
  {
    auto importPipeline = CreateImportPipeline();
    REQUIRE(importPipeline.preflight());
    auto importDataStructure = importPipeline[0]->getPreflightStructure();
    auto group1Obj = importDataStructure.getData(DataPath({DataNames::k_Group1Name}));
    auto size = importDataStructure.getSize();
    REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
    auto* dataArray = importDataStructure.getDataAs<DataArray<int8>>(DataPath({DataNames::k_ArrayName}));
    REQUIRE(dataArray != nullptr);
    REQUIRE(dataArray->getIDataStoreAs<EmptyDataStore<int8>>() != nullptr);
  }
}

TEST_CASE("Import/Export Multi-DREAM3D Filter Test")
{
  Application app;
  fs::path pluginPath = complex::unit_test::k_BuildDir.str();
  app.loadPlugins(pluginPath, false);

  std::lock_guard<std::mutex> lock(m_DataMutex);

  CreateMultiExportFiles();

  auto importPipeline = CreateMultiImportPipeline();
  REQUIRE(importPipeline.execute());
  auto importDataStructure = importPipeline[1]->getDataStructure();
  auto size = importDataStructure.getSize();
  REQUIRE(size == 2);
  REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
  REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group2Name})) != nullptr);
}
