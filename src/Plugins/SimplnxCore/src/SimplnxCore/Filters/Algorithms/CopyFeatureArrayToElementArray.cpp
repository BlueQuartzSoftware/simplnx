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
template <typename T>
class CopyFeatureArrayToElementArrayImpl
{
public:
  using StoreType = AbstractDataStore<T>;

  CopyFeatureArrayToElementArrayImpl(const IDataArray& selectedFeatureArray, const Int32AbstractDataStore& featureIdsStore, IDataArray& createdArray, const std::atomic_bool& shouldCancel)
  : m_SelectedFeatureStore(selectedFeatureArray.getIDataStoreRefAs<StoreType>())
  , m_FeatureIdsStore(featureIdsStore)
  , m_CreatedStore(createdArray.getIDataStoreRefAs<StoreType>())
  , m_ShouldCancel(shouldCancel)
  {
    // Raw-pointer fast path is only taken when all three stores are concrete in-memory
    // DataStore<T> objects. Each thread then writes a disjoint index range of a plain
    // buffer, so there is no shared mutable store state. Any other store type (e.g.
    // out-of-core) falls back to the virtual AbstractDataStore access path, which runs
    // serially because IParallelAlgorithm disables parallelization for OOC data.
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
        // Copy the feature tuple indexed by this cell's feature id down to the cell-level array
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

// -----------------------------------------------------------------------------
CopyFeatureArrayToElementArray::CopyFeatureArrayToElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               const CopyFeatureArrayToElementArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CopyFeatureArrayToElementArray::~CopyFeatureArrayToElementArray() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CopyFeatureArrayToElementArray::operator()()
{
  if(m_InputValues->SelectedFeatureArrayPaths.empty())
  {
    return {};
  }

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath);

  // Validate the FeatureIds value range once, against the first selected array. Preflight has
  // already verified (error -3020) that every selected feature array has the same tuple count,
  // so a single validation covers all of them and avoids re-scanning FeatureIds per array.
  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->SelectedFeatureArrayPaths[0], featureIds, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
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

    m_MessageHandler.sendInfoMessage(fmt::format("Copying data into target array '{}'...", createdArrayPath.toString()));
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, featureIds.getNumberOfTuples());
    dataAlg.requireArraysInMemory({&featureIds, &selectedFeatureArray, &createdArray});
    ExecuteParallelFunction<::CopyFeatureArrayToElementArrayImpl>(selectedFeatureArray.getDataType(), dataAlg, selectedFeatureArray, featureIds.getDataStoreRef(), createdArray, m_ShouldCancel);
  }

  return {};
}
