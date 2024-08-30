#pragma once

#include "simplnx/Common/Types.hpp"

#include <fmt/format.h>

#include <stdexcept>

namespace nx::core
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

template <class FuncT, class ArrayTypeOptions = ArrayUseAllTypes, class ParallelRunnerT, class... ArgsT>
auto ExecuteParallelFunctor(FuncT&& func, DataType dataType, ParallelRunnerT&& runner, ArgsT&&... args)
{
  switch(dataType)
  {
  case DataType::boolean: {
    if constexpr(ArrayTypeOptions::UsingBoolean)
    {
      return runner.template execute<>(func.template operator()<bool>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  case DataType::int8: {
    if constexpr(ArrayTypeOptions::UsingInt8)
    {
      return runner.template execute<>(func.template operator()<int8>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  case DataType::int16: {
    if constexpr(ArrayTypeOptions::UsingInt16)
    {
      return runner.template execute<>(func.template operator()<int16>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  case DataType::int32: {
    if constexpr(ArrayTypeOptions::UsingInt32)
    {
      return runner.template execute<>(func.template operator()<int32>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  case DataType::int64: {
    if constexpr(ArrayTypeOptions::UsingInt64)
    {
      return runner.template execute<>(func.template operator()<int64>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  case DataType::uint8: {
    if constexpr(ArrayTypeOptions::UsingUInt8)
    {
      return runner.template execute<>(func.template operator()<uint8>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  case DataType::uint16: {
    if constexpr(ArrayTypeOptions::UsingUInt16)
    {
      return runner.template execute<>(func.template operator()<uint16>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  case DataType::uint32: {
    if constexpr(ArrayTypeOptions::UsingUInt32)
    {
      return runner.template execute<>(func.template operator()<uint32>(std::forward<ArgsT>(args)...));
    }
  }
  case DataType::uint64: {
    if constexpr(ArrayTypeOptions::UsingUInt64)
    {
      return runner.template execute<>(func.template operator()<uint64>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  case DataType::float32: {
    if constexpr(ArrayTypeOptions::UsingFloat32)
    {
      return runner.template execute<>(func.template operator()<float32>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  case DataType::float64: {
    if constexpr(ArrayTypeOptions::UsingFloat64)
    {
      return runner.template execute<>(func.template operator()<float64>(std::forward<ArgsT>(args)...));
    }
    break;
  }
  default: {
    throw std::runtime_error(fmt::format("nx::core::{}: Error: Invalid DataType. {}:{}", __func__, __FILE__, __LINE__));
  }
  }

  throw std::runtime_error(fmt::format("nx::core::{}: Error: Valid DataType, but the template declaration has this as a restricted type. {}:{}", __func__, __FILE__, __LINE__));
}
} // namespace nx::core
