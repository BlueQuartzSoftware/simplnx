#include "CopyDataObjectFilter.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Filter/Actions/CopyDataObjectAction.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

namespace complex
{
namespace
{
} // namespace

std::string CopyDataObjectFilter::name() const
{
  return FilterTraits<CopyDataObjectFilter>::name;
}

std::string CopyDataObjectFilter::className() const
{
  return FilterTraits<CopyDataObjectFilter>::className;
}

Uuid CopyDataObjectFilter::uuid() const
{
  return FilterTraits<CopyDataObjectFilter>::uuid;
}

std::string CopyDataObjectFilter::humanName() const
{
  return "Copy Data Object";
}

Parameters CopyDataObjectFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_DataPath_Key, "Object to copy", "DataPath to the DataObject to be copied", DataPath{}));

  params.insertSeparator(Parameters::Separator{"Created Output Data Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_NewPath_Key, "Copied Group", "DataPath to new BaseGroup", DataPath{}));
  return params;
}

IFilter::UniquePointer CopyDataObjectFilter::clone() const
{
  return std::make_unique<CopyDataObjectFilter>();
}

IFilter::PreflightResult CopyDataObjectFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto dataArrayPath = args.value<DataPath>(k_DataPath_Key);
  auto newDataPath = args.value<DataPath>(k_NewPath_Key);

  std::vector<DataPath> allCreatedPaths = {newDataPath};
  if(data.getDataAs<BaseGroup>(dataArrayPath) != nullptr)
  {
    const auto pathsToBeCopied = GetAllChildDataPathsRecursive(data, dataArrayPath);
    if(pathsToBeCopied.has_value())
    {
      for(const auto& sourcePath : pathsToBeCopied.value())
      {
        std::string createdPathName = complex::StringUtilities::replace(sourcePath.toString(), dataArrayPath.getTargetName(), newDataPath.getTargetName());
        allCreatedPaths.push_back(DataPath::FromString(createdPathName).value());
      }
    }
  }
  auto action = std::make_unique<CopyDataObjectAction>(dataArrayPath, newDataPath, allCreatedPaths);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> CopyDataObjectFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
