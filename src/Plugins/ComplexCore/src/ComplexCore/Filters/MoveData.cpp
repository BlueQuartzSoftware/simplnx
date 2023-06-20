#include "MoveData.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Filter/Actions/MoveDataAction.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiPathSelectionParameter.hpp"

namespace complex
{
namespace
{
constexpr int64 k_TupleCountInvalidError = -250;
constexpr int64 k_MissingFeaturePhasesError = -251;

} // namespace

//------------------------------------------------------------------------------
std::string MoveData::name() const
{
  return FilterTraits<MoveData>::name;
}

//------------------------------------------------------------------------------
std::string MoveData::className() const
{
  return FilterTraits<MoveData>::className;
}

//------------------------------------------------------------------------------
Uuid MoveData::uuid() const
{
  return FilterTraits<MoveData>::uuid;
}

//------------------------------------------------------------------------------
std::string MoveData::humanName() const
{
  return "Move Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> MoveData::defaultTags() const
{
  return {"Move", "Memory Management", "Data Management", "Data Structure"};
}

//------------------------------------------------------------------------------
Parameters MoveData::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<MultiPathSelectionParameter>(k_Data_Key, "Data to Move", "The complete paths to the data object(s) to be moved", MultiPathSelectionParameter::ValueType{}));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_NewParent_Key, "New Parent", "The complete path to the parent data object to which the data will be moved", DataPath(),
                                                              BaseGroup::GetAllGroupTypes()));
  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer MoveData::clone() const
{
  return std::make_unique<MoveData>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult MoveData::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto dataPaths = args.value<MultiPathSelectionParameter::ValueType>(k_Data_Key);
  auto newParentPath = args.value<DataPath>(k_NewParent_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  for(const auto& dataPath : dataPaths)
  {
    auto moveDataAction = std::make_unique<MoveDataAction>(dataPath, newParentPath);
    resultOutputActions.value().appendAction(std::move(moveDataAction));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> MoveData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
