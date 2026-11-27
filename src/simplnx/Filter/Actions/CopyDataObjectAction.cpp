#include "CopyDataObjectAction.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <fmt/core.h>

#include <utility>

using namespace nx::core;

namespace nx::core
{
CopyDataObjectAction::CopyDataObjectAction(DataPath path, const DataPath& newPath, const std::vector<DataPath>& allCreatedPaths)
: IDataCreationAction(newPath)
, m_Path(std::move(path))
, m_NewPath(newPath)
, m_AllCreatedPaths(allCreatedPaths)
{
}

CopyDataObjectAction::~CopyDataObjectAction() noexcept = default;

std::shared_ptr<DataObject> CopyDataObjectAction::CopyData(DataStructure& dataStructure, const DataPath& sourcePath, const DataPath& destPath)
{
  auto* dataPtr = dataStructure.getData(sourcePath);
  std::shared_ptr<DataObject> copy = dataPtr->deepCopy(destPath);
  return copy;
}

Result<> CopyDataObjectAction::apply(DataStructure& dataStructure, Mode /*mode*/) const
{
  static constexpr StringLiteralType k_Prefix = "CopyDataObjectAction: ";

  if(dataStructure.getData(path()) == nullptr)
  {
    return MakeErrorResult(-5100, fmt::format("{}Cannot find data object at path '{}'", k_Prefix, path().toString()));
  }
  if(dataStructure.getData(newPath()) != nullptr)
  {
    return MakeErrorResult(-5101, fmt::format("{}Data already exists at path '{}'", k_Prefix, newPath().toString()));
  }

  if(CopyData(dataStructure, path(), newPath()) == nullptr)
  {
    return MakeErrorResult(-5102, fmt::format("{}Failed to copy data from '{}' to '{}'", k_Prefix, path().toString(), newPath().toString()));
  }

  return {};
}

IDataAction::UniquePointerType CopyDataObjectAction::clone() const
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
