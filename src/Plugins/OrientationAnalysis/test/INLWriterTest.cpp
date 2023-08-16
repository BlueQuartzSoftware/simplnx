#include <catch2/catch.hpp>

#include "OrientationAnalysis/Filters/INLWriterFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <fstream>

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
const std::string k_MaterialName = "MaterialName";
const std::string k_NumFeatures = "NumFeatures";

const fs::path k_ExemplarFilePath = fs::path(fmt::format("{}/INL_writer/INLWriterExemplar.inl", unit_test::k_TestFilesDir));
const fs::path k_WrittenFilePath = fs::path(fmt::format("{}/INLWriter.inl", unit_test::k_BinaryTestOutputDir));

void CompareResults() // compare hash of both file strings
{
  REQUIRE(fs::exists(k_WrittenFilePath));
  REQUIRE(fs::exists(k_ExemplarFilePath));
  std::ifstream exemplarFile(k_ExemplarFilePath, std::ios_base::in | std::ios_base::binary);
  if(!exemplarFile.is_open())
  {
    throw std::runtime_error(fmt::format("{} must be stream readable!", k_ExemplarFilePath.string()));
  }
  // skip 2 lines to avoid time write out and versioning
  exemplarFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  exemplarFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  std::ifstream generatedFile(k_WrittenFilePath, std::ios_base::in | std::ios_base::binary);
  if(!exemplarFile.is_open())
  {
    throw std::runtime_error(fmt::format("{} must be stream readable!", k_WrittenFilePath.string()));
  }

  // //ignore versioning
  // generatedFile.ignore('\n');

  char exemplar[128];
  char generated[128];
  for(usize i = 0; i < 1024; i++)
  {
    if(exemplarFile.eof())
    {
      break;
    }
    exemplarFile.getline(exemplar, std::numeric_limits<std::streamsize>::max(), '\n');
    generatedFile.getline(generated, std::numeric_limits<std::streamsize>::max(), '\n');

    std::string cExem(exemplar);
    std::string exem = cExem.substr(0 , cExem.find('\r'));

    std::string cGen(generated);
    std::string gen = cGen.substr(0, cGen.find('\n'));
    REQUIRE(exem == gen);
  }
}
}

TEST_CASE("OrientationAnalysis::INLWriterFilter: Valid Filter Execution","[OrientationAnalysis][INLWriterFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  INLWriterFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/INL_writer/6_6_INL_writer.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(INLWriterFilter::k_OutputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_WrittenFilePath));
  args.insertOrAssign(INLWriterFilter::k_ImageGeomPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100})));
  args.insertOrAssign(INLWriterFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_FeatureIds})));
  args.insertOrAssign(INLWriterFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_Phases})));
  args.insertOrAssign(INLWriterFilter::k_CellEulerAnglesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_EbsdScanData, Constants::k_EulerAngles})));
  args.insertOrAssign(INLWriterFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, Constants::k_CrystalStructures})));
  args.insertOrAssign(INLWriterFilter::k_MaterialNameArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, ::k_MaterialName})));
  args.insertOrAssign(INLWriterFilter::k_NumFeaturesArrayPath_Key, std::make_any<DataPath>(DataPath({Constants::k_SmallIN100, Constants::k_Phase_Data, ::k_NumFeatures})));


  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  ::CompareResults();
}
