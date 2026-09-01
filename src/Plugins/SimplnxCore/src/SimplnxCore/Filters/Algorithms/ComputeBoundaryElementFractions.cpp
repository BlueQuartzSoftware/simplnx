#include "ComputeBoundaryElementFractions.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
/// Limits cell-level bulk I/O to 64K tuples while keeping both synchronized input buffers bounded.
constexpr usize k_ChunkTuples = 65536;
} // namespace

ComputeBoundaryElementFractions::ComputeBoundaryElementFractions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                 ComputeBoundaryElementFractionsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeBoundaryElementFractions::~ComputeBoundaryElementFractions() noexcept = default;

Result<> ComputeBoundaryElementFractions::operator()()
{
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& boundaryCells = m_DataStructure.getDataRefAs<Int8Array>(m_InputValues->BoundaryCellsArrayPath);
  auto& boundaryCellFractions = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureDataAttributeMatrixPath.createChildPath(m_InputValues->BoundaryCellFractionsArrayName));

  const auto& featureIdsStore = featureIds.getDataStoreRef();
  const auto& boundaryCellsStore = boundaryCells.getDataStoreRef();
  auto& boundaryCellFractionsStore = boundaryCellFractions.getDataStoreRef();

  const usize totalPoints = featureIds.getNumberOfTuples();
  const usize numFeatures = boundaryCellFractions.getNumberOfTuples();

  std::vector<float32> surfVoxCounts(numFeatures, 0);
  std::vector<float32> voxCounts(numFeatures, 0);
  auto featureIdsBuffer = std::make_unique<int32[]>(k_ChunkTuples);
  auto boundaryCellsBuffer = std::make_unique<int8[]>(k_ChunkTuples);

  for(usize offset = 0; offset < totalPoints; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const usize count = std::min(k_ChunkTuples, totalPoints - offset);
    Result<> result = featureIdsStore.copyIntoBuffer(offset, nonstd::span<int32>(featureIdsBuffer.get(), count));
    if(result.invalid())
    {
      return result;
    }
    result = boundaryCellsStore.copyIntoBuffer(offset, nonstd::span<int8>(boundaryCellsBuffer.get(), count));
    if(result.invalid())
    {
      return result;
    }

    for(usize index = 0; index < count; index++)
    {
      const int32 gnum = featureIdsBuffer[index];
      voxCounts[gnum]++;
      if(boundaryCellsBuffer[index] > 0)
      {
        surfVoxCounts[gnum]++;
      }
    }
  }

  std::vector<float32> boundaryFractions(numFeatures);
  auto result = boundaryCellFractionsStore.copyIntoBuffer(0, nonstd::span<float32>(boundaryFractions.data(), numFeatures));
  if(result.invalid())
  {
    return result;
  }
  for(usize i = 1; i < numFeatures; i++)
  {
    // An unused positive feature produces NaN. Feature zero remains unchanged.
    boundaryFractions[i] = surfVoxCounts[i] / voxCounts[i];
  }
  return boundaryCellFractionsStore.copyFromBuffer(0, nonstd::span<const float32>(boundaryFractions.data(), numFeatures));
}
