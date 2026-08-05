#include "RequireMinNumNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_NoCoarseningProgress = -55572;
constexpr int32 k_CopyDestinationTupleOutOfRange = -55568;
constexpr int32 k_CopySourceTupleOutOfRange = -55573;

std::string ConvertDataPathsToString(const std::vector<DataPath>& paths)
{
  std::stringstream ss;
  ss << "[";
  for(usize i = 0; i < paths.size(); i++)
  {
    if(i != 0)
    {
      ss << ", ";
    }
    ss << paths[i].toString();
  }
  ss << "]";
  return ss.str();
}

Result<> CopyTupleFromArray(DataStructure& dataStructure, const DataPath& dataArrayPath, const std::vector<usize>& badFeatureIdIndexes, const AbstractDataStore<int32_t>& featureIds,
                            const std::vector<int32>& neighbors, const IFilter::MessageHandler& mesgHandler)
{
  auto& voxelArray = dataStructure.getDataRefAs<IDataArray>(dataArrayPath);
  const usize tupleCount = voxelArray.getNumberOfTuples();
  const usize featureIdTupleCount = featureIds.getNumberOfTuples();
  const usize neighborMapSize = neighbors.size();

  for(const usize featureIdIndex : badFeatureIdIndexes)
  {
    // Validate the destination before indexing featureIds or neighbors.
    if(featureIdIndex >= tupleCount || featureIdIndex >= featureIdTupleCount || featureIdIndex >= neighborMapSize)
    {
      const std::string message =
          fmt::format("Cannot copy into tuple index {} of array '{}'. The array contains {} tuples, the Feature Ids array contains {} tuples, and the neighbor map contains {} entries.",
                      featureIdIndex, dataArrayPath.toString(), tupleCount, featureIdTupleCount, neighborMapSize);
      mesgHandler.sendInfoMessage(message);
      return MakeErrorResult(k_CopyDestinationTupleOutOfRange, message);
    }

    const int32 featureName = featureIds.getValue(featureIdIndex);
    const int32 neighbor = neighbors[featureIdIndex];

    if(featureName >= 0 || neighbor < 0)
    {
      continue;
    }

    const usize neighborIndex = static_cast<usize>(neighbor);

    // Validate the source before reading it or passing it to copyTuple().
    if(neighborIndex >= tupleCount || neighborIndex >= featureIdTupleCount)
    {
      const std::string message = fmt::format("Cannot copy from tuple index {} to tuple index {} of array '{}'. The array contains {} tuples and the Feature Ids array contains {} tuples.",
                                              neighborIndex, featureIdIndex, dataArrayPath.toString(), tupleCount, featureIdTupleCount);
      mesgHandler.sendInfoMessage(message);
      return MakeErrorResult(k_CopySourceTupleOutOfRange, message);
    }

    if(featureIds.getValue(neighborIndex) >= 0)
    {
      voxelArray.copyTuple(neighborIndex, featureIdIndex);
    }
  }
  return {};
}
} // namespace

// -----------------------------------------------------------------------------
RequireMinNumNeighbors::RequireMinNumNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               RequireMinNumNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
RequireMinNumNeighbors::~RequireMinNumNeighbors() noexcept = default;

// -----------------------------------------------------------------------------
Result<> RequireMinNumNeighbors::operator()()
{
  // If running on a single phase, validate that the user has not entered a phase number
  // that is not in the system ; the filter would not crash otherwise, but the user should
  // be notified of unanticipated behavior ; this cannot be done in the dataCheck since
  // we don't have access to the data yet
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto& numNeighbors = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumNeighborsPath)->getDataStoreRef();

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeomPath);
  usize totalPoints = imageGeom.getNumberOfCells();
  usize totalFeatures = numNeighbors.getNumberOfTuples();

  // The Cell Attribute Matrix is the parent of the "Feature Ids" array. Always.
  DataPath cellDataAttrMatrixPath = m_InputValues->FeatureIdsPath.getParent();
  std::optional<std::vector<DataPath>> result = nx::core::GetAllChildDataPaths(m_DataStructure, cellDataAttrMatrixPath, DataObject::Type::DataArray, m_InputValues->IgnoredVoxelArrayPaths);
  if(!result.has_value())
  {
    return MakeErrorResult(-5556, fmt::format("Error fetching all Data Arrays from Attribute Matrix '{}'", cellDataAttrMatrixPath.toString()));
  }
  std::vector<DataPath> cellDataArrayPaths = result.value();

  // FeatureIds controls whether a voxel has been reassigned, so it must always be
  // updated even if it was included in the ignored-array selection. Keep it last
  // so the other cell arrays read the original FeatureIds during tuple copying.
  auto featureIdsIter = std::find(cellDataArrayPaths.begin(), cellDataArrayPaths.end(), m_InputValues->FeatureIdsPath);
  if(featureIdsIter != cellDataArrayPaths.end())
  {
    cellDataArrayPaths.erase(featureIdsIter);
  }
  cellDataArrayPaths.push_back(m_InputValues->FeatureIdsPath);

  // Run the algorithm.
  // This was checked up in the execute function (which is called before this function),
  // so if we got this far then all should be good with the return. We might get
  // an empty vector<> but that is OK.
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

  bool valid = false;
  std::vector<bool> activeObjects(totalFeatures, true);
  if(m_InputValues->ApplyToSinglePhase)
  {
    auto& featurePhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeaturePhasesPath)->getDataStoreRef();
    for(usize i = 1; i < totalFeatures; i++)
    {
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
    return MakeErrorResult(-55569, "The minimum number of neighbors is larger than the Feature with the most neighbors.  All Features would be removed");
  }
  if(m_ShouldCancel)
  {
    return {};
  }
  auto numInactiveObjects = std::count(activeObjects.begin(), activeObjects.end(), false);
  m_MessageHandler.sendInfoMessage(fmt::format("Removing {} features", numInactiveObjects));

  // Mark all features to be removed with a -1 value.
  for(usize i = 0; i < totalPoints; i++)
  {
    const int32 featureId = featureIds[i];

    if(featureId < 0)
    {
      continue;
    }

    if(static_cast<usize>(featureId) >= totalFeatures)
    {
      return MakeErrorResult(-55567, fmt::format("Feature ID '{}' in array '{}' is outside the valid range [0, {}). '{}' MAY HAVE BEEN MODIFIED.", featureId, m_InputValues->FeatureIdsPath.toString(),
                                                 totalFeatures, ConvertDataPathsToString(cellDataArrayPaths)));
    }

    if(!activeObjects[featureId])
    {
      featureIds[i] = -1;
    }
  }

  SizeVec3 udims = imageGeom.getDimensions();
  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  // Create a temp array to hold the neighbor values
  std::vector<int32> neighbors(featureIds.getNumberOfTuples(), -1);

  int32 current = 0;
  int32 most = 0;
  int64 neighborPoint = 0;

  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  usize counter = 1;
  int64 voxelIndex = 0;
  int64 kStride = 0;
  int64 jStride = 0;
  int32 featureName = 0;
  int32 feature = 0;
  std::vector<int32> voteCount(totalFeatures + 1, 0);
  std::vector<usize> badFeatureIdIndexes;

  while(counter != 0)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    counter = 0;
    bool madeFill = false;
    badFeatureIdIndexes.clear();
    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      kStride = dims[0] * dims[1] * zIdx;
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        jStride = dims[0] * yIdx;
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          voxelIndex = kStride + jStride + xIdx;
          featureName = featureIds[voxelIndex]; // Get the featureId value
          if(featureName < 0)                   // Was this voxel marked to be removed
          {
            badFeatureIdIndexes.push_back(voxelIndex);
            counter++;
            current = 0;
            most = 0;
            // Loop over the 6 face neighbors of the voxel
            const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }
              neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
              {
                feature = featureIds[neighborPoint];
                if(feature >= 0)
                {
                  voteCount[feature]++;
                  current = voteCount[feature];
                  if(current > most)
                  {
                    most = current;
                    neighbors[voxelIndex] = neighborPoint;
                    madeFill = true;
                  }
                }
              }
            }

            // Reset the vote counts touched by this voxel
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }

              const int64 resetNeighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
              const int32 resetFeature = featureIds[resetNeighborPoint];
              if(resetFeature >= 0)
              {
                voteCount[resetFeature] = 0;
              }
            }
          }
        }
      }
    }

    std::string message = fmt::format("{} voxels to update..", counter);
    m_MessageHandler.sendInfoMessage(message);

    if(counter != 0 && !madeFill)
    {
      return MakeErrorResult(
          k_NoCoarseningProgress,
          fmt::format("Unable to reassign {} cell(s) in Feature Ids array '{}' because none has a non-negative face neighbor. Ensure the array contains at least one cell assigned to a feature "
                      "that meets the minimum-neighbor requirement. THE FOLLOWING ARRAYS MAY HAVE BEEN MODIFIED: '{}'",
                      counter, m_InputValues->FeatureIdsPath.toString(), ConvertDataPathsToString(cellDataArrayPaths)));
    }

    // TODO: This can be parallelized much like NeighborOrientationCorrelation, just do not update the featureIds array during that section. Wait until everything is complete
    for(const auto& cellArrayPath : cellDataArrayPaths)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      auto copyResult = CopyTupleFromArray(m_DataStructure, cellArrayPath, badFeatureIdIndexes, featureIds, neighbors, m_MessageHandler);
      if(copyResult.invalid())
      {
        copyResult.warnings().push_back({-55574, fmt::format("THE FOLLOWING ARRAYS MAY HAVE BEEN MODIFIED: '{}'", ConvertDataPathsToString(cellDataArrayPaths))});
        return copyResult;
      }
    }
  }

  int32 count = 0;
  for(const auto& value : activeObjects)
  {
    if(value)
    {
      count++;
    }
  }

  m_MessageHandler.sendInfoMessage(fmt::format("Feature Count Changed: Previous: {} New: {}", totalFeatures, count));
  DataPath cellFeatureGroupPath = m_InputValues->NumNeighborsPath.getParent();
  if(!nx::core::RemoveInactiveObjects(m_DataStructure, cellFeatureGroupPath, activeObjects, featureIds, totalFeatures, m_MessageHandler, m_ShouldCancel))
  {
    return MakeErrorResult(
        -55570, fmt::format("Failed to remove inactive feature tuples from feature group '{}'. Check that its arrays match the tuple count of '{}'. THE FOLLOWING ARRAYS MAY HAVE BEEN MODIFIED: '{}'",
                            cellFeatureGroupPath.toString(), m_InputValues->NumNeighborsPath.toString(), ConvertDataPathsToString(cellDataArrayPaths)));
  }

  return {};
}
