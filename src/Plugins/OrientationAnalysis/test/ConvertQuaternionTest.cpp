#include <catch2/catch.hpp>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "OrientationAnalysis/Filters/ConvertQuaternionFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace complex;

namespace
{
const std::string k_QuatName = "Quats";
const std::string k_ConvertedName = "Converted";
const std::string k_Exemplar0 = "Exemplar0";
const std::string k_Exemplar1 = "Exemplar1";
constexpr ChoicesParameter::ValueType k_ToScalarVector = 0;
constexpr ChoicesParameter::ValueType k_ToVectorScalar = 1;

} // namespace

TEST_CASE("OrientationAnalysis::ConvertQuaternionFilter", "[OrientationAnalysis][ConvertQuaternionFilter]")
{
  Application::GetOrCreateInstance()->loadPlugins(unit_test::k_BuildDir.view(), true);

  DataStructure dataStructure;

  // Build up a simple Float32Array and place default data into the array
  Float32Array* quats = UnitTest::CreateTestDataArray<float32>(dataStructure, k_QuatName, {4ULL}, {4ULL}, {});

  for(size_t i = 0; i < 16; i++)
  {
    (*quats)[i] = static_cast<float32>(i);
  }
  Float32Array* toScalarVector = UnitTest::CreateTestDataArray<float32>(dataStructure, k_Exemplar0, {4ULL}, {4ULL}, {});
  (*toScalarVector)[0] = 3.0F;
  (*toScalarVector)[1] = 0.0F;
  (*toScalarVector)[2] = 1.0F;
  (*toScalarVector)[3] = 2.0F;
  (*toScalarVector)[4] = 7.0F;
  (*toScalarVector)[5] = 4.0F;
  (*toScalarVector)[6] = 5.0F;
  (*toScalarVector)[7] = 6.0F;
  (*toScalarVector)[8] = 11.0F;
  (*toScalarVector)[9] = 8.0F;
  (*toScalarVector)[10] = 9.0F;
  (*toScalarVector)[11] = 10.0F;
  (*toScalarVector)[12] = 15.0F;
  (*toScalarVector)[13] = 12.0F;
  (*toScalarVector)[14] = 13.0F;
  (*toScalarVector)[15] = 14.0F;

  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    const ConvertQuaternionFilter filter;

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ConvertQuaternionFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({k_QuatName})));
    args.insertOrAssign(ConvertQuaternionFilter::k_OutputDataArrayPath_Key, std::make_any<std::string>(k_ConvertedName));
    args.insertOrAssign(ConvertQuaternionFilter::k_DeleteOriginalData_Key, std::make_any<bool>(false));
    args.insertOrAssign(ConvertQuaternionFilter::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(k_ToScalarVector));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

    auto& outputArray = dataStructure.getDataRefAs<Float32Array>(DataPath({k_ConvertedName}));
    UnitTest::CompareDataArrays<float32>(*toScalarVector, outputArray);
  }

  // Now convert them back to the original order
  {
    // Instantiate the filter, a DataStructure object and an Arguments Object
    const ConvertQuaternionFilter filter;

    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(ConvertQuaternionFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({k_ConvertedName})));
    args.insertOrAssign(ConvertQuaternionFilter::k_OutputDataArrayPath_Key, std::make_any<std::string>(k_Exemplar1));
    args.insertOrAssign(ConvertQuaternionFilter::k_DeleteOriginalData_Key, std::make_any<bool>(false));
    args.insertOrAssign(ConvertQuaternionFilter::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(k_ToVectorScalar));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

    auto& outputArray = dataStructure.getDataRefAs<Float32Array>(DataPath({k_Exemplar1}));
    UnitTest::CompareDataArrays<float32>(*quats, outputArray);
  }
}
