#include "ComputeDifferencesMap.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;
namespace
{
struct ExecuteFindDifferenceMapFunctor
{
  template <typename DataType>
  void operator()(IDataArray* firstArrayPtr, IDataArray* secondArrayPtr, IDataArray* differenceMapPtr)
  {
    using store_type = AbstractDataStore<DataType>;

    auto& firstArray = firstArrayPtr->template getIDataStoreRefAs<store_type>();
    auto& secondArray = secondArrayPtr->template getIDataStoreRefAs<store_type>();
    auto& differenceMap = differenceMapPtr->template getIDataStoreRefAs<store_type>();

    usize numTuples = firstArray.getNumberOfTuples();
    int32 numComps = firstArray.getNumberOfComponents();

    for(usize i = 0; i < numTuples; i++)
    {
      for(int32 j = 0; j < numComps; j++)
      {
        auto firstVal = firstArray[numComps * i + j];
        auto secondVal = secondArray[numComps * i + j];
        auto diffVal = firstVal > secondVal ? firstVal - secondVal : secondVal - firstVal;
        differenceMap[numComps * i + j] = diffVal;
      }
    }
  }
};
} // namespace

// -----------------------------------------------------------------------------
ComputeDifferencesMap::ComputeDifferencesMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ComputeDifferencesMapInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeDifferencesMap::~ComputeDifferencesMap() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeDifferencesMap::operator()()
{

  auto* firstInputArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->FirstInputArrayPath);
  auto* secondInputArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SecondInputArrayPath);
  auto* differenceMapArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->DifferenceMapArrayPath);

  if(m_ShouldCancel)
  {
    return {};
  }

  ExecuteDataFunction(ExecuteFindDifferenceMapFunctor{}, firstInputArray->getDataType(), firstInputArray, secondInputArray, differenceMapArray);

  return {};
}
