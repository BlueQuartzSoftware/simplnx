#include "RenameDataAction.hpp"

#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/core.h>

using namespace nx::core;

namespace
{
Result<> TerminateNodesRecursively(DataStructure& dataStructure, DataObject::IdType id, IDataAction::Mode mode, const bool checkDependence)
{
  Result<> result = {};
  DataObject* node = dataStructure.getData(id);

  if(checkDependence)
  {
    auto parentIds = node->getParentIds();
    if(parentIds.size() > 1)
    {
      return {};
    }
  }

  auto childIds = dataStructure.getDataRefAs<BaseGroup>(id).GetChildrenIds();
  if(!childIds.empty())
  {
    for(const auto& childId : childIds)
    {
      result = MergeResults(result, std::move(TerminateNodesRecursively(dataStructure, childId, mode, checkDependence)));
    }
  }

  dataStructure.removeData(node->getId());

  return result;
}
} // namespace

namespace nx::core
{
RenameDataAction::RenameDataAction(const DataPath& path, const std::string& newName, bool overwrite)
: m_NewName(newName)
, m_Path(path)
, m_Overwrite(overwrite)
{
}

RenameDataAction::~RenameDataAction() noexcept = default;

Result<> RenameDataAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "RenameDataAction: ";
  auto dataObject = dataStructure.getData(m_Path);
  if(dataObject == nullptr)
  {
    std::string ss = fmt::format("{}Source DataPath '{}' does not exist to rename to {}", prefix, m_Path.toString(), m_NewName);
    return MakeErrorResult(-6601, ss);
  }

  Result<> result = {};

  bool validRename = dataObject->canRename(m_NewName);

  if(!validRename)
  {
    if(m_Overwrite)
    {
      if(mode == Mode::Preflight)
      {
        std::vector<std::string> pathVec = m_Path.getPathVector();

        // The canRename() function returns true if the base object already has the objects new name
        // so in that case we will never make it here
        for(const auto& name : pathVec)
        {
          if(name == m_NewName)
          {
            std::string ss = fmt::format("{}The object that would be overwritten is a parent container to {} cannot rename to {}", prefix, m_Path.getTargetName(), m_NewName);
            return MakeErrorResult(-6601, ss);
          }
        }

        std::string ss = fmt::format("{}Another object exists with that name, will overwrite destroying other DataObject at '{}' and replacing it with '{}'", prefix, m_NewName, m_Path.toString());
        result.warnings().emplace_back(Warning{-6602, ss});
      }
      else
      {
        DataObject::IdType targetId = std::numeric_limits<DataObject::IdType>::max();
        for(auto dataObjectID : dataStructure.getAllDataObjectIds())
        {
          if(dataStructure.getData(dataObjectID)->getName() == m_NewName)
          {
            targetId = dataObjectID;
            break;
          }
        }

        if(dataStructure.getDataAs<BaseGroup>(targetId) != nullptr)
        {
          // Recursive removal of overwritten object
          result = MergeResults(result, ::TerminateNodesRecursively(dataStructure, targetId, mode, true));
        }
        else
        {
          dataStructure.removeData(targetId);
        }
      }
    }
    else
    {
      std::string ss = fmt::format("{}Could not rename DataObject at '{}' to '{}'", prefix, m_Path.toString(), m_NewName);
      return MakeErrorResult(-6603, ss);
    }
  }

  dataObject->rename(m_NewName);
  return result;
}

IDataAction::UniquePointer RenameDataAction::clone() const
{
  return std::make_unique<RenameDataAction>(m_Path, m_NewName);
}

std::string RenameDataAction::newName() const
{
  return m_NewName;
}

const DataPath& RenameDataAction::path() const
{
  return m_Path;
}

bool RenameDataAction::overwrite() const
{
  return m_Overwrite;
}

} // namespace nx::core
