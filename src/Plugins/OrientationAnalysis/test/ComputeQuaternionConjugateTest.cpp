#include <catch2/catch.hpp>

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "OrientationAnalysis/Filters/ComputeQuaternionConjugateFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
namespace
{
const std::string k_QuatName = "Quats";
const std::string k_ConvertedName = "Converted";
const std::string k_Exemplar0 = "Exemplar0";

} // namespace

TEST_CASE("OrientationAnalysis::ComputeQuaternionConjugateFilter", "[OrientationAnalysis][ComputeQuaternionConjugateFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;

  // Build up a simple Float32Array and place default data into the array
  Float32Array* quats = UnitTest::CreateTestDataArray<float32>(dataStructure, k_QuatName, {4ULL}, {4ULL}, {});

  for(size_t i = 0; i < 16; i++)
  {
    (*quats)[i] = static_cast<float32>(i);
  }
  Float32Array* exemplarData = UnitTest::CreateTestDataArray<float32>(dataStructure, k_Exemplar0, {4ULL}, {4ULL}, {});

  (*exemplarData)[0] = -0.0F;
  (*exemplarData)[1] = -1.0F;
  (*exemplarData)[2] = -2.0F;
  (*exemplarData)[3] = 3.0F;

  (*exemplarData)[4] = -4.0F;
  (*exemplarData)[5] = -5.0F;
  (*exemplarData)[6] = -6.0F;
  (*exemplarData)[7] = 7.0F;

  (*exemplarData)[8] = -8.0F;
  (*exemplarData)[9] = -9.0F;
  (*exemplarData)[10] = -10.0F;
  (*exemplarData)[11] = 11.0F;

  (*exemplarData)[12] = -12.0F;
  (*exemplarData)[13] = -13.0F;
  (*exemplarData)[14] = -14.0F;
  (*exemplarData)[15] = 15.0F;
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    const ComputeQuaternionConjugateFilter filter;

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ComputeQuaternionConjugateFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({k_QuatName})));
    args.insertOrAssign(ComputeQuaternionConjugateFilter::k_OutputDataArrayName_Key, std::make_any<std::string>(k_ConvertedName));
    args.insertOrAssign(ComputeQuaternionConjugateFilter::k_DeleteOriginalData_Key, std::make_any<bool>(false));

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
