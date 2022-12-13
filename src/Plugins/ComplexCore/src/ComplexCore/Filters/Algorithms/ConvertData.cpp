#include "ConvertData.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

namespace Detail
{
template <typename O, typename D>
void ConvertData(DataArray<O>& inputArray, DataArray<D>& outputArray)
{
  size_t size = inputArray.getSize();
  for(size_t v = 0; v < size; ++v)
  {
    if constexpr(std::is_same<O, D>::value)
    {
      // inputArray and destination arrays have the same type
      outputArray[v] = inputArray[v];
    }
    else if constexpr(std::is_same<O, bool>::value)
    {
      // inputArray array is a boolean array
      outputArray[v] = (inputArray[v] ? 1 : 0);
    }
    else if constexpr(std::is_same<D, bool>::value)
    {
      // Destination array is a boolean array
      outputArray[v] = (inputArray[v] != 0);
    }
    else
    {
      // All other cases
      outputArray[v] = static_cast<D>(inputArray[v]);
    }
  }
}

template <typename T>
Result<> ConvertData(DataStructure& dataStructure, const ConvertDataInputValues* inputValues)
{
  DataArray<T>& inputArray = dataStructure.getDataRefAs<DataArray<T>>(inputValues->SelectedArrayPath);

  switch(inputValues->ScalarType)
  {
  case DataType::int8: {
    ConvertData<T, int8>(inputArray, dataStructure.getDataRefAs<Int8Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::uint8: {
    ConvertData<T, uint8>(inputArray, dataStructure.getDataRefAs<UInt8Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::int16: {
    ConvertData<T, int16>(inputArray, dataStructure.getDataRefAs<Int16Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::uint16: {
    ConvertData<T, uint16>(inputArray, dataStructure.getDataRefAs<UInt16Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::int32: {
    ConvertData<T, int32>(inputArray, dataStructure.getDataRefAs<Int32Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::uint32: {
    ConvertData<T, uint32>(inputArray, dataStructure.getDataRefAs<UInt32Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::int64: {
    ConvertData<T, int64>(inputArray, dataStructure.getDataRefAs<Int64Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::uint64: {
    ConvertData<T, uint64>(inputArray, dataStructure.getDataRefAs<UInt64Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::float32: {
    ConvertData<T, float32>(inputArray, dataStructure.getDataRefAs<Float32Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::float64: {
    ConvertData<T, float64>(inputArray, dataStructure.getDataRefAs<Float64Array>(inputValues->OutputArrayName));
    break;
  }
  case DataType::boolean: {
    ConvertData<T, bool>(inputArray, dataStructure.getDataRefAs<BoolArray>(inputValues->OutputArrayName));
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
