#include "MoveData.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Filter/Actions/MoveDataAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"

namespace complex
{
namespace
{
constexpr int64 k_TupleCountInvalidError = -250;
constexpr int64 k_MissingFeaturePhasesError = -251;

} // namespace

std::string MoveData::name() const
{
  return FilterTraits<MoveData>::name;
}

std::string MoveData::className() const
{
  return FilterTraits<MoveData>::className;
}

Uuid MoveData::uuid() const
{
  return FilterTraits<MoveData>::uuid;
}

std::string MoveData::humanName() const
{
  return "Move Data";
}

Parameters MoveData::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<DataPathSelectionParameter>(k_Data_Key, "Data to Move", "", DataPath()));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_NewParent_Key, "New Parent", "", DataPath()));
  return params;
}

IFilter::UniquePointer MoveData::clone() const
{
  return std::make_unique<MoveData>();
}

IFilter::PreflightResult MoveData::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto dataPath = args.value<DataPath>(k_Data_Key);
  auto newParentPath = args.value<DataPath>(k_NewParent_Key);

  auto moveDataAction = std::make_unique<MoveDataAction>(dataPath, newParentPath);

  OutputActions actions;
  actions.actions.push_back(std::move(moveDataAction));
  return {std::move(actions)};
}

Result<> MoveData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
