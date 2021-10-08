#include <catch2/catch.hpp>

#include "complex/Common/StringLiteral.hpp"
#include "complex/Core/Application.hpp"
#include "complex/Core/Filters/CreateDataGroup.hpp"
#include "complex/Core/Filters/ImportTextFilter.hpp"
#include "complex/Core/Parameters/FileSystemPathParameter.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Filter/IFilter.hpp"

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

TEST_CASE("Create Core Filter")
{
  Application app;
  auto filterList = app.getFilterList();
  REQUIRE(filterList != nullptr);

  // Only core filters should exists since plugins were not loaded
  const auto& handles = filterList->getFilterHandles();
  REQUIRE(handles.size() > 0);

  for(const auto& handle : handles)
  {
    auto coreFilter = filterList->createFilter(handle);
    REQUIRE(coreFilter != nullptr);
    auto params = coreFilter->parameters();
    for(const auto& [name, param] : params)
    {
      REQUIRE(!name.empty());
      REQUIRE(!param.isEmpty());
    }
  }
}

TEST_CASE("RunCoreFilter")
{
  static const fs::path k_FileName = "ascii_data.txt";
  static constexpr uint64 k_NLines = 25;

  SECTION("Create ASCII File")
  {
    std::ofstream file(k_FileName.c_str());
    for(int32 i = 0; i < k_NLines; i++)
    {
      file << i << "," << i + 1 << "," << i + 2 << "\n";
    }
  }
  SECTION("Run ImportTextFilter")
  {
    static constexpr uint64 k_NComp = 3;
    static constexpr uint64 k_NSkipLines = 0;

    ImportTextFilter filter;
    DataStructure ds;
    Arguments args;
    DataPath dataPath({"foo"});

    args.insert("input_file", std::make_any<fs::path>(k_FileName));
    args.insert("scalar_type", std::make_any<NumericType>(NumericType::int32));
    args.insert("n_tuples", std::make_any<uint64>(k_NLines));
    args.insert("n_comp", std::make_any<uint64>(k_NComp));
    args.insert("n_skip_lines", std::make_any<uint64>(k_NSkipLines));
    args.insert("delimiter_choice", std::make_any<uint64>(0));
    args.insert("output_data_array", std::make_any<DataPath>(dataPath));

    // auto callback = [](const IFilter::Message& message) { fmt::print("{}: {}\n", message.type, message.message); };
    // Result<> result = filter.execute(ds, args, IFilter::MessageHandler{callback});
    Result<> result = filter.execute(ds, args);
    for(const auto& warning : result.warnings())
    {
      // fmt::print("Warning {}: {}\n", warning.code, warning.message);
      UNSCOPED_INFO(fmt::format("Warning {}: {}", warning.code, warning.message));
    }
    // std::vector<Error> errors = result.valid() ? std::vector<Error>{} : result.errors();
    if(!result.valid())
    {
      for(const auto& error : result.errors())
      {
        // fmt::print("Error {}: {}\n", error.code, error.message);
        UNSCOPED_INFO(fmt::format("Error {}: {}", error.code, error.message));
      }
    }
    CAPTURE(result.warnings());
    // CAPTURE(errors);
    REQUIRE(result.valid());
    const auto* dataArrayPtr = dynamic_cast<DataArray<int32>*>(ds.getData(dataPath));
    REQUIRE(dataArrayPtr != nullptr);
    const auto& dataArray = *dataArrayPtr;
    for(int32 i = k_NSkipLines; i < k_NLines; i++)
    {
      usize index = (i - k_NSkipLines) * k_NComp;
      REQUIRE(dataArray[index] == i);
      REQUIRE(dataArray[index + 1] == i + 1);
      REQUIRE(dataArray[index + 2] == i + 2);
    }
  }
}

TEST_CASE("CreateDataGroup")
{
  DataStructure data;
  CreateDataGroup filter;
  Arguments args;
  const DataPath path({"foo", "bar", "baz"});
  args.insert(CreateDataGroup::k_DataObjectPath, path);
  auto result = filter.execute(data, args);
  REQUIRE(result.valid());
  DataObject* object = data.getData(path);
  REQUIRE(object != nullptr);
  auto* group = dynamic_cast<DataGroup*>(object);
  REQUIRE(group != nullptr);
  REQUIRE(data.getSize() == path.getLength());
}
