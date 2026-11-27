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
  Result<T> result = nx::core::string_interpretation_utilities::Convert<T>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template class SIMPLNX_TEMPLATE_EXPORT DataArray<int8>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<uint8>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<int16>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<uint16>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<int32>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<uint32>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<int64>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<uint64>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<float32>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<float64>;
template class SIMPLNX_TEMPLATE_EXPORT DataArray<bool>;
} // namespace nx::core
