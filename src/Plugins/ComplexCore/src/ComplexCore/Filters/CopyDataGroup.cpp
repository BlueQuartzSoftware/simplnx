#include "CopyDataGroup.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/CopyGroupAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

namespace complex
{
namespace
{
} // namespace

std::string CopyDataGroup::name() const
{
  return FilterTraits<CopyDataGroup>::name;
}

std::string CopyDataGroup::className() const
{
  return FilterTraits<CopyDataGroup>::className;
}

Uuid CopyDataGroup::uuid() const
{
  return FilterTraits<CopyDataGroup>::uuid;
}

std::string CopyDataGroup::humanName() const
{
  return "Copy Data Group";
}

Parameters CopyDataGroup::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_DataPath_Key, "DataGroup to copy", "DataPath to DataGroup", DataPath{}));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewPath_Key, "Copied DataGroup", "DataPath to new DataGroup", DataPath{}));
  return params;
}

IFilter::UniquePointer CopyDataGroup::clone() const
{
  return std::make_unique<CopyDataGroup>();
}

IFilter::PreflightResult CopyDataGroup::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto dataArrayPath = args.value<DataPath>(k_DataPath_Key);
  auto newDataPath = args.value<DataPath>(k_NewPath_Key);

  std::vector<DataPath> allCreatedPaths = {newDataPath};
  auto pathsToBeCopied = GetAllChildDataPathsRecursive(data, dataArrayPath);
  if(pathsToBeCopied.has_value())
  {
    for(const auto& sourcePath : pathsToBeCopied.value())
    {
      std::string createdPathName = complex::StringUtilities::replace(sourcePath.toString(), dataArrayPath.getTargetName(), newDataPath.getTargetName());
      allCreatedPaths.push_back(DataPath::FromString(createdPathName).value());
    }
  }
  auto action = std::make_unique<CopyGroupAction>(dataArrayPath, newDataPath, allCreatedPaths);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> CopyDataGroup::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
