#include "ImportH5ObjectPathsAction.hpp"

#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIOInternal.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include <fmt/core.h>

#include <cstdio>

#include <algorithm>
#include <unordered_set>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
/**
 * @struct SetImportedPlaceholderFormatFunctor
 * @brief Replaces an imported array store with a resolver-planned placeholder.
 */
struct PlanImportedArrayStoreFunctor
{
  /**
   * @brief Resolves and installs one imported array's storage plan.
   * @tparam T Specifies the array value type.
   * @param destination Provides the resolver and the context it judges.
   * @param targetPath Identifies where the array will live.
   * @param dataArray Receives the planned store.
   * @return Resolution or store-replacement failures.
   *
   * The plan is resolved against the destination because that is where the array will live. A
   * placeholder receives a planned placeholder; an array the cache already materialized is copied
   * into the destination's selected format so both routes honor the same decision.
   */
  template <typename T>
  Result<> operator()(DataStructure& destination, const DataPath& targetPath, IDataArray* dataArray) const
  {
    auto* typedArray = dynamic_cast<DataArray<T>*>(dataArray);
    if(typedArray == nullptr || typedArray->getIDataStore() == nullptr)
    {
      return {};
    }
    const IDataStore* sourceStore = typedArray->getIDataStore();

    Result<std::string> formatResult;
    try
    {
      const uint64 logicalBytes = CalculateStoreCopyBytes(typedArray->getTupleShape(), typedArray->getComponentShape(), sizeof(T));
      formatResult = ResolveNumericStorageFormat(destination, targetPath, GetDataType<T>(), logicalBytes, "");
    } catch(const std::bad_alloc&)
    {
      throw;
    } catch(const std::exception& error)
    {
      return MakeErrorResult(-6207, fmt::format("Cannot size imported numeric array '{}': {}", targetPath.toString(), error.what()));
    }
    if(formatResult.invalid())
    {
      return ConvertResult(std::move(formatResult));
    }
    const std::string selectedFormat = formatResult.value();
    auto warnings = std::move(formatResult.warnings());

    Result<> replaceResult;
    if(sourceStore->getStoreType() == IDataStore::StoreType::Empty)
    {
      auto plannedResult = EmptyDataStore<T>::Create(typedArray->getTupleShape(), typedArray->getComponentShape(), selectedFormat);
      if(plannedResult.invalid())
      {
        auto result = ConvertResult(std::move(plannedResult));
        result.warnings().insert(result.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
        return result;
      }
      replaceResult = typedArray->setDataStore(std::shared_ptr<EmptyDataStore<T>>(std::move(plannedResult.value())));
    }
    else
    {
      try
      {
        std::shared_ptr<IDataStore> freshBase(sourceStore->deepCopy(selectedFormat));
        auto freshStore = std::dynamic_pointer_cast<AbstractDataStore<T>>(freshBase);
        if(freshStore == nullptr)
        {
          throw std::runtime_error("the selected factory returned an incompatible numeric store");
        }
        replaceResult = typedArray->setDataStore(std::move(freshStore));
      } catch(const std::bad_alloc&)
      {
        throw;
      } catch(const std::exception& error)
      {
        auto result = MakeErrorResult(
            -6208, fmt::format("Cannot copy imported numeric array '{}' into selected format '{}': {}", targetPath.toString(), selectedFormat.empty() ? "in-memory" : selectedFormat, error.what()));
        result.warnings() = std::move(warnings);
        return result;
      }
    }
    replaceResult.warnings().insert(replaceResult.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
    return replaceResult;
  }
};

Result<> applyPreflightStoragePlan(DataStructure& dataStructure, const DataPath& targetPath, IDataArray& dataArray)
{
  return ExecuteDataFunction(PlanImportedArrayStoreFunctor{}, dataArray.getDataType(), dataStructure, targetPath, &dataArray);
}
/**
 * @brief Clears geometry references to objects outside the exact import selection.
 * @param sourceStructure Owns the imported object copies.
 * @param paths Specifies the objects that the action
 * inserts.
 *
 * Metadata and selected loaders can retain unselected objects. Their IDs must not survive renumbering as geometry references.
 */
void clearExcludedGeometryReferences(DataStructure& sourceStructure, const std::vector<DataPath>& paths)
{
  std::unordered_set<DataObject::IdType> selectedIds;
  selectedIds.reserve(paths.size());
  for(const auto& path : paths)
  {
    if(const auto* object = sourceStructure.getData(path); object != nullptr)
    {
      selectedIds.insert(object->getId());
    }
  }

  const auto selectedReference = [&selectedIds](DataObject::OptionalId id) { return id.has_value() && selectedIds.find(*id) == selectedIds.end() ? DataObject::OptionalId{} : id; };
  for(const auto& path : paths)
  {
    auto* geometry = sourceStructure.getDataAs<IGeometry>(path);
    if(geometry == nullptr)
    {
      continue;
    }
    geometry->setElementSizesId(selectedReference(geometry->getElementSizesId()));
    if(auto* grid = dynamic_cast<IGridGeometry*>(geometry); grid != nullptr)
    {
      grid->setCellData(selectedReference(grid->getCellDataId()));
    }
    if(auto* node = dynamic_cast<INodeGeometry0D*>(geometry); node != nullptr)
    {
      node->setVertexListId(selectedReference(node->getVertexListId()));
      node->setVertexDataId(selectedReference(node->getVertexAttributeMatrixId()));
    }
    if(auto* node = dynamic_cast<INodeGeometry1D*>(geometry); node != nullptr)
    {
      node->setEdgeListId(selectedReference(node->getEdgeListId()));
      node->setEdgeDataId(selectedReference(node->getEdgeAttributeMatrixId()));
      node->setElementContainingVertId(selectedReference(node->getElementContainingVertId()));
      node->setElementNeighborsId(selectedReference(node->getElementNeighborsId()));
      node->setElementCentroidsId(selectedReference(node->getElementCentroidsId()));
    }
    if(auto* node = dynamic_cast<INodeGeometry2D*>(geometry); node != nullptr)
    {
      node->setFaceListId(selectedReference(node->getFaceListId()));
      node->setFaceDataId(selectedReference(node->getFaceAttributeMatrixId()));
      node->setUnsharedEdgesId(selectedReference(node->getUnsharedEdgesId()));
    }
    if(auto* node = dynamic_cast<INodeGeometry3D*>(geometry); node != nullptr)
    {
      node->setPolyhedronListId(selectedReference(node->getPolyhedronListId()));
      node->setPolyhedraDataId(selectedReference(node->getPolyhedraAttributeMatrixId()));
      node->setUnsharedFacedId(selectedReference(node->getUnsharedFacesId()));
    }
    if(auto* rectGrid = dynamic_cast<RectGridGeom*>(geometry); rectGrid != nullptr)
    {
      rectGrid->setXBoundsId(selectedReference(rectGrid->getXBoundsId()));
      rectGrid->setYBoundsId(selectedReference(rectGrid->getYBoundsId()));
      rectGrid->setZBoundsId(selectedReference(rectGrid->getZBoundsId()));
    }
  }
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

  // Execute materializes selected objects before the merge. Preflight uses cached metadata.
  auto result = DREAM3D::Dream3dPreflightCache::Instance().fetch(m_H5FilePath);
  if(result.invalid())
  {
    return ConvertResult(std::move(result));
  }
  if(mode == Mode::Execute)
  {
    std::vector<DataPath> selectedPaths;
    selectedPaths.reserve(m_Paths.size());
    for(const auto& path : m_Paths)
    {
      if(result.value().containsData(path))
      {
        selectedPaths.push_back(path);
      }
    }
    result = DREAM3D::LoadDataStructureArrays(m_H5FilePath, selectedPaths);
  }
  if(result.invalid())
  {
    return ConvertResult(std::move(result));
  }

  DataStructure sourceStructure = std::move(result.value());
  Result<> importResult = ConvertResult(std::move(result));
  clearExcludedGeometryReferences(sourceStructure, m_Paths);
  // Renumber source objects before merge to avoid collisions with pipeline objects.
  sourceStructure.resetIds(dataStructure.getNextId());

  // Stage the whole import against a copy of the destination. Planning needs the destination's real
  // context, and a failure part way through must leave the caller's structure untouched, so nothing
  // reaches dataStructure until every selected path has been planned and materialized.
  DataStructure preparedStructure = dataStructure;

  // Insert parents before children because every child requires its parent path.
  auto sortedPaths = m_Paths;
  std::sort(sortedPaths.begin(), sortedPaths.end(), [](const DataPath& a, const DataPath& b) { return a.getLength() < b.getLength(); });

  std::vector<DataPath> preparedPaths;
  std::vector<DataPath> importedLeafPaths;
  for(const auto& targetPath : sortedPaths)
  {
    // preparedStructure starts as a copy of the destination, so this catches both objects that were
    // already there and objects staged earlier in this same import.
    if(preparedStructure.getDataAs<DataObject>(targetPath) != nullptr)
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6203, fmt::format("{}Unable to import DataObject at '{}' because an object "
                                                                                      "already exists at that path. Consider renaming the existing object before importing, or "
                                                                                      "exclude this path from the import selection.",
                                                                                      prefix, targetPath.toString())));
    }

    if(!sourceStructure.containsData(targetPath))
    {
      continue;
    }

    // The loaded source owns resolver-selected stores. Copy group shells without
    // children because selected child paths insert independently.
    const auto sourceObject = sourceStructure.getSharedData(targetPath);
    const auto objectCopy = std::shared_ptr<DataObject>(sourceObject->shallowCopy());
    // Both modes plan storage, so the resolver sees every imported numeric array and decides where it
    // lives before the array is inserted. Execute additionally fills the planned placeholders below.
    if(auto* dataArray = dynamic_cast<IDataArray*>(objectCopy.get()); dataArray != nullptr)
    {
      importResult = MergeResults(std::move(importResult), applyPreflightStoragePlan(preparedStructure, targetPath, *dataArray));
      if(importResult.invalid())
      {
        return importResult;
      }
      importedLeafPaths.push_back(targetPath);
    }
    if(const auto group = std::dynamic_pointer_cast<BaseGroup>(objectCopy); group != nullptr)
    {
      group->clear();
    }
    if(!preparedStructure.insert(objectCopy, targetPath.getParent()))
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6202, fmt::format("{}Unable to insert DataObject at path '{}' into the DataStructure. "
                                                                                      "The parent path '{}' may not exist.",
                                                                                      prefix, targetPath.toString(), targetPath.getParent().toString())));
    }
    preparedPaths.push_back(targetPath);
  }

  if(mode == Mode::Execute && !importedLeafPaths.empty())
  {
    auto fileReader = HDF5::FileIO::ReadFile(m_H5FilePath);
    if(!fileReader.isValid())
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6206, fmt::format("{}Unable to open '{}' for materialization.", prefix, m_H5FilePath.string())));
    }
    importResult = MergeResults(std::move(importResult), DREAM3D::detail::MaterializeImportedPaths(preparedStructure, fileReader, importedLeafPaths));
    if(importResult.invalid())
    {
      return importResult;
    }
  }

  // Every selected path planned and materialized, so publish the staged objects.
  for(const auto& targetPath : preparedPaths)
  {
    const auto preparedObject = preparedStructure.getSharedData(targetPath);
    if(preparedObject == nullptr)
    {
      continue;
    }
    const auto objectCopy = std::shared_ptr<DataObject>(preparedObject->shallowCopy());
    if(const auto group = std::dynamic_pointer_cast<BaseGroup>(objectCopy); group != nullptr)
    {
      group->clear();
    }
    if(!dataStructure.insert(objectCopy, targetPath.getParent()))
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6202, fmt::format("{}Unable to insert DataObject at path '{}' into the DataStructure. "
                                                                                      "The parent path '{}' may not exist.",
                                                                                      prefix, targetPath.toString(), targetPath.getParent().toString())));
    }
  }

  return importResult;
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
