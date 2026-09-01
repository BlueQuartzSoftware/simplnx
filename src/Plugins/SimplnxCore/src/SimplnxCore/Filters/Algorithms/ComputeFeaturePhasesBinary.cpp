#include "ComputeFeaturePhasesBinary.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <vector>

using namespace nx::core;
namespace
{
// One million Feature IDs and mask values use at most five MiB of cell staging memory.
constexpr usize k_TargetChunkTuples = 1'000'000;

/**
 * @struct ComputeFeaturePhasesBinaryFunctor
 * @brief Assigns binary feature phases from bounded cell buffers.
 *
 * Serial cell order preserves last-cell-wins semantics. The output cache grows to the largest
 * referenced Feature ID and retains unreferenced output values.
 */
struct ComputeFeaturePhasesBinaryFunctor
{
  /**
   * @brief Assigns phases from one mask element type.
   * @tparam MaskType Specifies the mask element type.
   * @param featureIdsArray Supplies cell Feature IDs.
   * @param maskArray Supplies cell mask values.
   * @param featurePhasesArray Receives feature phases.
   * @param featureIdsPath Identifies the Feature ID array for errors.
   * @param featurePhasesPath Identifies the output array for errors.
   * @param shouldCancel Signals cancellation between cell chunks.
   * @return Success, or a Feature ID or bulk-I/O error.
   *
   * When a checkpoint observes cancellation, the method returns success before
   * its final output write.
   */
  template <typename MaskType>
  Result<> operator()(const IDataArray& featureIdsArray, const IDataArray& maskArray, IDataArray& featurePhasesArray, const DataPath& featureIdsPath, const DataPath& featurePhasesPath,
                      const std::atomic_bool& shouldCancel) const
  {
    const auto& featureIdsStore = featureIdsArray.getIDataStoreRefAs<AbstractDataStore<int32>>();
    const auto& maskStore = maskArray.template getIDataStoreRefAs<AbstractDataStore<MaskType>>();
    auto& featurePhasesStore = featurePhasesArray.getIDataStoreRefAs<AbstractDataStore<int32>>();

    const usize numCells = featureIdsStore.getNumberOfTuples();
    if(numCells == 0)
    {
      return {};
    }

    const usize chunkTuples = std::min(k_TargetChunkTuples, numCells);
    auto featureIdsBuffer = std::make_unique<int32[]>(chunkTuples);
    auto maskBuffer = std::make_unique<MaskType[]>(chunkTuples);
    std::vector<int32> featurePhasesCache;

    const usize outputTupleCount = featurePhasesStore.getNumberOfTuples();
    for(usize cellOffset = 0; cellOffset < numCells; cellOffset += chunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize cellCount = std::min(chunkTuples, numCells - cellOffset);
      Result<> result = featureIdsStore.copyIntoBuffer(cellOffset, nonstd::span<int32>(featureIdsBuffer.get(), cellCount));
      if(result.invalid())
      {
        return result;
      }
      result = maskStore.copyIntoBuffer(cellOffset, nonstd::span<MaskType>(maskBuffer.get(), cellCount));
      if(result.invalid())
      {
        return result;
      }

      usize largestFeatureId = 0;
      for(usize index = 0; index < cellCount; index++)
      {
        const int32 featureId = featureIdsBuffer[index];
        if(featureId < 0 || static_cast<usize>(featureId) >= outputTupleCount)
        {
          return MakeErrorResult(-53801, fmt::format("Feature id {} at tuple {} in array '{}' is outside the valid output range [0, {}) for array '{}'.", featureId, cellOffset + index,
                                                     featureIdsPath.toString(), outputTupleCount, featurePhasesPath.toString()));
        }
        largestFeatureId = std::max(largestFeatureId, static_cast<usize>(featureId));
      }

      const usize requiredCacheSize = largestFeatureId + 1;
      if(requiredCacheSize > featurePhasesCache.size())
      {
        const usize previousCacheSize = featurePhasesCache.size();
        featurePhasesCache.resize(requiredCacheSize);
        result = featurePhasesStore.copyIntoBuffer(previousCacheSize, nonstd::span<int32>(featurePhasesCache.data() + previousCacheSize, requiredCacheSize - previousCacheSize));
        if(result.invalid())
        {
          return result;
        }
      }

      for(usize index = 0; index < cellCount; index++)
      {
        const usize featureId = static_cast<usize>(featureIdsBuffer[index]);
        featurePhasesCache[featureId] = maskBuffer[index] != static_cast<MaskType>(0);
      }
    }

    if(shouldCancel)
    {
      return {};
    }

    return featurePhasesStore.copyFromBuffer(0, nonstd::span<const int32>(featurePhasesCache.data(), featurePhasesCache.size()));
  }
};
} // namespace

ComputeFeaturePhasesBinary::ComputeFeaturePhasesBinary(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ComputeFeaturePhasesBinaryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeaturePhasesBinary::~ComputeFeaturePhasesBinary() noexcept = default;

Result<> ComputeFeaturePhasesBinary::operator()()
{
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
  const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
  const DataPath featurePhasesPath = m_InputValues->CellDataAttributeMatrixPath.createChildPath(m_InputValues->FeaturePhasesArrayName);
  auto& featurePhasesArray = m_DataStructure.getDataRefAs<IDataArray>(featurePhasesPath);

  if(maskArray.getDataType() != DataType::boolean && maskArray.getDataType() != DataType::uint8)
  {
    return MakeErrorResult(-53800, fmt::format("Mask array '{}' has data type '{}'. The mask must have a boolean or uint8 data type.", m_InputValues->MaskArrayPath.toString(),
                                               DataTypeToString(maskArray.getDataType())));
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  return ExecuteDataFunction(ComputeFeaturePhasesBinaryFunctor{}, maskArray.getDataType(), featureIdsArray, maskArray, featurePhasesArray, m_InputValues->FeatureIdsArrayPath, featurePhasesPath,
                             m_ShouldCancel);
}
