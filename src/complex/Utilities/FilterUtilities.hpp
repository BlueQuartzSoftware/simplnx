#pragma once

#include <stdexcept>

#include "complex/Common/Types.hpp"

namespace complex
{
template <class FuncT, class... ArgsT>
auto ExecuteDataFunction(FuncT&& func, DataType dataType, ArgsT&&... args)
{
  switch(dataType)
  {
  case DataType::boolean: {
    return func.template operator()<bool>(std::forward<ArgsT>(args)...);
  }
  case DataType::int8: {
    return func.template operator()<int8>(std::forward<ArgsT>(args)...);
  }
  case DataType::int16: {
    return func.template operator()<int16>(std::forward<ArgsT>(args)...);
  }
  case DataType::int32: {
    return func.template operator()<int32>(std::forward<ArgsT>(args)...);
  }
  case DataType::int64: {
    return func.template operator()<int64>(std::forward<ArgsT>(args)...);
  }
  case DataType::uint8: {
    return func.template operator()<uint8>(std::forward<ArgsT>(args)...);
  }
  case DataType::uint16: {
    return func.template operator()<uint16>(std::forward<ArgsT>(args)...);
  }
  case DataType::uint32: {
    return func.template operator()<uint32>(std::forward<ArgsT>(args)...);
  }
  case DataType::uint64: {
    return func.template operator()<uint64>(std::forward<ArgsT>(args)...);
  }
  case DataType::float32: {
    return func.template operator()<float32>(std::forward<ArgsT>(args)...);
  }
  case DataType::float64: {
    return func.template operator()<float64>(std::forward<ArgsT>(args)...);
  }
  default: {
    throw std::runtime_error("complex::ExecuteDataFunction<...>(FuncT&& func, DataType dataType, ArgsT&&... args). Error: Invalid DataType");
  }
  }
}
} // namespace complex
