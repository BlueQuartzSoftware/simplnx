#include "CopyGroupAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"

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

std::shared_ptr<DataObject> CopyGroupAction::copyData(DataStructure& dataStructure, const DataPath& sourcePath, const DataPath& destPath)
{
  auto* data = dataStructure.getData(sourcePath);
  std::shared_ptr<DataObject> copy = data->deepCopy(destPath);
  return copy;
}

Result<> CopyGroupAction::apply(DataStructure& dataStructure, Mode mode) const
{
  if(dataStructure.getDataAs<BaseGroup>(path()) == nullptr)
  {
    return MakeErrorResult(-5, fmt::format("Cannot find group at path '{}'", path().toString()));
  }
  if(dataStructure.getData(newPath()) != nullptr)
  {
    return MakeErrorResult(-6, fmt::format("Data already exists at path '{}'", newPath().toString()));
  }

  if(copyData(dataStructure, path(), newPath()) == nullptr)
  {
    return MakeErrorResult(-7, fmt::format("Failed to copy data from '{}' to '{}'", path().toString(), newPath().toString()));
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
