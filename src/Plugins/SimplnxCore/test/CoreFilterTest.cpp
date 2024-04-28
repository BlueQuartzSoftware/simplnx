#include "SimplnxCore/Filters/CreateDataGroupFilter.hpp"
#include "SimplnxCore/Filters/ReadTextDataArrayFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

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

TEST_CASE("CoreFilterTest:Create Core Filter")
{
  auto app = Application::GetOrCreateInstance();
  app->loadPlugins(unit_test::k_BuildDir.view());
  auto filterList = app->getFilterList();
  REQUIRE(filterList != nullptr);

  // Only core filters should exist since plugins were not loaded
  const auto& handles = filterList->getFilterHandles();
  REQUIRE(!handles.empty());

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

TEST_CASE("CoreFilterTest:RunCoreFilter")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  static const fs::path k_FileName = fmt::format("{}/ascii_data.txt", nx::core::unit_test::k_BinaryTestOutputDir);
  static constexpr uint64 k_NLines = 25;

  SECTION("Create ASCII File")
  {
    std::ofstream file(k_FileName.c_str());
    for(int32 i = 0; i < k_NLines; i++)
    {
      file << i << "," << i + 1 << "," << i + 2 << "\n";
    }
  }
  SECTION("Run ReadTextDataArrayFilter")
  {
    static constexpr uint64 k_NComp = 3;
    static constexpr uint64 k_NSkipLines = 0;
    const static DynamicTableInfo::TableDataType k_TupleDims = {{static_cast<double>(k_NLines)}};

    ReadTextDataArrayFilter filter;
    DataStructure dataStructure;
    Arguments args;
    DataPath dataPath({"foo"});

    args.insert(ReadTextDataArrayFilter::k_InputFile_Key, std::make_any<fs::path>(k_FileName));
    args.insert(ReadTextDataArrayFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::int32));
    args.insert(ReadTextDataArrayFilter::k_NTuples_Key, std::make_any<DynamicTableParameter::ValueType>(k_TupleDims));
    args.insert(ReadTextDataArrayFilter::k_NComp_Key, std::make_any<uint64>(k_NComp));
    args.insert(ReadTextDataArrayFilter::k_NSkipLines_Key, std::make_any<uint64>(k_NSkipLines));
    args.insert(ReadTextDataArrayFilter::k_DelimiterChoice_Key, std::make_any<uint64>(0));
    args.insert(ReadTextDataArrayFilter::k_DataArrayPath_Key, std::make_any<DataPath>(dataPath));

    // auto callback = [](const IFilter::Message& message) { fmt::print("{}: {}\n", message.type, message.message); };
    // Result<> result = filter.execute(dataStructure, args, IFilter::MessageHandler{callback});
    IFilter::ExecuteResult result = filter.execute(dataStructure, args);
    for(const auto& warning : result.result.warnings())
    {
      // fmt::print("Warning {}: {}\n", warning.code, warning.message);
      UNSCOPED_INFO(fmt::format("Warning {}: {}", warning.code, warning.message));
    }
    // std::vector<Error> errors = result.valid() ? std::vector<Error>{} : result.errors();
    if(!result.result.valid())
    {
      for(const auto& error : result.result.errors())
      {
        // fmt::print("Error {}: {}\n", error.code, error.message);
        UNSCOPED_INFO(fmt::format("Error {}: {}", error.code, error.message));
      }
    }
    CAPTURE(result.result.warnings());
    // CAPTURE(errors);
    REQUIRE(result.result.valid());
    const auto* dataArrayPtr = dynamic_cast<DataArray<int32>*>(dataStructure.getData(dataPath));
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

TEST_CASE("CoreFilterTest:CreateDataGroupFilter")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  DataStructure data;
  CreateDataGroupFilter filter;
  Arguments args;
  const DataPath path({"foo", "bar", "baz"});
  args.insert(CreateDataGroupFilter::k_DataObjectPath, path);
  auto result = filter.execute(data, args);
  REQUIRE(result.result.valid());
  DataObject* object = data.getData(path);
  REQUIRE(object != nullptr);
  auto* group = dynamic_cast<DataGroup*>(object);
  REQUIRE(group != nullptr);
  REQUIRE(data.getSize() == path.getLength());
}
