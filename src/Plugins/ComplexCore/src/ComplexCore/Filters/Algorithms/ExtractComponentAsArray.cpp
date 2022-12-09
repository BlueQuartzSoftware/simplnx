#include "ExtractComponentAsArray.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

using namespace complex;
namespace
{
struct RemoveComponentsFunctor
{
  template <class ScalarType>
  void operator()(IDataArray& originalArray, IDataArray& resizedArray, usize componentIndexToRemove) // Due to logic structure originalArray cannot be const
  {
    const auto& originalArrayRef = dynamic_cast<const DataArray<ScalarType>&>(originalArray);
    auto& resizedArrayRef = dynamic_cast<DataArray<ScalarType>&>(resizedArray);

    usize originalTupleCount = originalArrayRef.getNumberOfTuples();
    usize originalCompCount = originalArrayRef.getNumberOfComponents();

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
        usize index = tuple * originalCompCount + comp;
        resizedArrayRef[index - distanceToShuffle] = originalArrayRef[index];
      }
    }

    // inputArrayRef
  }
};

struct ExtractComponentsFunctor
{
  template <class ScalarType>
  void operator()(IDataArray& inputArray, IDataArray& extractedCompArray, usize componentIndexToExtract) // Due to logic structure inputArray cannot be const
  {
    const auto& inputArrayRef = dynamic_cast<const DataArray<ScalarType>&>(inputArray);
    auto& extractedArrayRef = dynamic_cast<DataArray<ScalarType>&>(extractedCompArray);

    usize inputTupleCount = inputArrayRef.getNumberOfTuples();
    usize inputCompCount = inputArrayRef.getNumberOfComponents();

    usize extractedCompCount = extractedArrayRef.getNumberOfComponents();

    for(usize tuple = 0; tuple < inputTupleCount; tuple++)
    {
      for(usize comp = 0; comp < inputCompCount; comp++)
      {
        if(comp == componentIndexToExtract)
        {
          extractedArrayRef[tuple] = inputArrayRef[tuple * inputCompCount + comp]; // extracted array will always have comp count of 1
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
  const usize compToRemoveNum = static_cast<usize>(abs(m_InputValues->CompNumber));
  // this will be the original array if components are not being removed, else it is resized array
  IDataArray& baseArrayRef = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->BaseArrayPath);
  if((!removeComponentsFromArrayBool) && moveComponentsToNewArrayBool)
  {
    ExecuteDataFunction(ExtractComponentsFunctor{}, baseArrayRef.getDataType(), baseArrayRef, m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->NewArrayPath), compToRemoveNum);
    return {};
  }
  // will not exist if remove components isnt occuring, hence the early bailout ^
  IDataArray& tempArrayRef = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->TempArrayPath); // will not exist if remove components isnt true, hence the early bailout ^

  if(moveComponentsToNewArrayBool)
  {
    auto& extractedCompArrayRef = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->NewArrayPath);
    ExecuteDataFunction(ExtractComponentsFunctor{}, tempArrayRef.getDataType(), tempArrayRef, extractedCompArrayRef, compToRemoveNum);
  }

  // remove by default, because the only case where they weren't removed was covered at start
  ExecuteDataFunction(RemoveComponentsFunctor{}, tempArrayRef.getDataType(), tempArrayRef, baseArrayRef, compToRemoveNum);

  return {};
}
