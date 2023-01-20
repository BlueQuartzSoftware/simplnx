#pragma once

#include "complex/Common/Types.hpp"

#include <stdexcept>

namespace complex
{
template <class ParallelRunnerT>
class ParallelRunner
{
public:
  explicit ParallelRunner(ParallelRunnerT& parallelRunner)
  : m_ParallelRunner(parallelRunner)
  {
  }

  template <typename Body>
  void execute(Body&& body)
  {
    m_ParallelRunner.execute(std::forward<Body>(body));
  }

private:
  ParallelRunnerT& m_ParallelRunner;
};

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

template <template <class> class ClassT, class ArrayTypeOptions, class ParallelRunner, class... ArgsT>
auto ExecuteParallelFunction(const IDataStore& dataStore, ParallelRunner&& runner, ArgsT&&... args)
{
  DataType dataType = dataStore.getDataType();

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
      return func.template operator()<int8>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt16)
  {
    if(dataType == DataType::int16)
    {
      return func.template operator()<int16>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt32)
  {
    if(dataType == DataType::int32)
    {
      return func.template operator()<int32>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingInt64)
  {
    if(dataType == DataType::int64)
    {
      return func.template operator()<int64>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt8)
  {
    if(dataType == DataType::uint8)
    {
      return func.template operator()<uint8>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt16)
  {
    if(dataType == DataType::uint16)
    {
      return func.template operator()<uint16>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt32)
  {
    if(dataType == DataType::uint32)
    {
      return func.template operator()<uint32>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingUInt64)
  {
    if(dataType == DataType::uint64)
    {
      return func.template operator()<uint64>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingFloat32)
  {
    if(dataType == DataType::float32)
    {
      return func.template operator()<float32>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }
  if constexpr(ArrayTypeOptions::UsingFloat64)
  {
    if(dataType == DataType::float64)
    {
      return func.template operator()<float64>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
    }
  }

  throw std::runtime_error("complex::ExecuteParallelFunction has all supported DataTypes disabled. Error: Invalid DataType");
}
} // namespace complex
