#include "CopyFeatureArrayToElementArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
template <typename T>
class CopyFeatureArrayToElementArrayImpl
{
public:
  using StoreType = AbstractDataStore<T>;

  CopyFeatureArrayToElementArrayImpl(const IDataArray* selectedFeatureArray, const Int32AbstractDataStore& featureIdsStore, IDataArray* createdArray, const std::atomic_bool& shouldCancel)
  : m_SelectedFeature(selectedFeatureArray->template getIDataStoreRefAs<StoreType>())
  , m_FeatureIdsStore(featureIdsStore)
  , m_CreatedStore(createdArray->template getIDataStoreRefAs<StoreType>())
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    const usize totalFeatureArrayComponents = m_SelectedFeature.getNumberOfComponents();

    for(usize i = range.min(); i < range.max(); ++i)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      for(usize faComp = 0; faComp < totalFeatureArrayComponents; faComp++)
      {
        // Get the feature identifier (or what ever the user has selected as their "Feature" identifier
        m_CreatedStore[totalFeatureArrayComponents * i + faComp] = m_SelectedFeature[totalFeatureArrayComponents * m_FeatureIdsStore[i] + faComp];
      }
    }
  }

private:
  const StoreType& m_SelectedFeature;
  const Int32AbstractDataStore& m_FeatureIdsStore;
  StoreType& m_CreatedStore;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

// -----------------------------------------------------------------------------
CopyFeatureArrayToElementArray::CopyFeatureArrayToElementArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               CopyFeatureArrayToElementArrayInputValues* inputValues)
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
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath);

  for(const auto& selectedFeatureArrayPath : m_InputValues->SelectedFeatureArrayPaths)
  {
    DataPath createdArrayPath = m_InputValues->FeatureIdsPath.replaceName(selectedFeatureArrayPath.getTargetName() + m_InputValues->CreatedArraySuffix);
    const auto* selectedFeatureArray = m_DataStructure.getDataAs<IDataArray>(selectedFeatureArrayPath);

    auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, selectedFeatureArrayPath, featureIds, false, m_MessageHandler);
    if(validateNumFeatResult.invalid())
    {
      return validateNumFeatResult;
    }

    m_MessageHandler(IFilter::ProgressMessage{IFilter::ProgressMessage::Type::Info, fmt::format("Copying data into target array '{}'...", createdArrayPath.toString())});
    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, featureIds.getNumberOfTuples());
    ExecuteParallelFunction<::CopyFeatureArrayToElementArrayImpl>(selectedFeatureArray->getDataType(), dataAlg, selectedFeatureArray, featureIds.getDataStoreRef(),
                                                                  m_DataStructure.getDataAs<IDataArray>(createdArrayPath), m_ShouldCancel);
  }

  return {};
}
