#include "RequireMinNumNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{

struct UpdateDataFunctor
{
  template <typename T>
  Result<> operator()(const DataStructure& dataStructure, const DataPath& dataArrayPath, const std::vector<usize>& badFeatureIdIndexes, const AbstractDataStore<int32_t>& featureIds,
                      const std::vector<int32>& neighbors, const IFilter::MessageHandler& mesgHandler)
  {
    auto voxelArray = dataStructure.getDataRefAs<DataArray<T>>(dataArrayPath);
    auto arraySize = voxelArray.getSize();
    for(const auto& featureIdIndex : badFeatureIdIndexes)
    {
      int32 featureName = featureIds[featureIdIndex];
      int32 neighbor = neighbors[featureIdIndex];
      if((neighbor >= arraySize || featureIdIndex >= arraySize) && (featureName < 0 && neighbor >= 0 && featureIds[neighbor] >= 0))
      {
        std::string message =
            fmt::format("Out of range: While trying to copy a tuple from index {} to index {}\n  Array Name: {}\n  Num. Tuples: {}", neighbor, featureIdIndex, dataArrayPath.toString(), arraySize);
        mesgHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, message});
        return MakeErrorResult(-55568, message);
      }
      if(int32 fId = featureIds[neighbor]; featureName < 0 && neighbor >= 0 && fId >= 0)
      {
        voxelArray.copyTuple(neighbor, featureIdIndex);
      }
    }
    return {};
  }
};

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
  m_MessageHandler({nx::core::IFilter::Message::Type::Info, fmt::format("Removing {} features", numInactiveObjects)});

  // Mark all features to be removed with a -1 value.
  for(usize i = 0; i < totalPoints; i++)
  {
    int32 featureId = featureIds[i];
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

  int32 good = 1;
  int32 current = 0;
  int32 most = 0;
  int64 neighborPoint = 0;
  usize numFeatures = numNeighbors.getNumberOfTuples();

  int64 neighborPointIdx[6] = {0, 0, 0, 0, 0, 0};
  neighborPointIdx[0] = -dims[0] * dims[1];
  neighborPointIdx[1] = -dims[0];
  neighborPointIdx[2] = -1;
  neighborPointIdx[3] = 1;
  neighborPointIdx[4] = dims[0];
  neighborPointIdx[5] = dims[0] * dims[1];

  usize counter = 1;
  int64 voxelIndex = 0;
  int64 kStride = 0;
  int64 jStride = 0;
  int32 featureName = 0;
  int32 feature = 0;
  int32 neighbor = 0;
  std::vector<int32> n(numFeatures + 1, 0);
  std::vector<usize> badFeatureIdIndexes;

  while(counter != 0)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    counter = 0;
    badFeatureIdIndexes.clear();
    for(int64 k = 0; k < dims[2]; k++)
    {
      kStride = dims[0] * dims[1] * k;
      for(int64 j = 0; j < dims[1]; j++)
      {
        jStride = dims[0] * j;
        for(int64 i = 0; i < dims[0]; i++)
        {
          voxelIndex = kStride + jStride + i;
          featureName = featureIds[voxelIndex]; // Get the featureId value
          if(featureName < 0)                   // Was this voxel marked to be removed
          {
            badFeatureIdIndexes.push_back(voxelIndex);
            counter++;
            current = 0;
            most = 0;
            // Loop over the 6 face neighbors of the voxel
            for(int32 l = 0; l < 6; l++)
            {
              good = 1;
              neighborPoint = voxelIndex + neighborPointIdx[l];
              if(l == 0 && k == 0)
              {
                good = 0;
              }
              if(l == 5 && k == (dims[2] - 1))
              {
                good = 0;
              }
              if(l == 1 && j == 0)
              {
                good = 0;
              }
              if(l == 4 && j == (dims[1] - 1))
              {
                good = 0;
              }
              if(l == 2 && i == 0)
              {
                good = 0;
              }
              if(l == 3 && i == (dims[0] - 1))
              {
                good = 0;
              }
              if(good == 1)
              {
                feature = featureIds[neighborPoint];
                if(feature >= 0)
                {
                  n[feature]++;
                  current = n[feature];
                  if(current > most)
                  {
                    most = current;
                    neighbors[voxelIndex] = neighborPoint;
                  }
                }
              }
            }
            // Loop over the 6 face neighbors of the voxel
            for(int32 l = 0; l < 6; l++)
            {
              good = 1;
              neighborPoint = voxelIndex + neighborPointIdx[l];
              if(l == 0 && k == 0)
              {
                good = 0;
              }
              if(l == 5 && k == (dims[2] - 1))
              {
                good = 0;
              }
              if(l == 1 && j == 0)
              {
                good = 0;
              }
              if(l == 4 && j == (dims[1] - 1))
              {
                good = 0;
              }
              if(l == 2 && i == 0)
              {
                good = 0;
              }
              if(l == 3 && i == (dims[0] - 1))
              {
                good = 0;
              }
              if(good == 1)
              {
                feature = featureIds[neighborPoint];
                if(feature >= 0)
                {
                  n[feature] = 0;
                }
              }
            }
          }
          else if(featureName >= numFeatures)
          {
            std::string message = fmt::format("Error: Found a feature Id '{}' that is >= the number of features '{}' at voxel index X={},Y={},Z={}.", featureName, numFeatures, i, j, k);
            m_MessageHandler(nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, message});
            return MakeErrorResult(-55567, message);
          }
        }
      }
    }

    std::string message = fmt::format("{} voxels to update..", counter);
    m_MessageHandler(nx::core::IFilter::Message::Type::Info, message);

    // TODO: This can be parallelized much like NeighborOrientationCorrelation, just do not update the featureIds array during that section. Wait until everything is complete
    //  This next section finds the "FeatureIds" array and moves that array to the end of the list
    auto featureIdsIter = std::find(cellDataArrayPaths.begin(), cellDataArrayPaths.end(), m_InputValues->FeatureIdsPath);
    if(featureIdsIter != cellDataArrayPaths.end())
    {
      cellDataArrayPaths.erase(featureIdsIter);
      cellDataArrayPaths.push_back(m_InputValues->FeatureIdsPath);
    }
    for(const auto& cellArrayPath : cellDataArrayPaths)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      auto* voxelArray = m_DataStructure.getDataAs<IDataArray>(cellArrayPath);
      ExecuteDataFunction(UpdateDataFunctor{}, voxelArray->getDataType(), m_DataStructure, cellArrayPath, badFeatureIdIndexes, featureIds, neighbors, m_MessageHandler);
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

  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Feature Count Changed: Previous: {} New: {}", totalFeatures, count));
  DataPath cellFeatureGroupPath = m_InputValues->NumNeighborsPath.getParent();
  nx::core::RemoveInactiveObjects(m_DataStructure, cellFeatureGroupPath, activeObjects, featureIds, totalFeatures, m_MessageHandler, m_ShouldCancel);

  return {};
}
