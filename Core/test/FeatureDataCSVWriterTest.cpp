#include <catch2/catch.hpp>

#include "Core/Filters/FeatureDataCSVWriterFilter.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "Core/Core_test_dirs.hpp"

using namespace complex;
using namespace complex::Constants;

namespace
{
const std::string k_TestOutput = fmt::format("{}", unit_test::k_BinaryTestOutputDir);
const std::string k_CSVExemplarFileName = "CSV_Exemplar.csv";
const usize k_NumTuples = 3;
const  std::vector<usize> k_VertexTupleDims = { k_NumTuples };
const std::vector<usize> k_VertexCompDims = { 2 };
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

TEST_CASE("Core::FeatureDataCSVWriterFilter: Instantiate Filter", "[FeatureDataCSVWriterFilter]")
{
  FeatureDataCSVWriterFilter filter;
  DataStructure dataGraph;
  Arguments args;
  AttributeMatrix& topLevelGroup = *AttributeMatrix::Create(dataGraph, "TestData");

  auto file = std::filesystem::path(fmt::format("{}/{}.csv", k_TestOutput, "CSV_Test"));
  auto path = dataGraph.getAllDataPaths()[0];

  args.insertOrAssign(FeatureDataCSVWriterFilter::k_FeatureDataFile_Key, std::make_any<FileSystemPathParameter::ValueType>(file));
  args.insertOrAssign(FeatureDataCSVWriterFilter::k_WriteNeighborListData_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureDataCSVWriterFilter::k_WriteNumFeaturesLine_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureDataCSVWriterFilter::k_DelimiterChoiceInt_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
  args.insertOrAssign(FeatureDataCSVWriterFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(path));

  // Preflight the filter
  auto preflightResult = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
}

TEST_CASE("Core::FeatureDataCSVWriterFilter: Test Algorithm", "[FeatureDataCSVWriterFilter]")
{
  FeatureDataCSVWriterFilter filter;
  DataStructure dataGraph;
  AttributeMatrix& topLevelGroup = *AttributeMatrix::Create(dataGraph, "TestData");
  topLevelGroup.setShape(k_VertexTupleDims);
  auto path = dataGraph.getAllDataPaths()[0];

  auto* neighborList = NeighborList<float32>::Create(dataGraph, "Neighbor List", k_NumTuples, topLevelGroup.getId());
  neighborList->resizeTotalElements(k_NumTuples);
  std::vector<float32> list1 = {117, 875, 1035, 3905, 4214};
  std::vector<float32> list2 = {750, 1905, 1912, 2015, 2586, 3180, 3592, 4041, 4772};
  std::vector<float32> list3 = {309, 775, 2625, 2818, 3061, 3751, 4235, 4817};
  neighborList->setList(0, std::make_shared<std::vector<float32>>(list1));
  neighborList->setList(1, std::make_shared<std::vector<float32>>(list2));
  neighborList->setList(2, std::make_shared<std::vector<float32>>(list3));

  auto dataArray1 = UnitTest::CreateTestDataArray<float32>(dataGraph, "floats", k_VertexTupleDims, k_VertexCompDims, topLevelGroup.getId());
  dataArray1->fill(1.23);
  auto dataArray2 = UnitTest::CreateTestDataArray<int32>(dataGraph, "negatives", k_VertexTupleDims, k_VertexCompDims, topLevelGroup.getId());
  dataArray2->fill(-678);
  auto dataArray3 = UnitTest::CreateTestDataArray<uint64>(dataGraph, "basic", k_VertexTupleDims, k_VertexCompDims, topLevelGroup.getId());
  dataArray3->fill(24343);

  auto file = std::filesystem::path(fmt::format("{}/{}.csv", k_TestOutput, "CSV_Test"));

  Arguments args;
  args.insertOrAssign(FeatureDataCSVWriterFilter::k_FeatureDataFile_Key, std::make_any<FileSystemPathParameter::ValueType>(file));
  args.insertOrAssign(FeatureDataCSVWriterFilter::k_WriteNeighborListData_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureDataCSVWriterFilter::k_WriteNumFeaturesLine_Key, std::make_any<bool>(true));
  args.insertOrAssign(FeatureDataCSVWriterFilter::k_DelimiterChoiceInt_Key, std::make_any<ChoicesParameter::ValueType>(2ULL));
  args.insertOrAssign(FeatureDataCSVWriterFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(path));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataGraph, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result);

  auto exemplarPath = fs::path(fmt::format("{}/write_csv_data_exemplars/{}", unit_test::k_TestDataSourceDir, k_CSVExemplarFileName));

  REQUIRE(readIn(file) == readIn(exemplarPath));
}
