#include "SplitAttributeArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

namespace
{
struct SplitArraysFunctor
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const IDataArray* inputIDataArray, const SplitAttributeArrayInputValues* inputValues)
  {
    const auto& inputStore = inputIDataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    usize numTuples = inputStore.getNumberOfTuples();
    usize numComponents = inputStore.getNumberOfComponents();
    for(const auto& j : inputValues->ExtractComponents)
    {
      std::string arrayName = inputValues->InputArrayPath.getTargetName() + inputValues->SplitArraysSuffix + StringUtilities::number(j);
      DataPath splitArrayPath = inputValues->InputArrayPath.replaceName(arrayName);
      auto& splitStore = dataStructure.getDataAs<DataArray<T>>(splitArrayPath)->getDataStoreRef();

      for(usize i = 0; i < numTuples; i++)
      {
        splitStore[i] = inputStore[numComponents * i + j];
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
SplitAttributeArray::SplitAttributeArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SplitAttributeArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SplitAttributeArray::~SplitAttributeArray() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SplitAttributeArray::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SplitAttributeArray::operator()()
{
  auto* inputArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputArrayPath);

  ExecuteDataFunction(SplitArraysFunctor{}, inputArray->getDataType(), m_DataStructure, inputArray, m_InputValues);

  return {};
}
