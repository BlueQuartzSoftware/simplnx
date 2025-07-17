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
constexpr nx::core::int32 k_InsertFailureError = -6202;

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
  bool preflighting = (mode == Mode::Preflight);

  auto fileReader = nx::core::HDF5::FileIO::ReadFile(m_H5FilePath);
  Result<DataStructure> dataStructureResult = DREAM3D::ImportDataStructureFromFile(fileReader, true);
  if(dataStructureResult.invalid())
  {
    return ConvertResult(std::move(dataStructureResult));
  }

  // Ensure there are no conflicting DataObject ID values
  DataStructure importStructure = std::move(dataStructureResult.value());
  importStructure.resetIds(dataStructure.getNextId());

  std::stringstream errorMessages;
  for(const auto& targetPath : m_Paths)
  {
    if(!importStructure.containsData(targetPath))
    {
      errorMessages << fmt::format("{}DataStructure Object Path '{}' does not exist for importing.", prefix, targetPath.toString()) << std::endl;
      continue;
    }
    auto importObject = importStructure.getSharedData(targetPath);
    auto importData = std::shared_ptr<DataObject>(importObject->shallowCopy());
    // Clear all children before inserting into the DataStructure
    if(auto importGroup = std::dynamic_pointer_cast<BaseGroup>(importData); importGroup != nullptr)
    {
      importGroup->clear();
    }

    if(!dataStructure.insert(importData, targetPath.getParent()))
    {
      return MakeErrorResult(k_InsertFailureError, fmt::format("{}Unable to import DataObject at '{}'", prefix, targetPath.toString()));
    }
    if(mode == Mode::Execute)
    {
      if(auto result = DREAM3D::FinishImportingObject(dataStructure, targetPath, fileReader); result.invalid())
      {
        return result;
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
