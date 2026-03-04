#include "IdentifySampleBFS.hpp"

#include "IdentifySample.hpp"
#include "IdentifySampleCommon.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

namespace
{
// BFS flood-fill algorithm for identifying the largest connected component.
// Uses std::vector<bool> (1 bit per voxel) for minimal memory overhead.
// Fast for in-core data where random access is O(1), but causes chunk
// thrashing in OOC mode due to BFS visiting neighbors across chunk boundaries.
struct IdentifySampleBFSFunctor
{
  template <typename T>
  void operator()(const ImageGeom* imageGeom, IDataArray* goodVoxelsPtr, bool fillHoles, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    auto& goodVoxels = goodVoxelsPtr->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const auto totalPoints = static_cast<int64>(goodVoxelsPtr->getNumberOfTuples());

    SizeVec3 udims = imageGeom->getDimensions();

    std::array<int64, 3> dims = {
        static_cast<int64>(udims[0]),
        static_cast<int64>(udims[1]),
        static_cast<int64>(udims[2]),
    };

    int64 neighborPoint = 0;
    std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
    std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

    std::vector<int64> currentVList;
    std::vector<bool> checked(totalPoints, false);
    std::vector<bool> sample(totalPoints, false);
    int64 biggestBlock = 0;

    // Find the largest contiguous set of good voxels using BFS flood-fill
    float threshold = 0.0f;
    for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
    {
      if(shouldCancel)
      {
        return;
      }
      const float percentIncrement = static_cast<float>(voxelIndex) / static_cast<float>(totalPoints) * 100.0f;
      if(percentIncrement > threshold)
      {
        messageHandler(IFilter::Message::Type::Info, fmt::format("Completed: {}", percentIncrement));
        threshold = threshold + 5.0f;
        if(threshold < percentIncrement)
        {
          threshold = percentIncrement;
        }
      }

      if(!checked[voxelIndex] && goodVoxels.getValue(voxelIndex))
      {
        currentVList.push_back(voxelIndex);
        usize count = 0;
        while(count < currentVList.size())
        {
          int64 index = currentVList[count];
          int64 xIdx = index % dims[0];
          int64 yIdx = (index / dims[0]) % dims[1];
          int64 zIdx = index / (dims[0] * dims[1]);
          std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }
            neighborPoint = index + neighborVoxelIndexOffsets[faceIndex];

            if(!checked[neighborPoint] && goodVoxels.getValue(neighborPoint))
            {
              currentVList.push_back(neighborPoint);
              checked[neighborPoint] = true;
            }
          }
          count++;
        }
        if(static_cast<int64>(currentVList.size()) >= biggestBlock)
        {
          biggestBlock = currentVList.size();
          sample.assign(totalPoints, false);
          for(int64 j = 0; j < biggestBlock; j++)
          {
            sample[currentVList[j]] = true;
          }
        }
        currentVList.clear();
      }
    }
    for(int64 i = 0; i < totalPoints; i++)
    {
      if(!sample[i] && goodVoxels.getValue(i))
      {
        goodVoxels.setValue(i, false);
      }
    }
    sample.clear();
    checked.assign(totalPoints, false);

    // Fill holes: flip bad voxels that are fully enclosed by the sample
    threshold = 0.0F;
    if(fillHoles)
    {
      messageHandler(IFilter::Message::Type::Info, fmt::format("Filling holes in sample..."));

      bool touchesBoundary = false;
      for(int64 voxelIndex = 0; voxelIndex < totalPoints; voxelIndex++)
      {
        if(shouldCancel)
        {
          return;
        }
        const float percentIncrement = static_cast<float>(voxelIndex) / static_cast<float>(totalPoints) * 100.0f;
        if(percentIncrement > threshold)
        {
          threshold = threshold + 5.0f;
          if(threshold < percentIncrement)
          {
            threshold = percentIncrement;
          }
        }

        if(!checked[voxelIndex] && !goodVoxels.getValue(voxelIndex))
        {
          currentVList.push_back(voxelIndex);
          usize count = 0;
          touchesBoundary = false;
          while(count < currentVList.size())
          {
            int64 index = currentVList[count];
            int64 xIdx = index % dims[0];
            int64 yIdx = (index / dims[0]) % dims[1];
            int64 zIdx = index / (dims[0] * dims[1]);
            if(xIdx == 0 || xIdx == (dims[0] - 1) || yIdx == 0 || yIdx == (dims[1] - 1) || zIdx == 0 || zIdx == (dims[2] - 1))
            {
              touchesBoundary = true;
            }
            std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }
              neighborPoint = index + neighborVoxelIndexOffsets[faceIndex];

              if(!checked[neighborPoint] && !goodVoxels.getValue(neighborPoint))
              {
                currentVList.push_back(neighborPoint);
                checked[neighborPoint] = true;
              }
            }
            count++;
          }
          if(!touchesBoundary)
          {
            for(int64 j : currentVList)
            {
              goodVoxels.setValue(j, true);
            }
          }
          currentVList.clear();
        }
      }
    }
    checked.clear();
  }
};
} // namespace

// -----------------------------------------------------------------------------
IdentifySampleBFS::IdentifySampleBFS(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const IdentifySampleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
IdentifySampleBFS::~IdentifySampleBFS() noexcept = default;

// -----------------------------------------------------------------------------
Result<> IdentifySampleBFS::operator()()
{
  auto* inputData = m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath);
  const auto* imageGeom = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->InputImageGeometryPath);

  if(m_InputValues->SliceBySlice)
  {
    ExecuteDataFunction(IdentifySampleSliceBySliceFunctor{}, inputData->getDataType(), imageGeom, inputData, m_InputValues->FillHoles,
                        static_cast<IdentifySampleSliceBySliceFunctor::Plane>(m_InputValues->SliceBySlicePlaneIndex), m_MessageHandler, m_ShouldCancel);
  }
  else
  {
    ExecuteDataFunction(IdentifySampleBFSFunctor{}, inputData->getDataType(), imageGeom, inputData, m_InputValues->FillHoles, m_MessageHandler, m_ShouldCancel);
  }

  return {};
}
