#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ExtractComponentAsArrayFilter.hpp"

#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
const std::string k_ExtractedComponents("Extracted Components");

const DataPath k_QuatsPath({Constants::k_DataContainer, Constants::k_EbsdScanData, Constants::k_Quats});
const DataPath k_ExtractedComponentsPath({Constants::k_DataContainer, Constants::k_EbsdScanData, k_ExtractedComponents});

const fs::path k_BaseDataFilePath = fs::path(fmt::format("{}/6_6_find_feature_centroids.dream3d", unit_test::k_TestFilesDir));
} // namespace

TEST_CASE("ComplexCore::ExtractComponentAsArrayFilter: Valid filter execution", "[ComplexCore][ExtractComponentAsArrayFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractComponentAsArrayFilter filter;

  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_find_feature_centroids.tar.gz",
                                                             "6_6_find_feature_centroids.dream3d");

  DataStructure alteredDs = UnitTest::LoadDataStructure(k_BaseDataFilePath);
  const int32 removeCompIndex = 1;

  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_MoveComponentsToNewArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_RemoveComponentsFromArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_CompNumber_Key, std::make_any<int32>(removeCompIndex));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_NewArrayPath_Key, std::make_any<std::string>(k_ExtractedComponents));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(alteredDs, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(alteredDs, args);
  REQUIRE(executeResult.result.valid());

  // Load a clean copy of the datastructure prior to resize because original array is terminated after execution
  DataStructure unalteredDs = UnitTest::LoadDataStructure(k_BaseDataFilePath);
  const auto& originalQuatsArray = unalteredDs.getDataRefAs<Float32Array>(k_QuatsPath); // clean array [exemplar]

  const auto& reducedQuatsArray = alteredDs.getDataRefAs<Float32Array>(k_QuatsPath);
  const auto& extractedComponentsArray = alteredDs.getDataRefAs<Float32Array>(k_ExtractedComponentsPath);

  const usize originalTupleCount = originalQuatsArray.getNumberOfTuples();
  REQUIRE(originalTupleCount == reducedQuatsArray.getNumberOfTuples());
  REQUIRE(originalTupleCount == extractedComponentsArray.getNumberOfTuples());

  const usize originalCompCount = originalQuatsArray.getNumberOfComponents();
  const usize reducedCompCount = reducedQuatsArray.getNumberOfComponents();
  REQUIRE((originalCompCount - 1) == reducedCompCount);
  REQUIRE(1 == extractedComponentsArray.getNumberOfComponents());

  usize extractedIndex = 0;
  for(usize tupleIndex = 0; tupleIndex < originalTupleCount; tupleIndex++)
  {
    for(usize compIndex = 0; compIndex < originalCompCount; compIndex++)
    {
      usize originalIndex = tupleIndex * originalCompCount + compIndex;
      usize reducedIndex = tupleIndex * reducedCompCount + compIndex;
      if(compIndex == removeCompIndex)
      {
        REQUIRE(originalQuatsArray[originalIndex] == extractedComponentsArray[extractedIndex]);
        extractedIndex++;
      }
      else
      {
        if(compIndex > removeCompIndex)
        {
          REQUIRE(originalQuatsArray[originalIndex] == reducedQuatsArray[reducedIndex - 1]); // account for having one less comp
        }
        else
        {
          REQUIRE(originalQuatsArray[originalIndex] == reducedQuatsArray[reducedIndex]);
        }
      }
    }
  }
}

TEST_CASE("ComplexCore::ExtractComponentAsArrayFilter: InValid filter execution", "[ComplexCore][ExtractComponentAsArrayFilter]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "6_6_find_feature_centroids.tar.gz",
                                                             "6_6_find_feature_centroids.dream3d");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractComponentAsArrayFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(k_BaseDataFilePath);
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_MoveComponentsToNewArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_RemoveComponentsFromArray_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_CompNumber_Key, std::make_any<int32>(5)); // Invalid
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_SelectedArrayPath_Key, std::make_any<DataPath>(k_QuatsPath));
  args.insertOrAssign(ExtractComponentAsArrayFilter::k_NewArrayPath_Key, std::make_any<std::string>(k_ExtractedComponents));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(!preflightResult.outputActions.valid());
}
