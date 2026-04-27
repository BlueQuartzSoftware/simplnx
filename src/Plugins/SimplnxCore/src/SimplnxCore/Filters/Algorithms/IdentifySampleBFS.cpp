#include "IdentifySampleBFS.hpp"

#include "IdentifySample.hpp"
#include "IdentifySampleCommon.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;

namespace
{
template <detail::ImageDimensionality ImageDimsStateT>
struct IdentifySampleFunctor
{
  template <typename T>
  void operator()(const ImageGeom* imageGeom, IDataArray* goodVoxelsPtr, bool fillHoles, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
  {
    constexpr FaceNeighborType k_NeighborCount = VoxelNeighbors<ImageDimsStateT>::k_FaceNeighborCount;

    MessageHelper messageHelper(messageHandler);
    ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

    ShapeType cDims = {1};
    auto& goodVoxels = goodVoxelsPtr->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const auto totalPoints = static_cast<int64>(goodVoxelsPtr->getNumberOfTuples());

    SizeVec3 udims = imageGeom->getDimensions();

    std::array<int64_t, 3> dims = {
        static_cast<int64_t>(udims[0]),
        static_cast<int64_t>(udims[1]),
        static_cast<int64_t>(udims[2]),
    };

    int64_t neighborPoint = 0;
    const std::array<int64, k_NeighborCount> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets<ImageDimsStateT>(dims);
    constexpr std::array<FaceNeighborType, k_NeighborCount> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx<ImageDimsStateT>();

    std::vector<int64> currentVList;
    std::vector<bool> checked(totalPoints, false);
    std::vector<bool> sample(totalPoints, false);
    int64 biggestBlock = 0;

    // In this loop over the data we are finding the biggest contiguous set of GoodVoxels and calling that the 'sample'  All GoodVoxels that do not touch the 'sample'
    // are flipped to be called 'bad' voxels or 'not sample'
    for(int64 zLoopIdx = 0; zLoopIdx < dims[2]; zLoopIdx++)
    {
      const int64 zStride = dims[0] * dims[1] * zLoopIdx;
      for(int64 yLoopIdx = 0; yLoopIdx < dims[1]; yLoopIdx++)
      {
        const int64 yStride = dims[0] * yLoopIdx;
        throttledMessenger.sendThrottledMessage([&] { return fmt::format("Identifying potential samples || {:.2f}% Complete", CalculatePercentComplete(zStride + yStride, totalPoints)); });
        if(shouldCancel)
        {
          return;
        }
        for(int64 xLoopIdx = 0; xLoopIdx < dims[0]; xLoopIdx++)
        {
          int64 voxelIndex = zStride + yStride + xLoopIdx;

          if(!checked[voxelIndex] && goodVoxels.getValue(voxelIndex))
          {
            currentVList.push_back(voxelIndex);
            usize count = 0;
            while(count < currentVList.size())
            {
              int64 index = currentVList[count];
              int64 xIndex = index % dims[0];
              int64 yIndex = (index / dims[0]) % dims[1];
              int64 zIndex = index / (dims[0] * dims[1]);
              const std::array<bool, k_NeighborCount> isValidFaceNeighbor = computeValidFaceNeighbors<ImageDimsStateT>(xIndex, yIndex, zIndex, dims);
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

    // In this loop we are going to 'close' all the 'holes' inside the region already identified as the 'sample' if the user chose to do so.
    // This is done by flipping all 'bad' voxel features that do not touch the outside of the sample (i.e. they are fully contained inside the 'sample').
    if(fillHoles)
    {
      messageHelper.sendMessage("Filling holes in sample...");
      for(int64 zLoopIdx = 0; zLoopIdx < dims[2]; zLoopIdx++)
      {
        const int64 zStride = dims[0] * dims[1] * zLoopIdx;
        for(int64 yLoopIdx = 0; yLoopIdx < dims[1]; yLoopIdx++)
        {
          const int64 yStride = dims[0] * yLoopIdx;
          throttledMessenger.sendThrottledMessage([&] { return fmt::format("Identifying potential samples || {:.2f}% Complete", CalculatePercentComplete(zStride + yStride, totalPoints)); });
          if(shouldCancel)
          {
            return;
          }
          for(int64 xLoopIdx = 0; xLoopIdx < dims[0]; xLoopIdx++)
          {
            int64 voxelIndex = zStride + yStride + xLoopIdx;

            if(!checked[voxelIndex] && !goodVoxels.getValue(voxelIndex))
            {
              bool touchesBoundary = false;
              currentVList.push_back(voxelIndex);
              usize count = 0;
              while(count < currentVList.size())
              {
                int64 index = currentVList[count];
                int64 xIndex = index % dims[0];
                int64 yIndex = (index / dims[0]) % dims[1];
                int64 zIndex = index / (dims[0] * dims[1]);
                // Loop over the 6 face neighbors of the voxel
                const std::array<bool, k_NeighborCount> isValidFaceNeighbor = computeValidFaceNeighbors<ImageDimsStateT>(xIndex, yIndex, zIndex, dims);
                for(const auto faceIndex : faceNeighborInternalIdx) // ref more expensive than trivial copy for scalar types
                {
                  if(!isValidFaceNeighbor[faceIndex])
                  {
                    touchesBoundary = true;
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
                for(int64_t j : currentVList)
                {
                  goodVoxels.setValue(j, true);
                }
              }
              currentVList.clear();
            }
          }
        }
      }
    }
    checked.clear();
  }
};

template <template <typename> class FunctorT, class... ArgsT>
void ProcessVoxels(const DataType& dataType, const ImageGeom* imageGeom, ArgsT&&... args)
{
  const bool xDimEmpty = imageGeom->getNumXCells() == 1;
  const bool yDimEmpty = imageGeom->getNumYCells() == 1;
  const bool zDimEmpty = imageGeom->getNumZCells() == 1;
  const uint8 emptyDimCount = static_cast<uint8>(xDimEmpty) + static_cast<uint8>(yDimEmpty) + static_cast<uint8>(zDimEmpty);

  // Treat dimensions of 1 as flat for image geom
  if(emptyDimCount == 0)
  {
    return ExecuteDataFunction(FunctorT<Image3D>{}, dataType, imageGeom, std::forward<ArgsT>(args)...);
  }
  if(emptyDimCount == 1)
  {
    if(zDimEmpty)
    {
      return ExecuteDataFunction(FunctorT<EmptyZImage2D>{}, dataType, imageGeom, std::forward<ArgsT>(args)...);
    }
    if(yDimEmpty)
    {
      return ExecuteDataFunction(FunctorT<EmptyYImage2D>{}, dataType, imageGeom, std::forward<ArgsT>(args)...);
    }
    if(xDimEmpty)
    {
      return ExecuteDataFunction(FunctorT<EmptyXImage2D>{}, dataType, imageGeom, std::forward<ArgsT>(args)...);
    }
  }
  if(emptyDimCount == 2)
  {
    if(xDimEmpty && yDimEmpty)
    {
      return ExecuteDataFunction(FunctorT<ZImage1D>{}, dataType, imageGeom, std::forward<ArgsT>(args)...);
    }
    if(xDimEmpty && zDimEmpty)
    {
      return ExecuteDataFunction(FunctorT<YImage1D>{}, dataType, imageGeom, std::forward<ArgsT>(args)...);
    }
    if(yDimEmpty && zDimEmpty)
    {
      return ExecuteDataFunction(FunctorT<XImage1D>{}, dataType, imageGeom, std::forward<ArgsT>(args)...);
    }
  }
  if(emptyDimCount == 3)
  {
    return ExecuteDataFunction(FunctorT<SingleVoxelImage>{}, dataType, imageGeom, std::forward<ArgsT>(args)...);
  }
}
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
    ProcessVoxels<IdentifySampleFunctor>(inputData->getDataType(), imageGeom, inputData, m_InputValues->FillHoles, m_MessageHandler, m_ShouldCancel);
  }

  return {};
}
