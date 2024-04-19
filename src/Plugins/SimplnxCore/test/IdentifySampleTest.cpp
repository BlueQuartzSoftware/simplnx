#include "SimplnxCore/Filters/IdentifySample.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;
using namespace nx::core::UnitTest;

TEST_CASE("SimplnxCore::IdentifySample : Valid filter execution", "[SimplnxCore][IdentifySample]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_identify_sample.tar.gz", "6_6_identify_sample");

  // Read Input/Exemplar DREAM3D File data
  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/6_6_identify_sample/6_6_identify_sample.dream3d", unit_test::k_TestFilesDir)));
  IdentifySample filter;
  Arguments args;
  args.insert(IdentifySample::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insert(IdentifySample::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));

  std::string k_ExemplarDataContainerName;

  SECTION("No Fill")
  {
    k_ExemplarDataContainerName = "Exemplar Data NoFill";
    args.insert(IdentifySample::k_FillHoles_Key, std::make_any<bool>(false));
  }
  SECTION("Fill")
  {
    k_ExemplarDataContainerName = "Exemplar Data Fill";
    args.insert(IdentifySample::k_FillHoles_Key, std::make_any<bool>(true));
  }

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  CompareExemplarToGeneratedData(dataStructure, dataStructure, Constants::k_CellAttributeMatrix, k_ExemplarDataContainerName);
}

TEST_CASE("SimplnxCore::IdentifySample : Invalid filter execution", "[SimplnxCore][IdentifySample]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "6_6_identify_sample.tar.gz", "6_6_identify_sample");

  // Read Input/Exemplar DREAM3D File data
  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/6_6_identify_sample/6_6_identify_sample.dream3d", unit_test::k_TestFilesDir)));
  auto& cellDataAM = dataStructure.getDataRefAs<AttributeMatrix>(Constants::k_CellAttributeMatrix);
  const std::string k_InvalidMaskArrayName = "InvalidMaskArray";
  Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_InvalidMaskArrayName, cellDataAM.getShape(), std::vector<usize>{1}, cellDataAM.getId());

  IdentifySample filter;
  Arguments args;
  args.insert(IdentifySample::k_FillHoles_Key, std::make_any<bool>(false));
  args.insert(IdentifySample::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insert(IdentifySample::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_CellAttributeMatrix.createChildPath(k_InvalidMaskArrayName)));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}
