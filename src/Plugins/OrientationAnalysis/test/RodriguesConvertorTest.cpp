#include <catch2/catch.hpp>

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "OrientationAnalysis/Filters/RodriguesConvertorFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
namespace
{
const std::string k_InputArrayName = "Input";
const std::string k_ConvertedName = "Output";
const std::string k_Exemplar0 = "Exemplar0";

} // namespace

TEST_CASE("OrientationAnalysis::RodriguesConvertorFilter", "[OrientationAnalysis][RodriguesConvertorFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;

  // Build up a simple Float32Array and place default data into the array
  Float32Array* quats = UnitTest::CreateTestDataArray<float32>(dataStructure, k_InputArrayName, {4ULL}, {3ULL}, {});

  for(size_t i = 0; i < 12; i++)
  {
    (*quats)[i] = static_cast<float32>(i);
  }
  Float32Array* exemplarData = UnitTest::CreateTestDataArray<float32>(dataStructure, k_Exemplar0, {4ULL}, {4ULL}, {});
  (*exemplarData)[0] = 0.0F;
  (*exemplarData)[1] = 0.447214F;
  (*exemplarData)[2] = 0.894427F;
  (*exemplarData)[3] = 2.23607F;
  (*exemplarData)[4] = 0.424264F;
  (*exemplarData)[5] = 0.565685F;
  (*exemplarData)[6] = 0.707107F;
  (*exemplarData)[7] = 7.07107F;
  (*exemplarData)[8] = 0.491539F;
  (*exemplarData)[9] = 0.573462F;
  (*exemplarData)[10] = 0.655386F;
  (*exemplarData)[11] = 12.2066F;
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    const RodriguesConvertorFilter filter;

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(RodriguesConvertorFilter::k_RodriguesDataArrayPath_Key, std::make_any<DataPath>(DataPath({k_InputArrayName})));
    args.insertOrAssign(RodriguesConvertorFilter::k_OutputDataArrayPath_Key, std::make_any<std::string>(k_ConvertedName));
    args.insertOrAssign(RodriguesConvertorFilter::k_DeleteOriginalData_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto& outputArray = dataStructure.getDataRefAs<Float32Array>(DataPath({k_ConvertedName}));

    UnitTest::CompareDataArrays<float32>(*exemplarData, outputArray);
  }
}
