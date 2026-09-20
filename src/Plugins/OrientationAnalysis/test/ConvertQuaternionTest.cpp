#include <array>
#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <nonstd/span.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "OrientationAnalysis/Filters/ConvertQuaternionFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

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
  UnitTest::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  DataStructure dataStructure;

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
    const ConvertQuaternionFilter filter;

    Arguments args;

    args.insertOrAssign(ConvertQuaternionFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({k_QuatName})));
    args.insertOrAssign(ConvertQuaternionFilter::k_OutputDataArrayName_Key, std::make_any<std::string>(k_ConvertedName));
    args.insertOrAssign(ConvertQuaternionFilter::k_DeleteOriginalData_Key, std::make_any<bool>(false));
    args.insertOrAssign(ConvertQuaternionFilter::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(k_ToScalarVector));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto& outputArray = dataStructure.getDataRefAs<Float32Array>(DataPath({k_ConvertedName}));
    UnitTest::CompareDataArrays<float32>(*toScalarVector, outputArray);
  }

  // The second conversion restores vector-scalar order.
  {
    const ConvertQuaternionFilter filter;

    Arguments args;

    args.insertOrAssign(ConvertQuaternionFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(DataPath({k_ConvertedName})));
    args.insertOrAssign(ConvertQuaternionFilter::k_OutputDataArrayName_Key, std::make_any<std::string>(k_Exemplar1));
    args.insertOrAssign(ConvertQuaternionFilter::k_DeleteOriginalData_Key, std::make_any<bool>(false));
    args.insertOrAssign(ConvertQuaternionFilter::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(k_ToVectorScalar));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

    auto& outputArray = dataStructure.getDataRefAs<Float32Array>(DataPath({k_Exemplar1}));
    UnitTest::CompareDataArrays<float32>(*quats, outputArray);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ConvertQuaternionFilter: Float64 conversion preserves exact components before source deletion", "[OrientationAnalysis][ConvertQuaternionFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  const auto conversionType = GENERATE(k_ToScalarVector, k_ToVectorScalar);
  CAPTURE(scenario, conversionType);
  UnitTest::AlgorithmTestScope scope(scenario);

  const DataPath inputPath({"InputQuaternions"});
  const DataPath outputPath({"OutputQuaternions"});
  const std::array<float64, 8> vectorScalar = {1.0000000000000002, -2.0000000000000004, 3.1415926535897931, 16777217.0, 0.10000000000000001, -0.33333333333333331, 1.0e-200, -1.0e200};
  const std::array<float64, 8> scalarVector = {16777217.0, 1.0000000000000002, -2.0000000000000004, 3.1415926535897931, -1.0e200, 0.10000000000000001, -0.33333333333333331, 1.0e-200};
  const auto& expected = conversionType == k_ToScalarVector ? scalarVector : vectorScalar;
  const auto& input = conversionType == k_ToScalarVector ? vectorScalar : scalarVector;

  DataStructure dataStructure;
  auto inputStore = DataStoreUtilities::CreateDataStore<float64>(dataStructure, inputPath, {2}, {4});
  REQUIRE(inputStore != nullptr);
  auto* inputArray = Float64Array::Create(dataStructure, inputPath.getTargetName(), inputStore);
  REQUIRE(inputArray != nullptr);
  for(usize valueIdx = 0; valueIdx < input.size(); valueIdx++)
  {
    (*inputArray)[valueIdx] = input[valueIdx];
  }
  scope.requireExpectedStore(*inputArray);

  const ConvertQuaternionFilter filter;
  Arguments args;
  args.insertOrAssign(ConvertQuaternionFilter::k_CellQuatsArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ConvertQuaternionFilter::k_OutputDataArrayName_Key, std::make_any<std::string>(outputPath.getTargetName()));
  args.insertOrAssign(ConvertQuaternionFilter::k_DeleteOriginalData_Key, std::make_any<bool>(true));
  args.insertOrAssign(ConvertQuaternionFilter::k_ConversionType_Key, std::make_any<ChoicesParameter::ValueType>(conversionType));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = scope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(outputPath));
  const auto& output = dataStructure.getDataRefAs<Float64Array>(outputPath);
  for(usize valueIdx = 0; valueIdx < expected.size(); valueIdx++)
  {
    INFO("value index " << valueIdx);
    REQUIRE(output[valueIdx] == expected[valueIdx]);
  }

  REQUIRE_FALSE(dataStructure.containsData(inputPath));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ConvertQuaternionFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ConvertQuaternionFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ConvertQuaternionFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ConvertQuaternionFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ConvertQuaternionFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ConvertQuaternionFilter::k_CellQuatsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(ConvertQuaternionFilter::k_OutputDataArrayName_Key) == "TestArray");
      CHECK(args.value<bool>(ConvertQuaternionFilter::k_DeleteOriginalData_Key) == true);
      CHECK(args.value<ChoicesParameter::ValueType>(ConvertQuaternionFilter::k_ConversionType_Key) == 0);
    }
  }
}
