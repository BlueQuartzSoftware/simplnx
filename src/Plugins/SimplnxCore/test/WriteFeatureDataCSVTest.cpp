#include "SimplnxCore/Filters/WriteFeatureDataCSVFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
const std::string k_TestOutput = fmt::format("{}", unit_test::k_BinaryTestOutputDir);
const std::string k_CSVExemplarFileName = "CSV_Exemplar.csv";
const usize k_NumTuples = 3;
const std::vector<usize> k_VertexTupleDims = {k_NumTuples};
const std::vector<usize> k_VertexCompDims = {2};
} // namespace

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

TEST_CASE("SimplnxCore::WriteFeatureDataCSVFilter: Test Algorithm", "[WriteFeatureDataCSVFilter]")
{
  WriteFeatureDataCSVFilter filter;
  DataStructure dataStructure;
  AttributeMatrix& topLevelGroup = *AttributeMatrix::Create(dataStructure, "TestData", k_VertexTupleDims);
  auto path = dataStructure.getAllDataPaths()[0];

  auto* neighborList = NeighborList<float32>::Create(dataStructure, "Neighbor List", k_NumTuples, topLevelGroup.getId());
  neighborList->resizeTotalElements(k_NumTuples);
  std::vector<float32> list1 = {117, 875, 1035, 3905, 4214};
  std::vector<float32> list2 = {750, 1905, 1912, 2015, 2586, 3180, 3592, 4041, 4772};
  std::vector<float32> list3 = {309, 775, 2625, 2818, 3061, 3751, 4235, 4817};
  neighborList->setList(0, std::make_shared<std::vector<float32>>(list1));
  neighborList->setList(1, std::make_shared<std::vector<float32>>(list2));
  neighborList->setList(2, std::make_shared<std::vector<float32>>(list3));

  auto dataArray1 = UnitTest::CreateTestDataArray<float32>(dataStructure, "floats", k_VertexTupleDims, k_VertexCompDims, topLevelGroup.getId());
  dataArray1->fill(1.23);
  auto dataArray2 = UnitTest::CreateTestDataArray<int32>(dataStructure, "negatives", k_VertexTupleDims, k_VertexCompDims, topLevelGroup.getId());
  dataArray2->fill(-678);
  auto dataArray3 = UnitTest::CreateTestDataArray<uint64>(dataStructure, "basic", k_VertexTupleDims, k_VertexCompDims, topLevelGroup.getId());
  dataArray3->fill(24343);

  auto file = std::filesystem::path(fmt::format("{}/{}.csv", k_TestOutput, "CSV_Test"));

  Arguments args;
  args.insertOrAssign(WriteFeatureDataCSVFilter::k_FeatureDataFile_Key, std::make_any<FileSystemPathParameter::ValueType>(file));
  args.insertOrAssign(WriteFeatureDataCSVFilter::k_WriteNeighborListData_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteFeatureDataCSVFilter::k_WriteNumFeaturesLine_Key, std::make_any<bool>(true));
  args.insertOrAssign(WriteFeatureDataCSVFilter::k_DelimiterChoiceInt_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
  args.insertOrAssign(WriteFeatureDataCSVFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(path));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "ascii_data_exemplars.tar.gz", "ascii_data_exemplars");

  auto exemplarPath = fs::path(fmt::format("{}/ascii_data_exemplars/{}", unit_test::k_TestFilesDir, k_CSVExemplarFileName));
  REQUIRE(fs::exists(exemplarPath));

  REQUIRE(readIn(file) == readIn(exemplarPath));
}
