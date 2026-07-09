#include "ImportH5ObjectPathsAction.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <sstream>

using namespace nx::core;

namespace
{
void sortImportPaths(std::vector<DataPath>& importPaths)
{
  std::sort(importPaths.begin(), importPaths.end(), [](const DataPath& first, const DataPath& second) { return first.getLength() < second.getLength(); });
}
} // namespace

namespace nx::core
{
ImportH5ObjectPathsAction::ImportH5ObjectPathsAction(const std::filesystem::path& importFile, const PathsType& paths)
: IDataCreationAction(DataPath{})
, m_H5FilePath(importFile)
, m_Paths(paths)
{
  sortImportPaths(m_Paths);
}

ImportH5ObjectPathsAction::~ImportH5ObjectPathsAction() noexcept = default;

Result<> ImportH5ObjectPathsAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "ImportH5ObjectPathsAction: ";

  // Metadata comes from Dream3dPreflightCache in BOTH modes: this action runs
  // on every pipeline preflight, and re-traversing the file's HDF5 metadata
  // each time freezes the UI on high-latency storage. The cache stat-validates
  // the file on every fetch, so a file modified between preflight and execute
  // is re-read rather than served stale.
  Result<DataStructure> dataStructureResult = DREAM3D::Dream3dPreflightCache::Instance().fetch(m_H5FilePath);
  if(dataStructureResult.invalid())
  {
    return ConvertResult(std::move(dataStructureResult));
  }

  // Ensure there are no conflicting DataObject ID values
  DataStructure importStructure = std::move(dataStructureResult.value());
  importStructure.resetIds(dataStructure.getNextId());

  const bool preflighting = mode == Mode::Preflight;

  // Execute mode needs an open file for the bulk-array reads performed by
  // FinishImportingObject; preflight never touches file contents, so the open
  // (a round-trip on network storage) is skipped entirely.
  nx::core::HDF5::FileIO fileReader;
  if(!preflighting)
  {
    fileReader = nx::core::HDF5::FileIO::ReadFile(m_H5FilePath);
    if(!fileReader.isValid())
    {
      return MakeErrorResult(-6204, fmt::format("{}Failed to open the HDF5 file at the specified path: '{}'", prefix, m_H5FilePath.string()));
    }
  }

  std::stringstream errorMessages;
  for(const auto& targetPath : m_Paths)
  {
    if(dataStructure.getDataAs<DataObject>(targetPath) != nullptr)
    {
      return MakeErrorResult(-6203, fmt::format("{}Unable to import DataObject at '{}' because an object already exists there. Consider a rename of existing object.", prefix, targetPath.toString()));
    }

    auto result = preflighting ? DREAM3D::FinishImportingObjectPreflight(importStructure, dataStructure, targetPath) :
                                 DREAM3D::FinishImportingObject(importStructure, dataStructure, targetPath, fileReader, preflighting);
    if(result.invalid())
    {
      for(const auto& errorResult : result.errors())
      {
        errorMessages << errorResult.message << std::endl;
      }
    }
  }
  if(!errorMessages.str().empty())
  {
    return MakeErrorResult(-6201, errorMessages.str());
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
