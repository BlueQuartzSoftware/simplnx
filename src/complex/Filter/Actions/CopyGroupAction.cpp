#include "CopyGroupAction.hpp"

#include <fmt/core.h>

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

using namespace complex;

namespace complex
{
CopyGroupAction::CopyGroupAction(const DataPath& path, const DataPath& newPath, const std::vector<DataPath> allCreatedPaths)
: IDataCreationAction(newPath)
, m_Path(path)
, m_NewPath(newPath)
, m_AllCreatedPaths(allCreatedPaths)
{
}

CopyGroupAction::~CopyGroupAction() noexcept = default;

std::shared_ptr<DataObject> CopyGroupAction::copyData(DataStructure& dataStructure, const DataPath& targetPath, const DataPath& copyPath) const
{
  auto* data = dataStructure.getData(targetPath);

  std::shared_ptr<DataObject> copy = std::shared_ptr<DataObject>(data->deepCopy());
  // copy->rename(newPath().getTargetName());
  // dataStructure.insert(copy, newPath());
  copy->rename(copyPath.getTargetName());
  dataStructure.insert(copy, copyPath.getParent());

  if(auto* groupData = dynamic_cast<BaseGroup*>(data))
  {
    for(const auto& [id, ptr] : *groupData)
    {
      if(ptr == nullptr)
      {
        continue;
      }

      std::string childName = ptr->getName();
      auto childPath = targetPath.createChildPath(childName);
      auto childCopyPath = copyPath.createChildPath(childName);

      auto childData = copyData(dataStructure, childPath, childCopyPath);
      dataStructure.insert(childData, childCopyPath);
    }
  }

  return copy;
}

Result<> CopyGroupAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto* baseGroup = dataStructure.getDataAs<BaseGroup>(path());
  if(baseGroup == nullptr)
  {
    std::string ss = fmt::format("Cannot find group at path '{}'", path().toString());
    return MakeErrorResult(-5, ss);
  }
  auto* newTargetData = dataStructure.getData(newPath());
  if(newTargetData != nullptr)
  {
    std::string ss = fmt::format("Data already exists at path '{}'", newPath().toString());
    return MakeErrorResult(-6, ss);
  }

  auto newData = copyData(dataStructure, path(), newPath());
  if(newData == nullptr)
  {
    std::string ss = fmt::format("Failed to copy data from '{}' to '{}'", path().toString(), newPath().toString());
    return MakeErrorResult(-7, ss);
  }

  return {};
}

const DataPath& CopyGroupAction::path() const
{
  return m_Path;
}

const DataPath& CopyGroupAction::newPath() const
{
  return m_NewPath;
}

std::vector<DataPath> CopyGroupAction::getAllCreatedPaths() const
{
  return m_AllCreatedPaths;
}
} // namespace complex
