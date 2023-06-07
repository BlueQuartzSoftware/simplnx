#include <catch2/catch.hpp>

#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/LosAlamosFFTWriterFilter.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

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
  REQUIRE(readIn(writtenFilePath) == readIn(exemplarFilePath));
}
} // namespace

TEST_CASE("ComplexCore::LosAlamosFFTWriterFilter: Valid Filter Execution", "[ComplexCore][LosAlamosFFTWriterFilter]")
{
  // Utilize the 6.6 Binary Feature Phases test file to conserve space
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/bin_feature_phases/6_6_find_feature_phases_binary.dream3d", unit_test::k_TestFilesDir)));

  {
    // Instantiate the filter and an Arguments Object
    LosAlamosFFTWriterFilter filter;
    Arguments args;

    // Create default Parameters for the filter.
    args.insertOrAssign(LosAlamosFFTWriterFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(writtenFilePath));

    args.insertOrAssign(LosAlamosFFTWriterFilter::k_ImageGeomPath, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100})));
    args.insertOrAssign(LosAlamosFFTWriterFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
    args.insertOrAssign(LosAlamosFFTWriterFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));
    args.insertOrAssign(LosAlamosFFTWriterFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, "BinaryPhases"})));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());
  }

  ::CompareResults();
}
