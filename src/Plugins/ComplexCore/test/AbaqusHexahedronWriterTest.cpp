#include <catch2/catch.hpp>

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/AbaqusHexahedronWriterFilter.hpp"

#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include "complex/UnitTest/UnitTestCommon.hpp"

#include <cstdlib>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
const DataPath k_FeatureIdsPath = DataPath({Constants::k_DataContainer}).createChildPath(Constants::k_EbsdScanData).createChildPath(Constants::k_FeatureIds);

const std::string k_ExemplarDir = fmt::format("{}/abaqus_hexahedron_writer_test", unit_test::k_TestFilesDir);

std::vector<char> readIn(const fs::path& filePath)
{
  std::ifstream file(filePath.string(), std::ios_base::binary);

  if(file)
  {
    // get file size
    file.seekg(0, std::ios::end);
    const std::streampos length = file.tellg();
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
  const fs::path writtenFilePath = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test.inp");
  REQUIRE(fs::exists(writtenFilePath));
  const fs::path exemplarFilePath = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test.inp");
  REQUIRE(fs::exists(exemplarFilePath));
  REQUIRE(readIn(writtenFilePath) == readIn(exemplarFilePath));
  const fs::path writtenFilePath2 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test_elems.inp");
  REQUIRE(fs::exists(writtenFilePath2));
  const fs::path exemplarFilePath2 = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test_elems.inp");
  REQUIRE(fs::exists(exemplarFilePath2));
  REQUIRE(readIn(writtenFilePath2) == readIn(exemplarFilePath2));
  const fs::path writtenFilePath3 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test_elset.inp");
  REQUIRE(fs::exists(writtenFilePath3));
  const fs::path exemplarFilePath3 = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test_elset.inp");
  REQUIRE(fs::exists(exemplarFilePath3));
  REQUIRE(readIn(writtenFilePath3) == readIn(exemplarFilePath3));
  const fs::path writtenFilePath4 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test_nodes.inp");
  REQUIRE(fs::exists(writtenFilePath4));
  const fs::path exemplarFilePath4 = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test_nodes.inp");
  REQUIRE(fs::exists(exemplarFilePath4));
  REQUIRE(readIn(writtenFilePath4) == readIn(exemplarFilePath4));
  const fs::path writtenFilePath5 = fs::path(std::string(unit_test::k_BinaryTestOutputDir) + "/6_6_Abaqus_Hexahedron_Writer_Test_sects.inp");
  REQUIRE(fs::exists(writtenFilePath5));
  const fs::path exemplarFilePath5 = fs::path(k_ExemplarDir + "/6_6_Abaqus_Hexahedron_Writer_Test_sects.inp");
  REQUIRE(fs::exists(exemplarFilePath5));
  REQUIRE(readIn(writtenFilePath5) == readIn(exemplarFilePath5));
}
} // namespace

/**
 * @brief This class will decompress a tar.gz file using the locally installed copy of cmake and when
 * then class goes out of scope the extracted contents will be deleted from disk.
 */
class TestFileSentinel
{
public:
  TestFileSentinel(std::string inputArchiveName, std::string expectedTopLevelOutput)
  : m_InputArchiveName(std::move(inputArchiveName))
  , m_ExpectedTopLevelOutput(std::move(expectedTopLevelOutput))
  {
    const int result = decompress();
    REQUIRE(result == 0);
  }

  ~TestFileSentinel()
  {
    const std::string kRemoveFileCommand =
        fmt::format(R"(cd "{}" && "{}" -E rm -rf "{}/{}")", unit_test::k_TestFilesDir, unit_test::k_CMakeExecutable, unit_test::k_TestFilesDir, m_ExpectedTopLevelOutput);
    const int result = std::system(kRemoveFileCommand.c_str());
    if(result != 0)
    {
      std::cout << "Removing decompressed data failed. The command was:\n  " << kRemoveFileCommand << std::endl;
    }
  }

  TestFileSentinel(const TestFileSentinel&) = delete;            // Copy Constructor Not Implemented
  TestFileSentinel(TestFileSentinel&&) = delete;                 // Move Constructor Not Implemented
  TestFileSentinel& operator=(const TestFileSentinel&) = delete; // Copy Assignment Not Implemented
  TestFileSentinel& operator=(TestFileSentinel&&) = delete;      // Move Assignment Not Implemented

  int decompress()
  {
    const std::string kDecompressCommand =
        fmt::format(R"(cd "{}" && "{}" -E tar xvzf "{}/{}")", unit_test::k_TestFilesDir, unit_test::k_CMakeExecutable, unit_test::k_TestFilesDir, m_InputArchiveName);
    return std::system(kDecompressCommand.c_str());
  }

private:
  std::string m_InputArchiveName;
  std::string m_ExpectedTopLevelOutput;
};

TEST_CASE("ComplexCore::AbaqusHexahedronWriterFilter: Valid Filter Execution", "[ComplexCore][AbaqusHexahedronWriterFilter]")
{

  const std::string kDataInputArchive = "6_6_find_feature_centroids.tar.gz";
  const std::string kExpectedOutputTopLevel = "6_6_find_feature_centroids.dream3d";
  const TestFileSentinel testDataSentinel(kDataInputArchive, kExpectedOutputTopLevel);

  const std::string kDataInputArchive1 = "abaqus_hexahedron_writer_test.tar.gz";
  const std::string kExpectedOutputTopLevel1 = "abaqus_hexahedron_writer_test";
  const TestFileSentinel testDataSentinel1(kDataInputArchive1, kExpectedOutputTopLevel1);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const AbaqusHexahedronWriterFilter filter;
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
