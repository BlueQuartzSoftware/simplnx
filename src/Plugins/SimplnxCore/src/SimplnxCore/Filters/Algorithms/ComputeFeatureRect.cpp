#include "ComputeFeatureRect.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
// Each bulk read contains 65,536 Feature IDs and keeps cell staging memory fixed.
constexpr usize k_ChunkTuples = 65536;
} // namespace

ComputeFeatureRect::ComputeFeatureRect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureRectInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureRect::~ComputeFeatureRect() noexcept = default;

const std::atomic_bool& ComputeFeatureRect::getCancel()
{
  return m_ShouldCancel;
}

Result<> ComputeFeatureRect::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& featureIdsStore = featureIds.getDataStoreRef();
  auto& corners = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->FeatureRectArrayPath);
  auto& cornersStore = corners.getDataStoreRef();

  const usize numFeatures = cornersStore.getNumberOfTuples();
  constexpr usize k_NumCornerComponents = 6;
  std::vector<uint32> featureCorners(numFeatures * k_NumCornerComponents);
  for(usize featureId = 0; featureId < numFeatures; featureId++)
  {
    const usize featureOffset = featureId * k_NumCornerComponents;
    featureCorners[featureOffset] = std::numeric_limits<uint32>::max();
    featureCorners[featureOffset + 1] = std::numeric_limits<uint32>::max();
    featureCorners[featureOffset + 2] = std::numeric_limits<uint32>::max();
    featureCorners[featureOffset + 3] = std::numeric_limits<uint32>::min();
    featureCorners[featureOffset + 4] = std::numeric_limits<uint32>::min();
    featureCorners[featureOffset + 5] = std::numeric_limits<uint32>::min();
  }

  std::vector<usize> imageDims = featureIdsStore.getTupleShape();

  // DataStore tuple shape uses reverse geometry dimension order. Bounds use X, Y, Z order.
  std::reverse(imageDims.rbegin(), imageDims.rend());

  const usize xDim = imageDims[0];
  const usize yDim = imageDims[1];
  const usize zDim = imageDims[2];
  const usize xySize = xDim * yDim;
  const usize totalVoxels = xySize * zDim;
  const auto writeCorners = [&cornersStore, &featureCorners]() { return cornersStore.copyFromBuffer(0, nonstd::span<const uint32>(featureCorners.data(), featureCorners.size())); };

  auto featureIdsBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  for(usize offset = 0; offset < totalVoxels; offset += k_ChunkTuples)
  {
    if(getCancel())
    {
      // Cancellation publishes bounds accumulated before this checkpoint.
      return writeCorners();
    }

    const usize chunkCount = std::min(k_ChunkTuples, totalVoxels - offset);
    auto readResult = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureIdsBuffer.get(), chunkCount));
    if(readResult.invalid())
    {
      return readResult;
    }

    for(usize chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++)
    {
      const int32 featureId = featureIdsBuffer[chunkIndex];
      if(featureId == 0)
      {
        // Feature zero is background and has no rectangle output.
        continue;
      }

      if(featureId < 0 || static_cast<usize>(featureId) >= numFeatures)
      {
        // Publish completed bounds before reporting an invalid Feature ID.
        auto writeResult = writeCorners();
        if(writeResult.invalid())
        {
          return writeResult;
        }

        const DataPath parentPath = m_InputValues->FeatureRectArrayPath.getParent();
        return MakeErrorResult(-31000, fmt::format("The parent data object '{}' of output array '{}' has a smaller tuple count than the maximum feature id in '{}'", parentPath.getTargetName(),
                                                   corners.getName(), featureIds.getName()));
      }

      const usize flatIndex = offset + chunkIndex;
      const uint32 x = static_cast<uint32>(flatIndex % xDim);
      const uint32 y = static_cast<uint32>((flatIndex / xDim) % yDim);
      const uint32 z = static_cast<uint32>(flatIndex / xySize);
      const usize featureOffset = static_cast<usize>(featureId) * k_NumCornerComponents;

      featureCorners[featureOffset] = std::min(featureCorners[featureOffset], x);
      featureCorners[featureOffset + 1] = std::min(featureCorners[featureOffset + 1], y);
      featureCorners[featureOffset + 2] = std::min(featureCorners[featureOffset + 2], z);
      featureCorners[featureOffset + 3] = std::max(featureCorners[featureOffset + 3], x);
      featureCorners[featureOffset + 4] = std::max(featureCorners[featureOffset + 4], y);
      featureCorners[featureOffset + 5] = std::max(featureCorners[featureOffset + 5], z);
    }
  }

  return writeCorners();
}
