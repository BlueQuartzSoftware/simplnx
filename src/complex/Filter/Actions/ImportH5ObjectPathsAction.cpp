#include "ImportH5ObjectPathsAction.hpp"

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/Readers/FileReader.hpp"

#include <fmt/core.h>

#include <algorithm>
#include <sstream>

using namespace complex;

namespace
{
constexpr complex::int32 k_InsertFailureError = -6202;

void sortImportPaths(std::vector<DataPath>& importPaths)
{
  std::sort(importPaths.begin(), importPaths.end(), [](const DataPath& first, const DataPath& second) { return first.getLength() < second.getLength(); });
}

std::vector<DataPath> getImportPaths(const DataStructure& importStructure, const std::optional<std::vector<DataPath>>& importPaths)
{
  std::vector<DataPath> paths;

  if(importPaths.has_value())
  {
    paths = importPaths.value();
  }
  else
  {

    //    importStructure.exportHeirarchyAsText(std::cout);
    //    std::cout << "*************************************************************" << std::endl;
    //    paths = importStructure.getAllDataPaths();
    //    std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
    //    for(const auto& path : paths)
    //    {
    //      std::cout << path.toString() << std::endl;
    //    }
    //    std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
    paths = importStructure.gatherAllPaths();
    //    for(const auto& path : paths)
    //    {
    //      std::cout << path.toString() << std::endl;
    //    }
  }

  sortImportPaths(paths);

  return paths;
}
} // namespace

namespace complex
{
ImportH5ObjectPathsAction::ImportH5ObjectPathsAction(const std::filesystem::path& importFile, const PathsType& paths)
: IDataCreationAction({})
, m_H5FilePath(importFile)
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
  static constexpr StringLiteral prefix = "ImportH5ObjectPathsAction: ";
  bool preflighting = (mode == Mode::Preflight);

  complex::HDF5::FileReader fileReader(m_H5FilePath);
  Result<DataStructure> dataStructureResult = DREAM3D::ImportDataStructureFromFile(fileReader, preflighting);
  if(dataStructureResult.invalid())
  {
    return ConvertResult(std::move(dataStructureResult));
  }

  // Ensure there are no conflicting DataObject ID values
  DataStructure importStructure = std::move(dataStructureResult.value());
  importStructure.resetIds(dataStructure.getNextId());

  auto importPaths = getImportPaths(importStructure, m_Paths);
  std::stringstream errorMessages;
  for(const auto& targetPath : importPaths)
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
      return {nonstd::make_unexpected(std::vector<Error>{{k_InsertFailureError, fmt::format("{}Unable to import DataObject at '{}'", prefix, targetPath.toString())}})};
    }
  }
  if(!errorMessages.str().empty())
  {
    return MakeErrorResult(-6201, errorMessages.str());
  }

  return {};
}

std::vector<DataPath> ImportH5ObjectPathsAction::getAllCreatedPaths() const
{
  if(m_Paths.has_value())
  {
    return m_Paths.value();
  }
  else
  {
    return {};
  }
}

} // namespace complex
