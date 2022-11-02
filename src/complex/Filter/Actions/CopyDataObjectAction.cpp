#include "CopyDataObjectAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"

using namespace complex;

namespace complex
{
CopyDataObjectAction::CopyDataObjectAction(const DataPath& path, const DataPath& newPath, const std::vector<DataPath> allCreatedPaths)
: IDataCreationAction(newPath)
, m_Path(path)
, m_NewPath(newPath)
, m_AllCreatedPaths(allCreatedPaths)
{
}

CopyDataObjectAction::~CopyDataObjectAction() noexcept = default;

std::shared_ptr<DataObject> CopyDataObjectAction::copyData(DataStructure& dataStructure, const DataPath& sourcePath, const DataPath& destPath)
{
  auto* data = dataStructure.getData(sourcePath);
  std::shared_ptr<DataObject> copy = data->deepCopy(destPath);
  return copy;
}

Result<> CopyDataObjectAction::apply(DataStructure& dataStructure, Mode mode) const
{
  if(dataStructure.getData(path()) == nullptr)
  {
    return MakeErrorResult(-5, fmt::format("Cannot find data object at path '{}'", path().toString()));
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

const DataPath& CopyDataObjectAction::path() const
{
  return m_Path;
}

const DataPath& CopyDataObjectAction::newPath() const
{
  return m_NewPath;
}

std::vector<DataPath> CopyDataObjectAction::getAllCreatedPaths() const
{
  return m_AllCreatedPaths;
}
} // namespace complex
