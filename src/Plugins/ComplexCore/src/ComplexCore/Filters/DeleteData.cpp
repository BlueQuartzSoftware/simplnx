#include "DeleteData.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiPathSelectionParameter.hpp"

/**The commented out code in this filter and the header is set up to offer deeper control
 * of the dataStructure. The current state of the dataStructure abstracts the underlying data graph
 * into a heirarchical structure, thus the advanced handling and data management is not necessary.
 * Should the code be uncommented one should review the test cases and round out the development.
 * The DeleteDataAction also implements code that has been commented out so it will need review as
 * well.
 */

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
  return {"Core", "Memory Management", "Remove Data", "Delete Data"};
}

Parameters DeleteData::parameters() const
{
  Parameters params;

  // params.insertSeparator(Parameters::Separator{"Required Parameters"});
  // params.insert(std::make_unique<ChoicesParameter>(k_DeletionType_Key, "Deletion Type", "", to_underlying(DeletionType::DeleteDataPath),
  //                                                  ChoicesParameter::Choices{"Delete DataObject", "Delete Path to DataObject", "Delete DataObject and Unshared Child Objects",
  //                                                                            "Delete DataObject and All Child Objects"})); // sequence dependent DO NOT REORDER

  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
  params.insert(std::make_unique<MultiPathSelectionParameter>(k_DataPath_Key, "DataPaths to remove", "The complete path to the DataObjects to be removed", MultiPathSelectionParameter::ValueType{}));

  return params;
}

IFilter::UniquePointer DeleteData::clone() const
{
  return std::make_unique<DeleteData>();
}

IFilter::PreflightResult DeleteData::preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  // auto deletionType = static_cast<DeletionType>(args.value<ChoicesParameter::ValueType>(k_DeletionType_Key));
  const auto dataObjectPaths = args.value<MultiPathSelectionParameter::ValueType>(k_DataPath_Key);

  OutputActions deleteActions;
  for(const auto& dataObjectPath : dataObjectPaths)
  {
    const auto* dataObject = dataStructure.getDataAs<DataObject>(dataObjectPath);

    // switch(deletionType)
    //{
    // case DeletionType::DeleteDataObject: {
    //   // original
    //   // Remove Singular Node
    //   // Rules:
    //   // - If multiparented, throw warning of effects and recommend DataPath removal instead
    //   // - Edges that point to node will all be deleted
    //   // - Node will be deleted
    //  - IN CURRENT STATE BEHAVES MORE LIKE REMOVE START NODE AND ALL CHILDREN

    if(dataObject->getParentIds().size() > 1)
    {
      return {ConvertResultTo<OutputActions>(
          MakeWarningVoidResult(-61501, fmt::format("The object is multiparented, deleting this data could affect other unintended nodes. Consider using the Delete DataPath instead.")), {})};
    }

    auto action = std::make_unique<DeleteDataAction>(dataObjectPath, DeleteDataAction::DeleteType::JustObject);
    deleteActions.appendAction(std::move(action));

    //  break;
    //}

    // case DeletionType::DeleteDataPath: {
    //   // Remove Edge (DataPath)
    //   // Rules:
    //   // - Neither of the nodes (parent or child) should be deleted
    //   // - The edge between the nodes should be removed from both the respective parent and child list

    //  // check parent that its not a top level path [ie no parent] (can't be removed)
    //  if(dataObjectPath.getLength() < 2)
    //  {
    //    return {MakeErrorResult<OutputActions>(
    //        -61502, fmt::format("The DataPath [{}] cannot be processed because it is a top level datapath. Consider using an DataObject removal instead.", dataObjectPath.toString()))};
    //  }

    //  auto action = std::make_unique<DeleteDataAction>(dataObjectPath, DeleteDataAction::DeleteType::JustPath);
    //  deleteActions.appendAction(std::move(action));

    //  break;
    //}

    // case DeletionType::DeleteUnsharedChildren: {
    //   // Remove Start Node and Children Recursively (Independent Removal Only)
    //   // Rules:
    //   // - If multiparented, any node and all children of that node will are kept
    //   // - Edges that point to deleted nodes wil be removed
    //   // - If start object is multiparented recommend edge removal instead

    //  // check parent count (Base Case)
    //  if(dataObject->getParentIds().size() > 1)
    //  {
    //    return {
    //        ConvertResultTo<OutputActions>(MakeWarningVoidResult(-61503, fmt::format("The object cannot be processed because it is multiparented. Consider using the Delete DataPath instead.")),
    //        {})};
    //  }
    //  else
    //  {
    //    const auto* baseGroup = dynamic_cast<const BaseGroup*>(dataObject);
    //    if(baseGroup == nullptr) // Object is the lowest level in its path
    //    {
    //      auto action = std::make_unique<DeleteDataAction>(dataObjectPath, DeleteDataAction::DeleteType::JustObject);
    //      deleteActions.appendAction(std::move(action));
    //      return {ConvertResultTo<OutputActions>(
    //          MakeWarningVoidResult(-61504, fmt::format("The object type cannot be processed because no children exist for it. Object given is of type '{}', consider remove ",
    //                                                    dataStructure.getData(dataObjectPath)->getTypeName())),
    //          {})};
    //    }
    //    else
    //    {
    //      // recurse action
    //      auto action = std::make_unique<DeleteDataAction>(dataObjectPath, DeleteDataAction::DeleteType::IndependentChildren);
    //      deleteActions.appendAction(std::move(action));
    //    }
    //  }

    //  break;
    //}

    // case DeletionType::DeleteChildren: {
    //   // Remove Start Node and Children Recursively (Complete Removal)
    //   // Rules:
    //   // - All children will be removed regardless of parent count
    //   // - Edges that point to deleted nodes wil be removed
    //   // - Warn of absolute deletion of data and effects

    //  // This needs to be implemented down the line
    //  // ConvertResultTo<OutputActions>(MakeWarningVoidResult(-61505, fmt::format("This action will remove the underlying data which could affect other structures if it is multiparented")), {});

    //  const auto* baseGroup = dynamic_cast<const BaseGroup*>(dataObject);
    //  if(baseGroup == nullptr) // Object is the lowest level in its path
    //  {
    //    auto action = std::make_unique<DeleteDataAction>(dataObjectPath, DeleteDataAction::DeleteType::JustObject);
    //    deleteActions.appendAction(std::move(action));
    //    return {ConvertResultTo<OutputActions>(
    //        MakeWarningVoidResult(-61505, fmt::format("The object type cannot be processed because no children exist for it. Object given is of type '{}', consider remove ",
    //                                                  dataStructure.getData(dataObjectPath)->getTypeName())),
    //        {})};
    //  }
    //  else
    //  {
    //    // recurse action
    //    auto action = std::make_unique<DeleteDataAction>(dataObjectPath, DeleteDataAction::DeleteType::AllChildren);
    //    deleteActions.appendAction(std::move(action));
    //  }

    //  break;
    //}
    //}
  }

  return {std::move(deleteActions)};
}

Result<> DeleteData::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const
{
  return {};
}
} // namespace complex
