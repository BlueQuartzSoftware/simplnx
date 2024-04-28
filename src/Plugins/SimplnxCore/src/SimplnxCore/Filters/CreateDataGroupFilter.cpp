#include "CreateDataGroupFilter.hpp"

#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Filter/Actions/CreateDataGroupAction.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
std::string CreateDataGroupFilter::name() const
{
  return FilterTraits<CreateDataGroupFilter>::name;
}

std::string CreateDataGroupFilter::className() const
{
  return FilterTraits<CreateDataGroupFilter>::className;
}

Uuid CreateDataGroupFilter::uuid() const
{
  return FilterTraits<CreateDataGroupFilter>::uuid;
}

std::string CreateDataGroupFilter::humanName() const
{
  return "Create Data Group";
}

//------------------------------------------------------------------------------
std::vector<std::string> CreateDataGroupFilter::defaultTags() const
{
  return {className(), "Core", "Generation", "DataGroup", "Create"};
}

Parameters CreateDataGroupFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataObjectPath, "DataObject Path", "The complete path to the DataObject being created", DataPath{}));
  return params;
}

IFilter::UniquePointer CreateDataGroupFilter::clone() const
{
  return std::make_unique<CreateDataGroupFilter>();
}

IFilter::PreflightResult CreateDataGroupFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel) const
{
  DataPath dataObjectPath = args.value<DataPath>(k_DataObjectPath);

  auto action = std::make_unique<CreateDataGroupAction>(dataObjectPath);

  OutputActions actions;
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

Result<> CreateDataGroupFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
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

Result<Arguments> CreateDataGroupFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args;

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_DataObjectPath));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
