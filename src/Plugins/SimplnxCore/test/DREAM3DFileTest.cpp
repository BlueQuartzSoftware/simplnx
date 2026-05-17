#include "SimplnxCore/Filters/CreateDataArrayFilter.hpp"
#include "SimplnxCore/Filters/CreateImageGeometryFilter.hpp"
#include "SimplnxCore/Filters/ReadDREAM3DFilter.hpp"
#include "SimplnxCore/Filters/WriteDREAM3DFilter.hpp"

#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Metadata/BoolMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/BoolVectorMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/DoubleMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/DoubleVectorMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/IntMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/IntVectorMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/StringMetadataValue.hpp"
#include "simplnx/DataStructure/Metadata/StringVectorMetadataValue.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/HDF5DatasetProbe.hpp"
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
constexpr StringLiteral k_ExportD3DFilterName = "Write DREAM3D-NX File";
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

fs::path GetMetaDataPath()
{
  auto app = Application::Instance();
  if(app == nullptr)
  {
    throw std::runtime_error("nx::core::Application instance not found");
  }

  return GetDataDir(*app) / "MetaDataTest.dream3d";
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

  ShapeType tupleShape = {10};
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

TEST_CASE("DREAM3DFileTest:DREAM3D File IO Test", "[WriteDREAM3DFilter]")
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
    REQUIRE(attMatrix->getShape() == ShapeType{10});
    REQUIRE(dataStructure.getData(DataPath({DataNames::k_Group1Name, DataNames::k_AttributeMatrixName, DataNames::k_Array2Name})) != nullptr);

    // Test reading the Pipeline
    REQUIRE(pipeline.size() == 3);
    REQUIRE(pipeline[0]->getName() == DataNames::k_CreateDataFilterName.str());
    REQUIRE(pipeline[2]->getName() == DataNames::k_ExportD3DFilterName.str());

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("DREAM3DFileTest::StringArray", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
{
  auto app = Application::GetOrCreateInstance();

  fs::path path = GetDataDir(*app) / "StringArray.dream3d";

  DataStructure exportDataStructure;

  DataPath stringArrayPath({"StringArray"});

  std::vector<std::string> values = {"foo", "bar", "baz"};

  REQUIRE(StringArray::CreateWithValues(exportDataStructure, stringArrayPath.getTargetName(), ShapeType{3}, values) != nullptr);

  WriteDREAM3DFilter writeDream3dFilter;
  Arguments writeArgs;
  writeArgs.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, path);
  writeArgs.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
  Result<> writeResult = writeDream3dFilter.execute(exportDataStructure, writeArgs).result;
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  DataStructure importDataStructure;

  ReadDREAM3DFilter readDream3dFilter;
  Arguments readArgs;
  Dream3dImportParameter::ImportData importData(path);
  readArgs.insertOrAssign(ReadDREAM3DFilter::k_ImportFileData, importData);
  Result<> readResult = readDream3dFilter.execute(importDataStructure, readArgs).result;
  SIMPLNX_RESULT_REQUIRE_VALID(readResult);

  const StringArray* stringArray = importDataStructure.getDataAs<StringArray>(stringArrayPath);
  REQUIRE(stringArray != nullptr);
  REQUIRE(std::equal(stringArray->begin(), stringArray->end(), values.begin(), values.end()));
}

TEST_CASE("DREAM3DFileTest:Import/Export DREAM3D Filter Test", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
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

TEST_CASE("DREAM3DFileTest:Import/Export Multi-DREAM3D Filter Test", "[ReadDREAM3DFilter][WriteDREAM3DFilter]")
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

TEST_CASE("DREAM3DFileTest: Existing Data Objects Test", "[ReadDREAM3DFilter]")
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
    const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");

    ReadDREAM3DFilter filter;
    Arguments args;
    Dream3dImportParameter::ImportData importData(fs::path(fmt::format("{}/Small_IN100.dream3d", unit_test::k_TestFilesDir)));
    args.insert(ReadDREAM3DFilter::k_ImportFileData, importData);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  UnitTest::CheckArraysInheritTupleDims(ds, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("DREAM3DFileTest: Path Import Policy Tests", "[ReadDREAM3DFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "Small_IN100_dream3d_v3.tar.gz", "Small_IN100.dream3d");
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

  UnitTest::CheckArraysInheritTupleDims(dataStructure, SmallIn100::k_TupleCheckIgnoredPaths);
}

TEST_CASE("SimplnxCore::ReadDREAM3DFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReadDREAM3DFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadDREAM3DFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadDREAM3DFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ReadDREAM3DFilter>::uuid);

      // Complex parameter type (DataContainerReaderFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}

TEST_CASE("SimplnxCore::WriteDREAM3DFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteDREAM3DFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteDREAM3DFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteDREAM3DFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<WriteDREAM3DFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteDREAM3DFilter::k_ExportFilePath) == fs::path("/test/path/output.dream3d"));
      CHECK(args.value<bool>(WriteDREAM3DFilter::k_WriteXdmf) == true);
    }
  }
}

TEST_CASE("DREAM3DFileTest: DataArray datasets are chunked+deflated when WriteOptions requests it", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();

  const fs::path outPath = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.view()) / "dream3d_compressed.dream3d";
  fs::remove(outPath);

  DataStructure dataStructure;
  const DataPath arrayPath({"LargeArray"});
  constexpr usize k_Tuples = 500'000; // 2 MB, above the 16 KiB small-array bypass
  auto createRes = ArrayCreationUtilities::CreateArray<float32>(dataStructure, std::vector<usize>{k_Tuples}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute,
                                                                ArrayCreationUtilities::k_DefaultDataFormat, "0");
  SIMPLNX_RESULT_REQUIRE_VALID(createRes);
  {
    auto& arr = dataStructure.getDataRefAs<DataArray<float32>>(arrayPath);
    auto& store = arr.getDataStoreRef();
    for(usize i = 0; i < k_Tuples; ++i)
    {
      store[i] = static_cast<float32>(i);
    }
  }

  HDF5::DataStructureWriter::WriteOptions options;
  options.compressionLevel = 5;
  auto writeResult = DREAM3D::WriteFile(outPath, dataStructure, Pipeline{}, false, options);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);

  const std::string hdfPath = std::string("/") + nx::core::Constants::k_DataStructureTag + "/LargeArray";
  auto info = nx::core::UnitTest::ProbeHdf5Dataset(outPath, hdfPath);
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::UnitTest::DatasetLayout::Chunked);
  REQUIRE(info->hasDeflate);
  REQUIRE(info->deflateLevel == 5);

  auto fileReader = HDF5::FileIO::ReadFile(outPath);
  REQUIRE(fileReader.isValid());
  auto fileResult = DREAM3D::ReadFile(fileReader);
  SIMPLNX_RESULT_REQUIRE_VALID(fileResult);
  auto [pipeline, importedDs] = std::move(fileResult.value());
  (void)pipeline;
  REQUIRE_NOTHROW(importedDs.getDataRefAs<DataArray<float32>>(arrayPath));
  const auto& imported = importedDs.getDataRefAs<DataArray<float32>>(arrayPath);
  const auto& original = dataStructure.getDataRefAs<DataArray<float32>>(arrayPath);
  REQUIRE(imported.getSize() == original.getSize());
  UnitTest::CompareDataArrays<float32>(original, imported);
  UnitTest::CheckArraysInheritTupleDims(importedDs);
}

TEST_CASE("WriteDREAM3DFilter: Compression_Off_IsContiguous", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();
  const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "compression_off.dream3d";
  fs::remove(outPath);

  DataStructure ds;
  const DataPath arrayPath({"A"});
  auto cr =
      ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{200'000}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, ArrayCreationUtilities::k_DefaultDataFormat, "1.5");
  SIMPLNX_RESULT_REQUIRE_VALID(cr);

  WriteDREAM3DFilter filter;
  Arguments args;
  args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
  args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
  args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, false);
  args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(5));
  auto r = filter.execute(ds, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(r);

  const std::string hdfPath = std::string("/") + nx::core::Constants::k_DataStructureTag + "/A";
  auto info = nx::core::UnitTest::ProbeHdf5Dataset(outPath, hdfPath);
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::UnitTest::DatasetLayout::Contiguous);
  REQUIRE(info->hasDeflate == false);
}

TEST_CASE("WriteDREAM3DFilter: Compression_On_IsChunkedAndDeflated", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();
  const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "compression_on.dream3d";
  fs::remove(outPath);

  DataStructure ds;
  const DataPath arrayPath({"A"});
  auto cr =
      ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{500'000}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, ArrayCreationUtilities::k_DefaultDataFormat, "0");
  SIMPLNX_RESULT_REQUIRE_VALID(cr);
  {
    auto& arr = ds.getDataRefAs<DataArray<float32>>(arrayPath);
    auto& store = arr.getDataStoreRef();
    for(usize i = 0; i < 500'000; ++i)
    {
      store[i] = static_cast<float32>(i);
    }
  }

  WriteDREAM3DFilter filter;
  Arguments args;
  args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
  args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
  args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
  args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(5));
  auto r = filter.execute(ds, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(r);

  const std::string hdfPath = std::string("/") + nx::core::Constants::k_DataStructureTag + "/A";
  auto info = nx::core::UnitTest::ProbeHdf5Dataset(outPath, hdfPath);
  REQUIRE(info.has_value());
  REQUIRE(info->layout == nx::core::UnitTest::DatasetLayout::Chunked);
  REQUIRE(info->hasDeflate);
  REQUIRE(info->deflateLevel == 5);

  auto fr = nx::core::HDF5::FileIO::ReadFile(outPath);
  auto fileResult = nx::core::DREAM3D::ReadFile(fr);
  SIMPLNX_RESULT_REQUIRE_VALID(fileResult);
  auto [unusedPipeline, imported] = std::move(fileResult.value());
  (void)unusedPipeline;
  REQUIRE_NOTHROW(imported.getDataRefAs<DataArray<float32>>(arrayPath));
  const auto& importedArr = imported.getDataRefAs<DataArray<float32>>(arrayPath);
  const auto& originalArr = ds.getDataRefAs<DataArray<float32>>(arrayPath);
  UnitTest::CompareDataArrays<float32>(originalArr, importedArr);
  UnitTest::CheckArraysInheritTupleDims(imported);
}

TEST_CASE("WriteDREAM3DFilter: Compression_SmallArray_Bypasses", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();
  const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "compression_small_bypass.dream3d";
  fs::remove(outPath);

  DataStructure ds;
  auto crSmall = ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{100}, std::vector<usize>{1}, DataPath({"Small"}), IDataAction::Mode::Execute,
                                                              ArrayCreationUtilities::k_DefaultDataFormat, "2");
  SIMPLNX_RESULT_REQUIRE_VALID(crSmall);
  auto crBig = ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{500'000}, std::vector<usize>{1}, DataPath({"Big"}), IDataAction::Mode::Execute,
                                                            ArrayCreationUtilities::k_DefaultDataFormat, "3");
  SIMPLNX_RESULT_REQUIRE_VALID(crBig);

  WriteDREAM3DFilter filter;
  Arguments args;
  args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
  args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
  args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
  args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(5));
  auto r = filter.execute(ds, args).result;
  SIMPLNX_RESULT_REQUIRE_VALID(r);

  const std::string dsRoot = std::string("/") + nx::core::Constants::k_DataStructureTag;
  auto smallInfo = nx::core::UnitTest::ProbeHdf5Dataset(outPath, dsRoot + "/Small");
  auto bigInfo = nx::core::UnitTest::ProbeHdf5Dataset(outPath, dsRoot + "/Big");
  REQUIRE(smallInfo.has_value());
  REQUIRE(bigInfo.has_value());
  REQUIRE(smallInfo->layout == nx::core::UnitTest::DatasetLayout::Contiguous);
  REQUIRE(smallInfo->hasDeflate == false);
  REQUIRE(bigInfo->layout == nx::core::UnitTest::DatasetLayout::Chunked);
  REQUIRE(bigInfo->hasDeflate);
}

TEST_CASE("WriteDREAM3DFilter: Compression_LevelsRoundTrip", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();

  // Accumulate sizes across a single TEST_CASE run so the monotonicity check at the end actually fires.
  // (DYNAMIC_SECTION would restart the body per section, losing accumulated state.)
  std::vector<std::uintmax_t> sizesByLevel;
  const std::vector<int32> levels = {1, 5, 9};

  for(int32 level : levels)
  {
    const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / fmt::format("compression_rt_level_{}.dream3d", level);
    fs::remove(outPath);

    DataStructure ds;
    const DataPath arrayPath({"A"});
    auto cr =
        ArrayCreationUtilities::CreateArray<float32>(ds, std::vector<usize>{1'000'000}, std::vector<usize>{1}, arrayPath, IDataAction::Mode::Execute, ArrayCreationUtilities::k_DefaultDataFormat, "0");
    SIMPLNX_RESULT_REQUIRE_VALID(cr);
    {
      auto& arr = ds.getDataRefAs<DataArray<float32>>(arrayPath);
      auto& store = arr.getDataStoreRef();
      for(usize i = 0; i < 1'000'000; ++i)
      {
        store[i] = static_cast<float32>(i % 1024);
      }
    }

    WriteDREAM3DFilter filter;
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, level);
    auto r = filter.execute(ds, args).result;
    SIMPLNX_RESULT_REQUIRE_VALID(r);

    auto fr = HDF5::FileIO::ReadFile(outPath);
    REQUIRE(fr.isValid());
    auto fileResult = DREAM3D::ReadFile(fr);
    SIMPLNX_RESULT_REQUIRE_VALID(fileResult);
    auto [unusedPipeline, imported] = std::move(fileResult.value());
    (void)unusedPipeline;
    REQUIRE_NOTHROW(imported.getDataRefAs<DataArray<float32>>(arrayPath));
    UnitTest::CompareDataArrays<float32>(ds.getDataRefAs<DataArray<float32>>(arrayPath), imported.getDataRefAs<DataArray<float32>>(arrayPath));

    // Push after round-trip validation — a corrupt level entry should not enter the monotonicity check.
    sizesByLevel.push_back(fs::file_size(outPath));
  }

  // Size non-increasing as level rises. Probabilistic in the general case but reliable for the i%1024 pattern
  // used here (small symbol alphabet, long runs → deflate dictionary is very effective).
  REQUIRE(sizesByLevel.size() == levels.size());
  REQUIRE(sizesByLevel[0] >= sizesByLevel[1]);
  REQUIRE(sizesByLevel[1] >= sizesByLevel[2]);
}

TEST_CASE("WriteDREAM3DFilter: Compression_Preflight_RejectsOutOfRangeLevel", "[WriteDREAM3DFilter][Compression]")
{
  UnitTest::LoadPlugins();
  const fs::path outPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "compression_preflight.dream3d";
  DataStructure ds;

  WriteDREAM3DFilter filter;

  // use_compression=true + level below [1,9] -> preflight error
  {
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(0));
    REQUIRE(filter.preflight(ds, args).outputActions.invalid());
  }

  // use_compression=true + level above [1,9] -> preflight error
  {
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, true);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(10));
    REQUIRE(filter.preflight(ds, args).outputActions.invalid());
  }

  // use_compression=false -> level is ignored, even if out of range
  {
    Arguments args;
    args.insertOrAssign(WriteDREAM3DFilter::k_ExportFilePath, outPath);
    args.insertOrAssign(WriteDREAM3DFilter::k_WriteXdmf, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_UseCompression, false);
    args.insertOrAssign(WriteDREAM3DFilter::k_CompressionLevel, static_cast<int32>(0));
    REQUIRE(filter.preflight(ds, args).outputActions.valid());
  }
}

TEST_CASE("SimplnxCore::WriteDREAM3DFilter: MetaData", "[SimplnxCore][WriteDREAM3DFilter][MetaData]")
{
  UnitTest::LoadPlugins();

  const std::string groupName = "meta test";
  const std::string boolDataName = "bool";
  const std::string boolVecDataName = "bool-vec";
  const std::string intDataName = "int";
  const std::string intVecDataName = "int-vec";
  const std::string doubleDataName = "double";
  const std::string doubleVecDataName = "double-vec";
  const std::string stringDataName = "string";
  const std::string stringVecDataName = "string-vec";

  constexpr bool boolValue = true;
  constexpr int32 intValue = 5;
  constexpr float64 doubleValue = 8.6;
  const std::string stringValue = "test string";
  const std::vector<bool> boolVecValue = {true, false, true};
  const std::vector<int32> intVecValue = {5, 6, 7};
  const std::vector<float64> doubleVecValue = {8.6, 1.2};
  const std::vector<std::string> stringVecValue = {"test string"};

  DataStructure dataStructure;
  auto* metaObject = DataGroup::Create(dataStructure, groupName);
  auto& metadata = metaObject->getMetadata();

  {
    // Bool metadata
    metadata.setData<BoolMetadataValue>(boolDataName, boolValue);
    metadata.setData<BoolVectorMetadataValue>(boolVecDataName, boolVecValue);

    // Int metadata
    metadata.setData<IntMetadataValue>(intDataName, intValue);
    metadata.setData<Int32VectorMetadataValue>(intVecDataName, intVecValue);

    // Double metadata
    metadata.setData<DoubleMetadataValue>(doubleDataName, doubleValue);
    metadata.setData<DoubleVectorMetadataValue>(doubleVecDataName, doubleVecValue);

    // String metadata
    metadata.setData<StringMetadataValue>(stringDataName, stringValue);
    metadata.setData<StringVectorMetadataValue>(stringVecDataName, stringVecValue);
  }

  {
    std::lock_guard<std::mutex> lock(m_DataMutex);
    std::filesystem::path metadataPath = GetMetaDataPath();
    // Write .dream3d file
    {
      auto writeResult = DREAM3D::WriteFile(metadataPath, dataStructure);
      SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
    }

    // Read .dream3d file
    {
      auto fileReader = HDF5::FileIO::ReadFile(metadataPath);
      auto fileResult = DREAM3D::ReadFile(fileReader);
      SIMPLNX_RESULT_REQUIRE_VALID(fileResult);

      auto [pipeline, dataStructureRead] = fileResult.value();

      // Get MetaData
      const auto& metaObjectRead = dataStructureRead.getDataRefAs<DataGroup>(DataPath({groupName}));
      const auto& metaDataRead = metaObjectRead.getMetadata();

      auto boolDataReadPtr = metaDataRead.getDataValuePtr(boolDataName);
      auto boolVecDataReadPtr = metaDataRead.getDataValuePtr(boolVecDataName);
      auto intDataReadPtr = metaDataRead.getDataValuePtr(intDataName);
      auto intVecDataReadPtr = metaDataRead.getDataValuePtr(intVecDataName);
      auto doubleDataReadPtr = metaDataRead.getDataValuePtr(doubleDataName);
      auto doubleVecDataReadPtr = metaDataRead.getDataValuePtr(doubleVecDataName);
      auto stringDataReadPtr = metaDataRead.getDataValuePtr(stringDataName);
      auto stringVecDataReadPtr = metaDataRead.getDataValuePtr(stringVecDataName);

      // Require metadata exists
      REQUIRE(boolDataReadPtr != nullptr);
      REQUIRE(boolVecDataReadPtr != nullptr);
      REQUIRE(intDataReadPtr != nullptr);
      REQUIRE(intVecDataReadPtr != nullptr);
      REQUIRE(doubleDataReadPtr != nullptr);
      REQUIRE(doubleVecDataReadPtr != nullptr);
      REQUIRE(stringDataReadPtr != nullptr);
      REQUIRE(stringVecDataReadPtr != nullptr);

      // Require metadata preserves typename
      REQUIRE(boolDataReadPtr->getTypeName() == BoolMetadataValue::k_TypeName);
      REQUIRE(boolVecDataReadPtr->getTypeName() == BoolVectorMetadataValue::k_TypeName);
      REQUIRE(intDataReadPtr->getTypeName() == IntMetadataValue::k_TypeName);
      REQUIRE(intVecDataReadPtr->getTypeName() == Int32VectorMetadataValue::k_TypeName);
      REQUIRE(doubleDataReadPtr->getTypeName() == DoubleMetadataValue::k_TypeName);
      REQUIRE(doubleVecDataReadPtr->getTypeName() == DoubleVectorMetadataValue::k_TypeName);
      REQUIRE(stringDataReadPtr->getTypeName() == StringMetadataValue::k_TypeName);
      REQUIRE(stringVecDataReadPtr->getTypeName() == StringVectorMetadataValue::k_TypeName);

      // Require metadata preserves values
      auto& boolDataReadRef = *metaDataRead.getDataValuePtrAs<BoolMetadataValue>(boolDataName).get();
      auto& boolVecDataReadRef = *metaDataRead.getDataValuePtrAs<BoolVectorMetadataValue>(boolVecDataName).get();
      auto& intDataReadRef = *metaDataRead.getDataValuePtrAs<IntMetadataValue>(intDataName).get();
      auto& intVecDataReadRef = *metaDataRead.getDataValuePtrAs<Int32VectorMetadataValue>(intVecDataName).get();
      auto& doubleDataReadRef = *metaDataRead.getDataValuePtrAs<DoubleMetadataValue>(doubleDataName).get();
      auto& doubleVecDataReadRef = *metaDataRead.getDataValuePtrAs<DoubleVectorMetadataValue>(doubleVecDataName).get();
      auto& stringDataReadRef = *metaDataRead.getDataValuePtrAs<StringMetadataValue>(stringDataName).get();
      auto& stringVecDataReadRef = *metaDataRead.getDataValuePtrAs<StringVectorMetadataValue>(stringVecDataName).get();

      REQUIRE(boolDataReadRef == boolValue);
      REQUIRE(boolVecDataReadRef == boolVecValue);
      REQUIRE(intDataReadRef == intValue);
      REQUIRE(intVecDataReadRef == intVecValue);
      REQUIRE(doubleDataReadRef == doubleValue);
      REQUIRE(doubleVecDataReadRef == doubleVecValue);
      REQUIRE(stringDataReadRef == stringValue);
      REQUIRE(stringVecDataReadRef == stringVecValue);

      // Simplified MetaData API
      REQUIRE(metaDataRead.getDataAs<bool>(boolDataName) == boolValue);
      REQUIRE(metaDataRead.getDataAs<int32>(intDataName) == intValue);
      REQUIRE(metaDataRead.getDataAs<float64>(doubleDataName) == doubleValue);
      REQUIRE(metaDataRead.getDataAs<std::string>(stringDataName) == stringValue);
      // vector data
      REQUIRE(metaDataRead.getDataAs<std::vector<bool>>(boolVecDataName) == boolVecValue);
      REQUIRE(metaDataRead.getDataAs<std::vector<int32>>(intVecDataName) == intVecValue);
      REQUIRE(metaDataRead.getDataAs<std::vector<float64>>(doubleVecDataName) == doubleVecValue);
      REQUIRE(metaDataRead.getDataAs<std::vector<std::string>>(stringVecDataName) == stringVecValue);
    }
  }
}
