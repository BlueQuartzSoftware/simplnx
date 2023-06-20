#include "DeleteDataAction.hpp"

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"

#include <fmt/core.h>

using namespace complex;

namespace
{
constexpr int32 k_TargetNotFoundErrorCode = -3245;
constexpr int32 k_ClassTypeErrorCode = -3246;

Result<> TerminateNode(DataStructure& dataStructure, const DataPath& path, IDataAction::Mode mode)
{
  DataObject* targetObject = dataStructure.getData(path);
  if(targetObject == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{k_TargetNotFoundErrorCode, fmt::format("Trying to delete DataObject '{}' which does not exist.", path.getTargetName())}})};
  }

  dataStructure.removeData(targetObject->getId());

  return {};
}

/* Can be implemented down the line (Also be sure to uncomment Enum and switch options) MAY NEED REVIEW! */
// std::optional<std::vector<DataPath>> GetEveryChildDataPath(DataStructure& dataStructure, const DataPath& parent)
//{
//   std::vector<DataPath> childDataObjects;
//   try
//   {
//     std::vector<std::string> childrenNames;
//     if(parent.empty())
//     {
//       childrenNames = dataStructure.getDataMap().getNames();
//     }
//     else
//     {
//       childrenNames = dataStructure.getDataRefAs<BaseGroup>(parent).GetChildrenNames();
//     }
//
//     for(const auto& childName : childrenNames)
//     {
//       DataPath childPath = parent.createChildPath(childName);
//       const DataObject* dataObject = dataStructure.getData(childPath);
//       childDataObjects.push_back(childPath);
//     }
//   } catch(std::exception& e)
//   {
//     return {};
//   }
//   return {childDataObjects};
// }
//
// Result<> TerminateNodesRecursively(DataStructure& dataStructure, const DataPath& path, IDataAction::Mode mode, const bool checkDependence)
//{
//   Result<> result = {};
//   DataObject* node = dataStructure.getData(path);
//
//   if(checkDependence)
//   {
//     auto parentIds = node->getParentIds();
//     if(parentIds.size() > 1)
//     {
//       return {};
//     }
//   }
//
//   auto optionalChildPaths = GetEveryChildDataPath(dataStructure, path);
//   if(optionalChildPaths.has_value() && optionalChildPaths.value().size() > 0)
//   {
//     for(const auto& child : optionalChildPaths.value())
//     {
//       result = MergeResults(result, std::move(TerminateNodesRecursively(dataStructure, child, mode, checkDependence)));
//     }
//   }
//
//   dataStructure.removeData(node->getId());
//
//   return result;
// }
//
// Result<> TerminateEdge(DataStructure& dataStructure, const DataPath& path, IDataAction::Mode mode)
//{
//   DataObject* targetObject = dataStructure.getData(path);
//   if(targetObject == nullptr)
//   {
//     return {nonstd::make_unexpected(std::vector<Error>{{k_TargetNotFoundErrorCode, fmt::format("Trying to delete DataPath '{}' which does not exist.", path.toString())}})};
//   }
//
//   auto parent = dataStructure.getDataAs<BaseGroup>(path.getParent());
//   parent->remove(targetObject);
//   return {};
// }
} // namespace

namespace complex
{
DeleteDataAction::DeleteDataAction(const DataPath& path, DeleteDataAction::DeleteType type)
: m_Path(path)
, m_Type(type)
{
}

DeleteDataAction::~DeleteDataAction() noexcept = default;

Result<> DeleteDataAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto* targetObject = dataStructure.getDataAs<BaseGroup>(path());
  switch(type())
  {
  case DeleteType::JustObject:
    return TerminateNode(dataStructure, path(), mode);

    /* Can be implemented down the line for better DataStructure handling*/
    // case DeleteType::IndependentChildren:
    //   if(dynamic_cast<BaseGroup*>(targetObject) == nullptr)
    //   {
    //     return {nonstd::make_unexpected(std::vector<Error>{{k_ClassTypeErrorCode, fmt::format("The type of DataObject '{}' is not a subclass of BaseGroup.", path().getTargetName())}})};
    //   }
    //   return TerminateNodesRecursively(dataStructure, path(), mode, true);
    // case DeleteType::AllChildren:
    //   if(dynamic_cast<BaseGroup*>(targetObject) == nullptr)
    //   {
    //     return {nonstd::make_unexpected(std::vector<Error>{{k_ClassTypeErrorCode, fmt::format("The type of DataObject '{}' is not a subclass of BaseGroup.", path().getTargetName())}})};
    //   }
    //   return TerminateNodesRecursively(dataStructure, path(), mode, false);
    // case DeleteType::JustPath:
    //   return TerminateEdge(dataStructure, path(), mode);
  }
  return {};
}

IDataAction::UniquePointer DeleteDataAction::clone() const
{
  return std::make_unique<DeleteDataAction>(m_Path);
}

DataPath DeleteDataAction::path() const
{
  return m_Path;
}

DeleteDataAction::DeleteType DeleteDataAction::type() const
{
  return m_Type;
}
} // namespace complex
