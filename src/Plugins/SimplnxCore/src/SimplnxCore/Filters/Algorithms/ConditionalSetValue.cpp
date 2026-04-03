#include "ConditionalSetValue.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

using namespace nx::core;

namespace
{
struct ReplaceValueInArrayFunctor
{
  template <typename ScalarType>
  void operator()(IDataArray& workingArray, const std::string& removeValue, const std::string& replaceValue)
  {
    auto& dataStore = workingArray.template getIDataStoreRefAs<AbstractDataStore<ScalarType>>();

    ScalarType removeVal = StringInterpretationUtilities::Convert<ScalarType>(removeValue).value();
    ScalarType replaceVal = StringInterpretationUtilities::Convert<ScalarType>(replaceValue).value();

    const auto size = dataStore.getNumberOfTuples() * dataStore.getNumberOfComponents();

    for(usize index = 0; index < size; index++)
    {
      if(dataStore[index] == removeVal)
      {
        dataStore[index] = replaceVal;
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
ConditionalSetValue::ConditionalSetValue(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConditionalSetValueInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ConditionalSetValue::~ConditionalSetValue() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ConditionalSetValue::operator()()
{
  MessageHelper messageHelper(m_MessageHandler);
  messageHelper.sendMessage("Starting ConditionalSetValue...");

  if(m_InputValues->UseConditional)
  {
    DataObject& inputDataObject = m_DataStructure.getDataRef(m_InputValues->SelectedArrayPath);

    const IDataArray& conditionalArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ConditionalArrayPath);

    Result<> result = ConditionalReplaceValueInArray(m_InputValues->ReplaceValue, inputDataObject, conditionalArray, m_InputValues->InvertMask);

    return result;
  }
  else
  {
    auto& inputDataArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPath);
    ExecuteDataFunction(ReplaceValueInArrayFunctor{}, inputDataArray.getDataType(), inputDataArray, m_InputValues->RemoveValue, m_InputValues->ReplaceValue);
  }

  return {};
}
