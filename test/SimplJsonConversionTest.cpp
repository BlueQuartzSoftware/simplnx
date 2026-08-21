#include <catch2/catch.hpp>

#include "simplnx/Common/NumericBounds.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <limits>
#include <map>

using namespace nx::core;

TEST_CASE("nx::core::Test SIMPL Json Conversion", "[simplnx][Filter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "simpl_json_exemplars.tar.gz", "simpl_json_exemplars");
  // Read Exemplar DREAM3D File Filter
  auto exemplarDirPath = fs::path(fmt::format("{}/simpl_json_exemplars/uuid", unit_test::k_TestFilesDir));

  auto appPtr = Application::GetOrCreateInstance();
  FilterList* filterList = appPtr->getFilterList();

  // Loop on each Plugin
  for(const auto* plugin : appPtr->getPluginList())
  {
    for(const auto& [simplID, conversionData] : plugin->getSimplToSimplnxMap())
    {
      IFilter::UniquePointer filter = filterList->createFilter(conversionData.simplnxUuid);
      REQUIRE(filter != nullptr);
      fs::path simplJsonFilePath = exemplarDirPath / fmt::format("{}.json", simplID.str());
      if(std::filesystem::exists(simplJsonFilePath))
      {
        Result<Pipeline> result = Pipeline::FromSIMPLFile(simplJsonFilePath, filterList);
        SIMPLNX_RESULT_REQUIRE_VALID(result)
      }
      else
      {
        fmt::print("{}::{}    SIMPL UUID FILE DOES NOT EXIST: {}\n", plugin->getName(), filter->className(), simplJsonFilePath.string());
      }
    }
  }
}

namespace
{
// The largest float64 that is strictly less than 2^63 and 2^64 respectively. Both round-trip
// through the corresponding integer type without overflowing.
constexpr float64 k_LargestInt64Double = 9223372036854774784.0;
constexpr float64 k_LargestUInt64Double = 18446744073709549568.0;

// 2^63 and 2^64. These are the first float64 values that do *not* fit in int64 / uint64. Because
// std::numeric_limits<int64>::max() rounds up to exactly 2^63 when converted to a float64, a naive
// `value > max()` range check accepts these and then invokes undefined behavior in the cast.
constexpr float64 k_TwoToThe63 = 9223372036854775808.0;
constexpr float64 k_TwoToThe64 = 18446744073709551616.0;
} // namespace

TEST_CASE("nx::core::SIMPL Json Conversion 64 Bit Integer Bounds", "[simplnx][Parameters]")
{
  SECTION("The bound helpers themselves")
  {
    STATIC_REQUIRE(ExceedsMaxOf<int64>(k_TwoToThe63));
    STATIC_REQUIRE_FALSE(ExceedsMaxOf<int64>(k_LargestInt64Double));
    STATIC_REQUIRE(ExceedsMaxOf<uint64>(k_TwoToThe64));
    STATIC_REQUIRE_FALSE(ExceedsMaxOf<uint64>(k_LargestUInt64Double));
    STATIC_REQUIRE(ExceedsLowestOf<int64>(-k_TwoToThe63 * 2.0));
    STATIC_REQUIRE_FALSE(ExceedsLowestOf<int64>(-k_TwoToThe63));
    STATIC_REQUIRE(ExceedsLowestOf<uint64>(-1.0));
    STATIC_REQUIRE_FALSE(ExceedsMaxOf<int32>(static_cast<float64>(std::numeric_limits<int32>::max())));
    STATIC_REQUIRE(ExceedsMaxOf<int32>(static_cast<float64>(std::numeric_limits<int32>::max()) + 1.0));
  }

  SECTION("IntFilterParameterConverter rejects a float64 that overflows the target type")
  {
    Result<int64> int64Result = SIMPLConversion::IntFilterParameterConverter<int64>::convert(nlohmann::json(k_TwoToThe63));
    SIMPLNX_RESULT_REQUIRE_INVALID(int64Result);

    Result<uint64> uint64Result = SIMPLConversion::IntFilterParameterConverter<uint64>::convert(nlohmann::json(k_TwoToThe64));
    SIMPLNX_RESULT_REQUIRE_INVALID(uint64Result);
  }

  SECTION("IntFilterParameterConverter accepts a float64 at the edge of the target type")
  {
    Result<int64> int64Result = SIMPLConversion::IntFilterParameterConverter<int64>::convert(nlohmann::json(k_LargestInt64Double));
    SIMPLNX_RESULT_REQUIRE_VALID(int64Result);
    REQUIRE(int64Result.value() == static_cast<int64>(k_LargestInt64Double));

    Result<uint64> uint64Result = SIMPLConversion::IntFilterParameterConverter<uint64>::convert(nlohmann::json(k_LargestUInt64Double));
    SIMPLNX_RESULT_REQUIRE_VALID(uint64Result);
    REQUIRE(uint64Result.value() == static_cast<uint64>(k_LargestUInt64Double));
  }

  SECTION("Vec3FilterParameterConverter rejects a float64 that overflows the target type")
  {
    nlohmann::json overflowJson = {{"x", 0.0}, {"y", 0.0}, {"z", k_TwoToThe63}};
    Result<std::vector<int64>> int64Result = SIMPLConversion::Vec3FilterParameterConverter<int64>::convert(overflowJson);
    SIMPLNX_RESULT_REQUIRE_INVALID(int64Result);

    nlohmann::json unsignedOverflowJson = {{"x", 0.0}, {"y", 0.0}, {"z", k_TwoToThe64}};
    Result<std::vector<uint64>> uint64Result = SIMPLConversion::Vec3FilterParameterConverter<uint64>::convert(unsignedOverflowJson);
    SIMPLNX_RESULT_REQUIRE_INVALID(uint64Result);
  }

  SECTION("Vec3FilterParameterConverter accepts a float64 at the edge of the target type")
  {
    nlohmann::json edgeJson = {{"x", 0.0}, {"y", 0.0}, {"z", k_LargestInt64Double}};
    Result<std::vector<int64>> int64Result = SIMPLConversion::Vec3FilterParameterConverter<int64>::convert(edgeJson);
    SIMPLNX_RESULT_REQUIRE_VALID(int64Result);
    REQUIRE(int64Result.value()[2] == static_cast<int64>(k_LargestInt64Double));
  }
}
