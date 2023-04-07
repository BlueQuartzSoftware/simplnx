#include <catch2/catch.hpp>

#include "ComplexCore/Filters/AbaqusHexahedronWriterFilter.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

namespace fs = std::filesystem;
using namespace complex;

namespace
{
 const DataPath k_FeatureIdsPath = DataPath({Constants::k_DataContainer}).createChildPath(Constants::k_EbsdScanData).createChildPath(Constants::k_FeatureIds);

 const std::string k_ExemplarDir = fmt::format("{}/abaqus_hexahedron_writer_test", unit_test::k_TestFilesDir);

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
   fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test.inp");
   REQUIRE(fs::exists(writtenFilePath));
   fs::path exemplarFilePath = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test.inp");
   REQUIRE(fs::exists(exemplarFilePath));
   REQUIRE(readIn(writtenFilePath) == readIn(exemplarFilePath));
   fs::path writtenFilePath2 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test_elems.inp");
   REQUIRE(fs::exists(writtenFilePath2));
   fs::path exemplarFilePath2 = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test_elems.inp");
   REQUIRE(fs::exists(exemplarFilePath2));
   REQUIRE(readIn(writtenFilePath2) == readIn(exemplarFilePath2));
   fs::path writtenFilePath3 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test_elset.inp");
   REQUIRE(fs::exists(writtenFilePath3));
   fs::path exemplarFilePath3 = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test_elset.inp");
   REQUIRE(fs::exists(exemplarFilePath3));
   REQUIRE(readIn(writtenFilePath3) == readIn(exemplarFilePath3));
   fs::path writtenFilePath4 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test_nodes.inp");
   REQUIRE(fs::exists(writtenFilePath4));
   fs::path exemplarFilePath4 = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test_nodes.inp");
   REQUIRE(fs::exists(exemplarFilePath4));
   REQUIRE(readIn(writtenFilePath4) == readIn(exemplarFilePath4));
   fs::path writtenFilePath5 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test_sects.inp");
   REQUIRE(fs::exists(writtenFilePath5));
   fs::path exemplarFilePath5 = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test_sects.inp");
   REQUIRE(fs::exists(exemplarFilePath5));
   REQUIRE(readIn(writtenFilePath5) == readIn(exemplarFilePath5));
 }
}

TEST_CASE("ComplexCore::AbaqusHexahedronWriterFilter: Valid Filter Execution","[ComplexCore][AbaqusHexahedronWriterFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  AbaqusHexahedronWriterFilter filter;
  DataStructure dataStructure = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/6_6_find_feature_centroids.dream3d", unit_test::k_TestFilesDir)));
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(AbaqusHexahedronWriterFilter::k_HourglassStiffness_Key, std::make_any<int32>(250));
  args.insertOrAssign(AbaqusHexahedronWriterFilter::k_JobName_Key, std::make_any<StringParameter::ValueType>("UnitTest"));
  args.insertOrAssign(AbaqusHexahedronWriterFilter::k_OutputPath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(std::string(unit_test::k_BinaryTestOutputDir))));
  args.insertOrAssign(AbaqusHexahedronWriterFilter::k_FilePrefix_Key, std::make_any<StringParameter::ValueType>("6_6_Abaqus_Hexahedron_Writer_Test"));
  args.insertOrAssign(AbaqusHexahedronWriterFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
  args.insertOrAssign(AbaqusHexahedronWriterFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(k_FeatureIdsPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  REQUIRE(executeResult.result.valid());

  ::CompareResults();
}
