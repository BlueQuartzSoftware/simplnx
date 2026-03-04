#include "ImportH5ObjectPathsAction.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
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
: AbstractDataCreationAction(DataPath{})
, m_H5FilePath(importFile)
, m_Paths(paths)
{
  sortImportPaths(m_Paths);
}

ImportH5ObjectPathsAction::~ImportH5ObjectPathsAction() noexcept = default;

Result<> ImportH5ObjectPathsAction::apply(DataStructure& dataStructure, Mode mode) const
{
  static constexpr StringLiteral prefix = "ImportH5ObjectPathsAction: ";

  auto fileReader = nx::core::HDF5::FileIO::ReadFile(m_H5FilePath);
  // Import as a preflight data structure to start to conserve memory and only allocate the data you want later
  Result<DataStructure> dataStructureResult = DREAM3D::ImportDataStructureFromFile(fileReader, true);
  if(dataStructureResult.invalid())
  {
    return ConvertResult(std::move(dataStructureResult));
  }

  // Ensure there are no conflicting AbstractDataObject ID values
  DataStructure importStructure = std::move(dataStructureResult.value());
  importStructure.resetIds(dataStructure.getNextId());

  const bool preflighting = mode == Mode::Preflight;
  std::stringstream errorMessages;
  for(const auto& targetPath : m_Paths)
  {
    if(dataStructure.getDataAs<AbstractDataObject>(targetPath) != nullptr)
    {
      return MakeErrorResult(-6203, fmt::format("{}Unable to import DataObject at '{}' because an object already exists there. Consider a rename of existing object.", prefix, targetPath.toString()));
    }

    auto result = DREAM3D::FinishImportingObject(importStructure, dataStructure, targetPath, fileReader, preflighting);
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

  return ConvertResult(std::move(dataStructureResult));
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
