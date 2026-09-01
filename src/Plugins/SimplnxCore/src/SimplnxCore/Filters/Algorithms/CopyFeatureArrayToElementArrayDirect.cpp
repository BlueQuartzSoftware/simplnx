#include "CopyFeatureArrayToElementArrayDirect.hpp"

#include "CopyFeatureArrayToElementArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>

using namespace nx::core;

namespace
{
/**
 * @class CopyFeatureArrayToElementArrayImpl
 * @brief Broadcasts one typed feature tuple to matching cells.
 * @tparam T Specifies the feature and output scalar type.
 *
 * Concrete stores use raw pointers. The generic fallback has no general
 * DataStore thread-safety guarantee during parallel access.
 */
template <typename T>
class CopyFeatureArrayToElementArrayImpl
{
public:
  using StoreType = AbstractDataStore<T>;

  /**
   * @brief Creates a typed feature broadcast worker.
   * @param selectedFeatureArray Provides feature tuples.
   * @param featureIdsStore Provides one Feature Id per cell.
   * @param createdArray Receives cell tuples.
   * @param shouldCancel Stops later cells when true.
   */
  CopyFeatureArrayToElementArrayImpl(const IDataArray& selectedFeatureArray, const Int32AbstractDataStore& featureIdsStore, IDataArray& createdArray, const std::atomic_bool& shouldCancel)
  : m_SelectedFeatureStore(selectedFeatureArray.getIDataStoreRefAs<StoreType>())
  , m_FeatureIdsStore(featureIdsStore)
  , m_CreatedStore(createdArray.getIDataStoreRefAs<StoreType>())
  , m_ShouldCancel(shouldCancel)
  {
    const auto* selectedFeatureStorePtr = dynamic_cast<const DataStore<T>*>(&m_SelectedFeatureStore);
    const auto* featureIdsStorePtr = dynamic_cast<const DataStore<int32>*>(&m_FeatureIdsStore);
    auto* createdStorePtr = dynamic_cast<DataStore<T>*>(&m_CreatedStore);
    if(selectedFeatureStorePtr != nullptr && featureIdsStorePtr != nullptr && createdStorePtr != nullptr)
    {
      m_SourcePtr = selectedFeatureStorePtr->data();
      m_FeatureIdsPtr = featureIdsStorePtr->data();
      m_DestPtr = createdStorePtr->data();
    }
  }

  /**
   * @brief Broadcasts feature values for one cell range.
   * @param range Specifies the half-open cell-index range.
   */
  void operator()(const Range& range) const
  {
    const usize numComps = m_SelectedFeatureStore.getNumberOfComponents();

    if(m_SourcePtr != nullptr)
    {
      for(usize i = range.min(); i < range.max(); ++i)
      {
        if((i & 0xFFFFULL) == 0ULL && m_ShouldCancel)
        {
          return;
        }
        std::copy_n(m_SourcePtr + numComps * static_cast<usize>(m_FeatureIdsPtr[i]), numComps, m_DestPtr + numComps * i);
      }
      return;
    }

    for(usize i = range.min(); i < range.max(); ++i)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const auto featureId = static_cast<usize>(m_FeatureIdsStore[i]);
      for(usize comp = 0; comp < numComps; comp++)
      {
        m_CreatedStore[numComps * i + comp] = m_SelectedFeatureStore[numComps * featureId + comp];
      }
    }
  }

private:
  const StoreType& m_SelectedFeatureStore;
  const Int32AbstractDataStore& m_FeatureIdsStore;
  StoreType& m_CreatedStore;
  const std::atomic_bool& m_ShouldCancel;
  const T* m_SourcePtr = nullptr;
  const int32* m_FeatureIdsPtr = nullptr;
  T* m_DestPtr = nullptr;
};
} // namespace

CopyFeatureArrayToElementArrayDirect::CopyFeatureArrayToElementArrayDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                           const CopyFeatureArrayToElementArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

CopyFeatureArrayToElementArrayDirect::~CopyFeatureArrayToElementArrayDirect() noexcept = default;

Result<> CopyFeatureArrayToElementArrayDirect::operator()()
{
  if(m_InputValues->SelectedFeatureArrayPaths.empty())
  {
    return {};
  }

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath);

  auto validateResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->SelectedFeatureArrayPaths[0], featureIds, false, m_MessageHandler, &m_ShouldCancel);
  if(validateResult.invalid())
  {
    return validateResult;
  }

  for(const auto& selectedFeatureArrayPath : m_InputValues->SelectedFeatureArrayPaths)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    DataPath createdArrayPath = m_InputValues->FeatureIdsPath.replaceName(selectedFeatureArrayPath.getTargetName() + m_InputValues->CreatedArraySuffix);
    const auto& selectedFeatureArray = m_DataStructure.getDataRefAs<IDataArray>(selectedFeatureArrayPath);
    auto& createdArray = m_DataStructure.getDataRefAs<IDataArray>(createdArrayPath);

    m_MessageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, fmt::format("Copying data into target array '{}'...", createdArrayPath.toString())});
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, featureIds.getNumberOfTuples());
    dataAlg.requireArraysInMemory({&featureIds, &selectedFeatureArray, &createdArray});
    ExecuteParallelFunction<::CopyFeatureArrayToElementArrayImpl>(selectedFeatureArray.getDataType(), dataAlg, selectedFeatureArray, featureIds.getDataStoreRef(), createdArray, m_ShouldCancel);
  }

  return {};
}
