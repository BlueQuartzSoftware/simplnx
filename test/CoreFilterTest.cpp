#include <catch2/catch.hpp>

#include "complex/Common/StringLiteral.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include <filesystem>
#include <fstream>

using namespace complex;
namespace fs = std::filesystem;

namespace Catch
{
template <>
struct StringMaker<Error>
{
  static std::string convert(const Error& value)
  {
    return fmt::format("Error {}: {}", value.code, value.message);
  }
};

template <>
struct StringMaker<Warning>
{
  static std::string convert(const Warning& value)
  {
    return fmt::format("Warning {}: {}", value.code, value.message);
  }
};
} // namespace Catch
