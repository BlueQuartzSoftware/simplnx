#include "DataArray.hpp"

#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

namespace nx::core
{
template <>
std::string SIMPLNX_EXPORT DataArray<int8>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const int8 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::format("{}", value);
}

template <>
std::string SIMPLNX_EXPORT DataArray<int16>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const int16 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::format("{}", value);
}

template <>
std::string SIMPLNX_EXPORT DataArray<int32>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const int32 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::format("{}", value);
}

template <>
std::string SIMPLNX_EXPORT DataArray<int64>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const int64 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::format("{}", value);
}

template <>
std::string SIMPLNX_EXPORT DataArray<uint8>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const uint8 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::format("{}", value);
}

template <>
std::string SIMPLNX_EXPORT DataArray<uint16>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const uint16 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::format("{}", value);
}

template <>
std::string SIMPLNX_EXPORT DataArray<uint32>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const uint32 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::format("{}", value);
}

template <>
std::string SIMPLNX_EXPORT DataArray<uint64>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const uint64 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::format("{}", value);
}

template <>
std::string SIMPLNX_EXPORT DataArray<float32>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const float32 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::vformat(format, fmt::make_format_args(value));
}

template <>
std::string SIMPLNX_EXPORT DataArray<float64>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const float64 value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::vformat(format, fmt::make_format_args(value));
}

template <>
std::string SIMPLNX_EXPORT DataArray<bool>::toString(const usize tupleIndex, const usize compIndex, const std::string& format) const
{
  const bool value = getValue(tupleIndex * getNumberOfComponents() + compIndex);
  return fmt::vformat(format, fmt::make_format_args(value));
}

template <>
bool SIMPLNX_EXPORT DataArray<int8>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<int8> result = nx::core::StringInterpretationUtilities::Convert<int8>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<uint8>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<uint8> result = nx::core::StringInterpretationUtilities::Convert<uint8>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<int16>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<int16> result = nx::core::StringInterpretationUtilities::Convert<int16>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<uint16>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<uint16> result = nx::core::StringInterpretationUtilities::Convert<uint16>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<int32>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<int32> result = nx::core::StringInterpretationUtilities::Convert<int32>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<uint32>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<uint32> result = nx::core::StringInterpretationUtilities::Convert<uint32>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<int64>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<int64> result = nx::core::StringInterpretationUtilities::Convert<int64>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<uint64>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<uint64> result = nx::core::StringInterpretationUtilities::Convert<uint64>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<float32>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<float32> result = nx::core::StringInterpretationUtilities::Convert<float32>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<float64>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<float64> result = nx::core::StringInterpretationUtilities::Convert<float64>(value);
  if(result.invalid())
  {
    return false;
  }
  setValue(tupleIndex * getNumberOfComponents() + compIndex, result.value());
  return true;
}

template <>
bool SIMPLNX_EXPORT DataArray<bool>::setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value)
{
  Result<bool> result = nx::core::StringInterpretationUtilities::Convert<bool>(value);
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
