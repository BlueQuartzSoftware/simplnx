#include "SimplnxCore/Filters/CreateDataArrayFilter.hpp"
#include "SimplnxCore/Filters/CreateImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/ReadDREAM3DFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
namespace Constants
{
const fs::path k_DataDir = "test/data";
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
constexpr StringLiteral k_AttributeMatrixName = "AttributeMatrix";
constexpr StringLiteral k_ArrayName = "Test-Array";
constexpr StringLiteral k_Array2Name = "Test-Array2";

constexpr StringLiteral k_CreateDataFilterName = "Create Data Group";
constexpr StringLiteral k_ExportD3DFilterName = "Write DREAM3D NX File";
} // namespace DataNames

const FilterHandle k_CreateDataGroupHandle(Uuid::FromString("e7d2f9b8-4131-4b28-a843-ea3c6950f101").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_CreateDataArrayHandle(Uuid::FromString("67041f9b-bdc6-4122-acc6-c9fe9280e90d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ExportD3DHandle(Uuid::FromString("b3a95784-2ced-41ec-8d3d-0242ac130003").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());
const FilterHandle k_ImportD3DHandle(Uuid::FromString("0dbd31c7-19e0-4077-83ef-f4a6459a0e2d").value(), Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f").value());

fs::path GetDataDir(const Application& app)
{
  return std::filesystem::path(unit_test::k_BinaryTestOutputDir.view());
}

fs::path GetIODataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_Dream3dFilename;
}

fs::path GetExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_ExportFilename1;
}

fs::path GetReExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_ExportFilename2;
}

fs::path GetMultiExportDataPath1()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename1;
}

fs::path GetMultiExportDataPath2()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename2;
}

fs::path GetReMultiExportDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / Constants::k_MultiExportFilename3;
}

DataStructure CreateTestDataStructure()
{
  DataStructure dataStructure;
  auto group1 = DataGroup::Create(dataStructure, DataNames::k_Group1Name);
  auto group2 = DataGroup::Create(dataStructure, DataNames::k_Group2Name, group1->getId());
  auto group3 = DataGroup::Create(dataStructure, DataNames::k_Group3Name, group2->getId());

  std::vector<usize> tupleShape = {10};
  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, DataNames::k_AttributeMatrixName, tupleShape, group1->getId());

  Result<> arrayCreationResults =
      ArrayCreationUtilities::CreateArray<int8>(dataStructure, tupleShape, std::vector<usize>{1}, DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName, DataNames::k_Array2Name}),
                                                IDataAction::Mode::Execute, ArrayCreationUtilities::k_DefaultDataFormat, "1");
  return dataStructure;
}

Pipeline CreateExportPipeline()
{
  Pipeline pipeline("Export DREAM3D Pipeline 1");
  {
    Arguments args;
    args.insert("data_object_path", DataPath({DataNames::k_Group1Name}));
    pipeline.push_back(k_CreateDataGroupHandle, args);
  }
  {
    Arguments args;
    args.insert("set_tuple_dimensions", std::make_any<bool>(true));
    args.insert("numeric_type_index", std::make_any<NumericType>(NumericType::int8));
    args.insert("component_count", std::make_any<uint64>(3));

    args.insert("tuple_dimensions", DynamicTableInfo::TableDataType{{1.0}});
    args.insert("initialization_value_str", std::make_any<std::string>("7"));
    args.insert("output_array_path", DataPath({DataNames::k_ArrayName}));
    args.insert("data_format", std::string(""));
    pipeline.push_back(k_CreateDataArrayHandle, args);
  }
  {
    Arguments args;
    args.insert("export_file_path", GetExportDataPath());
    args.insert("write_xdmf_file", true);
    pipeline.push_back(k_ExportD3DHandle, args);
  }
  return pipeline;
}

Pipeline CreateImportPipeline()
{
  Pipeline pipeline("Import DREAM3D Pipeline");
  {
    Arguments args;
    auto filePath = GetExportDataPath();
    std::vector<DataPath> dataPaths = std::vector<DataPath>{DataPath({DataNames::k_Group1Name}), DataPath({DataNames::k_ArrayName})};
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::IncludeList, dataPaths);
    args.insert("import_data_object", importData);
    pipeline.push_back(k_ImportD3DHandle, args);
  }
  {
    Arguments args;
    args.insert("export_file_path", GetReExportDataPath());
    args.insert("write_xdmf_file", true);
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
      args.insert("data_object_path", DataPath({DataNames::k_Group1Name}));
      pipeline.push_back(k_CreateDataGroupHandle, args);
    }
    {
      Arguments args;
      args.insert("export_file_path", GetMultiExportDataPath1());
      args.insert("write_xdmf_file", true);
      pipeline.push_back(k_ExportD3DHandle, args);
    }
    REQUIRE(pipeline.execute());
  }
  // Pipeline 2
  {
    Pipeline pipeline("Export Multi DREAM3D Pipeline 2");
    {
      Arguments args;
      args.insert("data_object_path", DataPath({DataNames::k_Group2Name}));
      pipeline.push_back(k_CreateDataGroupHandle, args);
    }
    {
      Arguments args;
      args.insert("export_file_path", GetMultiExportDataPath2());
      args.insert("write_xdmf_file", true);
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
    Dream3dImportParameter::ImportData importData(GetMultiExportDataPath1(), Dream3dImportParameter::PathImportPolicy::IncludeList, std::vector<DataPath>{DataPath({DataNames::k_Group1Name})});
    args.insert("import_data_object", importData);
    pipeline.push_back(k_ImportD3DHandle, args);
  }
  {
    Arguments args;
    Dream3dImportParameter::ImportData importData(GetMultiExportDataPath2(), Dream3dImportParameter::PathImportPolicy::IncludeList, std::vector<DataPath>{DataPath({DataNames::k_Group2Name})});
    args.insert("import_data_object", importData);
    pipeline.push_back(k_ImportD3DHandle, args);
  }
  {
    Arguments args;
    args.insert("export_file_path", GetReMultiExportDataPath());
    args.insert("write_xdmf_file", true);
    pipeline.push_back(k_ExportD3DHandle, args);
  }
  return pipeline;
}

DREAM3D::FileData CreateFileData()
{
  return {CreateExportPipeline(), CreateTestDataStructure()};
}

} // End Namespace

TEST_CASE("DREAM3DFileTest:DREAM3D File IO Test")
{
  UnitTest::LoadPlugins();

  std::lock_guard<std::mutex> lock(m_DataMutex);
  // Write .dream3d file
  {
    auto fileData = CreateFileData();
    auto fileWriter = HDF5::FileIO::WriteFile(GetIODataPath());

    auto writeResult = DREAM3D::WriteFile(fileWriter, fileData);
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }

  // Read .dream3d file
  {
    auto fileReader = HDF5::FileIO::ReadFile(GetIODataPath());
    auto fileResult = DREAM3D::ReadFile(fileReader);
    SIMPLNX_RESULT_REQUIRE_VALID(fileResult);

    auto [pipeline, dataStructure] = fileResult.value();

    // Test reading the DataStructure
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name})) != nullptr);
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_Group2Name, DataNames::k_Group3Name})) != nullptr);
    auto attMatrix = dataStructure.getDataAs<AttributeMatrix>(DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName}));
    REQUIRE(attMatrix != nullptr);
    REQUIRE(attMatrix->getShape() == AttributeMatrix::ShapeType{10});
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName, DataNames::k_Array2Name})) != nullptr);

    // Test reading the Pipeline
    REQUIRE(pipeline.size() == 3);
    REQUIRE(pipeline[0]->getName() == DataNames::k_CreateDataFilterName.str());
    REQUIRE(pipeline[2]->getName() == DataNames::k_ExportD3DFilterName.str());

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("DREAM3DFileTest:Import/Export DREAM3D Filter Test")
{
  UnitTest::LoadPlugins();

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
    REQUIRE(dataArray->getIDataStoreAs<AbstractDataStore<int8>>() != nullptr);

    UnitTest::CheckArraysInheritTupleDims(importDataStructure);
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
    REQUIRE(dataArray->template getIDataStoreAs<EmptyDataStore<int8>>() != nullptr);

    UnitTest::CheckArraysInheritTupleDims(importDataStructure);
  }
}

TEST_CASE("DREAM3DFileTest:Import/Export Multi-DREAM3D Filter Test")
{
  UnitTest::LoadPlugins();

  std::lock_guard<std::mutex> lock(m_DataMutex);

  CreateMultiExportFiles();

  auto importPipeline = CreateMultiImportPipeline();
  REQUIRE(importPipeline.execute());
  auto importDataStructure = importPipeline[1]->getDataStructure();
  auto size = importDataStructure.getSize();
  REQUIRE(size == 2);
  REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group1Name})) != nullptr);
  REQUIRE(importDataStructure.getData(DataPath({DataNames::k_Group2Name})) != nullptr);

  UnitTest::CheckArraysInheritTupleDims(importDataStructure);
}

TEST_CASE("DREAM3DFileTest: Existing Data Objects Test")
{
  UnitTest::LoadPlugins();

  DataStructure ds;
  {
    CreateImageGeometryFilter filter;
    Arguments args;
    args.insert(CreateImageGeometryFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(DataPath({"New Geometry"})));
    args.insert(CreateImageGeometryFilter::k_CellDataName_Key, std::make_any<std::string>("Cell Data"));
    args.insert(CreateImageGeometryFilter::k_Dimensions_Key, std::make_any<std::vector<uint64_t>>(std::vector<uint64_t>{480, 640, 1}));
    args.insert(CreateImageGeometryFilter::k_Origin_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0, 0, 0}));
    args.insert(CreateImageGeometryFilter::k_Spacing_Key, std::make_any<std::vector<float32>>(std::vector<float32>{0.5, 0.5, 0.12}));
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    CreateDataArrayFilter filter;
    Arguments args;
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"New Geometry", "Cell Data", "Array 1"})));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("0"));
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    CreateDataArrayFilter filter;
    Arguments args;
    args.insert(CreateDataArrayFilter::k_NumericType_Key, std::make_any<NumericType>(NumericType::float32));
    args.insert(CreateDataArrayFilter::k_NumComps_Key, std::make_any<uint64>(1));
    args.insert(CreateDataArrayFilter::k_DataPath_Key, std::make_any<DataPath>(DataPath({"New Geometry", "Cell Data", "Array 2"})));
    args.insert(CreateDataArrayFilter::k_InitializationValue_Key, std::make_any<std::string>("0"));
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  {
    const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

    ReadDREAM3DFilter filter;
    Arguments args;
    Dream3dImportParameter::ImportData importData(fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir)));
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("DREAM3DFileTest: Path Import Policy Tests")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");
  auto filePath = fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure;
  ReadDREAM3DFilter filter;
  Arguments args;

  SECTION("All")
  {
    Dream3dImportParameter::ImportData importData(filePath);
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    std::vector<std::string> baseDCNames = {"DataContainer", "SmallerDataContainer"};
    std::vector<std::string> nlAndStringArrayDCs = {"MirroredXDataContainer",      "MirroredYDataContainer", "MirroredZDataContainer", "MirroredXInconsistentArrays", "MirroredYInconsistentArrays",
                                                    "MirroredZInconsistentArrays", "XInconsistentArrays",    "YInconsistentArrays",    "ZInconsistentArrays"};
    std::vector<std::string> fooArrayDCs = {"MirroredXInconsistentArrays", "MirroredYInconsistentArrays", "MirroredZInconsistentArrays",
                                            "XInconsistentArrays",         "YInconsistentArrays",         "ZInconsistentArrays"};
    std::vector<std::string> dcNames;
    dcNames.reserve(baseDCNames.size() + nlAndStringArrayDCs.size() + fooArrayDCs.size());
    dcNames.insert(dcNames.end(), baseDCNames.begin(), baseDCNames.end());
    dcNames.insert(dcNames.end(), nlAndStringArrayDCs.begin(), nlAndStringArrayDCs.end());
    dcNames.insert(dcNames.end(), fooArrayDCs.begin(), fooArrayDCs.end());

    for(const auto& dcName : dcNames)
    {
      REQUIRE(dataStructure.containsData(DataPath({dcName})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Confidence Index"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "EulerAngles"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Fit"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Image Quality"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Phases"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "SEM Signal"})));
    }

    for(const auto& dcName : baseDCNames)
    {
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellEnsembleData"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellEnsembleData", "CrystalStructures"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellEnsembleData", "LatticeConstants"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellEnsembleData", "MaterialName"})));
    }

    for(const auto& dcName : nlAndStringArrayDCs)
    {
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "NeighborList"})));
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "StringArray"})));
    }

    for(const auto& dcName : fooArrayDCs)
    {
      REQUIRE(dataStructure.containsData(DataPath({dcName, "CellData", "Foo"})));
    }
  }
  SECTION("Include List - Leaf Node")
  {
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::IncludeList, {DataPath({"DataContainer", "CellData", "Confidence Index"})});
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData", "Confidence Index"})));
    REQUIRE(!dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData"})));
  }
  SECTION("Include List - Parent Node")
  {
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::IncludeList, {DataPath({"DataContainer", "CellData"})});
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData", "Confidence Index"})));
    REQUIRE(!dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData"})));
  }
  SECTION("Exclude List - Parent Node")
  {
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::ExcludeList, {DataPath({"DataContainer", "CellData"})});
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(!dataStructure.containsData(DataPath({"DataContainer", "CellData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"})));
  }

  SECTION("Exclude List - Leaf Node")
  {
    Dream3dImportParameter::ImportData importData(filePath, Dream3dImportParameter::PathImportPolicy::ExcludeList, {DataPath({"DataContainer", "CellData", "Confidence Index"})});
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData"})));
    REQUIRE(!dataStructure.containsData(DataPath({"DataContainer", "CellData", "Confidence Index"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellData", "Fit"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData"})));
    REQUIRE(dataStructure.containsData(DataPath({"DataContainer", "CellEnsembleData", "CrystalStructures"})));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
