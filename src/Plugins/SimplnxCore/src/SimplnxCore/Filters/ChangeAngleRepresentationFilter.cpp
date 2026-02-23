#include "ChangeAngleRepresentationFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ChangeAngleRepresentation.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ChangeAngleRepresentationFilter::name() const
{
  return FilterTraits<ChangeAngleRepresentationFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ChangeAngleRepresentationFilter::className() const
{
  return FilterTraits<ChangeAngleRepresentationFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ChangeAngleRepresentationFilter::uuid() const
{
  return FilterTraits<ChangeAngleRepresentationFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ChangeAngleRepresentationFilter::humanName() const
{
  return "Convert Angles to Degrees or Radians";
}

//------------------------------------------------------------------------------
std::vector<std::string> ChangeAngleRepresentationFilter::defaultTags() const
{
  return {className(), "Processing", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters ChangeAngleRepresentationFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_ConversionType_Key, "Conversion Type", "Tells the Filter which conversion is being made", 0,
                                                   ChoicesParameter::Choices{"Degrees to Radians", "Radians to Degrees"}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_AnglesArrayPath_Key, "Angles", "The DataArray containing the angles to be converted.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ChangeAngleRepresentationFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ChangeAngleRepresentationFilter::clone() const
{
  return std::make_unique<ChangeAngleRepresentationFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ChangeAngleRepresentationFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pAnglesDataPathValue = filterArgs.value<DataPath>(k_AnglesArrayPath_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  nx::core::MarkDataPathModified(dataStructure, resultOutputActions, pAnglesDataPathValue);

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ChangeAngleRepresentationFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                      const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ChangeAngleRepresentationInputValues inputValues;
  inputValues.ConversionTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);
  inputValues.AnglesArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_AnglesArrayPath_Key);

  return ChangeAngleRepresentation(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_ConversionTypeKey = "ConversionType";
constexpr StringLiteral k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ChangeAngleRepresentationFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ChangeAngleRepresentationFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_ConversionTypeKey, k_ConversionType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellEulerAnglesArrayPathKey, k_AnglesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
