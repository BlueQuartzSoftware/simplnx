#include "ExtractComponentAsArray.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;
namespace
{
//struct ExtractComponentsFunctor
//{
//  template <class T>
//  void operator()(IDataArray& inputDataRef, IDataArray& extractedCompRef, int32 componentsToExtract, bool insertInNew)
//  {
//    auto& inputDataRef = dynamic_cast<DataArray<T>&>(inputDataPtr);
//    auto& maskedData = dynamic_cast<DataArray<T>&>(maskedDataPtr);
//
//    inputDataRef.getIDataStore()->
//  }
//};
} // namespace

// -----------------------------------------------------------------------------
ExtractComponentAsArray::ExtractComponentAsArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ExtractComponentAsArrayInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ExtractComponentAsArray::~ExtractComponentAsArray() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ExtractComponentAsArray::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ExtractComponentAsArray::operator()()
{
  /*
   * Assumptions:
   * Both paths from the inputValues exist (provided you arent deleting the data)
   * Component count is less than than the number of components in the array
   */
  const bool moveToNewArrayBool = m_InputValues->MoveToNewArray;
  const int32 compToRemoveNum = m_InputValues->CompNumber;
  const auto& selectedArrayRef = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPath);
  const auto& extractedCompArrayRef = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->NewArrayPath);

  //ExecuteDataFunction(ExtractComponentsFunctor{}, selectedArrayRef.getDataType(), selectedArrayRef, extractedCompArrayRef, compToRemoveNum, moveToNewArrayBool);

  return {};
}
