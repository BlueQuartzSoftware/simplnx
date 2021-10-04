#include "CreateDataGroup.hpp"

#include "complex/Core/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Core/Parameters/StringParameter.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"

namespace complex
{
std::string CreateDataGroup::name() const
{
  return FilterTraits<CreateDataGroup>::name.str();
}

Uuid CreateDataGroup::uuid() const
{
  return FilterTraits<CreateDataGroup>::uuid;
}

std::string CreateDataGroup::humanName() const
{
  return "CreateDataGroup";
}

Parameters CreateDataGroup::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataObjectPath.str(), "DataObject Path", "The complete path to the DataObject being created", DataPath{}));
  return params;
}

IFilter::UniquePointer CreateDataGroup::clone() const
{
  return std::make_unique<CreateDataGroup>();
}

Result<OutputActions> CreateDataGroup::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
{
  DataPath dataObjectPath = args.value<DataPath>(k_DataObjectPath.view());

  auto action = std::make_unique<CreateDataGroupAction>(dataObjectPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> CreateDataGroup::executeImpl(DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler) const
{
  return {};
}
} // namespace complex
