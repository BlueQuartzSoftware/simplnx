#include "CreateFeatureArrayFromElementArray.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

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

    const usize totalCellArrayComponents = selectedCellStore.getNumberOfComponents();

    std::map<int32, usize> featureMap;
    Result<> result;

    const usize totalCellArrayTuples = selectedCellStore.getNumberOfTuples();
    for(usize cellTupleIdx = 0; cellTupleIdx < totalCellArrayTuples; cellTupleIdx++)
    {
      if(shouldCancel)
      {
        return {};
      }

      // Get the feature identifier (or whatever the user has selected as their "Feature" identifier
      const int32 featureIdx = featureIds[cellTupleIdx];

      // Store the index of the first tuple with this feature identifier in the map
      if(!featureMap.contains(featureIdx))
      {
        featureMap[featureIdx] = totalCellArrayComponents * cellTupleIdx;
      }

      // Check that the values at the current index match the value at the first index
      const usize firstInstanceCellTupleIdx = featureMap[featureIdx];
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
  const DataPath createdArrayPath = m_InputValues->CellFeatureAttributeMatrixPath.createChildPath(m_InputValues->CreatedArrayName);
  const auto* selectedCellArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SelectedCellArrayPath);
  const auto& featureIdsRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsPath)->getDataStoreRef();
  auto* createdArray = m_DataStructure.getDataAs<IDataArray>(createdArrayPath);

  // Resize the created array to the proper size
  const usize featureIdsMaxIdx = std::distance(featureIdsRef.begin(), std::max_element(featureIdsRef.cbegin(), featureIdsRef.cend()));
  const int32 maxValue = featureIdsRef[featureIdsMaxIdx];

  // Validate no underflow
  if(maxValue < 0)
  {
    return MakeErrorResult(-81880, "Invalid Input, Feature Ids Array must contain a positive value");
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

  return ExecuteDataFunction(CopyCellDataFunctor{}, selectedCellArray->getDataType(), selectedCellArray, featureIdsRef, createdArray, m_ShouldCancel);
}
