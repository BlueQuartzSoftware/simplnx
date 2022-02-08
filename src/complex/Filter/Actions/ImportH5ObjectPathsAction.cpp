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
ImportH5ObjectPathsAction::ImportH5ObjectPathsAction(const H5::FileReader& importObject, const PathsType& paths)
: m_H5FileReader(importObject)
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

  H5::ErrorType errorCode;
  const DREAM3D::FileData fileData = DREAM3D::ReadFile(m_H5FileReader, errorCode, preflighting);
  if(errorCode < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{errorCode, "Failed to import a DataStructure from the target HDF5 file."}})};
  }

  const auto& importStructure = fileData.second;

  auto importPaths = getImportPaths(importStructure, m_Paths);
  for(const auto& targetPath : importPaths)
  {
    auto importObject = importStructure.getSharedData(targetPath);
    auto importData = std::shared_ptr<DataObject>(importObject->deepCopy());
    // Clear all children before inserting into the DataStructure
    if(auto importGroup = std::dynamic_pointer_cast<BaseGroup>(importData))
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
