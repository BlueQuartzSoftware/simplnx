#include "DataArray.hpp"

#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

namespace nx::core
{
template <class T>
std::string DataArray<T>::toString(usize tupleIndex, usize compIndex, const std::string& format) const
{
  const T value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  if constexpr(std::is_floating_point_v<T>)
  {
    return fmt::format(fmt::runtime(format), value);
  }
  else
  {
    return fmt::format("{}", value);
  }
}

template <class T>
bool DataArray<T>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value)
{
  Result<T> result = nx::core::StringInterpretationUtilities::Convert<T>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<int8>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<int16>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<int32>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<int64>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<uint8>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<uint16>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<uint32>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<uint64>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<float32>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<float64>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
template SIMPLNX_TEMPLATE_EXPORT std::string DataArray<bool>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;

template SIMPLNX_TEMPLATE_EXPORT bool DataArray<int8>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<uint8>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<int16>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<uint16>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<int32>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<uint32>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<int64>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<uint64>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<float32>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<float64>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
template SIMPLNX_TEMPLATE_EXPORT bool DataArray<bool>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
} // namespace nx::core
