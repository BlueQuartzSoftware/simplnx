#include "FindArrayUniqueValues.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"
#include "complex/Utilities/ParallelAlgorithmUtilities.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{

} // namespace

template <typename T, typename V>
class FindUniqueValuesImpl
{
public:
  FindUniqueValuesImpl(DataStructure data, DataPath selectedArrayPath, DataPath maskArrayPath, bool useMask, std::map<V, int>& uniqueValuesMap)
  : m_Data(data)
  , m_SelectedArrayPath(selectedArrayPath)
  , m_MaskArrayPath(maskArrayPath)
  , m_UseMask(useMask)
  , m_UniqueValuesMap(uniqueValuesMap)
  {
  }

  virtual ~FindUniqueValuesImpl() = default;

  void compute(usize start, usize end) const
  {
    const T& selectedArray = m_Data.getDataRefAs<T>(m_SelectedArrayPath);

    for(int i = start; i < end; i++)
    {
      if(!m_UseMask)
      {
        if(m_UniqueValuesMap.find(selectedArray[i]) != m_UniqueValuesMap.end())
        {
          m_UniqueValuesMap[selectedArray[i]] = m_UniqueValuesMap[selectedArray[i]]++;
        }
        else
        {
          m_UniqueValuesMap[selectedArray[i]] = 1;
        }
      }
      else if(m_UseMask)
      {
        const BoolArray& maskArray = m_Data.getDataRefAs<BoolArray>(m_MaskArrayPath);

        if(maskArray[i])
        {
          if(m_UniqueValuesMap.find(selectedArray[i]) != m_UniqueValuesMap.end())
          {
            m_UniqueValuesMap[selectedArray[i]] = m_UniqueValuesMap[selectedArray[i]]++;
          }
          else
          {
            m_UniqueValuesMap[selectedArray[i]] = 1;
          }
        }
      }
    }
  }

  void operator()(const complex::Range& range) const
  {
    compute(range.min(), range.max());
  }

private:
  DataStructure m_Data;
  DataPath m_SelectedArrayPath;
  DataPath m_MaskArrayPath;
  bool m_UseMask;
  std::map<V, int>& m_UniqueValuesMap;
};

//------------------------------------------------------------------------------
template <typename T, typename V>
void findUniqueValues(DataStructure data, DataPath selectedArrayPath, DataPath maskArrayPath, DataPath destinationAttributeMatrixPath, bool useMask, std::string arrayName)
{
  auto& destinationAttributeMatrix = data.getDataRefAs<AttributeMatrix>(destinationAttributeMatrixPath);
  T& selectedArray = data.getDataRefAs<T>(selectedArrayPath);
  T& destinationArray = data.getDataRefAs<T>(destinationAttributeMatrixPath.createChildPath(arrayName));
  std::map<V, int> uniqueValuesMap;

  // #define TEST

#ifndef TEST

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, selectedArray.getSize());

  dataAlg.execute(FindUniqueValuesImpl<T, V>(data, selectedArrayPath, maskArrayPath, useMask, uniqueValuesMap));

#endif // TEST

#ifdef TEST
  for(int i = 0; i < selectedArray.getSize(); i++)
  {
    if(!useMask)
    {
      if(uniqueValuesMap.find(selectedArray[i]) != uniqueValuesMap.end())
      {
        uniqueValuesMap[selectedArray[i]] = uniqueValuesMap[selectedArray[i]]++;
      }
      else
      {
        uniqueValuesMap[selectedArray[i]] = 1;
      }
    }
    else if(useMask)
    {
      const BoolArray& maskArray = data.getDataRefAs<BoolArray>(maskArrayPath);

      if(maskArray[i])
      {
        if(uniqueValuesMap.find(selectedArray[i]) != uniqueValuesMap.end())
        {
          uniqueValuesMap[selectedArray[i]] = uniqueValuesMap[selectedArray[i]]++;
        }
        else
        {
          uniqueValuesMap[selectedArray[i]] = 1;
        }
      }
    }
  }
#endif // TEST

  for(int j = 0; j < destinationArray.getSize(); j++)
  {
    destinationArray[j] = uniqueValuesMap.size();
  }
}

void findArrayType(DataStructure data, DataPath selectedArrayPath, DataPath maskArrayPath, DataPath destinationAttributeMatrixPath, bool useMask, std::string arrayName,
                   IFilter::MessageHandler messageHandler)
{
  const auto* selectedArray = data.getDataAs<IDataArray>(selectedArrayPath);

  if(selectedArray->getTypeName() == "DataArray<int8>")
  {
    findUniqueValues<Int8Array, int8>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<uint8>")
  {
    findUniqueValues<UInt8Array, uint8>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<int16>")
  {
    findUniqueValues<Int16Array, int16>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<uint16>")
  {
    findUniqueValues<UInt16Array, uint16>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<int32>")
  {
    findUniqueValues<Int32Array, int32>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<uint32>")
  {
    findUniqueValues<UInt32Array, uint32>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<int64>")
  {
    findUniqueValues<Int64Array, int64>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<uint64>")
  {
    findUniqueValues<UInt64Array, uint64>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<float32>")
  {
    findUniqueValues<Float32Array, float32>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<float64>")
  {
    findUniqueValues<Float64Array, float64>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else if(selectedArray->getTypeName() == "DataArray<bool>")
  {
    findUniqueValues<BoolArray, bool>(data, selectedArrayPath, maskArrayPath, destinationAttributeMatrixPath, useMask, arrayName);
  }
  else
  {
    std::string err = "Invalid Data Array";
    messageHandler({IFilter::Message::Type::Error, err});
  }
}

// -----------------------------------------------------------------------------
FindArrayUniqueValues::FindArrayUniqueValues(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             FindArrayUniqueValuesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FindArrayUniqueValues::~FindArrayUniqueValues() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FindArrayUniqueValues::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FindArrayUniqueValues::operator()()
{

  return {};
}
