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
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/Messaging/DataRemovedMessage.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIOInternal.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dPreflightCache.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include <fmt/core.h>

#include <cstdio>

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <new>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
/**
 * @struct PlanImportedArrayStoreFunctor
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
   * @param allowPopulated Permits only eager values from the legacy Statistics contract.
   * @return Resolution or store-replacement failures.
   *
   * All selected shells are present before policy runs. Ordinary arrays remain placeholders; legacy Statistics values receive an independent selected-format copy.
   */
  template <typename T>
  Result<> operator()(DataStructure& destination, const DataPath& targetPath, IDataArray* dataArray, bool allowPopulated) const
  {
    auto* typedArray = dynamic_cast<DataArray<T>*>(dataArray);
    if(typedArray == nullptr || typedArray->getIDataStore() == nullptr)
    {
      return MakeErrorResult(-6207, fmt::format("Cannot plan imported numeric array '{}': its typed array or store is unavailable.", targetPath.toString()));
    }
    const IDataStore* sourceStore = typedArray->getIDataStore();
    if(sourceStore->getStoreType() != IDataStore::StoreType::Empty && !allowPopulated)
    {
      return MakeErrorResult(-6208, fmt::format("Cannot plan imported numeric array '{}': ordinary source values were materialized before destination planning.", targetPath.toString()));
    }

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
      warnings.insert(warnings.end(), std::make_move_iterator(plannedResult.warnings().begin()), std::make_move_iterator(plannedResult.warnings().end()));
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

Result<> applyPreflightStoragePlan(DataStructure& dataStructure, const DataPath& targetPath, IDataArray& dataArray, bool allowPopulated)
{
  return ExecuteDataFunction(PlanImportedArrayStoreFunctor{}, dataArray.getDataType(), dataStructure, targetPath, &dataArray, allowPopulated);
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
struct ImportH5ObjectPathsAction::TestState
{
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  PublicationFault fault = PublicationFault::None;
  bool consumed = false;
  bool refuseCleanup = false;
  bool cleanupConsumed = false;
  std::shared_ptr<DataObject> retainedOwner;
  std::vector<DataStructure::ImportPublicationRecord> retainedRecords;
  DataStructure* destination = nullptr;
  DataObject::IdType savedNextId = 0;
  DataObject::IdType reservedNextId = 0;
#endif
};

ImportH5ObjectPathsAction::ImportH5ObjectPathsAction(const fs::path& importFile, const PathsType& paths)
: IDataCreationAction(DataPath{})
, m_H5FilePath(importFile)
, m_Paths(paths)
{
  std::sort(m_Paths.begin(), m_Paths.end(), [](const DataPath& a, const DataPath& b) { return a.getLength() < b.getLength(); });
}

ImportH5ObjectPathsAction::~ImportH5ObjectPathsAction() noexcept = default;

bool ImportH5ObjectPathsAction::rollbackPublication(DataStructure& destination, std::vector<DataStructure::ImportPublicationRecord>& records, DataObject::IdType savedNextId,
                                                    DataObject::IdType reservedNextId, [[maybe_unused]] bool allowRefusal) const
{
  bool complete = true;
  // Finish all independent structural cleanup before any observer can change the destination.
  for(usize index = records.size(); index > 0; --index)
  {
    auto& record = records[index - 1];
    if(record.owner == nullptr)
    {
      continue;
    }
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    if(allowRefusal && m_TestState != nullptr && m_TestState->refuseCleanup && index == 2 && record.stage != DataStructure::ImportInsertionStage::None)
    {
      m_TestState->cleanupConsumed = true;
      record.cleanup = DataStructure::ImportCleanupStatus::Refused;
      complete = false;
      continue;
    }
#endif
    record.cleanup = destination.rollbackImportedObject(record);
    complete = complete && record.cleanup == DataStructure::ImportCleanupStatus::Complete;
  }
  for(auto iter = records.rbegin(); iter != records.rend(); ++iter)
  {
    auto& record = *iter;
    if(record.cleanup == DataStructure::ImportCleanupStatus::Complete && record.notifyRemoval && !record.notificationAttempted)
    {
      record.notificationAttempted = true;
      try
      {
        destination.notify(record.removalMessage);
      } catch(...)
      {
        record.notificationFailed = true;
      }
    }
  }
  for(auto& record : records)
  {
    if(record.cleanup == DataStructure::ImportCleanupStatus::Complete)
    {
      record.owner.reset();
      record.parent.reset();
    }
  }
  // A removal observer can create unrelated objects. Preserve its advanced counter.
  if(complete && destination.m_NextId == reservedNextId)
  {
    destination.m_NextId = savedNextId;
  }
  return complete;
}

Result<> ImportH5ObjectPathsAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto metadata = DREAM3D::Dream3dPreflightCache::Instance().fetchNeutralMetadata(m_H5FilePath);
  if(metadata.invalid())
  {
    return ConvertResult(std::move(metadata));
  }
  const bool legacy = metadata.value().fileVersion == DREAM3D::k_LegacyFileVersion;
  DataStructure sourceStructure = std::move(metadata.value().dataStructure);
  Result<> importResult = ConvertResult(std::move(metadata));
  if(m_Paths.empty())
  {
    return importResult;
  }
  std::set<DataPath> selectedPaths;
  for(const auto& path : m_Paths)
  {
    if(dataStructure.containsData(path))
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6203, fmt::format("Cannot import '{}' from '{}': the destination path already exists. Rename it or exclude this path.",
                                                                                      path.toString(), m_H5FilePath.string())));
    }
    if(sourceStructure.containsData(path) && !selectedPaths.insert(path).second)
    {
      return MergeResults(std::move(importResult),
                          MakeErrorResult(-6203, fmt::format("Cannot import '{}' from '{}': the same path is selected more than once.", path.toString(), m_H5FilePath.string())));
    }
  }
  if(selectedPaths.empty())
  {
    return importResult;
  }
  clearExcludedGeometryReferences(sourceStructure, m_Paths);
  const auto savedNextId = dataStructure.getNextId();
  const auto startId = std::max(savedNextId, DataObject::IdType{1});
  constexpr auto maximumId = std::numeric_limits<DataObject::IdType>::max();
  DataObject::IdType sourceCount = 0;
  for(const auto& entry : sourceStructure.m_DataObjects)
  {
    if(!entry.second.expired())
    {
      if(sourceCount == maximumId - startId)
      {
        return MergeResults(std::move(importResult), MakeErrorResult(-6213, fmt::format("Cannot import '{}': source identifiers starting at {} exceed the maximum next identifier {}.",
                                                                                        m_H5FilePath.string(), startId, maximumId)));
      }
      ++sourceCount;
    }
  }
  DataObject::IdType extraPlacementCount = 0;
  std::unordered_set<const DataObject*> countedSources;
  for(const auto& path : selectedPaths)
  {
    const auto* object = sourceStructure.getData(path);
    if(object == nullptr)
    {
      continue;
    }
    if(!countedSources.insert(object).second)
    {
      if(extraPlacementCount == maximumId - startId - sourceCount)
      {
        return MergeResults(
            std::move(importResult),
            MakeErrorResult(-6213, fmt::format("Cannot import '{}': additional placement count {} does not fit after {} source identifiers starting at {} (maximum next identifier {}).",
                                               m_H5FilePath.string(), extraPlacementCount + 1, sourceCount, startId, maximumId)));
      }
      ++extraPlacementCount;
    }
  }
  sourceStructure.resetIds(startId);
  const auto sourceNextId = sourceStructure.getNextId();

  std::set<DataObject::IdType> occupiedIds;
  for(const auto& entry : dataStructure.m_DataObjects)
  {
    occupiedIds.insert(entry.first);
  }
  // Registry enumeration omits hierarchy-only placements. Inspect actual maps before copying the destination.
  std::vector<const DataMap*> pendingMaps{&dataStructure.getDataMap()};
  std::unordered_set<const DataObject*> visitedObjects;
  while(!pendingMaps.empty())
  {
    const auto* map = pendingMaps.back();
    pendingMaps.pop_back();
    for(const auto& [id, owner] : *map)
    {
      occupiedIds.insert(id);
      if(owner == nullptr)
      {
        continue;
      }
      occupiedIds.insert(owner->getId());
      if(visitedObjects.insert(owner.get()).second)
      {
        if(const auto* group = dynamic_cast<const BaseGroup*>(owner.get()); group != nullptr)
        {
          pendingMaps.push_back(&group->getDataMap());
        }
      }
    }
  }

  struct PlacementPlan
  {
    DataPath path;
    std::shared_ptr<DataObject> source;
    DataObject::IdType sourceId = 0;
    DataObject::IdType assignedId = 0;
    bool extraPlacement = false;
  };
  auto sortedPaths = m_Paths;
  std::sort(sortedPaths.begin(), sortedPaths.end(), [](const DataPath& a, const DataPath& b) { return a.getLength() < b.getLength(); });
  std::vector<PlacementPlan> placements;
  placements.reserve(selectedPaths.size());
  std::unordered_set<const DataObject*> plannedSources;
  std::set<DataObject::IdType> assignedIds;
  auto nextCloneId = sourceNextId;
  // Extra copies use IDs beyond every source object, including source objects staged later or omitted from this selection.
  for(const auto& path : sortedPaths)
  {
    auto sourceObject = sourceStructure.getSharedData(path);
    if(sourceObject == nullptr)
    {
      continue;
    }
    const auto sourceId = sourceObject->getId();
    if(sourceStructure.getData(path) != sourceObject.get())
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6213, fmt::format("Cannot import '{}' from '{}': its source path and identifier {} do not name the same object.", path.toString(),
                                                                                      m_H5FilePath.string(), sourceId)));
    }
    if(sourceId < startId || sourceId >= sourceNextId || sourceId == maximumId || occupiedIds.contains(sourceId))
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6213, fmt::format("Cannot import '{}' from '{}': source identifier {} conflicts with the destination or its checked source range.",
                                                                                      path.toString(), m_H5FilePath.string(), sourceId)));
    }
    const bool extraPlacement = !plannedSources.insert(sourceObject.get()).second;
    auto assignedId = sourceId;
    if(extraPlacement)
    {
      if(nextCloneId == maximumId)
      {
        return MergeResults(std::move(importResult), MakeErrorResult(-6213, fmt::format("Cannot copy placement '{}' from '{}': identifier {} leaves no representable next identifier.", path.toString(),
                                                                                        m_H5FilePath.string(), nextCloneId)));
      }
      assignedId = nextCloneId++;
    }
    if(occupiedIds.contains(assignedId) || !assignedIds.insert(assignedId).second)
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6213, fmt::format("Cannot import '{}' from '{}': identifier {} conflicts with an existing or selected object.", path.toString(),
                                                                                      m_H5FilePath.string(), assignedId)));
    }
    placements.push_back({path, std::move(sourceObject), sourceId, assignedId, extraPlacement});
  }

  DataStructure preparedStructure = dataStructure;
  if(extraPlacementCount != 0)
  {
    preparedStructure.setNextId(sourceNextId);
  }
  std::unordered_map<const DataObject*, std::shared_ptr<DataObject>> firstStagedCopies;
  std::vector<DataPath> preparedPaths;
  preparedPaths.reserve(placements.size());
  for(const auto& placement : placements)
  {
    const auto& path = placement.path;
    if(preparedStructure.containsData(path))
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6203, fmt::format("Cannot import '{}' from '{}': the destination path already exists. Rename it or exclude this path.",
                                                                                      path.toString(), m_H5FilePath.string())));
    }
    auto* collision = preparedStructure.getData(placement.sourceId);
    if(placement.extraPlacement)
    {
      const auto first = firstStagedCopies.find(placement.source.get());
      if(first == firstStagedCopies.end() || collision != first->second.get() || first->second->getId() != placement.sourceId || preparedStructure.getNextId() != placement.assignedId)
      {
        return MergeResults(std::move(importResult),
                            MakeErrorResult(-6213, fmt::format("Cannot stage alias '{}' from '{}': source identifier {} or prepared next identifier {} does not match planned clone {}.",
                                                               path.toString(), m_H5FilePath.string(), placement.sourceId, preparedStructure.getNextId(), placement.assignedId)));
      }
    }
    else if(collision != nullptr)
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6213, fmt::format("Cannot stage '{}' from '{}': source identifier {} already belongs to another prepared object.", path.toString(),
                                                                                      m_H5FilePath.string(), placement.sourceId)));
    }
    auto shell = DataStructure::makeImportPublicationCopy(*placement.source);
    if(shell == nullptr)
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6202, fmt::format("Cannot prepare an independent copy of '{}' from '{}'.", path.toString(), m_H5FilePath.string())));
    }
    if(shell->getId() != placement.sourceId || shell.get() == collision)
    {
      return MergeResults(std::move(importResult),
                          MakeErrorResult(-6213, fmt::format("Cannot stage '{}' from '{}': detached copy identifier {} must equal source identifier {} and its owner must be independent.",
                                                             path.toString(), m_H5FilePath.string(), shell->getId(), placement.sourceId)));
    }
    // Only an exact known first copy can trigger insert's one-ID assignment in this unpublished structure.
    if(!preparedStructure.insert(shell, path.getParent()))
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6202, fmt::format("Cannot stage '{}' from '{}': parent '{}' is missing or rejects the object.", path.toString(),
                                                                                      m_H5FilePath.string(), path.getParent().toString())));
    }
    if(shell->getId() != placement.assignedId || preparedStructure.getData(path) != shell.get() || preparedStructure.getData(placement.assignedId) != shell.get())
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6213, fmt::format("Cannot stage '{}' from '{}': assigned identifier {} or its lookups do not match planned identifier {}.",
                                                                                      path.toString(), m_H5FilePath.string(), shell->getId(), placement.assignedId)));
    }
    if(!placement.extraPlacement)
    {
      firstStagedCopies.emplace(placement.source.get(), shell);
    }
    preparedPaths.push_back(path);
  }

  std::vector<DataPath> importedLeafPaths;
  importedLeafPaths.reserve(preparedPaths.size());
  // Policy sees all selected shells and the original destination context before ordinary values are read.
  for(const auto& path : preparedPaths)
  {
    auto* object = preparedStructure.getData(path);
    if(auto* array = dynamic_cast<IDataArray*>(object); array != nullptr)
    {
      const bool eagerStatistics = legacy && path.getLength() >= 2 && path[1] == "Statistics";
      auto planResult = applyPreflightStoragePlan(preparedStructure, path, *array, eagerStatistics);
      if(planResult.invalid())
      {
        return MergeResults(std::move(importResult), std::move(planResult));
      }
      auto& warnings = planResult.warnings();
      importResult.warnings().insert(importResult.warnings().end(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
      importedLeafPaths.push_back(path);
    }
    else if(dynamic_cast<INeighborList*>(object) != nullptr || dynamic_cast<StringArray*>(object) != nullptr)
    {
      importedLeafPaths.push_back(path);
    }
  }
  if(mode == Mode::Execute && !importedLeafPaths.empty())
  {
    auto fileReader = HDF5::FileIO::ReadFile(m_H5FilePath);
    if(!fileReader.isValid())
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6206, fmt::format("Cannot open '{}' to materialize the selected import.", m_H5FilePath.string())));
    }
    importResult = MergeResults(std::move(importResult), DREAM3D::detail::MaterializeImportedPaths(preparedStructure, fileReader, importedLeafPaths));
    if(importResult.invalid())
    {
      return importResult;
    }
  }

  std::vector<DataStructure::ImportPublicationRecord> records;
  records.reserve(preparedPaths.size());
  std::map<DataPath, std::shared_ptr<BaseGroup>> publicationGroups;
  auto reservedNextId = savedNextId;
  for(const auto& path : preparedPaths)
  {
    const auto object = preparedStructure.getSharedData(path);
    if(object == nullptr)
    {
      // Recovery may remove an invalid leaf with a warning. Preserve that partial-recovery contract.
      continue;
    }
    DataStructure::ImportPublicationRecord record;
    record.owner = DataStructure::makeImportPublicationCopy(*object);
    record.id = object->getId();
    record.path = path;
    if(record.owner == nullptr || record.id == 0 || record.id == maximumId || dataStructure.m_DataObjects.contains(record.id))
    {
      return MergeResults(std::move(importResult),
                          MakeErrorResult(-6213, fmt::format("Cannot publish '{}' from '{}': identifier {} or its prepared owner is invalid.", path.toString(), m_H5FilePath.string(), record.id)));
    }
    const auto parentPath = path.getParent();
    if(!parentPath.empty())
    {
      auto parent = publicationGroups.find(parentPath);
      record.parent = parent == publicationGroups.end() ? std::dynamic_pointer_cast<BaseGroup>(dataStructure.getSharedData(parentPath)) : parent->second;
      if(record.parent == nullptr)
      {
        return MergeResults(std::move(importResult), MakeErrorResult(-6202, fmt::format("Cannot publish '{}': destination parent '{}' is unavailable.", path.toString(), parentPath.toString())));
      }
      record.parentId = record.parent->getId();
    }
    record.removalMessage = std::make_shared<DataRemovedMessage>(&dataStructure, record.id, record.owner->getName());
    reservedNextId = std::max(reservedNextId, record.id + 1);
    if(auto group = std::dynamic_pointer_cast<BaseGroup>(record.owner); group != nullptr)
    {
      publicationGroups.emplace(path, std::move(group));
    }
    records.push_back(std::move(record));
  }
  publicationGroups.clear();
  if(records.empty())
  {
    return importResult;
  }

  // Publication owners, records, and removal messages are ready before the first destination mutation.
  dataStructure.m_NextId = reservedNextId;
  usize insertionIndex = 0;
  const auto retainFailure = [&]() {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    if(m_TestState != nullptr)
    {
      m_TestState->destination = &dataStructure;
      m_TestState->savedNextId = savedNextId;
      m_TestState->reservedNextId = reservedNextId;
      m_TestState->retainedRecords = std::move(records);
    }
#endif
  };
  const auto appendCleanupDiagnostics = [&]() {
    for(const auto& record : records)
    {
      if(record.cleanup != DataStructure::ImportCleanupStatus::Complete)
      {
        importResult.errors().push_back(
            {-6215, fmt::format("Import rollback retained '{}' (identifier {}) from '{}': {}. The destination ID counter remains advanced.", record.path.toString(), record.id, m_H5FilePath.string(),
                                record.cleanup == DataStructure::ImportCleanupStatus::Children ? "selected children remain attached" : "exact placement cleanup did not complete")});
      }
      if(record.notificationFailed)
      {
        importResult.errors().push_back({-6216, fmt::format("Import rollback removed '{}' (identifier {}), but a removal notification failed.", record.path.toString(), record.id)});
      }
    }
  };
  try
  {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    if(m_TestState != nullptr && m_TestState->fault == PublicationFault::BeforePublication)
    {
      m_TestState->consumed = true;
      m_TestState->fault = PublicationFault::None;
      throw std::runtime_error("injected import failure before publication");
    }
#endif
    for(; insertionIndex < records.size(); ++insertionIndex)
    {
      auto failAfter = DataStructure::ImportInsertionStage::None;
      bool rejectBeforeInsert = false;
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
      if(m_TestState != nullptr && insertionIndex == 1)
      {
        m_TestState->retainedOwner = records[insertionIndex].owner;
        if(m_TestState->fault == PublicationFault::AfterHierarchy)
        {
          failAfter = DataStructure::ImportInsertionStage::Hierarchy;
        }
        else if(m_TestState->fault == PublicationFault::BeforeTracking)
        {
          failAfter = DataStructure::ImportInsertionStage::Parent;
        }
        else if(m_TestState->fault == PublicationFault::ReturnFalse)
        {
          m_TestState->consumed = true;
          m_TestState->fault = PublicationFault::None;
          rejectBeforeInsert = true;
        }
      }
#endif
      if(!dataStructure.insertImportedObject(records[insertionIndex], failAfter, rejectBeforeInsert))
      {
        throw std::runtime_error("destination insertion rejected the prepared object");
      }
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
      if(m_TestState != nullptr && insertionIndex == 1 && (m_TestState->fault == PublicationFault::AfterSecondInsert || m_TestState->fault == PublicationFault::AllocationAfterSecond))
      {
        const bool allocation = m_TestState->fault == PublicationFault::AllocationAfterSecond;
        m_TestState->fault = PublicationFault::None;
        m_TestState->consumed = true;
        if(allocation)
        {
          throw std::bad_alloc();
        }
        throw std::runtime_error("injected import failure after second insertion");
      }
#endif
    }
  } catch(const std::bad_alloc&)
  {
    rollbackPublication(dataStructure, records, savedNextId, reservedNextId, true);
    retainFailure();
    throw;
  } catch(const std::exception& error)
  {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    if(m_TestState != nullptr && insertionIndex < records.size() &&
       ((m_TestState->fault == PublicationFault::AfterHierarchy && records[insertionIndex].stage == DataStructure::ImportInsertionStage::Hierarchy) ||
        (m_TestState->fault == PublicationFault::BeforeTracking && records[insertionIndex].stage == DataStructure::ImportInsertionStage::Parent)))
    {
      m_TestState->consumed = true;
      m_TestState->fault = PublicationFault::None;
    }
#endif
    rollbackPublication(dataStructure, records, savedNextId, reservedNextId, true);
    const auto& failed = records[std::min(insertionIndex, records.size() - 1)];
    importResult = MergeResults(std::move(importResult),
                                MakeErrorResult(-6214, fmt::format("Cannot publish '{}' (identifier {}) from '{}': {}", failed.path.toString(), failed.id, m_H5FilePath.string(), error.what())));
    appendCleanupDiagnostics();
    retainFailure();
    return importResult;
  } catch(...)
  {
    rollbackPublication(dataStructure, records, savedNextId, reservedNextId, true);
    importResult =
        MergeResults(std::move(importResult), MakeErrorResult(-6214, fmt::format("Cannot publish selected objects from '{}': an unknown insertion failure occurred.", m_H5FilePath.string())));
    appendCleanupDiagnostics();
    retainFailure();
    return importResult;
  }
  return importResult;
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
void ImportH5ObjectPathsAction::setPublicationFaultForTesting(PublicationFault fault, bool refuseCleanup) const
{
  m_TestState = std::make_shared<TestState>();
  m_TestState->fault = fault;
  m_TestState->refuseCleanup = refuseCleanup;
}

bool ImportH5ObjectPathsAction::publicationFaultConsumedForTesting() const
{
  return m_TestState != nullptr && m_TestState->consumed;
}

bool ImportH5ObjectPathsAction::cleanupFaultConsumedForTesting() const
{
  return m_TestState != nullptr && m_TestState->cleanupConsumed;
}

std::shared_ptr<DataObject> ImportH5ObjectPathsAction::retainedOwnerForTesting() const
{
  return m_TestState == nullptr ? nullptr : m_TestState->retainedOwner;
}

Result<> ImportH5ObjectPathsAction::cleanupFailedImportForTesting(DataStructure& destination) const
{
  if(m_TestState == nullptr || m_TestState->destination != &destination)
  {
    return MakeErrorResult(-6215, "The selected import has no retained rollback records for this destination.");
  }
  if(!rollbackPublication(destination, m_TestState->retainedRecords, m_TestState->savedNextId, m_TestState->reservedNextId, false))
  {
    return MakeErrorResult(-6215, "The selected import still has an exact-object cleanup conflict.");
  }
  return {};
}
#endif

IDataAction::UniquePointer ImportH5ObjectPathsAction::clone() const
{
  return std::make_unique<ImportH5ObjectPathsAction>(m_H5FilePath, m_Paths);
}

std::vector<DataPath> ImportH5ObjectPathsAction::getAllCreatedPaths() const
{
  return m_Paths;
}

} // namespace nx::core
