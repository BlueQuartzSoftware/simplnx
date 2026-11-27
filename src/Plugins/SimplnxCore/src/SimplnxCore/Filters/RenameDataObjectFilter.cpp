#include "RenameDataObjectFilter.hpp"

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

#include <stdexcept>

using namespace nx::core;

namespace
{
constexpr int32 k_InvalidNewName = -27350;
} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string RenameDataObjectFilter::name() const
{
  return FilterTraits<RenameDataObjectFilter>::name;
}

//------------------------------------------------------------------------------
std::string RenameDataObjectFilter::className() const
{
  return FilterTraits<RenameDataObjectFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RenameDataObjectFilter::uuid() const
{
  return FilterTraits<RenameDataObjectFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RenameDataObjectFilter::humanName() const
{
  return "Rename DataObject";
}

//------------------------------------------------------------------------------
std::vector<std::string> RenameDataObjectFilter::defaultTags() const
{
  return {className(), "Data Management", "Rename", "Data Structure", "Data Object"};
}

//------------------------------------------------------------------------------
Parameters RenameDataObjectFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_AllowOverwrite_Key, "Allow Overwrite",
                                                "Replace the object at New Name under the selected parent. Other placements and their children are kept. Objects left with no placement are deleted.",
                                                false));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_SourceDataObjectPath_Key, "DataObject to Rename", "DataPath pointing to the target DataObject", DataPath()));
  params.insert(std::make_unique<StringParameter>(k_NewName_Key, "New Name", "Target DataObject name", ""));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RenameDataObjectFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType RenameDataObjectFilter::clone() const
{
  return std::make_unique<RenameDataObjectFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RenameDataObjectFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                               const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto allowOverwrite = filterArgs.value<bool>(k_AllowOverwrite_Key);
  auto dataGroupPath = filterArgs.value<DataPath>(k_SourceDataObjectPath_Key);
  auto newName = filterArgs.value<std::string>(k_NewName_Key);
  if(!DataObject::IsValidName(newName))
  {
    return {.output_actions =
                MakeErrorResult<OutputActions>(k_InvalidNewName, fmt::format("New Name '{}' is invalid for source '{}'. Enter a non-empty name without '/'.", newName, dataGroupPath.toString()))};
  }

  auto action = std::make_unique<RenameDataAction>(dataGroupPath, newName, allowOverwrite);

  OutputActions actions;
  actions.appendAction(std::move(action));
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RenameDataObjectFilter::executeImpl(DataStructure& /*dataStructure*/, const Arguments& /*filterArgs*/, const PipelineFilter* /*pipelineNode*/, const MessageHandler& /*messageHandler*/,
                                             const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  return {};
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_SelectedAttributeMatrixPathKey = "SelectedAttributeMatrixPath";
constexpr StringLiteralType k_NewAttributeMatrixKey = "NewAttributeMatrix";
constexpr StringLiteralType k_SelectedDataContainerNameKey = "SelectedDataContainerName";
constexpr StringLiteralType k_NewDataContainerNameKey = "NewDataContainerName";
constexpr StringLiteralType k_SelectedArrayPathKey = "SelectedArrayPath";
constexpr StringLiteralType k_NewArrayNameKey = "NewArrayName";
} // namespace simpl
} // namespace

Result<Arguments> RenameDataObjectFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RenameDataObjectFilter().getDefaultArguments();

  std::vector<Result<>> results;

  if(json.contains(simpl::k_SelectedArrayPathKey.str()))
  {
    results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedArrayPathKey, k_SourceDataObjectPath_Key));
    results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_NewArrayNameKey, k_NewName_Key));
  }
  else if(json.contains(simpl::k_SelectedAttributeMatrixPathKey))
  {
    results.push_back(
        simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_SelectedAttributeMatrixPathKey, k_SourceDataObjectPath_Key));
    results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_NewAttributeMatrixKey, k_NewName_Key));
  }
  else
  {
    results.push_back(
        simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_SelectedDataContainerNameKey, k_SourceDataObjectPath_Key));
    results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerNameFilterParameterConverter>(args, json, simpl::k_NewDataContainerNameKey, k_NewName_Key));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
