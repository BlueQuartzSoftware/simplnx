#include <catch2/catch.hpp>

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/Filters/WriteLosAlamosFFTFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const fs::path exemplarFilePath = fs::path(fmt::format("{}/LosAlamosFFTExemplar.txt", unit_test::k_TestFilesDir));
const fs::path writtenFilePath = fs::path(fmt::format("{}/LosAlamosFFT.txt", unit_test::k_BinaryTestOutputDir));

std::vector<char> readIn(fs::path filePath)
{
  std::ifstream file(filePath.string(), std::ios_base::binary);

  if(file)
  {
    // get file size
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    // read whole file into a vector
    std::vector<char> contents(length); // act as a buffer
    file.read(contents.data(), length);

    // build string from psuedo-buffer
    return contents;
  }
  return {};
}

void CompareResults() // compare hash of both file strings
{
  REQUIRE(fs::exists(writtenFilePath));
  REQUIRE(fs::exists(exemplarFilePath));
  const std::vector<char> exemplar = readIn(exemplarFilePath);
  const std::vector<char> data = readIn(writtenFilePath);
  for(size_t i = 0; i < 1024; i++)
  {
    if(exemplar[i] != data[i])
    {
      std::cout << "Output difference at byte offset " << i << std::endl;
      REQUIRE(exemplar[i] == data[i]);
      break;
    }
  }
  REQUIRE(exemplar.size() == data.size());
}
} // namespace

TEST_CASE("SimplnxCore::WriteLosAlamosFFTFilter: Valid Filter Execution", "[SimplnxCore][WriteLosAlamosFFTFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "bin_feature_phases.tar.gz", "bin_feature_phases");

  const nx::core::UnitTest::TestFileSentinel testDataSentinel1(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "LosAlamosFFTExemplar.tar.gz", "LosAlamosFFTExemplar.txt");

  // Utilize the 6.6 Binary Feature Phases test file to conserve space
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/bin_feature_phases/6_6_find_feature_phases_binary.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    WriteLosAlamosFFTFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(writtenFilePath));

    args.insertOrAssign(WriteLosAlamosFFTFilter::k_ImageGeomPath, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100})));
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));
    args.insertOrAssign(WriteLosAlamosFFTFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "BinaryPhases"})));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  ::CompareResults();
}
