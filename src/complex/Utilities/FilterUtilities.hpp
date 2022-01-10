#pragma once

#include <exception>

#include "complex/Common/Types.hpp"

#define EXECUTE_DATA_FUNCTION_TEMPLATE(templateFunct, dataType, ...)                                                                                                                                   \
  switch(dataType)                                                                                                                                                                                     \
  {                                                                                                                                                                                                    \
  case complex::DataType::boolean:                                                                                                                                                                     \
    templateFunct<bool>(__VA_ARGS__);                                                                                                                                                                  \
    break;                                                                                                                                                                                             \
  case complex::DataType::int8:                                                                                                                                                                        \
    templateFunct<int8_t>(__VA_ARGS__);                                                                                                                                                                \
    break;                                                                                                                                                                                             \
  case complex::DataType::int16:                                                                                                                                                                       \
    templateFunct<int16_t>(__VA_ARGS__);                                                                                                                                                               \
    break;                                                                                                                                                                                             \
  case complex::DataType::int32:                                                                                                                                                                       \
    templateFunct<int32_t>(__VA_ARGS__);                                                                                                                                                               \
    break;                                                                                                                                                                                             \
  case complex::DataType::int64:                                                                                                                                                                       \
    templateFunct<int64_t>(__VA_ARGS__);                                                                                                                                                               \
    break;                                                                                                                                                                                             \
  case complex::DataType::uint8:                                                                                                                                                                       \
    templateFunct<uint8_t>(__VA_ARGS__);                                                                                                                                                               \
    break;                                                                                                                                                                                             \
  case complex::DataType::uint16:                                                                                                                                                                      \
    templateFunct<uint16_t>(__VA_ARGS__);                                                                                                                                                              \
    break;                                                                                                                                                                                             \
  case complex::DataType::uint32:                                                                                                                                                                      \
    templateFunct<uint32_t>(__VA_ARGS__);                                                                                                                                                              \
    break;                                                                                                                                                                                             \
  case complex::DataType::uint64:                                                                                                                                                                      \
    templateFunct<uint64_t>(__VA_ARGS__);                                                                                                                                                              \
    break;                                                                                                                                                                                             \
  case complex::DataType::float32:                                                                                                                                                                     \
    templateFunct<float>(__VA_ARGS__);                                                                                                                                                                 \
    break;                                                                                                                                                                                             \
  case complex::DataType::float64:                                                                                                                                                                     \
    templateFunct<double>(__VA_ARGS__);                                                                                                                                                                \
    break;                                                                                                                                                                                             \
  default:                                                                                                                                                                                             \
    throw std::runtime_error("Invalid DataType");                                                                                                                                                      \
    break;                                                                                                                                                                                             \
  }
