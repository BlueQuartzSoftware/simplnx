#include "ConvertData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace Detail
{
template <typename O, typename D>
class ConvertDataAlg
{
public:
  using InputStoreType = AbstractDataStore<O>;
  using OutputStoreType = AbstractDataStore<D>;

  ConvertDataAlg(InputStoreType& inputStore, OutputStoreType& outputStore)
  : m_InputStore(inputStore)
  , m_OutputStore(outputStore)
  {
  }

  void convert(size_t start, size_t end) const
  {
    // std::cout << "\tConvertDataAlg: " << start << " to " << end << std::endl;

    for(size_t v = start; v < end; ++v)
    {
      // std::cout << "\tConvertDataAlg: index " << v << std::endl;

      if constexpr(std::is_same<O, D>::value)
      {
        // inputArray and destination arrays have the same type
        m_OutputStore.setValue(v, m_InputStore.getValue(v));
      }
      else if constexpr(std::is_same<O, bool>::value)
      {
        // inputArray array is a boolean array
        m_OutputStore.setValue(v, (m_InputStore.getValue(v) ? 1 : 0));
      }
      else if constexpr(std::is_same<D, bool>::value)
      {
        // Destination array is a boolean array
        m_OutputStore.setValue(v, (m_InputStore.getValue(v) != 0));
      }
      else
      {
        // All other cases
        m_OutputStore.setValue(v, static_cast<D>(m_InputStore.getValue(v)));
      }
    }
  }

  /**
   * @brief operator () This is called from the TBB stye of code
   * @param range The range to compute the values
   */
  void operator()(const Range& range) const
  {
    convert(range.min(), range.max());
  }

private:
  InputStoreType& m_InputStore;
  OutputStoreType& m_OutputStore;
};

template <typename T>
Result<> ConvertData(DataStructure& dataStructure, const ConvertDataInputValues* inputValues)
{
  DataArray<T>& inputArray = dataStructure.getDataRefAs<DataArray<T>>(inputValues->SelectedArrayPath);
  AbstractDataStore<T>& inputStore = inputArray.getDataStoreRef();

  typename IParallelAlgorithm::AlgorithmArrays algArrays;
  algArrays.push_back(&inputArray);
  algArrays.push_back(dataStructure.getDataAs<IDataArray>(inputValues->OutputArrayName));

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, inputArray.size());
  dataAlg.requireArraysInMemory(algArrays);

  switch(inputValues->ScalarType)
  {
  case DataType::int8: {
    dataAlg.execute(ConvertDataAlg<T, int8>(inputStore, dataStructure.getDataRefAs<Int8Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::uint8: {
    dataAlg.execute(ConvertDataAlg<T, uint8>(inputStore, dataStructure.getDataRefAs<UInt8Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::int16: {
    dataAlg.execute(ConvertDataAlg<T, int16>(inputStore, dataStructure.getDataRefAs<Int16Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::uint16: {
    dataAlg.execute(ConvertDataAlg<T, uint16>(inputStore, dataStructure.getDataRefAs<UInt16Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::int32: {
    dataAlg.execute(ConvertDataAlg<T, int32>(inputStore, dataStructure.getDataRefAs<Int32Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::uint32: {
    dataAlg.execute(ConvertDataAlg<T, uint32>(inputStore, dataStructure.getDataRefAs<UInt32Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::int64: {
    dataAlg.execute(ConvertDataAlg<T, int64>(inputStore, dataStructure.getDataRefAs<Int64Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::uint64: {
    dataAlg.execute(ConvertDataAlg<T, uint64>(inputStore, dataStructure.getDataRefAs<UInt64Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::float32: {
    dataAlg.execute(ConvertDataAlg<T, float32>(inputStore, dataStructure.getDataRefAs<Float32Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::float64: {
    dataAlg.execute(ConvertDataAlg<T, float64>(inputStore, dataStructure.getDataRefAs<Float64Array>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  case DataType::boolean: {
    dataAlg.execute(ConvertDataAlg<T, bool>(inputStore, dataStructure.getDataRefAs<BoolArray>(inputValues->OutputArrayName).getDataStoreRef()));
    break;
  }
  default: {
    return MakeErrorResult(
        -399, fmt::format("Error Converting DataArray '{}' from type {} to type {}", inputArray.getName(), static_cast<int>(inputArray.getDataType()), static_cast<int>(inputValues->ScalarType)));
  }
  }
  return {};
}
} // namespace Detail

// -----------------------------------------------------------------------------
ConvertData::ConvertData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ConvertData::~ConvertData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ConvertData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ConvertData::operator()()
{
  DataType inputArrayType = m_DataStructure.getDataAs<IDataArray>(m_InputValues->SelectedArrayPath)->getDataType();
  switch(inputArrayType)
  {
  case DataType::int8: {
    return Detail::ConvertData<int8>(m_DataStructure, m_InputValues);
  }
  case DataType::uint8: {
    return Detail::ConvertData<uint8>(m_DataStructure, m_InputValues);
  }
  case DataType::int16: {
    return Detail::ConvertData<int16>(m_DataStructure, m_InputValues);
  }
  case DataType::uint16: {
    return Detail::ConvertData<uint16>(m_DataStructure, m_InputValues);
  }
  case DataType::int32: {
    return Detail::ConvertData<int32>(m_DataStructure, m_InputValues);
  }
  case DataType::uint32: {
    return Detail::ConvertData<uint32>(m_DataStructure, m_InputValues);
  }
  case DataType::int64: {
    return Detail::ConvertData<int64>(m_DataStructure, m_InputValues);
  }
  case DataType::uint64: {
    return Detail::ConvertData<uint64>(m_DataStructure, m_InputValues);
  }
  case DataType::float32: {
    return Detail::ConvertData<float32>(m_DataStructure, m_InputValues);
  }
  case DataType::float64: {
    return Detail::ConvertData<float64>(m_DataStructure, m_InputValues);
  }
  case DataType::boolean: {
    return Detail::ConvertData<bool>(m_DataStructure, m_InputValues);
  }
  }

  return {};
}
