#include "ImportH5ObjectPathsAction.hpp"

#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"

#include <fmt/core.h>

#include <algorithm>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
/**
 * @struct SetImportedPlaceholderFormatFunctor
 * @brief Replaces an imported array store with a resolver-planned placeholder.
 */
struct SetImportedPlaceholderFormatFunctor
{
  /**
   * @brief Replaces one typed EmptyDataStore.
   * @tparam T Specifies the array value type.
   * @param dataArray Receives the isolated placeholder.
   * @param dataFormat Specifies the resolved storage format.
   */
  template <typename T>
  void operator()(IDataArray* dataArray, const std::string& dataFormat) const
  {
    auto* typedArray = dynamic_cast<DataArray<T>*>(dataArray);
    if(typedArray == nullptr)
    {
      return;
    }
    auto* emptyStore = dynamic_cast<EmptyDataStore<T>*>(typedArray->getIDataStore());
    if(emptyStore == nullptr)
    {
      return;
    }
    typedArray->setDataStore(std::make_shared<EmptyDataStore<T>>(emptyStore->getTupleShape(), emptyStore->getComponentShape(), dataFormat));
  }
};

void applyPreflightStoragePlan(DataStructure& dataStructure, const DataPath& targetPath, IDataArray& dataArray)
{
  const auto* dataStore = dataArray.getIDataStore();
  if(dataStore == nullptr || dataStore->getStoreType() != IDataStore::StoreType::Empty)
  {
    return;
  }
  const std::string dataFormat = ArrayCreationUtilities::ResolveStorageFormat(dataStructure, targetPath, dataArray.getDataType(), dataArray.memoryUsage(), "");
  ExecuteDataFunction(SetImportedPlaceholderFormatFunctor{}, dataArray.getDataType(), &dataArray, dataFormat);
}
} // namespace

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

  // Preflight uses a stat-validated metadata cache to avoid repeated HDF5
  // hierarchy scans. Execute loads resolver-selected stores before merging.
  auto result = (mode == Mode::Preflight) ? DREAM3D::Dream3dPreflightCache::Instance().fetch(m_H5FilePath) : DREAM3D::LoadDataStructure(m_H5FilePath);

  if(result.invalid())
  {
    return ConvertResult(std::move(result));
  }

  DataStructure sourceStructure = std::move(result.value());
  // Renumber source objects before merge to avoid collisions with pipeline objects.
  sourceStructure.resetIds(dataStructure.getNextId());

  // Insert parents before children because every child requires its parent path.
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

    // The loaded source owns resolver-selected stores. Copy group shells without
    // children because selected child paths insert independently.
    const auto sourceObject = sourceStructure.getSharedData(targetPath);
    const auto objectCopy = std::shared_ptr<DataObject>(sourceObject->shallowCopy());
    if(mode == Mode::Preflight)
    {
      if(auto* dataArray = dynamic_cast<IDataArray*>(objectCopy.get()); dataArray != nullptr)
      {
        applyPreflightStoragePlan(dataStructure, targetPath, *dataArray);
      }
    }
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
