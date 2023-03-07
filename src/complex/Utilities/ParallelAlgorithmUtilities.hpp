#pragma once

#include <stdexcept>

namespace complex
{
template <bool UseBoolean, bool UseInt8V, bool UseUInt8V, bool UseInt16V, bool UseUInt16V, bool UseInt32V, bool UseUInt32V, bool UseInt64V, bool UseUInt64V, bool UseFloat32V, bool UseFloat64V>
struct ArrayTypeOptions
{
  static inline constexpr bool UsingBoolean = UseBoolean;
  static inline constexpr bool UsingInt8 = UseInt8V;
  static inline constexpr bool UsingUInt8 = UseUInt8V;
  static inline constexpr bool UsingInt16 = UseInt16V;
  static inline constexpr bool UsingUInt16 = UseUInt16V;
  static inline constexpr bool UsingInt32 = UseInt32V;
  static inline constexpr bool UsingUInt32 = UseUInt32V;
  static inline constexpr bool UsingInt64 = UseInt64V;
  static inline constexpr bool UsingUInt64 = UseUInt64V;
  static inline constexpr bool UsingFloat32 = UseFloat32V;
  static inline constexpr bool UsingFloat64 = UseFloat64V;
};

using ArrayUseAllTypes = ArrayTypeOptions<true, true, true, true, true, true, true, true, true, true, true>;
using NoBooleanType = ArrayTypeOptions<false, true, true, true, true, true, true, true, true, true, true>;
using ArrayUseIntegerTypes = ArrayTypeOptions<false, true, true, true, true, true, true, true, true, false, false>;
using ArrayUseFloatingTypes = ArrayTypeOptions<false, false, false, false, false, false, false, false, false, true, true>;
using ArrayUseSignedTypes = ArrayTypeOptions<false, true, false, true, false, true, false, true, false, true, true>;
using ArrayUseUnsignedTypes = ArrayTypeOptions<false, false, true, false, true, false, true, false, true, false, false>;

template <template <class> class ClassT, class ArrayTypeOptions = ArrayUseAllTypes, class ParallelRunnerT, class... ArgsT>
auto ExecuteParallelFunction(DataType dataType, ParallelRunnerT&& runner, ArgsT&&... args)
{
  if constexpr(ArrayTypeOptions::UsingBoolean)
  {
    if(dataType == DataType::boolean)
    {
      return runner.template execute<>(ClassT<bool>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt8)
  {
    if(dataType == DataType::int8)
    {
      return runner.template execute<>(ClassT<int8>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt16)
  {
    if(dataType == DataType::int16)
    {
      return runner.template execute<>(ClassT<int16>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt32)
  {
    if(dataType == DataType::int32)
    {
      return runner.template execute<>(ClassT<int32>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt64)
  {
    if(dataType == DataType::int64)
    {
      return runner.template execute<>(ClassT<int64>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt8)
  {
    if(dataType == DataType::uint8)
    {
      return runner.template execute<>(ClassT<uint8>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt16)
  {
    if(dataType == DataType::uint16)
    {
      return runner.template execute<>(ClassT<uint16>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt32)
  {
    if(dataType == DataType::uint32)
    {
      return runner.template execute<>(ClassT<uint32>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt64)
  {
    if(dataType == DataType::uint64)
    {
      return runner.template execute<>(ClassT<uint64>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingFloat32)
  {
    if(dataType == DataType::float32)
    {
      return runner.template execute<>(ClassT<float32>(std::forward<ArgsT>(args)...));
    }
  }
  if constexpr(ArrayTypeOptions::UsingFloat64)
  {
    if(dataType == DataType::float64)
    {
      return runner.template execute<>(ClassT<float64>(std::forward<ArgsT>(args)...));
    }
  }

  throw std::runtime_error("complex::ExecuteParallelFunction has all supported DataTypes disabled. Error: Invalid DataType");
}
} // namespace complex
