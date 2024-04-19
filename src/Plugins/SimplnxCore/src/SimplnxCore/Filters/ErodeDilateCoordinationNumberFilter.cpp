#include "ErodeDilateCoordinationNumberFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ErodeDilateCoordinationNumber.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::name() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::className() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateCoordinationNumberFilter::uuid() const
{
  return FilterTraits<ErodeDilateCoordinationNumberFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateCoordinationNumberFilter::humanName() const
{
  return "Erode/Dilate Coordination Number";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateCoordinationNumberFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Erode", "Dilate", "Smooth Bad Data"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateCoordinationNumberFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32Parameter>(k_CoordinationNumber_Key, "Coordination Number to Consider",
                                                 " Number of neighboring **Cells** that can be of opposite classification before a **Cell** will be removed", 6));
  params.insert(std::make_unique<BoolParameter>(k_Loop_Key, "Loop Until Gone", "Keep looping until all criteria is met", false));

  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Cell belongs", DataPath({"FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when performing the algorithm",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypes{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErodeDilateCoordinationNumberFilter::clone() const
{
  return std::make_unique<ErodeDilateCoordinationNumberFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateCoordinationNumberFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                            const std::atomic_bool& shouldCancel) const
{
  auto pCoordinationNumberValue = filterArgs.value<int32>(k_CoordinationNumber_Key);
  auto pLoopValue = filterArgs.value<bool>(k_Loop_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pIgnoredDataArrayPathsValue = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);

  if(pCoordinationNumberValue < 0 || pCoordinationNumberValue > 6)
  {
    MakeErrorResult(-16800, fmt::format("Coordination Number must be between 0 and 6. Current Value: {}", pCoordinationNumberValue));
  }

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumberFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  ErodeDilateCoordinationNumberInputValues inputValues;

  inputValues.CoordinationNumber = filterArgs.value<int32>(k_CoordinationNumber_Key);
  inputValues.Loop = filterArgs.value<bool>(k_Loop_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.IgnoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ErodeDilateCoordinationNumber(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CoordinationNumberKey = "CoordinationNumber";
constexpr StringLiteral k_LoopKey = "Loop";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace SIMPL
} // namespace

Result<Arguments> ErodeDilateCoordinationNumberFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ErodeDilateCoordinationNumberFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_CoordinationNumberKey, k_CoordinationNumber_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_LoopKey, k_Loop_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredDataArrayPaths_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
