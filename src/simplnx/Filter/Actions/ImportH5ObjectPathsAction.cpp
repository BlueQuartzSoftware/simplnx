#include "ImportH5ObjectPathsAction.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataMap.hpp"
#include <utility>

#include "simplnx/Common/StringLiteralFormatting.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGeometry.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
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

#include <fmt/format.h>

#include <cstdio>

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
/**
 * @struct PlacementPlan
 * @brief Records each selected path's source owner and reserved destination ID.
 */
struct PlacementPlan
{
  DataPath Path;
  std::shared_ptr<DataObject> SourcePtr;
  DataObject::IdType SourceId = 0;
  DataObject::IdType AssignedId = 0;
  bool ExtraPlacement = false;
};

/**
 * @struct PlacementPathLess
 * @brief Orders path components so all descendants of one path form a contiguous range.
 */
struct PlacementPathLess
{
  bool operator()(const DataPath& left, const DataPath& right) const
  {
    const auto commonLength = std::min(left.getLength(), right.getLength());
    for(usize component = 0; component < commonLength; ++component)
    {
      if(left[component] != right[component])
      {
        return left[component] < right[component];
      }
    }
    return left.getLength() < right.getLength();
  }
};

bool IsStrictDescendant(const DataPath& candidate, const DataPath& parent)
{
  if(candidate.getLength() <= parent.getLength())
  {
    return false;
  }
  for(usize component = 0; component < parent.getLength(); ++component)
  {
    if(candidate[component] != parent[component])
    {
      return false;
    }
  }
  return true;
}

/**
 * @class GeometryReferenceRemapper
 * @brief Maps geometry references through selected placement paths without reading array values.
 *
 * The finalized placement vector must outlive this index and remain unchanged.
 */
class GeometryReferenceRemapper
{
public:
  /**
   * @brief Indexes the complete selection after all placement IDs are assigned.
   * @param placements Supplies stable entries that must outlive this index.
   */
  explicit GeometryReferenceRemapper(const std::vector<PlacementPlan>& placements)
  {
    m_PlacementsBySourceId.reserve(placements.size());
    for(const auto& placement : placements)
    {
      m_PlacementsByPath.emplace(placement.Path, &placement);
      m_PlacementsBySourceId[placement.SourceId].push_back(&placement);
    }
    for(auto& sourceEntry : m_PlacementsBySourceId)
    {
      auto& candidates = sourceEntry.second;
      std::ranges::sort(candidates, PlacementPathLess{}, &PlacementPlan::Path);
    }
  }

  /**
   * @brief Selects a local dependency without changing the external or ambiguous-reference policy.
   * @param geometryPath Identifies the exact geometry placement.
   * @param sourceId Supplies a retained reference after source-wide ID reassignment.
   * @return The direct-child or unique descendant ID, or the unchanged reference when neither rule selects a target.
   */
  DataObject::OptionalId mapReference(const DataPath& geometryPath, DataObject::OptionalId sourceId) const
  {
    if(!sourceId.has_value())
    {
      return sourceId;
    }
    const auto candidates = m_PlacementsBySourceId.find(*sourceId);
    if(candidates == m_PlacementsBySourceId.end())
    {
      return sourceId;
    }
    const auto direct = m_PlacementsByPath.find(geometryPath.createChildPath(candidates->second.front()->SourcePtr->getName()));
    if(direct != m_PlacementsByPath.end() && direct->second->SourceId == *sourceId)
    {
      return direct->second->AssignedId;
    }
    const auto& sourcePlacements = candidates->second;
    auto first = std::ranges::lower_bound(sourcePlacements, geometryPath, PlacementPathLess{}, &PlacementPlan::Path);
    if(first != sourcePlacements.end() && (*first)->Path == geometryPath)
    {
      ++first;
    }
    if(first == sourcePlacements.end() || !IsStrictDescendant((*first)->Path, geometryPath))
    {
      // A selected external target can be shared when this geometry has no selected local copy.
      return sourceId;
    }
    // Component ordering keeps descendants contiguous, so two candidates establish ambiguity without a scan.
    const auto second = std::next(first);
    if(second != sourcePlacements.end() && IsStrictDescendant((*second)->Path, geometryPath))
    {
      // An ID does not identify a preferred alias. Keep the canonical reference when local paths are ambiguous.
      return sourceId;
    }
    return (*first)->AssignedId;
  }

private:
  std::map<DataPath, const PlacementPlan*, PlacementPathLess> m_PlacementsByPath;
  std::unordered_map<DataObject::IdType, std::vector<const PlacementPlan*>> m_PlacementsBySourceId;
};

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
 * @brief Applies one ID transformation to every serialized geometry reference.
 * @tparam Transform Maps an optional source ID to its retained or replacement ID.
 * @param geometry Receives the transformed references.
 * @param transform Preserves, clears, or replaces each reference without reading array values.
 */
template <typename Transform>
void TransformGeometryReferences(IGeometry& geometry, const Transform& transform)
{
  geometry.setElementSizesId(transform(geometry.getElementSizesId()));
  if(auto* gridPtr = dynamic_cast<IGridGeometry*>(&geometry); gridPtr != nullptr)
  {
    gridPtr->setCellData(transform(gridPtr->getCellDataId()));
  }
  if(auto* nodePtr = dynamic_cast<INodeGeometry0D*>(&geometry); nodePtr != nullptr)
  {
    nodePtr->setVertexListId(transform(nodePtr->getVertexListId()));
    nodePtr->setVertexDataId(transform(nodePtr->getVertexAttributeMatrixId()));
  }
  if(auto* nodePtr = dynamic_cast<INodeGeometry1D*>(&geometry); nodePtr != nullptr)
  {
    nodePtr->setEdgeListId(transform(nodePtr->getEdgeListId()));
    nodePtr->setEdgeDataId(transform(nodePtr->getEdgeAttributeMatrixId()));
    nodePtr->setElementContainingVertId(transform(nodePtr->getElementContainingVertId()));
    nodePtr->setElementNeighborsId(transform(nodePtr->getElementNeighborsId()));
    nodePtr->setElementCentroidsId(transform(nodePtr->getElementCentroidsId()));
  }
  if(auto* nodePtr = dynamic_cast<INodeGeometry2D*>(&geometry); nodePtr != nullptr)
  {
    nodePtr->setFaceListId(transform(nodePtr->getFaceListId()));
    nodePtr->setFaceDataId(transform(nodePtr->getFaceAttributeMatrixId()));
    nodePtr->setUnsharedEdgesId(transform(nodePtr->getUnsharedEdgesId()));
  }
  if(auto* nodePtr = dynamic_cast<INodeGeometry3D*>(&geometry); nodePtr != nullptr)
  {
    nodePtr->setPolyhedronListId(transform(nodePtr->getPolyhedronListId()));
    nodePtr->setPolyhedraDataId(transform(nodePtr->getPolyhedraAttributeMatrixId()));
    nodePtr->setUnsharedFacedId(transform(nodePtr->getUnsharedFacesId()));
  }
  if(auto* rectGridPtr = dynamic_cast<RectGridGeom*>(&geometry); rectGridPtr != nullptr)
  {
    rectGridPtr->setXBoundsId(transform(rectGridPtr->getXBoundsId()));
    rectGridPtr->setYBoundsId(transform(rectGridPtr->getYBoundsId()));
    rectGridPtr->setZBoundsId(transform(rectGridPtr->getZBoundsId()));
  }
}

/**
 * @brief Clears geometry references to objects outside the exact import selection.
 * @param sourceStructure Owns the imported object copies.
 * @param paths Specifies the objects that the action inserts.
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
    TransformGeometryReferences(*geometry, selectedReference);
  }
}
} // namespace

namespace nx::core
{
struct ImportH5ObjectPathsAction::TestState
{
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  PublicationFault Fault = PublicationFault::None;
  bool Consumed = false;
  bool RefuseCleanup = false;
  bool CleanupConsumed = false;
  std::shared_ptr<DataObject> RetainedOwner;
  std::vector<DataStructure::ImportPublicationRecord> RetainedRecords;
  DataStructure* Destination = nullptr;
  DataObject::IdType SavedNextId = 0;
  DataObject::IdType ReservedNextId = 0;
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
    if(record.Owner == nullptr)
    {
      continue;
    }
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    if(allowRefusal && m_TestState != nullptr && m_TestState->RefuseCleanup && index == 2 && record.Stage != DataStructure::ImportInsertionStage::None)
    {
      m_TestState->CleanupConsumed = true;
      record.Cleanup = DataStructure::ImportCleanupStatus::Refused;
      complete = false;
      continue;
    }
#endif
    record.Cleanup = destination.rollbackImportedObject(record);
    complete = complete && record.Cleanup == DataStructure::ImportCleanupStatus::Complete;
  }
  for(auto iter = records.rbegin(); iter != records.rend(); ++iter)
  {
    auto& record = *iter;
    if(record.Cleanup == DataStructure::ImportCleanupStatus::Complete && record.NotifyRemoval && !record.NotificationAttempted)
    {
      record.NotificationAttempted = true;
      try
      {
        destination.notify(record.RemovalMessage);
      } catch(...)
      {
        record.NotificationFailed = true;
      }
    }
  }
  for(auto& record : records)
  {
    if(record.Cleanup == DataStructure::ImportCleanupStatus::Complete)
    {
      record.Owner.reset();
      record.Parent.reset();
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
    const auto* objectPtr = sourceStructure.getData(path);
    if(objectPtr == nullptr)
    {
      continue;
    }
    if(!countedSources.insert(objectPtr).second)
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
    const auto* mapPtr = pendingMaps.back();
    pendingMaps.pop_back();
    for(const auto& [id, owner] : *mapPtr)
    {
      occupiedIds.insert(id);
      if(owner == nullptr)
      {
        continue;
      }
      occupiedIds.insert(owner->getId());
      if(visitedObjects.insert(owner.get()).second)
      {
        if(const auto* groupPtr = dynamic_cast<const BaseGroup*>(owner.get()); groupPtr != nullptr)
        {
          pendingMaps.push_back(&groupPtr->getDataMap());
        }
      }
    }
  }

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
    placements.push_back({.Path = path, .SourcePtr = std::move(sourceObject), .SourceId = sourceId, .AssignedId = assignedId, .ExtraPlacement = extraPlacement});
  }

  const GeometryReferenceRemapper referenceRemapper(placements);

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
    const auto& path = placement.Path;
    if(preparedStructure.containsData(path))
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6203, fmt::format("Cannot import '{}' from '{}': the destination path already exists. Rename it or exclude this path.",
                                                                                      path.toString(), m_H5FilePath.string())));
    }
    auto* collisionPtr = preparedStructure.getData(placement.SourceId);
    if(placement.ExtraPlacement)
    {
      const auto first = firstStagedCopies.find(placement.SourcePtr.get());
      if(first == firstStagedCopies.end() || collisionPtr != first->second.get() || first->second->getId() != placement.SourceId || preparedStructure.getNextId() != placement.AssignedId)
      {
        return MergeResults(std::move(importResult),
                            MakeErrorResult(-6213, fmt::format("Cannot stage alias '{}' from '{}': source identifier {} or prepared next identifier {} does not match planned clone {}.",
                                                               path.toString(), m_H5FilePath.string(), placement.SourceId, preparedStructure.getNextId(), placement.AssignedId)));
      }
    }
    else if(collisionPtr != nullptr)
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6213, fmt::format("Cannot stage '{}' from '{}': source identifier {} already belongs to another prepared object.", path.toString(),
                                                                                      m_H5FilePath.string(), placement.SourceId)));
    }
    auto shell = DataStructure::makeImportPublicationCopy(*placement.SourcePtr);
    if(shell == nullptr)
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6202, fmt::format("Cannot prepare an independent copy of '{}' from '{}'.", path.toString(), m_H5FilePath.string())));
    }
    if(shell->getId() != placement.SourceId || shell.get() == collisionPtr)
    {
      return MergeResults(std::move(importResult),
                          MakeErrorResult(-6213, fmt::format("Cannot stage '{}' from '{}': detached copy identifier {} must equal source identifier {} and its owner must be independent.",
                                                             path.toString(), m_H5FilePath.string(), shell->getId(), placement.SourceId)));
    }
    if(auto* geometryPtr = dynamic_cast<IGeometry*>(shell.get()); geometryPtr != nullptr)
    {
      TransformGeometryReferences(*geometryPtr, [&](DataObject::OptionalId referenceId) { return referenceRemapper.mapReference(path, referenceId); });
    }
    // Only an exact known first copy can trigger insert's one-ID assignment in this unpublished structure.
    if(!preparedStructure.insert(shell, path.getParent()))
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6202, fmt::format("Cannot stage '{}' from '{}': parent '{}' is missing or rejects the object.", path.toString(),
                                                                                      m_H5FilePath.string(), path.getParent().toString())));
    }
    if(shell->getId() != placement.AssignedId || preparedStructure.getData(path) != shell.get() || preparedStructure.getData(placement.AssignedId) != shell.get())
    {
      return MergeResults(std::move(importResult), MakeErrorResult(-6213, fmt::format("Cannot stage '{}' from '{}': assigned identifier {} or its lookups do not match planned identifier {}.",
                                                                                      path.toString(), m_H5FilePath.string(), shell->getId(), placement.AssignedId)));
    }
    if(!placement.ExtraPlacement)
    {
      firstStagedCopies.emplace(placement.SourcePtr.get(), shell);
    }
    preparedPaths.push_back(path);
  }

  std::vector<DataPath> importedLeafPaths;
  importedLeafPaths.reserve(preparedPaths.size());
  // Policy sees all selected shells and the original destination context before ordinary values are read.
  for(const auto& path : preparedPaths)
  {
    auto* objectPtr = preparedStructure.getData(path);
    if(auto* arrayPtr = dynamic_cast<IDataArray*>(objectPtr); arrayPtr != nullptr)
    {
      const bool eagerStatistics = legacy && path.getLength() >= 2 && path[1] == "Statistics";
      auto planResult = applyPreflightStoragePlan(preparedStructure, path, *arrayPtr, eagerStatistics);
      if(planResult.invalid())
      {
        return MergeResults(std::move(importResult), std::move(planResult));
      }
      auto& warnings = planResult.warnings();
      importResult.warnings().insert(importResult.warnings().end(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
      importedLeafPaths.push_back(path);
    }
    else if(dynamic_cast<INeighborList*>(objectPtr) != nullptr || dynamic_cast<StringArray*>(objectPtr) != nullptr)
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
    record.Owner = DataStructure::makeImportPublicationCopy(*object);
    record.Id = object->getId();
    record.Path = path;
    if(record.Owner == nullptr || record.Id == 0 || record.Id == maximumId || dataStructure.m_DataObjects.contains(record.Id))
    {
      return MergeResults(std::move(importResult),
                          MakeErrorResult(-6213, fmt::format("Cannot publish '{}' from '{}': identifier {} or its prepared owner is invalid.", path.toString(), m_H5FilePath.string(), record.Id)));
    }
    const auto parentPath = path.getParent();
    if(!parentPath.empty())
    {
      auto parent = publicationGroups.find(parentPath);
      record.Parent = parent == publicationGroups.end() ? std::dynamic_pointer_cast<BaseGroup>(dataStructure.getSharedData(parentPath)) : parent->second;
      if(record.Parent == nullptr)
      {
        return MergeResults(std::move(importResult), MakeErrorResult(-6202, fmt::format("Cannot publish '{}': destination parent '{}' is unavailable.", path.toString(), parentPath.toString())));
      }
      record.ParentId = record.Parent->getId();
    }
    record.RemovalMessage = std::make_shared<DataRemovedMessage>(&dataStructure, record.Id, record.Owner->getName());
    reservedNextId = std::max(reservedNextId, record.Id + 1);
    if(auto group = std::dynamic_pointer_cast<BaseGroup>(record.Owner); group != nullptr)
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
      m_TestState->Destination = &dataStructure;
      m_TestState->SavedNextId = savedNextId;
      m_TestState->ReservedNextId = reservedNextId;
      m_TestState->RetainedRecords = std::move(records);
    }
#endif
  };
  const auto appendCleanupDiagnostics = [&]() {
    for(const auto& record : records)
    {
      if(record.Cleanup != DataStructure::ImportCleanupStatus::Complete)
      {
        importResult.errors().push_back(
            {.code = -6215,
             .message =
                 fmt::format("Import rollback retained '{}' (identifier {}) from '{}': {}. The destination ID counter remains advanced.", record.Path.toString(), record.Id, m_H5FilePath.string(),
                             record.Cleanup == DataStructure::ImportCleanupStatus::Children ? "selected children remain attached" : "exact placement cleanup did not complete")});
      }
      if(record.NotificationFailed)
      {
        importResult.errors().push_back({.code = -6216, .message = fmt::format("Import rollback removed '{}' (identifier {}), but a removal notification failed.", record.Path.toString(), record.Id)});
      }
    }
  };
  try
  {
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
    if(m_TestState != nullptr && m_TestState->Fault == PublicationFault::BeforePublication)
    {
      m_TestState->Consumed = true;
      m_TestState->Fault = PublicationFault::None;
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
        m_TestState->RetainedOwner = records[insertionIndex].Owner;
        if(m_TestState->Fault == PublicationFault::AfterHierarchy)
        {
          failAfter = DataStructure::ImportInsertionStage::Hierarchy;
        }
        else if(m_TestState->Fault == PublicationFault::BeforeTracking)
        {
          failAfter = DataStructure::ImportInsertionStage::Parent;
        }
        else if(m_TestState->Fault == PublicationFault::ReturnFalse)
        {
          m_TestState->Consumed = true;
          m_TestState->Fault = PublicationFault::None;
          rejectBeforeInsert = true;
        }
      }
#endif
      if(!dataStructure.insertImportedObject(records[insertionIndex], failAfter, rejectBeforeInsert))
      {
        throw std::runtime_error("destination insertion rejected the prepared object");
      }
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
      if(m_TestState != nullptr && insertionIndex == 1 && (m_TestState->Fault == PublicationFault::AfterSecondInsert || m_TestState->Fault == PublicationFault::AllocationAfterSecond))
      {
        const bool allocation = m_TestState->Fault == PublicationFault::AllocationAfterSecond;
        m_TestState->Fault = PublicationFault::None;
        m_TestState->Consumed = true;
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
       ((m_TestState->Fault == PublicationFault::AfterHierarchy && records[insertionIndex].Stage == DataStructure::ImportInsertionStage::Hierarchy) ||
        (m_TestState->Fault == PublicationFault::BeforeTracking && records[insertionIndex].Stage == DataStructure::ImportInsertionStage::Parent)))
    {
      m_TestState->Consumed = true;
      m_TestState->Fault = PublicationFault::None;
    }
#endif
    rollbackPublication(dataStructure, records, savedNextId, reservedNextId, true);
    const auto& failed = records[std::min(insertionIndex, records.size() - 1)];
    importResult = MergeResults(std::move(importResult),
                                MakeErrorResult(-6214, fmt::format("Cannot publish '{}' (identifier {}) from '{}': {}", failed.Path.toString(), failed.Id, m_H5FilePath.string(), error.what())));
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
  m_TestState->Fault = fault;
  m_TestState->RefuseCleanup = refuseCleanup;
}

bool ImportH5ObjectPathsAction::publicationFaultConsumedForTesting() const
{
  return m_TestState != nullptr && m_TestState->Consumed;
}

bool ImportH5ObjectPathsAction::cleanupFaultConsumedForTesting() const
{
  return m_TestState != nullptr && m_TestState->CleanupConsumed;
}

std::shared_ptr<DataObject> ImportH5ObjectPathsAction::retainedOwnerForTesting() const
{
  return m_TestState == nullptr ? nullptr : m_TestState->RetainedOwner;
}

Result<> ImportH5ObjectPathsAction::cleanupFailedImportForTesting(DataStructure& destination) const
{
  if(m_TestState == nullptr || m_TestState->Destination != &destination)
  {
    return MakeErrorResult(-6215, "The selected import has no retained rollback records for this destination.");
  }
  if(!rollbackPublication(destination, m_TestState->RetainedRecords, m_TestState->SavedNextId, m_TestState->ReservedNextId, false))
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
