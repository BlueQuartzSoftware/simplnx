#include "CreateFeatureArrayFromElementArray.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Returns true when the two values should be considered consistent for the purposes of
 * the per-feature "all elements have the same value" warning. Two NaNs are treated as
 * consistent so that NaN-padded features do not consume the single warning (the copy itself is
 * unaffected — NaN values are still copied verbatim).
 */
template <typename T>
bool ValuesAreConsistent(T currentValue, T firstValue)
{
  if constexpr(std::is_floating_point_v<T>)
  {
    if(std::isnan(currentValue) && std::isnan(firstValue))
    {
      return true;
    }
  }
  return currentValue == firstValue;
}

struct CopyCellDataFunctor
{
  template <typename T>
  Result<> operator()(const IDataArray* selectedCellArrayPtr, const Int32AbstractDataStore& featureIdsRef, IDataArray* createdArrayPtr, int32 maxFeatureId, const std::atomic_bool& shouldCancel,
                      ThrottledMessenger& throttledMessenger)
  {
    const auto& selectedCellStoreRef = selectedCellArrayPtr->getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& createdStoreRef = createdArrayPtr->getIDataStoreRefAs<AbstractDataStore<T>>();

    const usize numComps = selectedCellStoreRef.getNumberOfComponents();
    const usize numFeatureTuples = static_cast<usize>(maxFeatureId) + 1;

    // Feature-level staging buffers (O(features) memory, on par with the created array itself):
    // seenFeature marks ids already encountered; firstValues caches each feature's first tuple so
    // the consistency check never reads back into the (possibly out-of-core) cell array;
    // outputStage accumulates the result and is written to the store once, sequentially, at the
    // end. outputStage starts as a copy of the freshly resized store so tuples for feature ids
    // that never appear ("gap" ids) keep the store's fill value.
    std::vector<uint8> seenFeature(numFeatureTuples, 0);
    std::vector<T> firstValues(numFeatureTuples * numComps);
    std::vector<T> outputStage(numFeatureTuples * numComps);
    const usize outputSize = outputStage.size();
    for(usize i = 0; i < outputSize; i++)
    {
      outputStage[i] = createdStoreRef[i];
    }

    Result<> result;
    bool warned = false;

    const usize totalCellTuples = selectedCellStoreRef.getNumberOfTuples();
    for(usize cellTupleIdx = 0; cellTupleIdx < totalCellTuples; cellTupleIdx++)
    {
      // Check for cancellation and report progress every 1024 cells; per-cell checks measurably
      // slow the hot loop (atomic load + steady_clock read per iteration)
      if((cellTupleIdx & 0x3FF) == 0)
      {
        if(shouldCancel)
        {
          return {};
        }
        throttledMessenger.sendThrottledMessage([cellTupleIdx, totalCellTuples]() { return fmt::format("Copying cell data to feature data: {}/{} cells", cellTupleIdx, totalCellTuples); });
      }

      // Get the feature identifier (or whatever the user selected as their "Feature" identifier);
      // values were validated non-negative before dispatch
      const int32 featureIdx = featureIdsRef[cellTupleIdx];
      const usize srcBegin = numComps * cellTupleIdx;
      const usize dstBegin = numComps * static_cast<usize>(featureIdx);

      // First cell seen with this feature identifier: record its tuple for later consistency
      // checks and stage it as the feature's value
      if(seenFeature[static_cast<usize>(featureIdx)] == 0)
      {
        seenFeature[static_cast<usize>(featureIdx)] = 1;
        for(usize comp = 0; comp < numComps; comp++)
        {
          const T value = selectedCellStoreRef[srcBegin + comp];
          firstValues[dstBegin + comp] = value;
          outputStage[dstBegin + comp] = value;
        }
        continue;
      }

      // Repeat cell for this feature: check the values against the first cell seen with this
      // feature identifier and stage the current values (the last cell of a feature wins). Only
      // ONE warning is ever emitted no matter how many features are inconsistent — this matches
      // the legacy DREAM3D 6.5 behavior; once it has fired the comparisons are skipped.
      for(usize comp = 0; comp < numComps; comp++)
      {
        const T currentCellVal = selectedCellStoreRef[srcBegin + comp];
        // Explicit <T>: std::vector<bool>'s proxy reference would otherwise break deduction
        if(!warned && !ValuesAreConsistent<T>(currentCellVal, firstValues[dstBegin + comp]))
        {
          result.warnings().push_back(
              Warning{-1000, fmt::format("Elements from Feature {} do not all have the same value. The last value copied into Feature {} will be used", featureIdx, featureIdx)});
          warned = true;
        }
        outputStage[dstBegin + comp] = currentCellVal;
      }
    }

    // Single sequential write of the staged output into the created array's store
    for(usize i = 0; i < outputSize; i++)
    {
      createdStoreRef[i] = outputStage[i];
    }

    return result;
  }
};
} // namespace

// -----------------------------------------------------------------------------
CreateFeatureArrayFromElementArray::CreateFeatureArrayFromElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                       CreateFeatureArrayFromElementArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CreateFeatureArrayFromElementArray::~CreateFeatureArrayFromElementArray() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CreateFeatureArrayFromElementArray::operator()()
{
  const DataPath createdArrayPath = m_InputValues->CellFeatureAttributeMatrixPath.createChildPath(m_InputValues->CreatedArrayName);
  const auto* selectedCellArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SelectedCellArrayPath);
  const auto& featureIdsRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath).getDataStoreRef();
  auto* createdArrayPtr = m_DataStructure.getDataAs<IDataArray>(createdArrayPath);

  // Find the largest Feature Id, which determines the created array's tuple count. Negative
  // Feature Ids would index before the start of the created array, so reject them here since
  // preflight cannot see the array values.
  int32 maxFeatureId = 0;
  int32 minFeatureId = 0;
  const usize numFeatureIdTuples = featureIdsRef.getNumberOfTuples();
  for(usize i = 0; i < numFeatureIdTuples; i++)
  {
    const int32 featureId = featureIdsRef[i];
    maxFeatureId = std::max(maxFeatureId, featureId);
    minFeatureId = std::min(minFeatureId, featureId);
  }
  if(minFeatureId < 0)
  {
    return MakeErrorResult(-5570,
                           fmt::format("Feature Ids array '{}' contains negative values (minimum found: {}). All Feature Ids must be >= 0.", m_InputValues->FeatureIdsPath.toString(), minFeatureId));
  }

  Result<> result;
  const usize numFeatureTuples = static_cast<usize>(maxFeatureId) + 1;

  // A max Feature Id larger than the cell count means the Feature Ids are sparse — legal, but
  // usually a sign of corrupt input, and it drives the size of every array in the destination
  // Attribute Matrix. Warn before committing to the allocation.
  if(static_cast<usize>(maxFeatureId) > numFeatureIdTuples)
  {
    const std::string message = fmt::format("The largest Feature Id ({}) exceeds the number of cells ({}). Every array in '{}' will be resized to {} tuples — verify the Feature Ids array is correct.",
                                            maxFeatureId, numFeatureIdTuples, m_InputValues->CellFeatureAttributeMatrixPath.toString(), numFeatureTuples);
    m_MessageHandler({IFilter::Message::Type::Warning, message});
    result.warnings().push_back(Warning{-5574, message});
  }

  // Resize the destination Attribute Matrix (which resizes EVERY array inside it, including the
  // created array) to hold every Feature Id. Warn when this truncates existing Feature data.
  auto& cellFeatureAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);
  if(numFeatureTuples < cellFeatureAttrMat.getNumberOfTuples())
  {
    const std::string message = fmt::format("Attribute Matrix '{}' is being resized from {} to {} tuples: every array it contains will be truncated to match max(Feature Ids) + 1.",
                                            m_InputValues->CellFeatureAttributeMatrixPath.toString(), cellFeatureAttrMat.getNumberOfTuples(), numFeatureTuples);
    m_MessageHandler({IFilter::Message::Type::Warning, message});
    result.warnings().push_back(Warning{-5573, message});
  }
  cellFeatureAttrMat.resizeTuples(std::vector<usize>{numFeatureTuples});

  // Throttled progress updates (at most 1 per second)
  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();

  Result<> copyResult =
      ExecuteDataFunction(CopyCellDataFunctor{}, selectedCellArrayPtr->getDataType(), selectedCellArrayPtr, featureIdsRef, createdArrayPtr, maxFeatureId, m_ShouldCancel, throttledMessenger);
  return MergeResults(std::move(result), std::move(copyResult));
}
