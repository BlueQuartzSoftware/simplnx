#include "ImportH5ObjectPathsAction.hpp"

#include <algorithm>

#include <fmt/core.h>

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"

using namespace complex;

namespace
{
constexpr complex::int32 k_InsertFailureError = -2;

void sortImportPaths(std::vector<DataPath>& importPaths)
{
  std::sort(importPaths.begin(), importPaths.end(), [](const DataPath& first, const DataPath& second) { return first.getLength() < second.getLength(); });
}

std::vector<DataPath> getImportPaths(const DataStructure& importStructure, const std::optional<std::vector<DataPath>>& importPaths)
{
  std::vector<DataPath> paths = (importPaths.has_value() ? importPaths.value() : importStructure.getAllDataPaths());
  sortImportPaths(paths);
  return paths;
}
} // namespace

namespace complex
{
ImportH5ObjectPathsAction::ImportH5ObjectPathsAction(const std::filesystem::path& importFile, const PathsType& paths)
: m_H5FilePath(importFile)
, m_Paths(paths)
{
  if(m_Paths.has_value())
  {
    sortImportPaths(*m_Paths);
  }
}

ImportH5ObjectPathsAction::~ImportH5ObjectPathsAction() noexcept = default;

Result<> ImportH5ObjectPathsAction::apply(DataStructure& dataStructure, Mode mode) const
{
  bool preflighting = (mode == Mode::Preflight);

  H5::FileReader fileReader(m_H5FilePath);
  Result<DataStructure> dataStructureResult = DREAM3D::ImportDataStructureFromFile(fileReader, preflighting);
  if(dataStructureResult.invalid())
  {
    return ConvertResult(std::move(dataStructureResult));
  }

  // Ensure there are no conflicting DataObject ID values
  DataStructure importStructure = std::move(dataStructureResult.value());
  importStructure.resetIds(dataStructure.getNextId());

  auto importPaths = getImportPaths(importStructure, m_Paths);
  for(const auto& targetPath : importPaths)
  {
    auto importObject = importStructure.getSharedData(targetPath);
    auto importData = std::shared_ptr<DataObject>(importObject->deepCopy());
    // Clear all children before inserting into the DataStructure
    if(auto importGroup = std::dynamic_pointer_cast<BaseGroup>(importData); importGroup != nullptr)
    {
      importGroup->clear();
    }

    if(!dataStructure.insert(importData, targetPath.getParent()))
    {
      return {nonstd::make_unexpected(std::vector<Error>{{k_InsertFailureError, fmt::format("Unable to import DataObject at '{}'", targetPath.toString())}})};
    }
  }

  return {};
}
} // namespace complex
