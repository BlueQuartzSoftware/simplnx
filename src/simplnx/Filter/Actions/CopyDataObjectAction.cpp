#include "CopyDataObjectAction.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace nx::core
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
  static constexpr StringLiteral prefix = "CopyDataObjectAction: ";

  if(dataStructure.getData(path()) == nullptr)
  {
    return MakeErrorResult(-5100, fmt::format("{}Cannot find data object at path '{}'", prefix, path().toString()));
  }
  if(dataStructure.getData(newPath()) != nullptr)
  {
    return MakeErrorResult(-5101, fmt::format("{}Data already exists at path '{}'", prefix, newPath().toString()));
  }

  if(copyData(dataStructure, path(), newPath()) == nullptr)
  {
    return MakeErrorResult(-5102, fmt::format("{}Failed to copy data from '{}' to '{}'", prefix, path().toString(), newPath().toString()));
  }

  return {};
}

IDataAction::UniquePointer CopyDataObjectAction::clone() const
{
  return std::make_unique<CopyDataObjectAction>(path(), newPath(), getAllCreatedPaths());
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
} // namespace nx::core
