#include "StringInterpretationUtilities.hpp"

#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
struct ConvertFunctor
{
  template <typename T>
  Result<> operator()(const std::string& value)
  {
    return ConvertResult(string_interpretation_utilities::Convert<T>(value));
  }
};
} // namespace

Result<> string_interpretation_utilities::CheckValueConverts(DataType type, const std::string& value)
{
  return ExecuteDataFunction(ConvertFunctor{}, type, value);
}
