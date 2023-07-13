#pragma once

#include "complex/Common/Types.hpp"

#include <stdexcept>

namespace complex
{
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
}
} // namespace complex
