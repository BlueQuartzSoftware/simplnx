#include "RenameDataObject.hpp"

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/DataPathSelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <stdexcept>

using namespace nx::core;

namespace
{
constexpr int32 k_EmptyParameterError = -520;
} // namespace

namespace nx::core
{

//------------------------------------------------------------------------------
std::string RenameDataObject::name() const
{
  return FilterTraits<RenameDataObject>::name;
}

//------------------------------------------------------------------------------
std::string RenameDataObject::className() const
{
  return FilterTraits<RenameDataObject>::className;
}

//------------------------------------------------------------------------------
Uuid RenameDataObject::uuid() const
{
  return FilterTraits<RenameDataObject>::uuid;
}

//------------------------------------------------------------------------------
std::string RenameDataObject::humanName() const
{
  return "Rename DataObject";
}

//------------------------------------------------------------------------------
std::vector<std::string> RenameDataObject::defaultTags() const
{
  return {className(), "Data Management", "Rename", "Data Structure", "Data Object"};
}

//------------------------------------------------------------------------------
Parameters RenameDataObject::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_AllowOverwrite_Key, "Allow Overwrite",
                                                "If true existing object with `New Name` and all of its nested objects will be removed to free up the name for the target DataObject", false));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_SourceDataObjectPath_Key, "DataObject to Rename", "DataPath pointing to the target DataObject", DataPath()));
  params.insert(std::make_unique<StringParameter>(k_NewName_Key, "New Name", "Target DataObject name", ""));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RenameDataObject::clone() const
{
  return std::make_unique<RenameDataObject>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RenameDataObject::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto allowOverwrite = filterArgs.value<bool>(k_AllowOverwrite_Key);
  auto dataGroupPath = filterArgs.value<DataPath>(k_SourceDataObjectPath_Key);
  auto newName = filterArgs.value<std::string>(k_NewName_Key);

  auto action = std::make_unique<RenameDataAction>(dataGroupPath, newName, allowOverwrite);

  OutputActions actions;
  actions.appendAction(std::move(action));
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> RenameDataObject::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedAttributeMatrixPathKey = "SelectedAttributeMatrixPath";
constexpr StringLiteral k_NewAttributeMatrixKey = "NewAttributeMatrix";
constexpr StringLiteral k_SelectedDataContainerNameKey = "SelectedDataContainerName";
constexpr StringLiteral k_NewDataContainerNameKey = "NewDataContainerName";
constexpr StringLiteral k_SelectedArrayPathKey = "SelectedArrayPath";
constexpr StringLiteral k_NewArrayNameKey = "NewArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> RenameDataObject::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RenameDataObject().getDefaultArguments();

  std::vector<Result<>> results;

  if(json.contains(SIMPL::k_SelectedArrayPathKey.str()))
  {
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedArrayPathKey, k_SourceDataObjectPath_Key));
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewArrayNameKey, k_NewName_Key));
  }
  else if(json.contains(SIMPL::k_SelectedAttributeMatrixPathKey))
  {
    results.push_back(
        SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedAttributeMatrixPathKey, k_SourceDataObjectPath_Key));
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewAttributeMatrixKey, k_NewName_Key));
  }
  else
  {
    results.push_back(
        SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedDataContainerNameKey, k_SourceDataObjectPath_Key));
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerNameFilterParameterConverter>(args, json, SIMPL::k_NewDataContainerNameKey, k_NewName_Key));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
