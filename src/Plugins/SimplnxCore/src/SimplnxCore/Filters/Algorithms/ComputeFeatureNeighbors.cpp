#include "ComputeFeatureNeighbors.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

#include <sstream>

using namespace nx::core;

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
  auto storeBoundaryCells = m_InputValues->StoreBoundaryCells;
  auto storeSurfaceFeatures = m_InputValues->StoreSurfaceFeatures;
  auto imageGeomPath = m_InputValues->InputImageGeometryPath;
  auto featureIdsPath = m_InputValues->FeatureIdsPath;
  auto boundaryCellsName = m_InputValues->BoundaryCellsName;
  auto numNeighborsName = m_InputValues->NumberOfNeighborsName;
  auto neighborListName = m_InputValues->NeighborListName;
  auto sharedSurfaceAreaName = m_InputValues->SharedSurfaceAreaListName;
  auto surfaceFeaturesName = m_InputValues->SurfaceFeaturesName;
  auto featureAttrMatrixPath = m_InputValues->CellFeatureArrayPath;

  DataPath boundaryCellsPath = featureIdsPath.replaceName(boundaryCellsName);
  DataPath numNeighborsPath = featureAttrMatrixPath.createChildPath(numNeighborsName);
  DataPath neighborListPath = featureAttrMatrixPath.createChildPath(neighborListName);
  DataPath sharedSurfaceAreaPath = featureAttrMatrixPath.createChildPath(sharedSurfaceAreaName);
  DataPath surfaceFeaturesPath = featureAttrMatrixPath.createChildPath(surfaceFeaturesName);

  auto& featureIds = m_DataStructure.getDataAs<Int32Array>(featureIdsPath)->getDataStoreRef();
  auto& numNeighbors = m_DataStructure.getDataAs<Int32Array>(numNeighborsPath)->getDataStoreRef();

  auto& neighborList = m_DataStructure.getDataRefAs<Int32NeighborList>(neighborListPath);
  auto& sharedSurfaceAreaList = m_DataStructure.getDataRefAs<Float32NeighborList>(sharedSurfaceAreaPath);

  auto* boundaryCells = storeBoundaryCells ? m_DataStructure.getDataAs<Int8Array>(boundaryCellsPath)->getDataStore() : nullptr;
  auto* surfaceFeatures = storeSurfaceFeatures ? m_DataStructure.getDataAs<BoolArray>(surfaceFeaturesPath)->getDataStore() : nullptr;

  usize totalPoints = featureIds.getNumberOfTuples();
  usize totalFeatures = numNeighbors.getNumberOfTuples();

  /* Ensure that we will be able to work with the user selected featureId Array */
  const auto [minFeatureId, maxFeatureId] = std::minmax_element(featureIds.begin(), featureIds.end());
  if(static_cast<usize>(*maxFeatureId) >= totalFeatures)
  {
    std::stringstream out;
    out << "Data Array " << featureIdsPath.getTargetName() << " has a maximum value of " << *maxFeatureId << " which is greater than the "
        << " number of features from array " << numNeighborsPath.getTargetName() << " which has " << totalFeatures << ". Did you select the "
        << " incorrect array for the 'FeatureIds' array?";
    return MakeErrorResult(-24500, out.str());
  }

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
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
  uint8 onsurf = 0;

  std::vector<std::vector<int32>> neighborlist(totalFeatures);
  std::vector<std::vector<float>> neighborsurfacearealist(totalFeatures);

  int32 nListSize = 100;

  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();
  // Initialize the neighbor lists
  for(usize featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    auto now = std::chrono::steady_clock::now();
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Initializing Neighbor Lists || {:.2f}% Complete", CalculatePercentComplete(featureIdx, totalFeatures)); });

    if(m_ShouldCancel)
    {
      return {};
    }

    numNeighbors[featureIdx] = 0;
    neighborlist[featureIdx].resize(nListSize);
    neighborsurfacearealist[featureIdx].assign(nListSize, -1.0f);
    if(storeSurfaceFeatures && surfaceFeatures != nullptr)
    {
      surfaceFeatures->setValue(featureIdx, false);
    }
  }

  // Loop over all points to generate the neighbor lists
  for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
  {
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Determining Neighbor Lists || {:.2f}% Complete", CalculatePercentComplete(voxelIndex, totalPoints)); });

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

      if(storeSurfaceFeatures && surfaceFeatures != nullptr)
      {
        if((xIdx == 0 || xIdx == static_cast<int64>((dims[0] - 1)) || yIdx == 0 || yIdx == static_cast<int64>((dims[1]) - 1) || zIdx == 0 || zIdx == static_cast<int64>((dims[2] - 1))) && dims[2] != 1)
        {
          surfaceFeatures->setValue(feature, true);
        }
        if((xIdx == 0 || xIdx == static_cast<int64>((dims[0] - 1)) || yIdx == 0 || yIdx == static_cast<int64>((dims[1] - 1))) && dims[2] == 1)
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
    if(storeBoundaryCells && boundaryCells != nullptr)
    {
      boundaryCells->setValue(voxelIndex, static_cast<int32>(onsurf));
    }
  }

  FloatVec3 spacing = imageGeom.getSpacing();

  // We do this to create new set of NeighborList objects
  for(usize i = 1; i < totalFeatures; i++)
  {
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Calculating Surface Areas || {:.2f}% Complete", CalculatePercentComplete(i, totalFeatures)); });

    if(m_ShouldCancel)
    {
      return {};
    }

    std::map<int32, int32> neighToCount;
    auto numneighs = static_cast<int32>(neighborlist[i].size());

    // this increments the voxel counts for each feature
    for(int32 j = 0; j < numneighs; j++)
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
    NeighborList<int32>::SharedVectorType sharedNeiLst(new std::vector<int32>);
    sharedNeiLst->assign(neighborlist[i].begin(), neighborlist[i].end());
    neighborList.setList(static_cast<int32>(i), sharedNeiLst);

    NeighborList<float32>::SharedVectorType sharedSAL(new std::vector<float32>);
    sharedSAL->assign(neighborsurfacearealist[i].begin(), neighborsurfacearealist[i].end());
    sharedSurfaceAreaList.setList(static_cast<int32>(i), sharedSAL);
  }

  return {};
}
