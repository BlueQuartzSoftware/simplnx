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

template <class FuncT, class ParallelRunner, class... ArgsT>
auto ExecuteParallelFunction(FuncT&& func, DataType dataType, ParallelRunner&& runner, ArgsT&&... args)
{
  switch(dataType)
  {
  case DataType::boolean: {
    return func.template operator()<bool>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::int8: {
    return func.template operator()<int8>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::int16: {
    return func.template operator()<int16>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::int32: {
    return func.template operator()<int32>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::int64: {
    return func.template operator()<int64>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::uint8: {
    return func.template operator()<uint8>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::uint16: {
    return func.template operator()<uint16>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::uint32: {
    return func.template operator()<uint32>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::uint64: {
    return func.template operator()<uint64>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::float32: {
    return func.template operator()<float32>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  case DataType::float64: {
    return func.template operator()<float64>(std::forward<ParallelRunner>(runner), std::forward<ArgsT>(args)...);
  }
  default: {
    throw std::runtime_error("complex::ExecuteParallelFunction<...>(FuncT&& func, DataType dataType, ArgsT&&... args). Error: Invalid DataType");
  }
  }
}
} // namespace complex
