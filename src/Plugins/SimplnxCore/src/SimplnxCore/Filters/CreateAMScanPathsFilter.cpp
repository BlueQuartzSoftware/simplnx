#include "CreateAMScanPathsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CreateAMScanPaths.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CreateAMScanPathsFilter::name() const
{
  return FilterTraits<CreateAMScanPathsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CreateAMScanPathsFilter::className() const
{
  return FilterTraits<CreateAMScanPathsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CreateAMScanPathsFilter::uuid() const
{
  return FilterTraits<CreateAMScanPathsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CreateAMScanPathsFilter::humanName() const
{
  return "Generate Scan Vectors";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateAMScanPathsFilter::defaultTags() const
{
  return {className(), "AFRLDistributionC", "Scan Path"};
}

//------------------------------------------------------------------------------
Parameters CreateAMScanPathsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<Float32Parameter>(k_StripeWidth_Key, "Vector Width", "The stripe width", 7.0f));
  params.insert(std::make_unique<Float32Parameter>(k_HatchSpacing_Key, "Vector Spacing", "The hatch spacing", 0.14f));
  params.insert(std::make_unique<Float32Parameter>(k_Power_Key, "Power", "Laser Power", 100.0f));
  params.insert(std::make_unique<Float32Parameter>(k_Speed_Key, "Speed", "Scan Speed", 1000.0f));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_CADSliceDataContainerName_Key, "Slice Data Container", "The input edge geometry from which to create the scan paths", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Edge}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CADSliceIdsArrayPath_Key, "Slice Ids", "Identifies the slice to which each edge belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CADRegionIdsArrayPath_Key, "Region Ids", "Identifies the region to which each edge belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Objects"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(k_HatchDataContainerName_Key, "Scan Vector Geometry", "The created edge geometry representing the scan paths", DataPath({"ScanVectorGeometry"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexAttributeMatrixName_Key, "Vector Node Attribute Matrix", "The name of the attribute matrix containing the scan paths' node data",
                                                          "Vector Node Data"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_HatchAttributeMatrixName_Key, "Vector Attribute Matrix", "The name of the attribute matrix containing the scan paths' data", "Vector Data"));
  params.insertSeparator(Parameters::Separator{"Vector Node Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_TimeArrayName_Key, "Times", "The name of the array containing scan paths' node times", "Times"));
  params.insertSeparator(Parameters::Separator{"Vector Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_RegionIdsArrayName_Key, "Region Ids", "The name of the array identifying the region to which each scan path belongs", "RegionIds"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PowersArrayName_Key, "Powers", "The name of the array containing the scan vectors' laser power", "Powers"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CreateAMScanPathsFilter::clone() const
{
  return std::make_unique<CreateAMScanPathsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CreateAMScanPathsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pStripeWidthValue = filterArgs.value<float32>(k_StripeWidth_Key);
  auto pHatchSpacingValue = filterArgs.value<float32>(k_HatchSpacing_Key);
  auto pPowerValue = filterArgs.value<float32>(k_Power_Key);
  auto pSpeedValue = filterArgs.value<float32>(k_Speed_Key);
  auto pCADSliceDataContainerNameValue = filterArgs.value<DataPath>(k_CADSliceDataContainerName_Key);
  auto pCADSliceIdsArrayPathValue = filterArgs.value<DataPath>(k_CADSliceIdsArrayPath_Key);
  auto pCADRegionIdsArrayPathValue = filterArgs.value<DataPath>(k_CADRegionIdsArrayPath_Key);
  auto pHatchDataContainerNameValue = filterArgs.value<DataPath>(k_HatchDataContainerName_Key);
  auto pVertexAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  auto pHatchAttributeMatrixNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_HatchAttributeMatrixName_Key);
  auto pTimeArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_TimeArrayName_Key);
  auto pRegionIdsArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_RegionIdsArrayName_Key);
  auto pPowersArrayNameValue = filterArgs.value<DataObjectNameParameter::ValueType>(k_PowersArrayName_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
    auto createGeometryAction = std::make_unique<CreateEdgeGeometryAction>(pHatchDataContainerNameValue, 1, 2, pVertexAttributeMatrixNameValue, pHatchAttributeMatrixNameValue,
                                                                           CreateEdgeGeometryAction::k_DefaultVerticesName, CreateEdgeGeometryAction::k_DefaultEdgesName);
    resultOutputActions.value().appendAction(std::move(createGeometryAction));
  }
  const DataPath hatchAttributeMatrixPath = pHatchDataContainerNameValue.createChildPath(pHatchAttributeMatrixNameValue);
  std::vector<size_t> tDims = {1};
  const std::vector<size_t> compDims = {1};
  {
    DataPath path = hatchAttributeMatrixPath.createChildPath(pCADSliceIdsArrayPathValue.getTargetName());
    auto createArray = std::make_unique<CreateArrayAction>(DataType::int32, tDims, compDims, path);
    resultOutputActions.value().appendAction(std::move(createArray));
  }
  {
    DataPath path = hatchAttributeMatrixPath.createChildPath(pPowersArrayNameValue);
    auto createArray = std::make_unique<CreateArrayAction>(DataType::float32, tDims, compDims, path);
    resultOutputActions.value().appendAction(std::move(createArray));
  }
  {
    DataPath path = hatchAttributeMatrixPath.createChildPath(pRegionIdsArrayNameValue);
    auto createArray = std::make_unique<CreateArrayAction>(DataType::int32, tDims, compDims, path);
    resultOutputActions.value().appendAction(std::move(createArray));
  }
  {
    DataPath path = pHatchDataContainerNameValue.createChildPath(pVertexAttributeMatrixNameValue).createChildPath(pTimeArrayNameValue);
    auto createArray = std::make_unique<CreateArrayAction>(DataType::float64, std::vector<size_t>{2}, compDims, path);
    resultOutputActions.value().appendAction(std::move(createArray));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CreateAMScanPathsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  CreateAMScanPathsInputValues inputValues;

  inputValues.StripeWidth = filterArgs.value<float32>(k_StripeWidth_Key);
  inputValues.HatchSpacing = filterArgs.value<float32>(k_HatchSpacing_Key);
  inputValues.Power = filterArgs.value<float32>(k_Power_Key);
  inputValues.Speed = filterArgs.value<float32>(k_Speed_Key);
  inputValues.CADSliceDataContainerName = filterArgs.value<DataPath>(k_CADSliceDataContainerName_Key);
  inputValues.CADSliceIdsArrayPath = filterArgs.value<DataPath>(k_CADSliceIdsArrayPath_Key);
  inputValues.CADRegionIdsArrayPath = filterArgs.value<DataPath>(k_CADRegionIdsArrayPath_Key);
  inputValues.HatchDataContainerName = filterArgs.value<DataPath>(k_HatchDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.HatchAttributeMatrixName = filterArgs.value<DataObjectNameParameter::ValueType>(k_HatchAttributeMatrixName_Key);
  inputValues.TimeArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_TimeArrayName_Key);
  inputValues.RegionIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_RegionIdsArrayName_Key);
  inputValues.PowersArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PowersArrayName_Key);

  return CreateAMScanPaths(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_StripeWidth_Key = "StripeWidth";
constexpr StringLiteral k_HatchSpacing_Key = "HatchSpacing";
constexpr StringLiteral k_Power_Key = "Power";
constexpr StringLiteral k_Speed_Key = "Speed";
constexpr StringLiteral k_CADSliceDataContainerName_Key = "CADSliceDataContainerName";
constexpr StringLiteral k_CADSliceIdsArrayPath_Key = "CADSliceIdsArrayPath";
constexpr StringLiteral k_CADRegionIdsArrayPath_Key = "CADRegionIdsArrayPath";
constexpr StringLiteral k_HatchDataContainerName_Key = "HatchDataContainerName";
constexpr StringLiteral k_VertexAttributeMatrixName_Key = "VertexAttributeMatrixName";
constexpr StringLiteral k_HatchAttributeMatrixName_Key = "HatchAttributeMatrixName";
constexpr StringLiteral k_TimeArrayName_Key = "TimeArrayName";
constexpr StringLiteral k_RegionIdsArrayName_Key = "RegionIdsArrayName";
constexpr StringLiteral k_PowersArrayName_Key = "PowersArrayName";
} // namespace SIMPL
} // namespace

//------------------------------------------------------------------------------
Result<Arguments> CreateAMScanPathsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = CreateAMScanPathsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_StripeWidth_Key, k_StripeWidth_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_HatchSpacing_Key, k_HatchSpacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_Power_Key, k_Power_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatFilterParameterConverter<float32>>(args, json, SIMPL::k_Speed_Key, k_Speed_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_CADSliceDataContainerName_Key, k_CADSliceDataContainerName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CADSliceIdsArrayPath_Key, k_CADSliceIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CADRegionIdsArrayPath_Key, k_CADRegionIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringToDataPathFilterParameterConverter>(args, json, SIMPL::k_HatchDataContainerName_Key, k_HatchDataContainerName_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_VertexAttributeMatrixName_Key, k_VertexAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_HatchAttributeMatrixName_Key, k_HatchAttributeMatrixName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_TimeArrayName_Key, k_TimeArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_RegionIdsArrayName_Key, k_RegionIdsArrayName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_PowersArrayName_Key, k_PowersArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
