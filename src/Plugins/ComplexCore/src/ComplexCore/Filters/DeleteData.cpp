#include "DeleteData.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"

namespace complex
{
namespace
{
} // namespace

std::string DeleteData::name() const
{
  return FilterTraits<DeleteData>::name;
}

std::string DeleteData::className() const
{
  return FilterTraits<DeleteData>::className;
}

Uuid DeleteData::uuid() const
{
  return FilterTraits<DeleteData>::uuid;
}

std::string DeleteData::humanName() const
{
  return "Delete Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> DeleteData::defaultTags() const
{
  return {"#Core", "#Memory Management", "#Remove Data", "#Delete Data"};
}

Parameters DeleteData::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<DataPathSelectionParameter>(k_DataPath_Key, "DataPath to remove", "The complete path to the DataObject to be removed", DataPath({"DataStructure", "DataObject"})));

  return params;
}

IFilter::UniquePointer DeleteData::clone() const
{
  return std::make_unique<DeleteData>();
}

IFilter::PreflightResult DeleteData::preflightImpl(const DataStructure& dataGraph, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  auto dataObjectPath = args.value<DataPath>(k_DataPath_Key);

  // original
  if(auto iArray = dataGraph.getDataAs<IArray>(dataObjectPath); iArray != nullptr)
  {
    OutputActions arrayActions;
    auto action = std::make_unique<DeleteDataAction>(dataObjectPath, DeleteDataAction::DeleteType::JustObject);
    arrayActions.actions.push_back(std::move(action));
    return {std::move(arrayActions)};
  }

  OutputActions baseGroupActions;

  auto baseGroup = dataGraph.getDataAs<BaseGroup>(dataObjectPath);
  if(baseGroup == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-61501, fmt::format("The object type cannot be processed. Value given is '{}'", dataGraph.getData(dataObjectPath)->getTypeName()))};
  }

  // Remove Start Node and Children Recursively
  // Rules:
  // - If multiparented, any node and all children of that node will are kept
  // - Edges that point to deleted nodes wil be removed
  // - If start object is multiparented recommend edge removal instead
  {
    // check parent count (Base Case)
    if(baseGroup->getParentIds().size() > 1)
    {
      return {
          ConvertResultTo<OutputActions>(MakeWarningVoidResult(-61502, fmt::format("The object cannot be processed because it is multiparented. Consider using the Delete DataPath instead.")), {})};
    }

    // recurse action
    auto action = std::make_unique<DeleteDataAction>(dataObjectPath, DeleteDataAction::DeleteType::IndependentChildren);
    baseGroupActions.actions.push_back(std::move(action));
  }

  return {std::move(baseGroupActions)};
}

Result<> DeleteData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
