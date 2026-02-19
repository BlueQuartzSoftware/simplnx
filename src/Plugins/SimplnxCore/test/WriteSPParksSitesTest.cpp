#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/WriteSPParksSitesFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const fs::path k_ExemplarFilePath = fs::path(fmt::format("{}/6_5_spparks_sites_writer/6_5_spparks_sites_file.txt", unit_test::k_TestFilesDir));
const fs::path k_WrittenFilePath = fs::path(fmt::format("{}/7_0_spparks_sites_writer.txt", unit_test::k_BinaryTestOutputDir));

} // namespace

TEST_CASE("SimplnxCore::WriteSPParksSitesFilter: Single File Valid", "[SimplnxCore][WriteSPParksSitesFilter]")
{

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "INL_writer.tar.gz", "INL_writer");
  const nx::core::UnitTest::TestFileSentinel testDataSentinel2(nx::core::unit_test::k_TestFilesDir, "6_5_spparks_sites_writer.tar.gz", "6_5_spparks_sites_writer");

  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/INL_writer/6_6_INL_writer.dream3d", unit_test::k_TestFilesDir)));

  WriteSPParksSitesFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(WriteSPParksSitesFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_WrittenFilePath));
  args.insertOrAssign(WriteSPParksSitesFilter::k_ImageGeomPath, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100})));
  args.insertOrAssign(WriteSPParksSitesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  std::ifstream computedFile1(k_WrittenFilePath);
  std::ifstream exemplarFile1(k_ExemplarFilePath);

  UnitTest::CompareAsciiFiles(computedFile1, exemplarFile1, {0});

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
