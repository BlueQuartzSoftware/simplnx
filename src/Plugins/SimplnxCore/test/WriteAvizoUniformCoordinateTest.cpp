#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "SimplnxCore/Filters/WriteAvizoUniformCoordinateFilter.hpp"

#include <array>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>
namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::Constants;

namespace
{
DataPath CreateUniformWriterInput(DataStructure& dataStructure, const SizeVec3& dims, const std::vector<int32>& labels)
{
  auto* geometry = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(geometry != nullptr);
  geometry->setDimensions(dims);
  geometry->setSpacing({2, 3, 4});
  geometry->setOrigin({10, -5, 2});
  const ShapeType shape = {dims[2], dims[1], dims[0]};
  auto* cells = AttributeMatrix::Create(dataStructure, "Cells", shape, geometry->getId());
  REQUIRE(cells != nullptr);
  geometry->setCellData(cells->getId());
  const DataPath labelsPath({"Image", "Cells", "Labels"});
  auto* array = Int32Array::Create(dataStructure, "Labels", DataStoreUtilities::CreateDataStore<int32>(dataStructure, labelsPath, shape, {1}), cells->getId());
  REQUIRE(array != nullptr);
  REQUIRE(labels.size() == array->getNumberOfTuples());
  auto writeResult = array->getDataStoreRef().copyFromBuffer(0, labels);
  SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  return labelsPath;
}

std::string WriteUniformFile(DataStructure& dataStructure, const DataPath& labelsPath, bool binary, const std::string& name)
{
  WriteAvizoUniformCoordinateFilter filter;
  Arguments args = filter.getDefaultArguments();
  const fs::path path = fs::path(unit_test::k_BinaryTestOutputDir.view()) / name;
  args.insertOrAssign(WriteAvizoUniformCoordinateFilter::k_OutputFile_Key, std::make_any<fs::path>(path));
  args.insertOrAssign(WriteAvizoUniformCoordinateFilter::k_WriteBinaryFile_Key, std::make_any<bool>(binary));
  args.insertOrAssign(WriteAvizoUniformCoordinateFilter::k_GeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(WriteAvizoUniformCoordinateFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(labelsPath));
  args.insertOrAssign(WriteAvizoUniformCoordinateFilter::k_Units_Key, std::make_any<std::string>("microns"));
  auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  std::ifstream file(path, std::ios::binary);
  REQUIRE(file.is_open());
  return std::string(std::istreambuf_iterator<char>(file), {});
}

std::string WithoutAuthorAndDate(const std::string& content)
{
  std::istringstream stream(content);
  std::string result;
  std::string line;
  usize lineIndex = 0;
  while(std::getline(stream, line))
  {
    if(lineIndex != 6 && lineIndex != 7)
    {
      result += line + '\n';
    }
    ++lineIndex;
  }
  return result;
}
} // namespace

TEST_CASE("SimplnxCore::WriteAvizoUniformCoordinateFilter: Valid Filter Execution", "[SimplnxCore][WriteAvizoUniformCoordinateFilter]")
{
  const bool binary = GENERATE(false, true);
  CAPTURE(binary);
  DataStructure dataStructure;
  const DataPath labelsPath = CreateUniformWriterInput(dataStructure, {3, 2, 1}, {101, 102, 103, 104, 105, 106});
  const std::string actual = WriteUniformFile(dataStructure, labelsPath, binary, binary ? "UniformCentersBinary.am" : "UniformCentersAscii.am");
  const std::string firstLine = binary ? (std::endian::native == std::endian::little ? "# AmiraMesh BINARY-LITTLE-ENDIAN 2.1\n" : "# AmiraMesh BINARY 2.1\n") : "# AmiraMesh 3D ASCII 2.0\n";
  // X centers are 10+(i+0.5)*2=11,13,15; Y centers are -3.5,-0.5; the only Z center is 4.
  const std::string expectedHeader = firstLine + R"(
# Dimensions in x-, y-, and z-direction
define Lattice 3 2 1
Parameters {
     DREAM3DParams {
         Author "ignored",
         DateTime "ignored"
         FeatureIds Path "Image/Cells/Labels"
     }
     Units {
         Coordinates "microns"
     }
     Content "3x2x1 int, uniform coordinates",
     # Bounding Box is xmin xmax ymin ymax zmin zmax
     BoundingBox 11.000000 15.000000 -3.500000 -0.500000 4.000000 4.000000
     CoordType "uniform"
}

Lattice { int FeatureIds } = @1
# Data section follows
@1
)";
  const usize payloadStart = actual.find("\n@1\n");
  REQUIRE(payloadStart != std::string::npos);
  CHECK(WithoutAuthorAndDate(actual.substr(0, payloadStart + 4)) == WithoutAuthorAndDate(expectedHeader));
  const usize boxStart = actual.find("     BoundingBox ");
  REQUIRE(boxStart != std::string::npos);
  std::istringstream boxStream(actual.substr(boxStart));
  std::string boxName;
  boxStream >> boxName;
  CHECK(boxName == "BoundingBox");
  const std::array<float64, 6> expectedBox = {11, 15, -3.5, -0.5, 4, 4};
  for(float64 expected : expectedBox)
  {
    float64 value = 0;
    REQUIRE(static_cast<bool>(boxStream >> value));
    CHECK(value == expected);
  }
  const std::string payload = actual.substr(payloadStart + 4);
  std::string expectedPayload = "101 102 103 104 105 106 \n";
  if(binary)
  {
    // The six Int32 labels have literal little-endian byte sequences.
    constexpr std::array<uint8, 24> k_Bytes = {101, 0, 0, 0, 102, 0, 0, 0, 103, 0, 0, 0, 104, 0, 0, 0, 105, 0, 0, 0, 106, 0, 0, 0};
    expectedPayload.assign(k_Bytes.begin(), k_Bytes.end());
    expectedPayload += '\n';
    REQUIRE(payload.size() == 25);
    for(usize i = 0; i < 6; ++i)
    {
      int32 value = 0;
      std::memcpy(&value, payload.data() + 4 * i, sizeof(value));
      CHECK(value == static_cast<int32>(101 + i));
    }
  }
  // The expected file is independent of writer output. Only author and date lines are ignored.
  const fs::path expectedPath = fs::path(unit_test::k_BinaryTestOutputDir.view()) / (binary ? "UniformCentersExpectedBinary.am" : "UniformCentersExpectedAscii.am");
  {
    std::ofstream expectedFile(expectedPath, std::ios::binary);
    expectedFile << expectedHeader << expectedPayload;
    REQUIRE(expectedFile.good());
  }
  std::ifstream expectedFile(expectedPath, std::ios::binary);
  REQUIRE(expectedFile.is_open());
  const std::string expected(std::istreambuf_iterator<char>(expectedFile), {});
  if(!binary || std::endian::native == std::endian::little)
  {
    CHECK(WithoutAuthorAndDate(actual) == WithoutAuthorAndDate(expected));
    CHECK(payload == expectedPayload);
  }
  else
  {
    WARN("The explicit little-endian byte comparison is skipped on a big-endian platform.");
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAvizoUniformCoordinateFilter: Preserve every value across transfer blocks", "[SimplnxCore][WriteAvizoUniformCoordinateFilter]")
{
  const bool binary = GENERATE(false, true);
  CAPTURE(binary);
  constexpr usize k_Count = 75000;
  std::vector<int32> labels(k_Count);
  for(usize i = 0; i < k_Count; ++i)
  {
    // Truncation modulo 2^32 keeps multiple nonzero bytes and distinct values across the 65536-value boundary.
    labels[i] = std::bit_cast<int32>(static_cast<uint32>(static_cast<uint64>(i) * 65537 + 7));
  }
  CHECK(labels[65535] == 6);
  CHECK(labels[65536] == 65543);
  CHECK(labels[74999] == 620242174);
  DataStructure dataStructure;
  const DataPath labelsPath = CreateUniformWriterInput(dataStructure, {300, 250, 1}, labels);
  const std::string actual = WriteUniformFile(dataStructure, labelsPath, binary, binary ? "UniformBlocksBinary.am" : "UniformBlocksAscii.am");
  const usize marker = actual.find("\n@1\n");
  REQUIRE(marker != std::string::npos);
  const std::string header = actual.substr(0, marker + 4);
  CHECK(header.find("define Lattice 300 250 1\n") != std::string::npos);
  CHECK(header.find("Coordinates \"microns\"") != std::string::npos);
  const std::string payload = actual.substr(marker + 4);
  if(binary)
  {
    const std::string token = std::endian::native == std::endian::little ? "# AmiraMesh BINARY-LITTLE-ENDIAN 2.1\n" : "# AmiraMesh BINARY 2.1\n";
    CHECK(header.rfind(token, 0) == 0);
    // 75000 Int32 values occupy 300000 bytes, followed by the writer's single final newline.
    REQUIRE(payload.size() == 300001);
    CHECK(payload.back() == '\n');
    std::string expectedBytes;
    expectedBytes.reserve(300001);
    for(usize i = 0; i < k_Count; ++i)
    {
      int32 value = 0;
      std::memcpy(&value, payload.data() + 4 * i, sizeof(value));
      REQUIRE(value == labels[i]);
      const uint32 bits = static_cast<uint32>(static_cast<uint64>(i) * 65537 + 7);
      for(usize byte = 0; byte < 4; ++byte)
      {
        expectedBytes += static_cast<char>((bits >> (8 * byte)) & 0xffU);
      }
    }
    expectedBytes += '\n';
    if(std::endian::native == std::endian::little)
    {
      CHECK(payload == expectedBytes);
    }
    else
    {
      WARN("The explicit little-endian byte comparison is skipped on a big-endian platform.");
    }
  }
  else
  {
    CHECK(header.rfind("# AmiraMesh 3D ASCII 2.0\n", 0) == 0);
    std::string expectedPayload;
    for(usize i = 0; i < k_Count; ++i)
    {
      expectedPayload += std::to_string(labels[i]);
      expectedPayload += i % 21 == 20 ? '\n' : ' ';
    }
    expectedPayload += '\n';
    CHECK(payload.size() == expectedPayload.size());
    CHECK(payload == expectedPayload);
    std::istringstream lines(payload);
    std::string line;
    usize lineCount = 0;
    usize valueCount = 0;
    while(std::getline(lines, line))
    {
      std::istringstream values(line);
      int32 value = 0;
      usize lineValues = 0;
      while(values >> value)
      {
        REQUIRE(valueCount < k_Count);
        REQUIRE(value == labels[valueCount]);
        ++valueCount;
        ++lineValues;
      }
      // 75000=3571*21+9, so the final partial line has nine values.
      REQUIRE(lineValues == (lineCount < 3571 ? 21 : 9));
      ++lineCount;
    }
    CHECK(lineCount == 3572);
    CHECK(valueCount == k_Count);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::WriteAvizoUniformCoordinateFilter: SIMPL Backwards Compatibility", "[SimplnxCore][WriteAvizoUniformCoordinateFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "WriteAvizoUniformCoordinateFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "WriteAvizoUniformCoordinateFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<WriteAvizoUniformCoordinateFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(WriteAvizoUniformCoordinateFilter::k_OutputFile_Key) == fs::path("/test/path/file.txt"));
      CHECK(args.value<bool>(WriteAvizoUniformCoordinateFilter::k_WriteBinaryFile_Key) == true);
      CHECK(args.value<DataPath>(WriteAvizoUniformCoordinateFilter::k_GeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(WriteAvizoUniformCoordinateFilter::k_FeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<std::string>(WriteAvizoUniformCoordinateFilter::k_Units_Key) == "TestName");
    }
  }
}

TEST_CASE("SimplnxCore::WriteAvizoUniformCoordinateFilter: Geometry cell tuple count", "[SimplnxCore][WriteAvizoUniformCoordinateFilter][TupleCount]")
{
  UnitTest::LoadPlugins();
  using FilterType = WriteAvizoUniformCoordinateFilter;
  DataStructure dataStructure;
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image");
  REQUIRE(imageGeomPtr != nullptr);
  imageGeomPtr->setDimensions({2, 3, 2});
  imageGeomPtr->setSpacing({1.0F, 1.0F, 1.0F});
  auto* cellAMPtr = AttributeMatrix::Create(dataStructure, "Cell Data", ShapeType{12}, imageGeomPtr->getId());
  REQUIRE(cellAMPtr != nullptr);
  imageGeomPtr->setCellData(*cellAMPtr);

  const usize tuples = GENERATE(usize{0}, usize{11}, usize{12}, usize{13});
  CAPTURE(tuples);

  // A matching input in another Attribute Matrix must remain valid.
  auto* otherAMPtr = AttributeMatrix::Create(dataStructure, "Other Data", ShapeType{tuples});
  REQUIRE(otherAMPtr != nullptr);
  REQUIRE(DataArray<int32>::Create(dataStructure, "Values", std::make_shared<DataStore<int32>>(ShapeType{tuples}, ShapeType{1}, 1), otherAMPtr->getId()) != nullptr);
  const DataPath inputPath({"Other Data", "Values"});

  FilterType filter;
  Arguments args;
  args.insertOrAssign(FilterType::k_GeometryPath_Key, std::make_any<DataPath>(DataPath({"Image"})));
  args.insertOrAssign(FilterType::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(FilterType::k_OutputFile_Key, std::make_any<fs::path>(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "TupleCountWriteAvizoUniformCoordinate.am"));

  const auto result = filter.preflight(dataStructure, args);
  if(tuples == 12)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().size() == 1);
    const auto& errorRef = result.outputActions.errors()[0];
    REQUIRE(errorRef.code == -5835);
    const DataPath expectedPath = inputPath;
    const usize expectedTuples = tuples;
    REQUIRE(errorRef.message.find(expectedPath.toString()) != std::string::npos);
    REQUIRE(errorRef.message.find(fmt::format("has {} tuples", expectedTuples)) != std::string::npos);
    REQUIRE(errorRef.message.find("'Image' has 12 cells") != std::string::npos);
  }
}
