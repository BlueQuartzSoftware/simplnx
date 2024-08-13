#include "ExtractComponentAsArray.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;
namespace
{
struct RemoveComponentsFunctor
{
  template <class ScalarType>
  void operator()(IDataArray* originalArray, IDataArray* resizedArray, usize componentIndexToRemove) // Due to logic structure originalArray cannot be const
  {
    const auto& originalStoreRef = dynamic_cast<const DataArray<ScalarType>*>(originalArray)->getDataStoreRef();
    auto& resizedStoreRef = dynamic_cast<DataArray<ScalarType>*>(resizedArray)->getDataStoreRef();

    const usize originalTupleCount = originalStoreRef.getNumberOfTuples();
    const usize originalCompCount = originalStoreRef.getNumberOfComponents();

    usize distanceToShuffle = 0;
    for(usize tuple = 0; tuple < originalTupleCount; tuple++)
    {
      for(usize comp = 0; comp < originalCompCount; comp++)
      {
        if(comp == componentIndexToRemove)
        {
          distanceToShuffle++;
          continue;
        }
        const usize index = tuple * originalCompCount + comp;
        resizedStoreRef[index - distanceToShuffle] = originalStoreRef[index];
      }
    }

    // inputArrayRef
  }
};

struct ExtractComponentsFunctor
{
  template <class ScalarType>
  void operator()(IDataArray* inputArray, IDataArray* extractedCompArray, usize componentIndexToExtract) // Due to logic structure inputArray cannot be const
  {
    const auto& inputStoreRef = dynamic_cast<const DataArray<ScalarType>*>(inputArray)->getDataStoreRef();
    auto& extractedStoreRef = dynamic_cast<DataArray<ScalarType>*>(extractedCompArray)->getDataStoreRef();

    const usize inputTupleCount = inputStoreRef.getNumberOfTuples();
    const usize inputCompCount = inputStoreRef.getNumberOfComponents();

    for(usize tuple = 0; tuple < inputTupleCount; tuple++)
    {
      for(usize comp = 0; comp < inputCompCount; comp++)
      {
        if(comp == componentIndexToExtract)
        {
          extractedStoreRef[tuple] = inputStoreRef[tuple * inputCompCount + comp]; // extracted array will always have comp count of 1
        }
      }
    }
  }
};
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
  /* baseArrayRef CANNOT be const because it can either be the original array [can be const] OR the resized array [can't be const]*/
  /* tempArrayRef CANNOT be const because the functor has to be capable of handling both cases of remove components*/
  const bool moveComponentsToNewArrayBool = m_InputValues->MoveComponentsToNewArray;
  const bool removeComponentsFromArrayBool = m_InputValues->RemoveComponentsFromArray;
  const auto compToRemoveNum = static_cast<usize>(abs(m_InputValues->CompNumber));
  // this will be the original array if components are not being removed, else it is resized array
  auto* baseArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->BaseArrayPath);
  if((!removeComponentsFromArrayBool) && moveComponentsToNewArrayBool)
  {
    ExecuteDataFunction(ExtractComponentsFunctor{}, baseArrayPtr->getDataType(), baseArrayPtr, m_DataStructure.getDataAs<IDataArray>(m_InputValues->NewArrayPath), compToRemoveNum);
    return {};
  }
  // will not exist if remove components is not occurring, hence the early bailout ^
  auto* tempArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->TempArrayPath); // will not exist if remove components is not true, hence the early bailout ^

  if(moveComponentsToNewArrayBool)
  {
    auto* extractedCompArrayPtr = m_DataStructure.getDataAs<IDataArray>(m_InputValues->NewArrayPath);
    ExecuteDataFunction(ExtractComponentsFunctor{}, tempArrayPtr->getDataType(), tempArrayPtr, extractedCompArrayPtr, compToRemoveNum);
  }

  // remove by default, because the only case where they weren't removed was covered at start
  ExecuteDataFunction(RemoveComponentsFunctor{}, tempArrayPtr->getDataType(), tempArrayPtr, baseArrayPtr, compToRemoveNum);

  return {};
}
