#include "ErodeDilateMaskFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ErodeDilateMask.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/EmptyAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ErodeDilateMaskFilter::name() const
{
  return FilterTraits<ErodeDilateMaskFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateMaskFilter::className() const
{
  return FilterTraits<ErodeDilateMaskFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateMaskFilter::uuid() const
{
  return FilterTraits<ErodeDilateMaskFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateMaskFilter::humanName() const
{
  return "Erode/Dilate Mask";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateMaskFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Erode", "Dilate"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateMaskFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Operation_Key, "Operation", "Whether to dilate (0) or erode (1)", 0ULL, detail::k_OperationChoices));
  params.insert(std::make_unique<Int32Parameter>(k_NumIterations_Key, "Number of Iterations", "Number of erode/dilate iterations to perform", 1));
  params.insert(std::make_unique<BoolParameter>(k_XDirOn_Key, "X Direction", "Whether to erode/dilate in the X direction", true));
  params.insert(std::make_unique<BoolParameter>(k_YDirOn_Key, "Y Direction", "Whether to erode/dilate in the Y direction", true));
  params.insert(std::make_unique<BoolParameter>(k_ZDirOn_Key, "Z Direction", "Whether to erode/dilate in the Z direction", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array Path", "Boolean array where true voxels are used. False voxels are ignored.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ErodeDilateMaskFilter::clone() const
{
  return std::make_unique<ErodeDilateMaskFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateMaskFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  auto pOperationValue = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  auto pNumIterationsValue = filterArgs.value<int32>(k_NumIterations_Key);
  auto pXDirOnValue = filterArgs.value<bool>(k_XDirOn_Key);
  auto pYDirOnValue = filterArgs.value<bool>(k_YDirOn_Key);
  auto pZDirOnValue = filterArgs.value<bool>(k_ZDirOn_Key);
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateMaskFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel) const
{
  ErodeDilateMaskInputValues inputValues;

  inputValues.Operation = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  inputValues.NumIterations = filterArgs.value<int32>(k_NumIterations_Key);
  inputValues.XDirOn = filterArgs.value<bool>(k_XDirOn_Key);
  inputValues.YDirOn = filterArgs.value<bool>(k_YDirOn_Key);
  inputValues.ZDirOn = filterArgs.value<bool>(k_ZDirOn_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ErodeDilateMask(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_DirectionKey = "Direction";
constexpr StringLiteral k_NumIterationsKey = "NumIterations";
constexpr StringLiteral k_XDirOnKey = "XDirOn";
constexpr StringLiteral k_YDirOnKey = "YDirOn";
constexpr StringLiteral k_ZDirOnKey = "ZDirOn";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ErodeDilateMaskFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ErodeDilateMaskFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_DirectionKey, k_Operation_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_NumIterationsKey, k_NumIterations_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_XDirOnKey, k_XDirOn_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_YDirOnKey, k_YDirOn_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_ZDirOnKey, k_ZDirOn_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_SelectedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
