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
  void operator()(DataStructure& dataStructure, const SplitAttributeArrayInputValues* inputValues)
  {
    auto& inputArray = dataStructure.getDataRefAs<DataArray<T>>(inputValues->InputArrayPath);
    usize numTuples = inputArray.getNumberOfTuples();
    usize numComponents = inputArray.getNumberOfComponents();
    for(const auto& j : inputValues->ExtractComponents)
    {
      std::string arrayName = inputValues->InputArrayPath.getTargetName() + inputValues->SplitArraysSuffix + StringUtilities::number(j);
      DataPath splitArrayPath = inputValues->InputArrayPath.getParent().createChildPath(arrayName);
      auto& splitArray = dataStructure.getDataRefAs<DataArray<T>>(splitArrayPath);

      for(usize i = 0; i < numTuples; i++)
      {
        splitArray[i] = inputArray[numComponents * i + j];
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
  auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->InputArrayPath);

  ExecuteDataFunction(SplitArraysFunctor{}, inputArray.getDataType(), m_DataStructure, m_InputValues);

  return {};
}
