#include "ImportH5ObjectPathsAction.hpp"

#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

#include <fmt/core.h>

#include <algorithm>

using namespace nx::core;
namespace fs = std::filesystem;

namespace nx::core
{
ImportH5ObjectPathsAction::ImportH5ObjectPathsAction(const fs::path& importFile, const PathsType& paths)
: IDataCreationAction(DataPath{})
, m_H5FilePath(importFile)
, m_Paths(paths)
{
  std::sort(m_Paths.begin(), m_Paths.end(), [](const DataPath& a, const DataPath& b) { return a.getLength() < b.getLength(); });
}

ImportH5ObjectPathsAction::~ImportH5ObjectPathsAction() noexcept = default;

Result<> ImportH5ObjectPathsAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "ImportH5ObjectPathsAction: ";

  // Get the source DataStructure — metadata only for preflight, loaded arrays for execute
  // Preflight: metadata only. Execute: full load (not LoadDataStructureArrays, because the
  // merge loop below selectively copies only m_Paths — pruning would break Geometry/AttributeMatrix
  // relationships in the source structure before the merge has a chance to pick the right objects).
  auto result = (mode == Mode::Preflight) ? DREAM3D::LoadDataStructureMetadata(m_H5FilePath) : DREAM3D::LoadDataStructure(m_H5FilePath);

  if(result.invalid())
  {
    return ConvertResult(std::move(result));
  }

  DataStructure sourceStructure = std::move(result.value());
  sourceStructure.resetIds(dataStructure.getNextId());

  // Merge source objects into the pipeline's DataStructure.
  // Sort paths shortest-first so parents are inserted before children.
  auto sortedPaths = m_Paths;
  std::sort(sortedPaths.begin(), sortedPaths.end(), [](const DataPath& a, const DataPath& b) { return a.getLength() < b.getLength(); });

  for(const auto& targetPath : sortedPaths)
  {
    if(dataStructure.getDataAs<DataObject>(targetPath) != nullptr)
    {
      return MakeErrorResult(-6203, fmt::format("{}Unable to import DataObject at '{}' because an object "
                                                "already exists at that path. Consider renaming the existing object before importing, or "
                                                "exclude this path from the import selection.",
                                                prefix, targetPath.toString()));
    }

    if(!sourceStructure.containsData(targetPath))
    {
      continue;
    }

    // Shallow-copy the object from the source structure (which has real stores)
    // and insert it into the pipeline's DataStructure. Clear children on groups
    // because child objects will be inserted by their own paths in the loop.
    const auto sourceObject = sourceStructure.getSharedData(targetPath);
    const auto objectCopy = std::shared_ptr<DataObject>(sourceObject->shallowCopy());
    if(const auto group = std::dynamic_pointer_cast<BaseGroup>(objectCopy); group != nullptr)
    {
      group->clear();
    }
    if(!dataStructure.insert(objectCopy, targetPath.getParent()))
    {
      return MakeErrorResult(-6202, fmt::format("{}Unable to insert DataObject at path '{}' into the DataStructure. "
                                                "The parent path '{}' may not exist.",
                                                prefix, targetPath.toString(), targetPath.getParent().toString()));
    }
  }

  return {};
}

IDataAction::UniquePointer ImportH5ObjectPathsAction::clone() const
{
  return std::make_unique<ImportH5ObjectPathsAction>(m_H5FilePath, m_Paths);
}

std::vector<DataPath> ImportH5ObjectPathsAction::getAllCreatedPaths() const
{
  return m_Paths;
}

} // namespace nx::core
