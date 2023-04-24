#include "CopyDataObjectFilter.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Filter/Actions/CopyDataObjectAction.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiPathSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

namespace complex
{

//------------------------------------------------------------------------------
std::string CopyDataObjectFilter::name() const
{
  return FilterTraits<CopyDataObjectFilter>::name;
}

//------------------------------------------------------------------------------
std::string CopyDataObjectFilter::className() const
{
  return FilterTraits<CopyDataObjectFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CopyDataObjectFilter::uuid() const
{
  return FilterTraits<CopyDataObjectFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CopyDataObjectFilter::humanName() const
{
  return "Copy Data Object";
}

//------------------------------------------------------------------------------
std::vector<std::string> CopyDataObjectFilter::defaultTags() const
{
  return {"Copy", "Data Management", "Memory Management", "Data Structure", "Duplicate"};
}

//------------------------------------------------------------------------------
Parameters CopyDataObjectFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<MultiPathSelectionParameter>(k_DataPath_Key, "Objects to copy", "A list of DataPaths to the DataObjects to be copied", MultiPathSelectionParameter::ValueType{}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseNewParent_Key, "Copy to New Parent", "Copy all the DataObjects to a new BaseGroup", false));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_NewPath_Key, "Copied Parent Group", "DataPath to parent BaseGroup in which to store the copied DataObject(s)", DataPath{},
                                                              BaseGroup::GetAllGroupTypes()));
  params.insert(std::make_unique<StringParameter>(k_NewPathSuffix_Key, "Copied Object(s) Suffix", "Suffix string to be appended to each copied DataObject", "_COPY"));

  params.linkParameters(k_UseNewParent_Key, k_NewPath_Key, true);
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer CopyDataObjectFilter::clone() const
{
  return std::make_unique<CopyDataObjectFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CopyDataObjectFilter::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto dataArrayPaths = args.value<MultiPathSelectionParameter::ValueType>(k_DataPath_Key);
  auto useNewParent = args.value<bool>(k_UseNewParent_Key);
  auto suffix = args.value<std::string>(k_NewPathSuffix_Key);

  if(!useNewParent && suffix.empty())
  {
    return {MakeErrorResult<OutputActions>(-27360, "Copied Object(s) have the same parent as original data but the suffix is empty."), {}};
  }

  OutputActions actions;

  for(const auto& dataArrayPath : dataArrayPaths)
  {
    DataPath parentPath = dataArrayPath.getParent();
    if(useNewParent)
    {
      parentPath = args.value<DataPath>(k_NewPath_Key);
    }
    std::string newTargetName = dataArrayPath.getTargetName() + suffix;
    DataPath newDataPath = parentPath.createChildPath(newTargetName);

    std::vector<DataPath> allCreatedPaths = {newDataPath};
    if(data.getDataAs<BaseGroup>(dataArrayPath) != nullptr)
    {
      const auto pathsToBeCopied = GetAllChildDataPathsRecursive(data, dataArrayPath);
      if(pathsToBeCopied.has_value())
      {
        for(const auto& sourcePath : pathsToBeCopied.value())
        {
          std::string createdPathName = complex::StringUtilities::replace(sourcePath.toString(), dataArrayPath.getTargetName(), newTargetName);
          allCreatedPaths.push_back(DataPath::FromString(createdPathName).value());
        }
      }
    }
    auto action = std::make_unique<CopyDataObjectAction>(dataArrayPath, newDataPath, allCreatedPaths);
    actions.appendAction(std::move(action));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> CopyDataObjectFilter::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
