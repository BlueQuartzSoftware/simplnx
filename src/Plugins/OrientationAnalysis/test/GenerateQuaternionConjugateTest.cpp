#include <catch2/catch.hpp>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "OrientationAnalysis/Filters/GenerateQuaternionConjugateFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;
namespace
{
const std::string k_QuatName = "Quats";
const std::string k_ConvertedName = "Converted";
const std::string k_Exemplar0 = "Exemplar0";

} // namespace

TEST_CASE("OrientationAnalysis::GenerateQuaternionConjugateFilter", "[OrientationAnalysis][GenerateQuaternionConjugateFilter]")
{
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
    const GenerateQuaternionConjugateFilter filter;

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(GenerateQuaternionConjugateFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({k_QuatName})));
    args.insertOrAssign(GenerateQuaternionConjugateFilter::k_OutputDataArrayPath_Key, std::make_any<DataPath>(DataPath({k_ConvertedName})));
    args.insertOrAssign(GenerateQuaternionConjugateFilter::k_DeleteOriginalData_Key, std::make_any<bool>(false));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

    auto& outputArray = dataStructure.getDataRefAs<Float32Array>(DataPath({k_ConvertedName}));
    UnitTest::CompareDataArrays<float32>(*exemplarData, outputArray);
  }
}
