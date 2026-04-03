#include "CreateFeatureArrayFromElementArray.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

using namespace nx::core;

namespace
{
struct CopyCellDataFunctor
{
  template <typename T>
  Result<> operator()(const IDataArray* selectedCellArray, const Int32AbstractDataStore& featureIds, IDataArray* createdArray, const std::atomic_bool& shouldCancel)
  {
    const auto& selectedCellStore = selectedCellArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& createdDataStore = createdArray->template getIDataStoreRefAs<AbstractDataStore<T>>();

    usize totalCellArrayComponents = selectedCellStore.getNumberOfComponents();

    std::map<int32, usize> featureMap;
    Result<> result;

    usize totalCellArrayTuples = selectedCellStore.getNumberOfTuples();
    for(usize cellTupleIdx = 0; cellTupleIdx < totalCellArrayTuples; cellTupleIdx++)
    {
      if(shouldCancel)
      {
        return {};
      }

      // Get the feature identifier (or what ever the user has selected as their "Feature" identifier
      int32 featureIdx = featureIds[cellTupleIdx];

      // Store the index of the first tuple with this feature identifier in the map
      if(featureMap.find(featureIdx) == featureMap.end())
      {
        featureMap[featureIdx] = totalCellArrayComponents * cellTupleIdx;
      }

      // Check that the values at the current index match the value at the first index
      usize firstInstanceCellTupleIdx = featureMap[featureIdx];
      for(usize cellCompIdx = 0; cellCompIdx < totalCellArrayComponents; cellCompIdx++)
      {
        T firstInstanceCellVal = selectedCellStore[firstInstanceCellTupleIdx + cellCompIdx];
        T currentCellVal = selectedCellStore[totalCellArrayComponents * cellTupleIdx + cellCompIdx];
        if(currentCellVal != firstInstanceCellVal && result.warnings().empty())
        {
          // The values are inconsistent with the first values for this feature identifier, so throw a warning
          result.warnings().push_back(
              Warning{-1000, fmt::format("Elements from Feature {} do not all have the same value. The last value copied into Feature {} will be used", featureIdx, featureIdx)});
        }

        createdDataStore[totalCellArrayComponents * featureIdx + cellCompIdx] = selectedCellStore[totalCellArrayComponents * cellTupleIdx + cellCompIdx];
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
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Starting CreateFeatureArrayFromElementArray...");

  const DataPath createdArrayPath = m_InputValues->CellFeatureAttributeMatrixPath.createChildPath(m_InputValues->CreatedArrayName);
  const auto* selectedCellArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SelectedCellArrayPath);
  const auto& featureIdsRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto* createdArray = m_DataStructure.getDataAs<IDataArray>(createdArrayPath);

  // Resize the created array to the proper size
  usize featureIdsMaxIdx = std::distance(featureIdsRef.begin(), std::max_element(featureIdsRef.cbegin(), featureIdsRef.cend()));
  usize maxValue = featureIdsRef[featureIdsMaxIdx];
  auto& cellFeatureAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);

  auto* createdArrayStore = createdArray->template getIDataStoreAs<IDataStore>();
  createdArrayStore->resizeTuples(std::vector<usize>{maxValue + 1});
  cellFeatureAttrMat.resizeTuples(std::vector<usize>{maxValue + 1});

  return ExecuteDataFunction(CopyCellDataFunctor{}, selectedCellArray->getDataType(), selectedCellArray, featureIdsRef, createdArray, m_ShouldCancel);
}
