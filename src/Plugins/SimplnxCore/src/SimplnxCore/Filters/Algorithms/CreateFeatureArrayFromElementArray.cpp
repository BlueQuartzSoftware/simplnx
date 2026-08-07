#include "CreateFeatureArrayFromElementArray.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <limits>
#include <vector>

using namespace nx::core;

namespace
{
constexpr auto k_NotSeen = std::numeric_limits<usize>::max();

struct CopyCellDataFunctor
{
  template <typename T>
  Result<> operator()(const IDataArray* selectedCellArray, const Int32AbstractDataStore& featureIds, IDataArray* createdArray, int32 maxValue, const std::atomic_bool& shouldCancel)
  {
    const auto& selectedCellStore = selectedCellArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& createdDataStore = createdArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    const usize totalCellArrayComponents = selectedCellStore.getNumberOfComponents();

    std::vector<usize> featureFirstCellOffset(static_cast<usize>(maxValue) + 1, k_NotSeen);
    Result<> result;

    const usize totalCellArrayTuples = selectedCellStore.getNumberOfTuples();
    for(usize cellTupleIdx = 0; cellTupleIdx < totalCellArrayTuples; cellTupleIdx++)
    {
      if(shouldCancel)
      {
        return {};
      }

      const int32 featureIdx = featureIds[cellTupleIdx];

      if(featureFirstCellOffset[featureIdx] == k_NotSeen)
      {
        featureFirstCellOffset[featureIdx] = totalCellArrayComponents * cellTupleIdx;
      }

      const usize firstInstanceCellTupleIdx = featureFirstCellOffset[featureIdx];
      for(usize cellCompIdx = 0; cellCompIdx < totalCellArrayComponents; cellCompIdx++)
      {
        const T firstInstanceCellVal = selectedCellStore[firstInstanceCellTupleIdx + cellCompIdx];
        const T currentCellVal = selectedCellStore[totalCellArrayComponents * cellTupleIdx + cellCompIdx];
        if(currentCellVal != firstInstanceCellVal && result.warnings().empty())
        {
          result.warnings().push_back(
              Warning{-1000, fmt::format("Elements from Feature {} do not all have the same value. The last value copied into Feature {} will be used", featureIdx, featureIdx)});
        }

        createdDataStore[totalCellArrayComponents * featureIdx + cellCompIdx] = currentCellVal;
      }
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
  const auto* selectedCellArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SelectedCellArrayPath);
  const auto& featureIdsRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto* createdArray = m_DataStructure.getDataAs<IDataArray>(createdArrayPath);

  // Resize the created array to the proper size
  if(featureIdsRef.getNumberOfTuples() == 0)
  {
    return MakeErrorResult(-81882, "Invalid Input, Feature Ids Array must not be empty");
  }

  const auto [minIt, maxIt] = std::minmax_element(featureIdsRef.cbegin(), featureIdsRef.cend());
  const int32 minValue = *minIt;
  const int32 maxValue = *maxIt;

  // Validate no negative feature IDs — a negative featureIdx in the copy loop converts to
  // a huge usize index (int32 → usize wrapping), causing an out-of-bounds write
  if(minValue < 0)
  {
    return MakeErrorResult(-81880, "Invalid Input, Feature Ids Array must not contain negative values");
  }

  auto& cellFeatureAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);

  // validate resize won't shrink child arrays
  if(maxValue + 1 > cellFeatureAttrMat.getNumberOfTuples())
  {
    for(const auto& childObject : cellFeatureAttrMat)
    {
      const auto* iArray = dynamic_cast<IArray*>(childObject.second.get());
      if(iArray != nullptr && iArray->getNumberOfTuples() > (maxValue + 1))
      {
        return MakeErrorResult(-81881, fmt::format("Resizing would cause data loss in {}. Make sure all objects in {} have tuple counts equal to or less then the max Feature ID {}!",
                                                   iArray->getName(), m_InputValues->CellFeatureAttributeMatrixPath.toString(), maxValue + 1));
      }
    }

    cellFeatureAttrMat.resizeTuples(std::vector<usize>{static_cast<usize>(maxValue) + 1});
  }

  return ExecuteDataFunction(CopyCellDataFunctor{}, selectedCellArray->getDataType(), selectedCellArray, featureIdsRef, createdArray, maxValue, m_ShouldCancel);
}
