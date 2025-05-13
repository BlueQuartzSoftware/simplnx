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
    return ConvertResult(StringInterpretationUtilities::Convert<T>(value));
  }
};
} // namespace

Result<> StringInterpretationUtilities::CheckValueConverts(DataType type, const std::string& value)
{
  return ExecuteDataFunction(ConvertFunctor{}, type, value);
}
