#include "RenameDataGroup.hpp"

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/Actions/RenameDataAction.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include <stdexcept>

using namespace complex;

namespace
{
constexpr int32 k_EmptyParameterError = -520;

} // namespace

namespace complex
{
std::string RenameDataGroup::name() const
{
  return FilterTraits<RenameDataGroup>::name;
}

std::string RenameDataGroup::className() const
{
  return FilterTraits<RenameDataGroup>::className;
}

Uuid RenameDataGroup::uuid() const
{
  return FilterTraits<RenameDataGroup>::uuid;
}

std::string RenameDataGroup::humanName() const
{
  return "Rename DataGroup";
}

Parameters RenameDataGroup::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataPathSelectionParameter>(k_DataGroup_Key, "DataGroup to Rename", "DataPath pointing to the target DataGroup", DataPath()));
  params.insert(std::make_unique<StringParameter>(k_NewName_Key, "New DataGroup Name", "Target DataGroup rename", ""));
  return params;
}

IFilter::UniquePointer RenameDataGroup::clone() const
{
  return std::make_unique<RenameDataGroup>();
}

IFilter::PreflightResult RenameDataGroup::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel) const
{
  auto dataGroupPath = filterArgs.value<DataPath>(k_DataGroup_Key);
  auto newName = filterArgs.value<std::string>(k_NewName_Key);

  auto* group = dataStructure.getDataAs<BaseGroup>(dataGroupPath);
  if(group == nullptr)
  {
    std::string ss = fmt::format("Target DataObject at path '{}' is not derived from BaseGroup", dataGroupPath.toString());
    return {MakeErrorResult<OutputActions>(-100, ss)};
  }

  auto action = std::make_unique<RenameDataAction>(dataGroupPath, newName);

  OutputActions actions;
  actions.actions.push_back(std::move(action));
  return {std::move(actions)};
}

Result<> RenameDataGroup::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
