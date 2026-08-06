#include "SimplnxCore/Filters/ReadStlFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include "SimplnxCore/utils/StlUtilities.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
/**
 * @brief Describes one triangle record to write into a synthetic binary STL file.
 */
struct StlTriangleSpec
{
  /** @brief The value written into the 2 byte "attribute byte count" field. */
  uint16_t attributeByteCount = 0;
  /** @brief How many real payload bytes to write after that field. Often 0 in the wild. */
  uint16_t payloadBytes = 0;
};

/**
 * @brief Writes a synthetic binary STL file so the reader can be exercised against files that do
 * not obey the STL specification without shipping binary fixtures in the data archive.
 *
 * Triangle @c t is given vertices (10t, 0, 0), (10t + 1, 0, 0) and (10t, 1, 0) so that every vertex
 * in the file is unique and the exact coordinates can be asserted afterwards. A misaligned read
 * produces garbage coordinates, which is what makes the attribute-byte-count tests meaningful.
 */
void WriteBinaryStlFile(const fs::path& path, const std::string& headerText, int32_t declaredTriangleCount, const std::vector<StlTriangleSpec>& triangleSpecs)
{
  fs::create_directories(path.parent_path());
  std::ofstream outputFile(path, std::ios::binary | std::ios::trunc);
  REQUIRE(outputFile.is_open());

  std::string header(StlConstants::k_STL_HEADER_LENGTH, '\0');
  const size_t copyLength = std::min(headerText.size(), StlConstants::k_STL_HEADER_LENGTH);
  std::copy_n(headerText.begin(), copyLength, header.begin());
  outputFile.write(header.data(), static_cast<std::streamsize>(StlConstants::k_STL_HEADER_LENGTH));
  outputFile.write(reinterpret_cast<const char*>(&declaredTriangleCount), sizeof(int32_t));

  for(size_t t = 0; t < triangleSpecs.size(); t++)
  {
    const float base = static_cast<float>(t) * 10.0F;
    const std::array<float, StlConstants::k_StlElementCount> values = {
        0.0F,        0.0F, 1.0F, // facet normal
        base,        0.0F, 0.0F, // vertex 0
        base + 1.0F, 0.0F, 0.0F, // vertex 1
        base,        1.0F, 0.0F  // vertex 2
    };
    outputFile.write(reinterpret_cast<const char*>(values.data()), static_cast<std::streamsize>(values.size() * sizeof(float)));
    outputFile.write(reinterpret_cast<const char*>(&triangleSpecs[t].attributeByteCount), sizeof(uint16_t));

    const std::vector<char> payload(triangleSpecs[t].payloadBytes, 0x5A);
    if(!payload.empty())
    {
      outputFile.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    }
  }
  outputFile.close();
}

/**
 * @brief Runs ReadStlFileFilter over @p inputFile and returns the execute result.
 */
IFilter::ExecuteResult RunReadStlFileFilter(DataStructure& dataStructure, const fs::path& inputFile, const DataPath& triangleGeomDataPath)
{
  Arguments args;
  ReadStlFileFilter filter;
  args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(inputFile));
  args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  return filter.execute(dataStructure, args);
}

/**
 * @brief Asserts that every triangle written by WriteBinaryStlFile() came back with the exact
 * coordinates it was written with. Reads the coordinates through the face connectivity so the test
 * is insensitive to any vertex reordering done by EliminateDuplicateNodes().
 */
void CheckSyntheticTriangleGeometry(const TriangleGeom& triangleGeom, size_t expectedTriangleCount)
{
  REQUIRE(triangleGeom.getNumberOfFaces() == expectedTriangleCount);
  REQUIRE(triangleGeom.getNumberOfVertices() == expectedTriangleCount * 3);

  const auto& facesRef = triangleGeom.getFaces()->getDataStoreRef();
  const auto& verticesRef = triangleGeom.getVertices()->getDataStoreRef();

  for(size_t t = 0; t < expectedTriangleCount; t++)
  {
    const float base = static_cast<float>(t) * 10.0F;
    const usize vertex0 = facesRef[t * 3 + 0];
    const usize vertex1 = facesRef[t * 3 + 1];
    const usize vertex2 = facesRef[t * 3 + 2];

    REQUIRE(verticesRef[vertex0 * 3 + 0] == Approx(base));
    REQUIRE(verticesRef[vertex0 * 3 + 1] == Approx(0.0F));
    REQUIRE(verticesRef[vertex0 * 3 + 2] == Approx(0.0F));
    REQUIRE(verticesRef[vertex1 * 3 + 0] == Approx(base + 1.0F));
    REQUIRE(verticesRef[vertex1 * 3 + 1] == Approx(0.0F));
    REQUIRE(verticesRef[vertex2 * 3 + 0] == Approx(base));
    REQUIRE(verticesRef[vertex2 * 3 + 1] == Approx(1.0F));
  }
}
} // namespace

TEST_CASE("SimplnxCore::ReadStlFileFilter:Valid_File", "[SimplnxCore][ReadStlFileFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ReadSTLFileTest.tar.gz", "ReadSTLFileTest");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ReadStlFileFilter filter;

  DataPath triangleGeomDataPath({"[Triangle Geometry]"});

  std::string inputFile = fmt::format("{}/ReadSTLFileTest/ASTMD638_specimen.stl", unit_test::k_TestFilesDir);

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
  REQUIRE(triangleGeom.getNumberOfFaces() == 92);
  REQUIRE(triangleGeom.getNumberOfVertices() == 48);

  // Write the DataStructure out to the file system
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/StlFileReaderTest.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadStlFileFilter:STLParseError", "[SimplnxCore][ReadStlFileFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ReadSTLFileTest.tar.gz", "ReadSTLFileTest");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ReadStlFileFilter filter;

  DataPath triangleGeomDataPath({"[Triangle Geometry]"});

  std::string inputFile = fmt::format("{}/ReadSTLFileTest/stl_test_wrong_num_triangles.stl", unit_test::k_TestFilesDir);

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  REQUIRE(executeResult.result.errors().front().code == -1108);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadStlFileFilter:TriangleParseError", "[SimplnxCore][ReadStlFileFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ReadSTLFileTest.tar.gz", "ReadSTLFileTest");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ReadStlFileFilter filter;

  DataPath triangleGeomDataPath({"[Triangle Geometry]"});

  std::string inputFile = fmt::format("{}/ReadSTLFileTest/stl_test_2_TriangleParseError.stl", unit_test::k_TestFilesDir);

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  REQUIRE(executeResult.result.errors().front().code == -1106);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadStlFileFilter:AttributeParseError", "[SimplnxCore][ReadStlFileFilter]")
{
  UnitTest::LoadPlugins();

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ReadSTLFileTest.tar.gz", "ReadSTLFileTest");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  DataStructure dataStructure;
  Arguments args;
  ReadStlFileFilter filter;

  DataPath triangleGeomDataPath({"[Triangle Geometry]"});

  std::string inputFile = fmt::format("{}/ReadSTLFileTest/stl_test_2_AttributeParseError.stl", unit_test::k_TestFilesDir);

  // Create default Parameters for the filter.
  args.insertOrAssign(ReadStlFileFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(fs::path(inputFile)));
  args.insertOrAssign(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key, std::make_any<DataPath>(triangleGeomDataPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);

  REQUIRE(executeResult.result.errors().front().code == -1107);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ReadStlFileFilter:NonConformingAttributeByteCount", "[SimplnxCore][ReadStlFileFilter]")
{
  UnitTest::LoadPlugins();

  const size_t k_TriangleCount = 8;
  const DataPath triangleGeomDataPath({"[Triangle Geometry]"});

  // Every case below writes a file whose size is exactly 84 + 50 * numTriangles, meaning there is
  // no room for attribute payload at all. The reader must therefore ignore the attribute byte count
  // of every triangle no matter what value it holds. Note that in each case triangle 0 carries a
  // count of 0, so any heuristic that samples only the first triangle concludes the file is
  // well behaved and then desynchronizes on the very first non-zero triangle.
  std::vector<StlTriangleSpec> triangleSpecs(k_TriangleCount);
  for(size_t t = 1; t < k_TriangleCount; t++)
  {
    triangleSpecs[t].attributeByteCount = static_cast<uint16_t>(0x3C1F + t); // A packed color, not a length
  }

  const std::string k_MagicsHeader = "COLOR=\xFF\xFF\xFF\xFF,MATERIAL=\xFF\xFF\xFF";
  const std::string k_VxElementsHeader = "Exported by VXelements";
  const std::string k_AnonymousHeader = "Written by some CAD package that nobody has heard of";

  auto headerText = GENERATE_COPY(k_MagicsHeader, k_VxElementsHeader, k_AnonymousHeader);

  DYNAMIC_SECTION("Header: '" << headerText << "'")
  {
    const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadStlFileTest" / "non_conforming_attribute_byte_count.stl";
    WriteBinaryStlFile(inputFile, headerText, static_cast<int32_t>(k_TriangleCount), triangleSpecs);
    REQUIRE(fs::file_size(inputFile) == StlConstants::k_StlFixedHeaderBytes + (k_TriangleCount * StlConstants::k_StlTriangleBytes));

    // The file size proves there is no payload, so no seeking may happen regardless of vendor.
    const StlConstants::StlFileCheck stlFileCheck = StlUtilities::SanityCheckFile(inputFile);
    REQUIRE(stlFileCheck.error == 0);
    REQUIRE(stlFileCheck.numTriangles == static_cast<int32_t>(k_TriangleCount));
    REQUIRE(stlFileCheck.attributePayloadPresent == false);

    DataStructure dataStructure;
    auto executeResult = RunReadStlFileFilter(dataStructure, inputFile, triangleGeomDataPath);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath));
    const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
    CheckSyntheticTriangleGeometry(triangleGeom, k_TriangleCount);

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::ReadStlFileFilter:ConformingAttributePayload", "[SimplnxCore][ReadStlFileFilter]")
{
  UnitTest::LoadPlugins();

  const size_t k_TriangleCount = 8;
  const uint16_t k_PayloadBytes = 6;
  const DataPath triangleGeomDataPath({"[Triangle Geometry]"});

  // A spec-conforming file that really does carry attribute payload after every triangle. Here the
  // byte counts are genuine lengths and MUST be honored, otherwise the read desynchronizes.
  std::vector<StlTriangleSpec> triangleSpecs(k_TriangleCount);
  for(StlTriangleSpec& spec : triangleSpecs)
  {
    spec.attributeByteCount = k_PayloadBytes;
    spec.payloadBytes = k_PayloadBytes;
  }

  const fs::path inputFile = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadStlFileTest" / "conforming_attribute_payload.stl";
  WriteBinaryStlFile(inputFile, "A well behaved binary STL writer", static_cast<int32_t>(k_TriangleCount), triangleSpecs);
  REQUIRE(fs::file_size(inputFile) == StlConstants::k_StlFixedHeaderBytes + (k_TriangleCount * (StlConstants::k_StlTriangleBytes + k_PayloadBytes)));

  const StlConstants::StlFileCheck stlFileCheck = StlUtilities::SanityCheckFile(inputFile);
  REQUIRE(stlFileCheck.error == 0);
  REQUIRE(stlFileCheck.numTriangles == static_cast<int32_t>(k_TriangleCount));
  REQUIRE(stlFileCheck.attributePayloadPresent == true);

  DataStructure dataStructure;
  auto executeResult = RunReadStlFileFilter(dataStructure, inputFile, triangleGeomDataPath);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath));
  const auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeomDataPath);
  CheckSyntheticTriangleGeometry(triangleGeom, k_TriangleCount);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::StlUtilities:SanityCheckFile", "[SimplnxCore][ReadStlFileFilter]")
{
  const fs::path testDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadStlFileTest";
  fs::create_directories(testDir);

  SECTION("Missing file returns an error instead of throwing")
  {
    const fs::path inputFile = testDir / "this_file_does_not_exist.stl";
    fs::remove(inputFile);

    StlConstants::StlFileCheck stlFileCheck;
    REQUIRE_NOTHROW(stlFileCheck = StlUtilities::SanityCheckFile(inputFile));
    REQUIRE(stlFileCheck.error == StlConstants::k_ErrorOpeningFile);
    // The message has to name the file the user actually chose.
    REQUIRE(stlFileCheck.errorMessage.find(inputFile.string()) != std::string::npos);
  }

  SECTION("File too short to hold an 80 byte header")
  {
    const fs::path inputFile = testDir / "truncated_header.stl";
    std::ofstream outputFile(inputFile, std::ios::binary | std::ios::trunc);
    const std::vector<char> shortHeader(20, 'x');
    outputFile.write(shortHeader.data(), static_cast<std::streamsize>(shortHeader.size()));
    outputFile.close();

    const StlConstants::StlFileCheck stlFileCheck = StlUtilities::SanityCheckFile(inputFile);
    REQUIRE(stlFileCheck.error == StlConstants::k_StlHeaderParseError);
    REQUIRE(stlFileCheck.errorMessage.find(inputFile.string()) != std::string::npos);
  }

  SECTION("File with a header but no triangle count")
  {
    const fs::path inputFile = testDir / "missing_triangle_count.stl";
    std::ofstream outputFile(inputFile, std::ios::binary | std::ios::trunc);
    const std::vector<char> header(StlConstants::k_STL_HEADER_LENGTH, 'x');
    outputFile.write(header.data(), static_cast<std::streamsize>(header.size()));
    outputFile.close();

    const StlConstants::StlFileCheck stlFileCheck = StlUtilities::SanityCheckFile(inputFile);
    REQUIRE(stlFileCheck.error == StlConstants::k_TriangleCountParseError);
  }

  SECTION("Header declaring a triangle count with the high bit set is rejected")
  {
    // 0xFFFFFFFF reads back as -1 in the signed field. Left unchecked this reaches
    // resizeFaceList() as an enormous unsigned value.
    const fs::path inputFile = testDir / "negative_triangle_count.stl";
    WriteBinaryStlFile(inputFile, "Corrupt triangle count", -1, std::vector<StlTriangleSpec>(1));

    const StlConstants::StlFileCheck stlFileCheck = StlUtilities::SanityCheckFile(inputFile);
    REQUIRE(stlFileCheck.error == StlConstants::k_TriangleCountParseError);
    REQUIRE(stlFileCheck.numTriangles == 0);
  }

  SECTION("Truncated triangle data reports no payload so the reader does not seek")
  {
    // The header claims more triangles than the file can hold. The read loop is responsible for
    // reporting exactly where it ran out, so SanityCheckFile must not seek past anything.
    const fs::path inputFile = testDir / "truncated_triangle_data.stl";
    WriteBinaryStlFile(inputFile, "Header count is too large", 100, std::vector<StlTriangleSpec>(4));

    const StlConstants::StlFileCheck stlFileCheck = StlUtilities::SanityCheckFile(inputFile);
    REQUIRE(stlFileCheck.error == 0);
    REQUIRE(stlFileCheck.numTriangles == 100);
    REQUIRE(stlFileCheck.attributePayloadPresent == false);
  }
}

TEST_CASE("SimplnxCore::ReadStlFileFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ReadStlFileFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadStlFileFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ReadStlFileFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ReadStlFileFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<bool>(ReadStlFileFilter::k_ScaleOutput) == true);
        CHECK(args.value<float32>(ReadStlFileFilter::k_ScaleFactor) == 2.5f);
        CHECK(args.value<std::string>(ReadStlFileFilter::k_VertexAttributeMatrixName_Key) == "TestName");
      }
      CHECK(args.value<FileSystemPathParameter::ValueType>(ReadStlFileFilter::k_StlFilePath_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<DataPath>(ReadStlFileFilter::k_CreatedTriangleGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(ReadStlFileFilter::k_FaceAttributeMatrixName_Key) == "TestName");
      CHECK(args.value<std::string>(ReadStlFileFilter::k_FaceNormalsName_Key) == "TestName");
    }
  }
}
