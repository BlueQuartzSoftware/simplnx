#include "AbstractDataStore.hpp"

using namespace complex;

template <>
DataType COMPLEX_EXPORT AbstractDataStore<int8>::getDataType() const
{
  return DataType::int8;
}

template <>
DataType COMPLEX_EXPORT AbstractDataStore<int16>::getDataType() const
{
  return DataType::int16;
}

template <>
DataType COMPLEX_EXPORT AbstractDataStore<int32>::getDataType() const
{
  return DataType::int32;
}

template <>
DataType COMPLEX_EXPORT AbstractDataStore<int64>::getDataType() const
{
  return DataType::int64;
}

template <>
DataType COMPLEX_EXPORT AbstractDataStore<uint8>::getDataType() const
{
  return DataType::uint8;
}

template <>
DataType COMPLEX_EXPORT AbstractDataStore<uint16>::getDataType() const
{
  return DataType::uint16;
}

template <>
DataType COMPLEX_EXPORT AbstractDataStore<uint32>::getDataType() const
{
  return DataType::uint32;
}

template <>
DataType COMPLEX_EXPORT AbstractDataStore<uint64>::getDataType() const
{
  return DataType::uint64;
}

#if defined(__APPLE__)
template <>
DataType COMPLEX_EXPORT AbstractDataStore<unsigned long>::getDataType() const
{
  return DataType::uint64;
}
#endif

template <>
DataType COMPLEX_EXPORT AbstractDataStore<float32>::getDataType() const
{
  return DataType::float32;
}

template <>
DataType COMPLEX_EXPORT AbstractDataStore<float64>::getDataType() const
{
  return DataType::float64;
}

template <>
DataType COMPLEX_EXPORT AbstractDataStore<bool>::getDataType() const
{
  return DataType::boolean;
}
