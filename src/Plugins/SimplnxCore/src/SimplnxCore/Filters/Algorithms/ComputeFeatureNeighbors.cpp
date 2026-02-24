#include "ComputeFeatureNeighbors.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_DefaultNeighborListSize = 100;
}

// -----------------------------------------------------------------------------
ComputeFeatureNeighbors::ComputeFeatureNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ComputeFeatureNeighborsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureNeighbors::~ComputeFeatureNeighbors() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureNeighbors::operator()()
{
  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto& numNeighbors = m_DataStructure.getDataAs<Int32Array>(m_InputValues->NumberOfNeighborsPath)->getDataStoreRef();

  auto& neighborList = m_DataStructure.getDataRefAs<Int32NeighborList>(m_InputValues->NeighborListPath);
  auto& sharedSurfaceAreaList = m_DataStructure.getDataRefAs<Float32NeighborList>(m_InputValues->SharedSurfaceAreaListPath);

  auto* boundaryCells = m_InputValues->StoreBoundaryCells ? m_DataStructure.getDataAs<Int8Array>(m_InputValues->BoundaryCellsPath)->getDataStore() : nullptr;
  auto* surfaceFeatures = m_InputValues->StoreSurfaceFeatures ? m_DataStructure.getDataAs<BoolArray>(m_InputValues->SurfaceFeaturesPath)->getDataStore() : nullptr;

  usize totalPoints = featureIds.getNumberOfTuples();
  usize totalFeatures = numNeighbors.getNumberOfTuples();

  /* Ensure that we will be able to work with the user selected featureId Array */
  const auto [minFeatureId, maxFeatureId] = std::minmax_element(featureIds.begin(), featureIds.end());
  if(static_cast<usize>(*maxFeatureId) >= totalFeatures)
  {
    std::stringstream out;
    out << "Data Array " << m_InputValues->FeatureIdsPath.getTargetName() << " has a maximum value of " << *maxFeatureId << " which is greater than the "
        << " number of features from array " << m_InputValues->NumberOfNeighborsPath.getTargetName() << " which has " << totalFeatures << ". Did you select the "
        << " incorrect array for the 'FeatureIds' array?";
    return MakeErrorResult(-24500, out.str());
  }

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometryPath);
  SizeVec3 uDims = imageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(uDims[0]),
      static_cast<int64>(uDims[1]),
      static_cast<int64>(uDims[2]),
  };

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  int32 feature = 0;
  int32 nnum = 0;
  int8 onsurf = 0;

  std::vector<std::vector<int32>> neighborlist(totalFeatures);
  std::vector<std::vector<float>> neighborsurfacearealist(totalFeatures);

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();
  // Initialize the neighbor lists
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    throttledMessenger.sendThrottledMessage([&] { return fmt::format("Initializing Neighbor Lists || {:.2f}% Complete", CalculatePercentComplete(featureIdx, totalFeatures)); });

    if(m_ShouldCancel)
    {
      return {};
    }

    numNeighbors[featureIdx] = 0;
    neighborlist[featureIdx].resize(k_DefaultNeighborListSize);
    neighborsurfacearealist[featureIdx].assign(k_DefaultNeighborListSize, -1.0f);
    if(m_InputValues->StoreSurfaceFeatures && surfaceFeatures != nullptr)
    {
      surfaceFeatures->setValue(featureIdx, false);
    }
  }

  // Loop over all points to generate the neighbor lists
  for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
  {
    throttledMessenger.sendThrottledMessage([&] { return fmt::format("Determining Neighbor Lists || {:.2f}% Complete", CalculatePercentComplete(voxelIndex, totalPoints)); });

    if(m_ShouldCancel)
    {
      return {};
    }

    onsurf = 0;
    feature = featureIds[voxelIndex];
    if(feature > 0 && feature < neighborlist.size())
    {
      int64 xIdx = voxelIndex % dims[0];
      int64 yIdx = (voxelIndex / dims[0]) % dims[1];
      int64 zIdx = voxelIndex / (dims[0] * dims[1]);

      if(m_InputValues->StoreSurfaceFeatures && surfaceFeatures != nullptr)
      {
        if((xIdx == 0 || xIdx == dims[0] - 1 || yIdx == 0 || yIdx == dims[1] - 1 || zIdx == 0 || zIdx == dims[2] - 1) && dims[2] != 1)
        {
          surfaceFeatures->setValue(feature, true);
        }
        if((xIdx == 0 || xIdx == dims[0] - 1 || yIdx == 0 || yIdx == dims[1] - 1) && dims[2] == 1)
        {
          surfaceFeatures->setValue(feature, true);
        }
      }

      // Loop over the 6 face neighbors of the voxel
      std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
      for(const auto& faceIndex : faceNeighborInternalIdx)
      {
        if(!isValidFaceNeighbor[faceIndex])
        {
          continue;
        }

        const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

        if(featureIds[neighborPoint] != feature && featureIds[neighborPoint] > 0)
        {
          onsurf++;
          nnum = numNeighbors[feature];
          neighborlist[feature].push_back(featureIds[neighborPoint]);
          nnum++;
          numNeighbors[feature] = nnum;
        }
      }
    }
    if(m_InputValues->StoreBoundaryCells && boundaryCells != nullptr)
    {
      boundaryCells->setValue(voxelIndex, onsurf);
    }
  }

  FloatVec3 spacing = imageGeom.getSpacing();

  // We do this to create new set of NeighborList objects
  for(usize i = 1; i < totalFeatures; i++)
  {
    throttledMessenger.sendThrottledMessage([&] { return fmt::format("Calculating Surface Areas || {:.2f}% Complete", CalculatePercentComplete(i, totalFeatures)); });

    if(m_ShouldCancel)
    {
      return {};
    }

    std::map<int32, int32> neighToCount;
    auto neighborCount = static_cast<int32>(neighborlist[i].size());

    // this increments the voxel counts for each feature
    for(int32 j = 0; j < neighborCount; j++)
    {
      neighToCount[neighborlist[i][j]]++;
    }

    auto neighborIter = neighToCount.find(0);
    neighToCount.erase(neighborIter);
    neighborIter = neighToCount.find(-1);
    if(neighborIter != neighToCount.end())
    {
      neighToCount.erase(neighborIter);
    }
    // Resize the features neighbor list to zero
    neighborlist[i].resize(0);
    neighborsurfacearealist[i].resize(0);

    for(const auto [neigh, number] : neighToCount)
    {
      float area = static_cast<float>(number) * spacing[0] * spacing[1];

      // Push the neighbor feature identifier back onto the list, so we stay synced up
      neighborlist[i].push_back(neigh);
      neighborsurfacearealist[i].push_back(area);
    }
    numNeighbors[i] = static_cast<int32>(neighborlist[i].size());

    // Set the vector for each list into the NeighborList Object
    auto sharedNeiLst = std::make_shared<NeighborList<int32>::VectorType>();
    sharedNeiLst->assign(neighborlist[i].begin(), neighborlist[i].end());
    neighborList.setList(static_cast<int32>(i), sharedNeiLst);

    auto sharedSAL = std::make_shared<NeighborList<float32>::VectorType>();
    sharedSAL->assign(neighborsurfacearealist[i].begin(), neighborsurfacearealist[i].end());
    sharedSurfaceAreaList.setList(static_cast<int32>(i), sharedSAL);
  }

  return {};
}
