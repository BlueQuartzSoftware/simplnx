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

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataObjectPath, "DataObject Path", "The complete path to the DataObject being created", DataPath{}));
  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CreateDataGroupFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer CreateDataGroupFilter::clone() const
{
  return std::make_unique<CreateDataGroupFilter>();
}

IFilter::PreflightResult CreateDataGroupFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                              const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  DataPath dataObjectPath = filterArgs.value<DataPath>(k_DataObjectPath);

  auto action = std::make_unique<CreateDataGroupAction>(dataObjectPath);

  OutputActions actions;
  actions.appendAction(std::move(action));

  return {std::move(actions)};
}

Result<> CreateDataGroupFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  return {};
}

namespace
{
namespace SIMPL
{
// 6.5
constexpr StringLiteral k_CreatedDataContainerKey = "CreatedDataContainer";
// 6.6
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
} // namespace SIMPL
} // namespace

Result<Arguments> CreateDataGroupFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args;

  std::vector<Result<>> results;

  Result<> dcResult = SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_DataObjectPath);
  if(dcResult.invalid())
  {
    // Key renamed at some point (some 6.5 pipelines have this key, others have new one)
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_CreatedDataContainerKey, k_DataObjectPath));
  }
  else
  {
    results.push_back(std::move(dcResult));
  }

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
