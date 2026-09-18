#include "RequireMinNumNeighbors.hpp"

#include "FillBadVoxels.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;

namespace
{
// One marking and renumbering transfer contains at most 65,536 Feature IDs.
constexpr usize k_ChunkTuples = 65536;
} // namespace

RequireMinNumNeighbors::RequireMinNumNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               RequireMinNumNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

RequireMinNumNeighbors::~RequireMinNumNeighbors() noexcept = default;

Result<> RequireMinNumNeighbors::operator()()
{
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto& numNeighbors = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumNeighborsPath)->getDataStoreRef();

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);
  usize totalPoints = imageGeom.getNumberOfCells();
  usize totalFeatures = numNeighbors.getNumberOfTuples();

  // Fail early when the selected phase has no feature. The feature scan uses
  // direct feature-level access and occurs before cell data changes.
  if(m_InputValues->ApplyToSinglePhase)
  {
    auto& featurePhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesPath)->getDataStoreRef();

    usize numFeatures = featurePhases.getNumberOfTuples();
    bool unavailablePhase = true;
    for(usize i = 0; i < numFeatures; i++)
    {
      if(featurePhases[i] == m_InputValues->PhaseNumber)
      {
        unavailablePhase = false;
        break;
      }
    }

    if(unavailablePhase)
    {
      std::string ss =
          fmt::format("The phase number ({}) is not available in the supplied Feature phases array with path ({})", m_InputValues->PhaseNumber, m_InputValues->FeaturePhasesPath.toString());
      return MakeErrorResult(-5555, ss);
    }
  }

  // Mark removed cells and compact surviving IDs in one cell pass.
  Error errorReturn = {0, ""};
  Result<std::vector<bool>> activeObjectsResult = removeFeaturesUnderNeighborThreshold(featureIds, numNeighbors, totalPoints, errorReturn);
  if(activeObjectsResult.invalid())
  {
    return ConvertResult(std::move(activeObjectsResult));
  }
  std::vector<bool> activeObjects = std::move(activeObjectsResult.value());
  if(errorReturn.code < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{errorReturn})};
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  auto numInactiveObjects = std::count(activeObjects.begin(), activeObjects.end(), false);
  m_MessageHandler({nx::core::IFilter::Message::Type::Info, fmt::format("Removing {} features", numInactiveObjects)});

  // Fill negative cells after compaction so filled cells inherit compact IDs.
  Result<> assignResult = assignBadVoxels(imageGeom.getDimensions(), totalFeatures);
  if(assignResult.invalid())
  {
    return assignResult;
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  int32 count = 0;
  for(const auto& value : activeObjects)
  {
    if(value)
    {
      count++;
    }
  }
  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Feature Count Changed: Previous: {} New: {}", totalFeatures, count));

  DataPath cellFeatureGroupPath = m_InputValues->NumNeighborsPath.getParent();
  // Cell IDs already use the shared compaction map. Skip another cell pass and
  // compact only the feature-level arrays.
  Result<> removeResult = nx::core::RemoveInactiveObjects(m_DataStructure, cellFeatureGroupPath, activeObjects, featureIds, totalFeatures, m_MessageHandler, m_ShouldCancel,
                                                          /*cellFeatureIdsRenumbered=*/true);
  if(removeResult.invalid())
  {
    return removeResult;
  }
  // RemoveInactiveObjects reports a cancelled compaction as success. No work follows this
  // call, so a cancelled and a completed run both leave through the empty valid Result below.

  return {};
}

Result<std::vector<bool>> RequireMinNumNeighbors::removeFeaturesUnderNeighborThreshold(Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& numNeighbors, usize totalPoints,
                                                                                       Error& errorReturn)
{
  usize totalFeatures = numNeighbors.getNumberOfTuples();
  std::vector<bool> activeObjects(totalFeatures, true);

  // A feature survives when it meets the threshold or is outside the selected
  // phase. At least one nonbackground feature must survive.
  bool valid = false;
  if(m_InputValues->ApplyToSinglePhase)
  {
    auto& featurePhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesPath)->getDataStoreRef();
    for(usize i = 1; i < totalFeatures; i++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      if(numNeighbors[i] >= m_InputValues->MinNumNeighbors || featurePhases[i] != m_InputValues->PhaseNumber)
      {
        valid = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
  }
  else
  {
    for(usize i = 1; i < totalFeatures; i++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      if(numNeighbors[i] >= m_InputValues->MinNumNeighbors)
      {
        valid = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
  }
  if(!valid)
  {
    errorReturn = Error{-55569, "The minimum number of neighbors is larger than the Feature with the most neighbors.  All Features would be removed"};
    return {std::move(activeObjects)};
  }

  // Use the same stable mapping that later compacts feature arrays.
  const FeatureRenumbering renumbering = ComputeFeatureRenumbering(activeObjects);
  const std::vector<size_t>& newNames = renumbering.newNames;

  // Fuse marking and renumbering because both operations require the same cell read. Write only chunks that change.
  auto featureIdBuf = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < totalPoints; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkTuples, totalPoints - offset);
    Result<> ioResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuf.get(), count));
    if(ioResult.invalid())
    {
      return ConvertInvalidResult<std::vector<bool>>(std::move(ioResult));
    }

    bool modified = false;
    for(usize i = 0; i < count; i++)
    {
      const int32 oldId = featureIdBuf[i];

      if(oldId >= 0 && static_cast<usize>(oldId) >= totalFeatures)
      {
        errorReturn = Error{
            -55567, fmt::format("Feature ID '{}' in array '{}' is outside the valid range [0, {}). The array may have been modified.", oldId, m_InputValues->FeatureIdsPath.toString(), totalFeatures)};
        return {std::move(activeObjects)};
      }

      const int32 newId = (oldId >= 0 && activeObjects[oldId]) ? static_cast<int32>(newNames[oldId]) : -1;
      if(newId != oldId)
      {
        featureIdBuf[i] = newId;
        modified = true;
      }
    }
    if(modified)
    {
      ioResult = featureIds.copyFromBuffer(offset, nonstd::span<const int32>(featureIdBuf.get(), count));
      if(ioResult.invalid())
      {
        return ConvertInvalidResult<std::vector<bool>>(std::move(ioResult));
      }
    }
  }
  return {std::move(activeObjects)};
}

Result<> RequireMinNumNeighbors::assignBadVoxels(SizeVec3 dimensions, usize totalFeatures)
{
  return FillBadVoxels(m_DataStructure, m_InputValues->FeatureIdsPath, dimensions, m_InputValues->IgnoredVoxelArrayPaths, totalFeatures, m_MessageHandler, m_ShouldCancel);
}
