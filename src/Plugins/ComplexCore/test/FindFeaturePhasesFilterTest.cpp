#include <catch2/catch.hpp>

#include "ComplexCore/Filters/FindFeaturePhasesFilter.hpp"
#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
const DataPath k_CellPhasesPath({"Cell Phases"});
const DataPath k_FeatureIDsPath({"Feature IDs"});
const DataPath k_FeaturePhasesPath({"Feature Phases"});
const std::string k_PhasesFileName = "Phases.raw";
const std::string k_FeatureIdsFileName = "FeatureIds.raw";

DataStructure createDataStructure()
{
  DataStructure ds;
  Arguments args;

  RawBinaryReaderFilter rbrFilter;
  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_PhasesFileName));
  args.insertOrAssign(RawBinaryReaderFilter::k_NumberOfComponents_Key, std::make_any<uint64>(1));
  args.insertOrAssign(RawBinaryReaderFilter::k_ScalarType_Key, NumericType::int32);
  args.insertOrAssign(RawBinaryReaderFilter::k_TupleDims_Key, DynamicTableParameter::ValueType({{{1000000}}, {}, {}}));
  args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));

  auto result = rbrFilter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  args.insertOrAssign(RawBinaryReaderFilter::k_InputFile_Key, fs::path(unit_test::k_TestDataSourceDir.str()).append(k_FeatureIdsFileName));
  args.insertOrAssign(RawBinaryReaderFilter::k_CreatedAttributeArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));

  result = rbrFilter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  return ds;
}
} // namespace

TEST_CASE("FindFeaturePhasesFilter(Valid Parameters)", "[ComplexCore][FindFeaturePhasesFilter]")
{
  FindFeaturePhasesFilter ffpFilter;
  Arguments args;
  DataStructure ds = createDataStructure();

  args.insert(FindFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(k_CellPhasesPath));
  args.insert(FindFeaturePhasesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIDsPath));
  args.insert(FindFeaturePhasesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(k_FeaturePhasesPath));

  auto preflightResult = ffpFilter.preflight(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto result = ffpFilter.execute(ds, args);
  COMPLEX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(ds.getDataRefAs<Int32Array>(k_FeaturePhasesPath));

  Int32Array& featurePhases = ds.getDataRefAs<Int32Array>(k_FeaturePhasesPath);
  REQUIRE(featurePhases.getSize() == 796);

  Int32Array::Iterator iter = featurePhases.begin();
  REQUIRE(*iter == 0);

  iter++;
  for(; iter != featurePhases.end(); iter++)
  {
    REQUIRE(*iter == 1);
  }
}
