#include "CreateDataGroup.hpp"

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Filter/Actions/CreateDataGroupAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
std::string CreateDataGroup::name() const
{
  return FilterTraits<CreateDataGroup>::name;
}

std::string CreateDataGroup::className() const
{
  return FilterTraits<CreateDataGroup>::className;
}

Uuid CreateDataGroup::uuid() const
{
  return FilterTraits<CreateDataGroup>::uuid;
}

std::string CreateDataGroup::humanName() const
{
  return "Create Data Group";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateDataGroup::defaultTags() const
{
  return {className(), "Core", "Generation", "DataGroup", "Create"};
}

Parameters CreateDataGroup::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataObjectPath, "DataObject Path", "The complete path to the DataObject being created", DataPath{}));
  return params;
}

IFilter::UniquePointer CreateDataGroup::clone() const
{
  return std::make_unique<CreateDataGroup>();
}

IFilter::PreflightResult CreateDataGroup::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  DataPath dataObjectPath = args.value<DataPath>(k_DataObjectPath);

  auto action = std::make_unique<CreateDataGroupAction>(dataObjectPath);

  OutputActions actions;
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

Result<> CreateDataGroup::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                      const std::atomic_bool& shouldCancel) const
{
  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
} // namespace SIMPL
} // namespace

Result<Arguments> CreateDataGroup::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args;

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_DataObjectPath));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
