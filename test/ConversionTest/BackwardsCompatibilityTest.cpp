#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/CalculatorParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/Dream3dImportParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/ReadCSVFileParameter.hpp"
#include "simplnx/Parameters/ReadHDF5DatasetParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include "OrientationAnalysis/Parameters/OEMEbsdScanSelectionParameter.h"
#include "OrientationAnalysis/Parameters/ReadH5EbsdFileParameter.h"

#include <filesystem>
#include <map>
#include <string>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_SIMPLPipelineNameKey = "Name";
constexpr StringLiteral k_SIMPLNumFilterseKey = "Number_Filters";
constexpr StringLiteral k_SIMPLPipelineBuilderKey = "PipelineBuilder";

constexpr StringLiteral k_ArgsKey = "args";
constexpr StringLiteral k_FilterKey = "filter";
constexpr StringLiteral k_FilterNameKey = "name";
constexpr StringLiteral k_FilterUuidKey = "uuid";
constexpr StringLiteral k_FilterCommentsKey = "comments";
constexpr StringLiteral k_UnknownFilterValue = "UnknownFilter";

constexpr StringLiteral k_SIMPLFilterUuidKey = "Filter_Uuid";
constexpr StringLiteral k_SIMPLFilterHumanNameKey = "Filter_Human_Label";
constexpr StringLiteral k_SIMPLFilterClassNameKey = "Filter_Name";

nlohmann::json CreateFilterJson(std::string_view uuid, std::string_view name, nlohmann::json argsArray, std::string_view comments)
{
  nlohmann::json json;

  auto filterObjectJson = nlohmann::json::object();

  filterObjectJson[k_FilterUuidKey] = uuid;
  filterObjectJson[k_FilterNameKey] = name;

  nlohmann::json argsJsonArray = std::move(argsArray);

  json[k_FilterKey] = std::move(filterObjectJson);
  json[k_ArgsKey] = std::move(argsJsonArray);
  json[k_FilterCommentsKey] = comments;

  return json;
}

std::optional<AbstractPlugin::SIMPLData> FindComplexConversionFromSIMPL(const Uuid& uuid, const FilterList& filterList)
{
  auto plugins = filterList.getLoadedPlugins();
  for(const auto* plugin : plugins)
  {
    auto filterMap = plugin->getSimplToSimplnxMap();
    if(filterMap.count(uuid) > 0)
    {
      return filterMap.at(uuid);
    }
  }
  return {};
}

std::string GenerateSIMPLPipelineStringIndex(int32 index, int32 maxIndex)
{
  std::string numStr = fmt::format("{}", index);

  if(maxIndex >= 10)
  {
    int32 mag = 0;
    int32 max = maxIndex;
    while(max > 0)
    {
      mag++;
      max /= 10;
    }
    std::string formatString = fmt::format("{{:0{}}}", mag);
    numStr = fmt::format(fmt::runtime(formatString), index);
  }
  return numStr;
}

std::map<Uuid, std::pair<std::vector<std::any>, std::function<bool(const std::any&, const std::any&)>>> k_ParamMap = {};

template <typename T>
void CreateMapInput(Uuid&& uuid, T&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<T>(value));
  }
  else
  {
    k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<T>(value)},
                                            [](const std::any& imported, const std::any& exemplar) -> bool { return GetAnyRef<T>(imported) == GetAnyRef<T>(exemplar); }));
  }
}

void CreateMapInput(Uuid&& uuid, GeneratedFileListParameter::ValueType&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<GeneratedFileListParameter::ValueType>(value));
  }
  else
  {
    k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<GeneratedFileListParameter::ValueType>(value)}, [](const std::any& imported, const std::any& exemplar) -> bool {
                         auto importedRef = GetAnyRef<GeneratedFileListParameter::ValueType>(imported);
                         auto exemplarRef = GetAnyRef<GeneratedFileListParameter::ValueType>(exemplar);
                         return importedRef.startIndex == exemplarRef.startIndex && importedRef.endIndex == exemplarRef.endIndex && importedRef.incrementIndex == exemplarRef.incrementIndex &&
                                importedRef.paddingDigits == exemplarRef.paddingDigits && importedRef.ordering == exemplarRef.ordering && importedRef.inputPath == exemplarRef.inputPath &&
                                importedRef.filePrefix == exemplarRef.filePrefix && importedRef.fileSuffix == exemplarRef.fileSuffix && importedRef.fileExtension == exemplarRef.fileExtension;
                       }));
  }
}

void CreateMapInput(Uuid&& uuid, CalculatorParameter::ValueType&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<CalculatorParameter::ValueType>(value));
  }
  else
  {
    k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<CalculatorParameter::ValueType>(value)}, [](const std::any& imported, const std::any& exemplar) -> bool {
                         auto importedRef = GetAnyRef<CalculatorParameter::ValueType>(imported);
                         auto exemplarRef = GetAnyRef<CalculatorParameter::ValueType>(exemplar);
                         return importedRef.m_Equation == exemplarRef.m_Equation;
                       }));
  }
}

void CreateMapInput(Uuid&& uuid, ArrayThresholdsParameter::ValueType&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<ArrayThresholdsParameter::ValueType>(value));
  }
  else
  {
    k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<ArrayThresholdsParameter::ValueType>(value)}, [](const std::any& imported, const std::any& exemplar) -> bool {
                         auto importedRef = dynamic_cast<ArrayThreshold*>(GetAnyRef<ArrayThresholdsParameter::ValueType>(imported).getArrayThresholds().at(0).get());
                         auto exemplarRef = dynamic_cast<ArrayThreshold*>(GetAnyRef<ArrayThresholdsParameter::ValueType>(exemplar).getArrayThresholds().at(0).get());

                         return GetAnyRef<ArrayThresholdsParameter::ValueType>(imported).getArrayThresholds().size() ==
                                    GetAnyRef<ArrayThresholdsParameter::ValueType>(exemplar).getArrayThresholds().size() &&
                                importedRef->getUnionOperator() == exemplarRef->getUnionOperator() && importedRef->getComparisonType() == exemplarRef->getComparisonType() &&
                                importedRef->getArrayPath() == exemplarRef->getArrayPath() && importedRef->getComparisonValue() == exemplarRef->getComparisonValue();
                       }));
  }
}

void CreateMapInput(Uuid&& uuid, Dream3dImportParameter::ValueType&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<Dream3dImportParameter::ValueType>(value));
  }
  else
  {
    k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<Dream3dImportParameter::ValueType>(value)}, [](const std::any& imported, const std::any& exemplar) -> bool {
                         auto importedRef = GetAnyRef<Dream3dImportParameter::ValueType>(imported);
                         auto exemplarRef = GetAnyRef<Dream3dImportParameter::ValueType>(exemplar);
                         return importedRef.FilePath == exemplarRef.FilePath;
                       }));
  }
}

void CreateMapInput(Uuid&& uuid, ReadHDF5DatasetParameter::ValueType&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<ReadHDF5DatasetParameter::ValueType>(value));
  }
  else
  {
    k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<ReadHDF5DatasetParameter::ValueType>(value)}, [](const std::any& imported, const std::any& exemplar) -> bool {
                         auto importedRef = GetAnyRef<ReadHDF5DatasetParameter::ValueType>(imported);
                         auto exemplarRef = GetAnyRef<ReadHDF5DatasetParameter::ValueType>(exemplar);
                         return importedRef.inputFile == exemplarRef.inputFile;
                       }));
  }
}

void CreateMapInput(Uuid&& uuid, ReadCSVFileParameter::ValueType&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<ReadCSVFileParameter::ValueType>(value));
  }
  else
  {
    k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<ReadCSVFileParameter::ValueType>(value)}, [](const std::any& imported, const std::any& exemplar) -> bool {
                         auto importedRef = GetAnyRef<ReadCSVFileParameter::ValueType>(imported);
                         auto exemplarRef = GetAnyRef<ReadCSVFileParameter::ValueType>(exemplar);
                         return importedRef.inputFilePath == exemplarRef.inputFilePath;
                       }));
  }
}

void CreateMapInput(Uuid&& uuid, ReadH5EbsdFileParameter::ValueType&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<ReadH5EbsdFileParameter::ValueType>(value));
  }
  else
  {
    k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<ReadH5EbsdFileParameter::ValueType>(value)}, [](const std::any& imported, const std::any& exemplar) -> bool {
                         auto importedRef = GetAnyRef<ReadH5EbsdFileParameter::ValueType>(imported);
                         auto exemplarRef = GetAnyRef<ReadH5EbsdFileParameter::ValueType>(exemplar);
                         return importedRef.inputFilePath == exemplarRef.inputFilePath;
                       }));
  }
}

void CreateMapInput(Uuid&& uuid, OEMEbsdScanSelectionParameter::ValueType&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<OEMEbsdScanSelectionParameter::ValueType>(value));
  }
  else
  {
    k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<OEMEbsdScanSelectionParameter::ValueType>(value)}, [](const std::any& imported, const std::any& exemplar) -> bool {
                         auto importedRef = GetAnyRef<OEMEbsdScanSelectionParameter::ValueType>(imported);
                         auto exemplarRef = GetAnyRef<OEMEbsdScanSelectionParameter::ValueType>(exemplar);
                         return importedRef.inputFilePath == exemplarRef.inputFilePath && importedRef.scanNames == exemplarRef.scanNames;
                       }));
  }
}

void InitializeMap()
{
  // BoolParameter <- BooleanFilterParameter
  CreateMapInput(Uuid::FromString("b6936d18-7476-4855-9e13-e795d717c50f").value(), true);
  // BoolParameter <- LinkedBooleanFilterParameter
  CreateMapInput(Uuid::FromString("b6936d18-7476-4855-9e13-e795d717c50f").value(), true);

  // ArrayCreationParameter <- DataArrayCreationFilterParameter
  CreateMapInput(Uuid::FromString("ab047a7d-f81b-4e6f-99b5-610e7b69fc5b").value(), DataPath({"DC-A", "AM-A", "DA-A"}));
  // ArrayCreationParameter <- StringFilterParameter
  CreateMapInput(Uuid::FromString("ab047a7d-f81b-4e6f-99b5-610e7b69fc5b").value(), DataPath({"StringFilterParameter"}));

  // ArraySelectionParameter <- DataArraySelectionFilterParameter
  CreateMapInput(Uuid::FromString("ab047a7f-f9ab-4e6f-99b5-610e7b69fc5b").value(), DataPath({"DC-B", "AM-B", "DA-B"}));

  // NeighborListSelectionParameter <- DataArraySelectionFilterParameter
  CreateMapInput(Uuid::FromString("ab0b7a7f-f9ab-4e6f-99b5-610e7b69fc5b").value(), DataPath({"DC-B", "AM-B", "DA-B"}));

  // ChoicesParameter <- ChoiceFilterParameter
  CreateMapInput(Uuid::FromString("ee4d5ce2-9582-48fa-b182-8a766ce0feff").value(), static_cast<ChoicesParameter::ValueType>(42));
  // ChoicesParameter <- LinkedChoicesFilterParameter
  CreateMapInput(Uuid::FromString("ee4d5ce2-9582-48fa-b182-8a766ce0feff").value(), static_cast<ChoicesParameter::ValueType>(1));

  // DataGroupCreationParameter <- AttributeMatrixCreationFilterParameter
  CreateMapInput(Uuid::FromString("bff2d4ac-04a6-5251-b188-4f83f7865074").value(), DataPath({"DC-C", "AM-C"}));
  // DataGroupCreationParameter <- DataContainerCreationFilterParameter
  CreateMapInput(Uuid::FromString("bff2d4ac-04a6-5251-b188-4f83f7865074").value(), DataPath({"DC-E"}));
  // DataGroupCreationParameter <- StringFilterParameter
  CreateMapInput(Uuid::FromString("bff2d4ac-04a6-5251-b188-4f83f7865074").value(), DataPath({"StringFilterParameter"}));
  // DataGroupCreationParameter <- AttributeMatrixSelectionFilterParameter
  CreateMapInput(Uuid::FromString("bff2d4ac-04a6-5251-b188-4f83f7865074").value(), DataPath({"DC-D", "AM-D"}));
  // DataGroupSelectionParameter <- DataArraySelectionFilterParameter + LinkedPathCreationFilterParameter
  CreateMapInput(Uuid::FromString("bff2d4ac-04a6-5251-b188-4f83f7865074").value(), DataPath({"DC-B", "LinkedPathCreationFilterParameter"}));

  // AttributeMatrixSelectionParameter <- AttributeMatrixSelectionFilterParameter
  CreateMapInput(Uuid::FromString("a3619d74-a1d9-4bc2-9e03-ca001d65b119").value(), DataPath({"DC-D", "AM-D"}));
  // AttributeMatrixSelectionParameter <- DataArrayCreationFilterParameter
  CreateMapInput(Uuid::FromString("a3619d74-a1d9-4bc2-9e03-ca001d65b119").value(), DataPath({"DC-A", "AM-A"}));
  // AttributeMatrixSelectionParameter <- DataArraySelectionFilterParameter
  CreateMapInput(Uuid::FromString("a3619d74-a1d9-4bc2-9e03-ca001d65b119").value(), DataPath({"DC-B", "AM-B"}));

  // DataGroupSelectionParameter <- DataContainerSelectionFilterParameter
  CreateMapInput(Uuid::FromString("bff3d4ac-04a6-5251-b178-4f83f7865074").value(), DataPath({"DC-F"}));
  // DataGroupSelectionParameter <- LinkedDataContainerSelectionFilterParameter
  CreateMapInput(Uuid::FromString("bff3d4ac-04a6-5251-b178-4f83f7865074").value(), DataPath({"DC-G"}));

  // GeometrySelectionParameter <- DataArraySelectionFilterParameter
  CreateMapInput(Uuid::FromString("3804cd7f-4ee4-400f-80ad-c5af17735de2").value(), DataPath({"DC-B"}));
  // GeometrySelectionParameter <- DataContainerSelectionFilterParameter
  CreateMapInput(Uuid::FromString("3804cd7f-4ee4-400f-80ad-c5af17735de2").value(), DataPath({"DC-F"}));
  // GeometrySelectionParameter <- AttributeMatrixSelectionFilterParameter
  CreateMapInput(Uuid::FromString("3804cd7f-4ee4-400f-80ad-c5af17735de2").value(), DataPath({"DC-D"}));
  // GeometrySelectionParameter <- MultiDataArraySelectionFilterParameter
  CreateMapInput(Uuid::FromString("3804cd7f-4ee4-400f-80ad-c5af17735de2").value(), DataPath({"DC-H"}));

  // DataPathSelectionParameter <- DataArraySelectionFilterParameter
  CreateMapInput(Uuid::FromString("cd12d081-fbf0-46c4-8f4a-15e2e06e98b8").value(), DataPath({"DC-B", "AM-B", "DA-B"}));
  // DataPathSelectionParameter <- AttributeMatrixSelectionFilterParameter
  CreateMapInput(Uuid::FromString("cd12d081-fbf0-46c4-8f4a-15e2e06e98b8").value(), DataPath({"DC-D", "AM-D"}));
  // DataPathSelectionParameter <- DataCointainerSelectionFilterParameter
  CreateMapInput(Uuid::FromString("cd12d081-fbf0-46c4-8f4a-15e2e06e98b8").value(), DataPath({"DC-F"}));

  // DO NOT REORDER THE FOLLOWING SET OF 4, ORDER CORRESPONDS TO PathType for Indexing
  // FileSystemPathParameter <- InputFileFilterParameter
  CreateMapInput(Uuid::FromString("f9a93f3d-21ef-43a1-a958-e57cbf3b2909").value(), std::filesystem::path("/Input/File/Filter/Parameter.txt"));
  // FileSystemPathParameter <- InputPathFilterParameter
  CreateMapInput(Uuid::FromString("f9a93f3d-21ef-43a1-a958-e57cbf3b2909").value(), std::filesystem::path("/Input/Path/Filter/Parameter"));
  // FileSystemPathParameter <- OutputFileFilterParameter
  CreateMapInput(Uuid::FromString("f9a93f3d-21ef-43a1-a958-e57cbf3b2909").value(), std::filesystem::path("/Output/File/Filter/Parameter.txt"));
  // FileSystemPathParameter <- OutputFileFilterParameter
  CreateMapInput(Uuid::FromString("f9a93f3d-21ef-43a1-a958-e57cbf3b2909").value(), std::filesystem::path("/Output/File/Filter/Parameter"));
  // FileSystemPathParameter <- OutputPathFilterParameter
  CreateMapInput(Uuid::FromString("f9a93f3d-21ef-43a1-a958-e57cbf3b2909").value(), std::filesystem::path("/Output/Path/Filter/Parameter"));

  // GeneratedFileListParameter <- FileListInfoFilterParameter
  CreateMapInput(Uuid::FromString("aac15aa6-b367-508e-bf73-94ab6be0058b").value(), GeneratedFileListParameter::ValueType{.startIndex = 12,
                                                                                                                         .endIndex = 167,
                                                                                                                         .incrementIndex = 3,
                                                                                                                         .paddingDigits = 2,
                                                                                                                         .ordering = static_cast<GeneratedFileListParameter::Ordering>(1),
                                                                                                                         .inputPath = "/File/List/Info/Filter/Parameter",
                                                                                                                         .filePrefix = "prefix-",
                                                                                                                         .fileSuffix = "-suffix",
                                                                                                                         .fileExtension = ".txt"});

  // Float32Parameter <- FloatFilterParameter
  CreateMapInput(Uuid::FromString("e4452dfe-2f70-4833-819e-0cbbec21289b").value(), 92.27f);

  // Float64Parameter <- DoubleFilterParameter
  CreateMapInput(Uuid::FromString("f2a18fff-a095-47d7-b436-ede41b5ea21a").value(), 184.54);

  // UInt8Parameter <- IntFilterParameter
  CreateMapInput(Uuid::FromString("6c3efeff-ce8f-47c0-83d1-262f2b2dd6cc").value(), static_cast<uint8>(0));

  // Int32Parameter <- IntFilterParameter
  CreateMapInput(Uuid::FromString("21acff45-a653-45db-a0d1-f43cd344b93a").value(), -132);
  // Int32Parameter <- FloatFilterParameter
  CreateMapInput(Uuid::FromString("21acff45-a653-45db-a0d1-f43cd344b93a").value(), static_cast<int32>(92.27f));

  // UInt32Parameter <- DoubleFilterParameter
  CreateMapInput(Uuid::FromString("e9521130-276c-40c7-95d7-0b4cb4f80649").value(), static_cast<uint32>(184.54));
  // UInt32Parameter <- IntFilterParameter
  CreateMapInput(Uuid::FromString("e9521130-276c-40c7-95d7-0b4cb4f80649").value(), static_cast<uint32>(0));

  // UInt64Parameter <- UInt64FilterParameter
  CreateMapInput(Uuid::FromString("36d91b23-5500-4ed4-bdf3-d680f54ee5d1").value(), static_cast<uint64>(132));
  // UInt64Parameter <- IntFilterParameter
  CreateMapInput(Uuid::FromString("36d91b23-5500-4ed4-bdf3-d680f54ee5d1").value(), static_cast<uint64>(0));
  // UInt64Parameter <- DoubleFilterParameter
  CreateMapInput(Uuid::FromString("36d91b23-5500-4ed4-bdf3-d680f54ee5d1").value(), static_cast<uint64>(184.54));

  // VectorFloat32Parameter <- AxisAngleFilterParameter
  CreateMapInput(Uuid::FromString("88f231a1-7956-41f5-98b7-4471705d2805").value(), std::vector<float32>{90.0100021f, 23.7000008f, -62.9000015f, 36.4000015f}); // SIMPL inverted input {23.7f, -62.9f, 36.4f, 90.01f}
  // VectorFloat32Parameter <- FloatVec3FilterParameter + FloatFilterParameter
  CreateMapInput(Uuid::FromString("88f231a1-7956-41f5-98b7-4471705d2805").value(), std::vector<float32>{782.6199951171875f, -15.479999542236328f, 49.11000061035156f, 92.2699966430664f});
  // VectorFloat32Parameter <- FloatVec2FilterParameter
  CreateMapInput(Uuid::FromString("88f231a1-7956-41f5-98b7-4471705d2805").value(), std::vector<float32>{-71.63f, 26.81f});
  // VectorFloat32Parameter <- FloatVec3FilterParameter
  CreateMapInput(Uuid::FromString("88f231a1-7956-41f5-98b7-4471705d2805").value(), std::vector<float32>{782.62f, -15.48f, 49.11f});
  // VectorFloat32Parameter <- SecondOrderPolynomialFilterParameter
  CreateMapInput(Uuid::FromString("88f231a1-7956-41f5-98b7-4471705d2805").value(), std::vector<float32>{0.0f, 0.1f, 1.0f, 1.1f, 0.2f, 2.2f});
  // VectorFloat32Parameter <- ThirdOrderPolynomialFilterParameter
  CreateMapInput(Uuid::FromString("88f231a1-7956-41f5-98b7-4471705d2805").value(), std::vector<float32>{0.0f, 0.1f, 1.0f, 1.1f, 0.2f, 2.0f, 1.2f, 2.1f, 0.3f, 3.0f});
  // VectorFloat32Parameter <- FourthOrderPolynomialFilterParameter
  CreateMapInput(Uuid::FromString("88f231a1-7956-41f5-98b7-4471705d2805").value(), std::vector<float32>{0.0f, 0.1f, 1.0f, 1.1f, 0.2f, 2.0f, 1.2f, 2.1f, 0.3f, 3.0f, 2.2f, 1.3f, 3.1f, 0.4f, 4.0f});

  // VectorFloat64Parameter <- RangeFilterParameter
  CreateMapInput(Uuid::FromString("57cbdfdf-9d1a-4de8-95d7-71d0c01c5c96").value(), std::vector<float64>{-2.8, 77.36});
  // VectorFloat64Parameter <- FloatVec2FilterParameter
  CreateMapInput(Uuid::FromString("57cbdfdf-9d1a-4de8-95d7-71d0c01c5c96").value(), std::vector<float64>{-71.63, 26.81}); // These should map to float32 but some are upscaled to double
  // VectorFloat64Parameter <- FloatVec3FilterParameter
  CreateMapInput(Uuid::FromString("57cbdfdf-9d1a-4de8-95d7-71d0c01c5c96").value(), std::vector<float64>{782.6199951171875, -15.479999542236328, 49.110000610351562}); // These should map to float32 but some are upscaled to double

  // VectorInt32Parameter <- IntVec2FilterParameter
  CreateMapInput(Uuid::FromString("d3188e18-e383-4727-ab32-88b5fda56ae8").value(), std::vector<int32>{-23, 61});
  // VectorInt32Parameter <- IntVec3FilterParameter
  CreateMapInput(Uuid::FromString("d3188e18-e383-4727-ab32-88b5fda56ae8").value(), std::vector<int32>{35, -56, 92});

  // VectorUInt32Parameter <- FloatVec3FilterParameter
  CreateMapInput(Uuid::FromString("37322aa6-1a2f-4ecb-9aa1-8922d7ac1e49").value(), std::vector<uint32>{782, 0, 49});

  // NumericTypeParameter <- NumericTypeFilterParameter
  CreateMapInput(Uuid::FromString("a8ff9dbd-45e7-4ed6-8537-12dd53069bce").value(), NumericType::uint16);

  // StringParameter <- StringFilterParameter
  CreateMapInput(Uuid::FromString("5d6d1868-05f8-11ec-9a03-0242ac130003").value(), std::string("StringFilterParameter"));
  // StringParameter <- LinkedPathCreationFilterParameter
  CreateMapInput(Uuid::FromString("5d6d1868-05f8-11ec-9a03-0242ac130003").value(), std::string("LinkedPathCreationFilterParameter"));
  // StringParameter <- LinkedPathCreationFilterParameter
  CreateMapInput(Uuid::FromString("5d6d1868-05f8-11ec-9a03-0242ac130003").value(), std::string("184.540000"));
  // StringParameter <- LinkedPathCreationFilterParameter
  CreateMapInput(Uuid::FromString("5d6d1868-05f8-11ec-9a03-0242ac130003").value(), std::string("DC-E"));

  // SeparatorParameter <- SeparatorFilterParameter
  CreateMapInput(Uuid::FromString("e6936d18-7476-4855-9e13-e795d717c50f").value(), std::string(""));

  // DataObjectNameParameter <- LinkedPathCreationFilterParameter
  CreateMapInput(Uuid::FromString("fbc89375-3ca4-4eb2-8257-aad9bf8e1c94").value(), std::string("LinkedPathCreationFilterParameter"));
  // DataObjectNameParameter <- DataArrayCreationFilterParameter
  CreateMapInput(Uuid::FromString("fbc89375-3ca4-4eb2-8257-aad9bf8e1c94").value(), std::string("DA-A"));
  // DataObjectNameParameter <- StringFilterParameter
  CreateMapInput(Uuid::FromString("fbc89375-3ca4-4eb2-8257-aad9bf8e1c94").value(), std::string("StringFilterParameter"));

  // MultiArraySelectionParameter <- MultiDataArraySelectionFilterParameter
  CreateMapInput(Uuid::FromString("d11e0bd8-f227-4fd1-b618-b6f16b259fc8").value(), std::vector<DataPath>{DataPath({"DC-H", "AM-E", "DA-C"}), DataPath({"DC-I", "AM-F", "DA-D"})});
  // MultiArraySelectionParameter <- DataArraySelectionFilterParameter
  CreateMapInput(Uuid::FromString("d11e0bd8-f227-4fd1-b618-b6f16b259fc8").value(), std::vector<DataPath>{DataPath({"DC-B", "AM-B", "DA-B"})});

  // CalculatorParameter <- CalculatorFilterParameter
  CreateMapInput(Uuid::FromString("ba2d4937-dbec-5536-8c5c-c0a406e80f77").value(), CalculatorParameter::ValueType{.m_SelectedGroup = DataPath{}, .m_Equation = "57+92"});

  // ArrayThresholdsParameter <- ComparisonSelectionAdvancedFilterParameter
  {
    ArrayThreshold arrayThreshold = ArrayThreshold{};
    arrayThreshold.setUnionOperator(static_cast<ArrayThreshold::UnionOperator>(1));
    arrayThreshold.setArrayPath(DataPath({"DC-J", "AM-G", "DA-E"}));
    arrayThreshold.setComparisonType(static_cast<ArrayThreshold::ComparisonType>(1));
    arrayThreshold.setComparisonValue(3.76);
    ArrayThresholdsParameter::ValueType atSet = ArrayThresholdsParameter::ValueType{};
    atSet.setArrayThresholds(ArrayThresholdSet::CollectionType{std::make_shared<ArrayThreshold>(ArrayThreshold(arrayThreshold))});
    CreateMapInput(Uuid::FromString("e93251bc-cdad-44c2-9332-58fe26aedfbe").value(), std::move(atSet));
  }
  // ArrayThresholdsParameter <- ComparisonSelectionFilterParameter
  {
    ArrayThreshold arrayThreshold = ArrayThreshold{};
    arrayThreshold.setArrayPath(DataPath({"DC-K", "AM-H", "DA-F"}));
    arrayThreshold.setComparisonType(static_cast<ArrayThreshold::ComparisonType>(1));
    arrayThreshold.setComparisonValue(84.301);
    ArrayThresholdsParameter::ValueType atSet = ArrayThresholdsParameter::ValueType{};
    atSet.setArrayThresholds(ArrayThresholdSet::CollectionType{std::make_shared<ArrayThreshold>(ArrayThreshold(arrayThreshold))});
    CreateMapInput(Uuid::FromString("e93251bc-cdad-44c2-9332-58fe26aedfbe").value(), std::move(atSet));
  }

  // Dream3dImportParameter <- DataContainerReaderFilterParameter
  CreateMapInput(Uuid::FromString("170a257d-5952-4854-9a91-4281cd06f4f5").value(), Dream3dImportParameter::ValueType(std::filesystem::path("/DataContainer/Reader/Filter/Parameter.dream3d")));

  // DynamicTableParameter <- DynamicTableFilterParameter
  CreateMapInput(Uuid::FromString("eea76f1a-fab9-4704-8da5-4c21057cf44e").value(), DynamicTableParameter::ValueType{{1.1, 1.2, 1.3}, {2.1, 2.2, 2.3}, {3.1, 3.2, 3.3}});

  // EnsembleInfoParameter <- EnsembleInfoFilterParameter
  CreateMapInput(Uuid::FromString("10d3924f-b4c9-4e06-9225-ce11ec8dff89").value(),
                 std::vector<std::array<std::string, 3>>{{"Cubic-High m-3m", "Precipitate", "EnsembleInfo"}, {"Cubic-Low m-3 (Tetrahedral)", "Transformation", "FilterParameter"}});

  // ImportHDF5DatasetParameter <- ImportHDF5DatasetFilterParameter
  CreateMapInput(Uuid::FromString("32e83e13-ee4c-494e-8bab-4e699df74a5a").value(), ReadHDF5DatasetParameter::ValueType{.inputFile = "/Import/HDF5/Dataset/Filter/Parameter.h5"});

  // DataTypeParameter <- ScalarTypeFilterParameter
  CreateMapInput(Uuid::FromString("d31358d5-3253-4c69-aff0-eb98618f851b").value(), DataType::int16);

  // ReadCSVFileParameter <- ReadASCIIDataFilterParameter
  CreateMapInput(Uuid::FromString("4f6d6a33-48da-427a-8b17-61e07d1d5b45").value(), ReadCSVFileParameter::ValueType{.inputFilePath = "/Read/ASCII/Data/Filter/Parameter.csv"});

  // MultiPathSelectionParameter <- DataContainerArrayProxyFilterParameter
  CreateMapInput(Uuid::FromString("b5632f4f-fc13-4234-beb2-8fd8820eb6b6").value(), std::vector<DataPath>{DataPath({"DC-L", "AM-I", "DA-G"}), DataPath({"DC-M", "AM-J"}), DataPath({"DC-N"})});
  // MultiPathSelectionParameter <- MultiAttributeMatrixSelectionFilterParameter
  CreateMapInput(Uuid::FromString("b5632f4f-fc13-4234-beb2-8fd8820eb6b6").value(), std::vector<DataPath>{DataPath({"DC-O", "AM-K"}), DataPath({"DC-P", "AM-L"}), DataPath({"DC-Q", "AM-M"})});
  // MultiPathSelectionParameter <- MultiDataContainerSelectionFilterParameter
  CreateMapInput(Uuid::FromString("b5632f4f-fc13-4234-beb2-8fd8820eb6b6").value(), std::vector<DataPath>{DataPath({"DC-R"}), DataPath({"DC-S"})});

  // OEMEbsdScanSelectionParameter <- OEMEbsdScanSelectionFilterParameter (in OrientationAnalysis parameter)
  CreateMapInput(Uuid::FromString("3935c833-aa51-4a58-81e9-3a51972c05ea").value(),
                 OEMEbsdScanSelectionParameter::ValueType{.inputFilePath = "/Input/File/Filter/Parameter.txt", .scanNames = std::list<std::string>{"Scan A", "Scan B", "Scan C"}});
  // ReadH5EbsdFileParameter <- ReadH5EbsdFilterParameter (in OrientationAnalysis parameter)
  CreateMapInput(Uuid::FromString("FAC15aa6-b367-508e-bf73-94ab6be0058b").value(), ReadH5EbsdFileParameter::ValueType{.inputFilePath = "/Read/H5Ebsd/Filter/Parameter.h5ebsd"});

  //  // From Filters Not Ported Yet (Parameter Doesn't Exist YET)
  //  s_ParameterMapping["MultiInputFileFilterParameter"] = "MultiInputFileFilterParameter";
  //  s_ParameterMapping["EbsdWarpPolynomialFilterParameter"] = "EbsdWarpPolynomialFilterParameter";
  //  s_ParameterMapping["EbsdMontageImportFilterParameter"] = "EbsdMontageImportFilterParameter";
  //  s_ParameterMapping["KbrRecisConfigFilterParameter"] = "KbrRecisConfigFilterParameter";
  //  s_ParameterMapping["ImportVectorImageStackFilterParameter"] = "ImportVectorImageStackFilterParameter";
  //  s_ParameterMapping["MontageSelectionFilterParameter"] = "MontageSelectionFilterParameter";
  //  s_ParameterMapping["MontageStructureSelectionFilterParameter"] = "MontageStructureSelectionFilterParameter";
  //  s_ParameterMapping["DynamicChoiceFilterParameter"] = "DynamicChoiceFilterParameter";
  //  s_ParameterMapping["EMMPMFilterParameter"] = "EMMPMFilterParameter";
  //  s_ParameterMapping["ParagraphFilterParameter"] = "ParagraphFilterParameter";
  //  s_ParameterMapping["PhaseTypeSelectionFilterParameter"] = "PhaseTypeSelectionFilterParameter";
  //  s_ParameterMapping["ShapeTypeSelectionFilterParameter"] = "ShapeTypeSelectionFilterParameter";
  //  s_ParameterMapping["StatsGeneratorFilterParameter"] = "StatsGeneratorFilterParameter";
  //  s_ParameterMapping["Symmetric6x6FilterParameter"] = "Symmetric6x6FilterParameter";

  //  // From Parameters Broken Up or Moved to Separate Module (Parameter Won't Exist)
  //  s_ParameterMapping["OrientationUtilityFilterParameter"] = "OrientationUtilityFilterParameter";
  //  s_ParameterMapping["ConvertHexGridToSquareGridFilterParameter"] = "ConvertHexGridToSquareGridFilterParameter";
  //  s_ParameterMapping["EbsdToH5EbsdFilterParameter"] = "EbsdToH5EbsdFilterParameter";
}

constexpr StringLiteral k_Separator = "\n-----------------------------------------------------------------------";

// These are filter specific keys for parameters that are new to NX (ie not ported)
const std::map<Uuid, std::vector<std::string>> k_KeyIgnoreMap = {
    // MapPointCloudToRegularGridFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("af53ac60-092f-4e4a-9e13-57f0034ce2c7").value(), std::vector<std::string>{"out_of_bounds_value", "out_of_bounds_handling_index", "cell_data_name"}},
    // PointSampleTriangleGeometryFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("ee34ef95-aa04-4ad3-8232-5783a880d279").value(), std::vector<std::string>{"seed_value", "seed_array_name", "use_seed"}},
    // ComputeKMedoidsFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("7643cb64-bcb9-4867-b85a-bf158cd0f54a").value(), std::vector<std::string>{"seed_value", "seed_array_name", "use_seed"}},
    // ComputeKMeansFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("b8682b04-ccc0-49a2-9ced-32d5f5c512f3").value(), std::vector<std::string>{"seed_value", "seed_array_name", "use_seed"}},
    // ComputeArrayStatisticsFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("645ecae2-cb30-4b53-8165-c9857dfa754f").value(), std::vector<std::string>{"range_type_index", "range", "feature_ids_indexing_name", "number_unique_values_name", "mode_array_name", "find_unique_values", "find_mode", "feature_has_data_array_name"}},
    // ExtractInternalSurfacesFromTriangleGeometryFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("e020f76f-a77f-4999-8bf1-9b7529f06d0a").value(), std::vector<std::string>{"vertex_attribute_matrix_name", "triangle_attribute_matrix_name", "node_type_range", "copy_vertex_array_paths", "copy_triangle_array_paths"}},
    // DBSCANFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("763dad44-fad7-4606-808f-617867257b98").value(), std::vector<std::string>{"seed_value", "seed_array_name", "parse_order_index"}},
    // CombineStlFilesFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("76b56f80-fcbe-4d48-a34d-a73d0fc6e5ae").value(), std::vector<std::string>{"vertex_label_name", "vertex_attribute_matrix_name", "part_numbers_name", "output_file_list_name", "face_labels_name", "cell_feature_attribute_matrix_name", "active_array_name"}},
    // ApplyTransformationToGeometryFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("f5bbc16b-3426-4ae0-b27b-ba7862dc40fe").value(), std::vector<std::string>{"save_transform_matrix", "rotation", "output_transform_matrix_path", "translate_geometry_to_global_origin", "input_image_geometry_path"}},
    // ComputeArrayHistogramFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("c6b6d9e5-301d-4767-abf7-530f5ef5007d").value(), std::vector<std::string>{"calculate_modal_bin_ranges", "histogram_bin_range_name", "histogram_modal_bin_ranges_name", "histogram_most_populated_bin_name", "mask_array_path", "use_mask", "output_data_group_path"}},
    // ComputeFeatureClusteringFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("d6e01678-3a03-433f-89ad-4e9adf1f9a45").value(), std::vector<std::string>{"seed_array_name", "rdf_array_name"}},
    // AddBadDataFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("f44c66d1-a095-4e33-871c-c7699d89a011").value(), std::vector<std::string>{"seed_array_name"}},
    // ITKOtsuMultipleThresholdsImageFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("30f37bcd-701f-4e64-aa9d-1181469d3fb5").value(), std::vector<std::string>{"return_bin_midpoint"}},
    // ITKImportImageStackFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("dcf980b7-ecca-46d1-af31-ac65f6e3b6bb").value(), std::vector<std::string>{"scaling", "resample_images_index", "image_data_type_index", "exact_xy_dimensions", "cropping_options", "convert_to_gray_scale", "color_weights", "change_image_data_type"}},
    // ITKImageReaderFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("d72eaf98-9b1d-44c9-88f2-a5c3cf57b4f2").value(), std::vector<std::string>{"origin", "spacing", "length_unit_index", "image_data_type_index", "cropping_options", "change_spacing", "change_origin", "change_image_data_type", "center_origin"}},
    // ITKBinaryProjectionImageFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("04ea495e-2cf0-4dba-8d29-cf33a38c094d").value(), std::vector<std::string>{"output_image_geometry_name"}},
    // ScalarSegmentFeaturesFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("e067cd97-9bbf-4c92-89a6-3cb4fdb76c93").value(), std::vector<std::string>{"neighbor_scheme_index", "is_periodic"}},
    // MergeTwinsFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("f173786a-50cd-4c3c-9518-48ef6fc2bac9").value(), std::vector<std::string>{"use_seed", "seed_value", "seed_array_name"}},
    // MergeColoniesFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("7e3dbc15-51a3-482c-97c2-f82f7af685bf").value(), std::vector<std::string>{"use_seed", "seed_value"}},
    // EBSDSegmentFeaturesFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("1810c2c7-63e3-41db-b204-a5821e6271c0").value(), std::vector<std::string>{"neighbor_scheme_index", "is_periodic"}},
    // CAxisSegmentFeaturesFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("9fe07e17-aef1-4bf1-834c-d3a73dafc27d").value(), std::vector<std::string>{"neighbor_scheme_index"}},
    // AlignSectionsMutualInformationFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("3cf33ad9-8322-4d40-96de-14bbe40969cc").value(), std::vector<std::string>{"slices_array_name", "relative_shifts_array_name", "cumulative_shifts_array_name", "alignment_attribute_matrix_name"}},
    // AlignSectionsMisorientationFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("8df2135c-7079-49f4-9756-4f3c028a5ced").value(), std::vector<std::string>{"slices_array_name", "relative_shifts_array_name", "cumulative_shifts_array_name", "alignment_attribute_matrix_name"}},
    // AlignSectionsFeatureCentroidFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("b83f9bae-9ccf-4932-96c3-7f2fdb091452").value(), std::vector<std::string>{"slices_array_name", "relative_shifts_array_name", "cumulative_shifts_array_name", "alignment_attribute_matrix_name", "centroids_array_name"}},
    // AlignSectionsListFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("6c0e9fcf-bea2-4939-999e-e26379ed7aad").value(), std::vector<std::string>{"shifts_array_path", "input_array_type_index"}},
    // WriteStlFileFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("54a293f4-1366-46ca-b284-fe5965545dd2").value(), std::vector<std::string>{"part_number_path", "output_stl_file", "grouping_type_index", "feature_phases_path"}},
    // ReadVtkStructuredPointsFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("60d55662-ca7a-4be7-b0a0-ed6e785eb51b").value(), std::vector<std::string>{"input_image_geometry_path"}},
    // ReadStlFileFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("2f64bd45-9d28-4254-9e07-6aa7c6d3d015").value(), std::vector<std::string>{"face_labels_name"}},
    // SplitDataArrayByComponentFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("55da791f-4d1c-4413-8673-742f27d2b22b").value(), std::vector<std::string>{"select_components_to_extract", "delete_original_array", "components_to_extract"}},
    // SetImageGeomOriginScalingFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("057bc7fd-c84a-4902-9397-87e51b1b1fe0").value(), std::vector<std::string>{"center_origin"}},
    // RotateSampleRefFrameFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("d2451dc1-a5a1-4ac2-a64d-7991669dcffc").value(), std::vector<std::string>{"keep_input_geometry_origin", "output_image_geometry_path", "rotate_slice_by_slice"}},
    // ConditionalSetValueFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("bad9b7bd-1dc9-4f21-a889-6520e7a41881").value(), std::vector<std::string>{"remove_value", "invert_mask", "use_conditional", "conditional_array_path"}},
    // RenameDataObjectFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("911a3aa9-d3c2-4f66-9451-8861c4b726d5").value(), std::vector<std::string>{"allow_overwrite"}},
    // ExtractComponentAsArrayFilter
    std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("fcc1c1cc-c37a-40fc-97fa-ce40201d34e3").value(), std::vector<std::string>{"remove_components_from_array", "move_components_to_new_array"}},
};
} // namespace

TEST_CASE("nx::core::Test Filter Parameter Conversion", "[simplnx][Filter]")
{
  InitializeMap();

  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  std::string pipelinePath = "/tmp/mega-pipeline.d3dpipeline";

  std::ifstream file(pipelinePath);

  REQUIRE(file.is_open());

  nlohmann::json pipelineJson;

  try
  {
    pipelineJson = nlohmann::json::parse(file);
  } catch(const nlohmann::json::parse_error& exception)
  {
    REQUIRE(false);
  }

  REQUIRE(pipelineJson.contains(k_SIMPLPipelineBuilderKey));

  const auto& pipelineBuilderObject = pipelineJson[k_SIMPLPipelineBuilderKey];

  REQUIRE(pipelineBuilderObject.contains(k_SIMPLPipelineNameKey));

  auto name = pipelineBuilderObject[k_SIMPLPipelineNameKey].get<std::string>();

  REQUIRE(pipelineBuilderObject.contains(k_SIMPLNumFilterseKey));

  auto numFilters = pipelineBuilderObject[k_SIMPLNumFilterseKey].get<int32>();

  std::vector<std::string> errorStrings = {};
  Pipeline pipeline(name, filterList);
  for(int32 i = 0; i < numFilters; i++)
  {
    std::string filterKey = GenerateSIMPLPipelineStringIndex(i, numFilters - 1);

    REQUIRE(pipelineJson.contains(filterKey));

    const auto& filterJson = pipelineJson[filterKey];
    REQUIRE(filterJson.contains(k_SIMPLFilterUuidKey));

    auto uuidString = filterJson[k_SIMPLFilterUuidKey].get<std::string>();
    std::optional<Uuid> filterUuid = Uuid::FromString(uuidString);

    REQUIRE(filterUuid.has_value());

    std::optional<AbstractPlugin::SIMPLData> simplData = FindComplexConversionFromSIMPL(*filterUuid, *filterList);

    if(!simplData.has_value())
    {
      continue; // No NX equivalent (check UUID Maps at plugin level if UB encountered)
    }

    IFilter::UniquePointer filter = filterList->createFilter(simplData->simplnxUuid);
    if(filter == nullptr)
    {
      continue; // No NX equivalent (check UUID Maps at plugin level if UB encountered)
    }
    Result<Arguments> argumentsResult = simplData->convertJson(filterJson);

    std::vector<std::string> ignoredParameterKeys = {};
    if(k_KeyIgnoreMap.contains(filter->uuid()))
    {
      ignoredParameterKeys = k_KeyIgnoreMap.at(filter->uuid());
    }

    const auto filterName = filter->name();
    const auto defaultArguments = filter->getDefaultArguments();
    auto pipelineFilter = std::make_unique<PipelineFilter>(std::move(filter));
    if(argumentsResult.invalid())
    {
      std::string prefix = fmt::format("Filter: '{}' ", filterName);
      for(const auto& error : argumentsResult.errors())
      {
        errorStrings.emplace_back(prefix + error.message);
        errorStrings.emplace_back(k_Separator);
      }
      pipelineFilter->setArguments(defaultArguments);
      continue;
    }

    // This section validates that the mapping from SIMPL Parameter to the SIMPLNX Parameter
    for(const auto& [parameterName, parameter] : pipelineFilter->getFilter()->parameters())
    {
      bool shouldIgnore = false;
      for(const auto& ignoredKey : ignoredParameterKeys)
      {
        if(ignoredKey == parameterName)
        {
          shouldIgnore = true;
          break;
        }
      }
      if(shouldIgnore)
      {
        continue;
      }

      std::string prefix = fmt::format("SIMPL Json conversion error.\n  Filter: '{}'\n  Parameter Key: '{}'\n", filterName, parameterName);
      IParameter::AcceptedTypes acceptedTypes = parameter->acceptedTypes();
      auto iter = std::find(acceptedTypes.cbegin(), acceptedTypes.cend(), std::type_index(argumentsResult.value().at(parameterName).type()));
      if(iter == acceptedTypes.cend())
      {
        errorStrings.emplace_back(prefix +
                                  "The mapping from SIMPL Parameter type to SIMPLNX Parameter type is incorrect. This "
                                  "usually indicates an incorrect conversion in the filter's 'FromSIMPLJson()' method.");
        errorStrings.emplace_back(k_Separator);
      }

      std::pair<std::vector<std::any>, std::function<bool(const std::any&, const std::any&)>> parameterCheck = k_ParamMap[parameter->uuid()];
      std::any importedValue = argumentsResult.value().at(parameterName);
      bool found = false;
      for(const auto& value : parameterCheck.first)
      {
        if(parameterCheck.second(value, importedValue))
        {
          found = true;
          break;
        }
      }
      if(!found)
      {
        errorStrings.emplace_back(prefix + "The value read in from the conversion function does not match any of the expected values. Conversion function should be reviewed.");
        errorStrings.emplace_back(k_Separator);
      }
    }

    pipelineFilter->setArguments(argumentsResult.value());
  }

  CAPTURE(errorStrings);
  REQUIRE(errorStrings.empty());
}
