#include "MoveData.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Filter/Actions/MoveDataAction.hpp"
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

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_Data_Key, "Data to Move", "The complete path to the data object to be moved", DataPath()));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_NewParent_Key, "New Parent", "The complete path to the parent data object to which the data will be moved", DataPath()));
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

  const auto* parentPathGroup = data.getDataAs<BaseGroup>(newParentPath);
  if(parentPathGroup == nullptr)
  {
    std::string ss = fmt::format("New parent path {} is not of the correct type.  Must select an attribute matrix, data group, or geometry as the new parent", newParentPath.toString());
    return {nonstd::make_unexpected(std::vector<Error>{Error{-6540, ss}})};
  }

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
