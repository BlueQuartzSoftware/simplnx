#include "simplnx/UnitTest/ReaderGeometryTest.hpp"

#include <catch2/catch.hpp>

#include "NXImageProcessing/Filters/ReadVolumeGraphicsFileFilter.hpp"
#include "NXImageProcessing/NXImageProcessing_test_dirs.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;

using namespace nx::core;

const fs::path k_VgiSrcFile = fs::path(nx::core::unit_test::k_DREAM3DDataDir.str()) / "TestFiles" / "volume_graphics_test.vgi";
const fs::path k_VolFile = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.str()) / "VolumeGraphicsTest.vol";
const fs::path k_VgiDestFile = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.str()) / "VolumeGraphicsTest.vgi";
const SizeVec3Type k_Dimensions = {50, 20, 80};

// -----------------------------------------------------------------------------
static void WriteVolTestFile()
{
  // Write out the data file
  FILE* outputFilePtr = fopen(k_VolFile.string().c_str(), "wb");
  REQUIRE(outputFilePtr != nullptr);
  const size_t count = k_Dimensions[0] * k_Dimensions[1] * k_Dimensions[2];
  std::vector<float> data(count, 2.0F);
  const usize bytesWritten = fwrite(data.data(), sizeof(float), count, outputFilePtr);
  REQUIRE(bytesWritten == count);
  REQUIRE(fclose(outputFilePtr) == 0);

  // Copy the vgi file
  const auto copyOptions = fs::copy_options::skip_existing;
  REQUIRE_NOTHROW(fs::copy(k_VgiSrcFile, k_VgiDestFile, copyOptions)); // copy vgi file
}

TEST_CASE("NXImageProcessing::ReadVolumeGraphicsFileFilter - Valid filter execution", "[Plugins][ReadVolumeGraphicsFileFilter]")
{
  unit_test::LoadPlugins();

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "volume_graphics_test.tar.gz", "volume_graphics_test.vgi");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const ReadVolumeGraphicsFileFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Write the .vol test file
  WriteVolTestFile();

  {
    args.insertOrAssign(ReadVolumeGraphicsFileFilter::k_VGHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_VgiDestFile));
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    DataPath dap = DataPath({"VolumeGraphics"});
    const ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(dap);
    SizeVec3Type dims = imageGeom.getDimensions();
    REQUIRE(dims[0] == 50);
    REQUIRE(dims[1] == 20);
    REQUIRE(dims[2] == 80);

    dap = dap.createChildPath("CT Data");
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(dap));

    dap = dap.createChildPath("Density");
    const Float32ArrayType& data = dataStructure.getDataRefAs<Float32ArrayType>(dap);

    const size_t numTuples = data.getNumberOfTuples();
    for(size_t i = 0; i < numTuples; i++)
    {
      REQUIRE(data[i] == 2.0F);
    }
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("NXImageProcessing::ReadVolumeGraphicsFileFilter: SIMPL Backwards Compatibility", "[NXImageProcessing][ReadVolumeGraphicsFileFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ReadVolumeGraphicsFileFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<ReadVolumeGraphicsFileFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
      CHECK(args.value<FileSystemPathParameter::ValueType>(ReadVolumeGraphicsFileFilter::k_VGHeaderFile_Key) == fs::path("/test/path/file.vgi"));
      CHECK(args.value<DataPath>(ReadVolumeGraphicsFileFilter::k_CreatedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<std::string>(ReadVolumeGraphicsFileFilter::k_CellAttributeMatrixName_Key) == "CellData");
      CHECK(args.value<std::string>(ReadVolumeGraphicsFileFilter::k_DensityArrayName_Key) == "Density");
    }
  }
}

TEST_CASE("NXImageProcessing::ReadVolumeGraphicsFileFilter rejects unsupported voxel types", "[NXImageProcessing][ReadVolumeGraphicsFileFilter]")
{
  const int32 bits = GENERATE(16, 32, 64);
  const std::string datatype = bits == 64 ? "float" : "unsigned integer";
  CAPTURE(bits, datatype);
  const fs::path outputDir(nx::core::unit_test::k_BinaryTestOutputDir.str());
  const fs::path volFile = outputDir / ("unsupported_voxel_" + std::to_string(bits) + ".vol");
  const fs::path vgiFile = outputDir / ("unsupported_voxel_" + std::to_string(bits) + ".vgi");
  auto cleanup = MakeScopeGuard([&vgiFile, &volFile]() noexcept {
    std::error_code error;
    fs::remove(vgiFile, error);
    fs::remove(volFile, error);
  });
  fs::create_directories(outputDir);
  {
    std::ofstream vol(volFile, std::ios::binary);
    REQUIRE(vol.is_open());
    // Hand-set voxel values, including the unsigned range boundary.
    if(bits == 16)
    {
      const std::array<uint16, 4> values = {0, 1, 32768, 65535};
      vol.write(reinterpret_cast<const char*>(values.data()), sizeof(values));
    }
    else if(bits == 64)
    {
      const std::array<float64, 4> values = {0.0, -1.5, 2.25, 1024.0};
      vol.write(reinterpret_cast<const char*>(values.data()), sizeof(values));
    }
    else
    {
      const std::array<uint32, 4> values = {0, 1, 2147483648U, 4294967295U};
      vol.write(reinterpret_cast<const char*>(values.data()), sizeof(values));
    }
    REQUIRE(vol.good());
  }
  {
    std::ofstream vgi(vgiFile);
    REQUIRE(vgi.is_open());
    vgi << "[file1]\nSize = 4 1 1\nName = " << volFile.filename().string() << "\nDatatype = " << datatype << "\nBitsPerElement = " << bits << "\n[geometry]\nresolution = 1 1 1\n[ end ]\n";
    REQUIRE(vgi.good());
  }

  const ReadVolumeGraphicsFileFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(ReadVolumeGraphicsFileFilter::k_VGHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(vgiFile));
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors().size() == 1);
  CHECK(preflightResult.output_actions.errors()[0].code == -91505);
  CHECK(preflightResult.output_actions.errors()[0].message.find("Datatype '" + (bits == 64 ? std::string("float") : std::string("unsigned")) + "'") != std::string::npos);
  CHECK(preflightResult.output_actions.errors()[0].message.find("BitsPerElement " + std::to_string(bits)) != std::string::npos);
}

TEST_CASE("NXImageProcessing::ReadVolumeGraphicsFileFilter preserves float32 defaults", "[NXImageProcessing][ReadVolumeGraphicsFileFilter]")
{
  const bool declareType = GENERATE(false, true);
  CAPTURE(declareType);
  const fs::path outputDir(nx::core::unit_test::k_BinaryTestOutputDir.str());
  const fs::path volFile = outputDir / "float32_voxel.vol";
  const fs::path vgiFile = outputDir / "float32_voxel.vgi";
  auto cleanup = MakeScopeGuard([&vgiFile, &volFile]() noexcept {
    std::error_code error;
    fs::remove(vgiFile, error);
    fs::remove(volFile, error);
  });
  fs::create_directories(outputDir);
  const std::array<float32, 4> values = {0.0F, -1.5F, 2.25F, 1024.0F};
  {
    std::ofstream vol(volFile, std::ios::binary);
    REQUIRE(vol.is_open());
    vol.write(reinterpret_cast<const char*>(values.data()), sizeof(values));
    REQUIRE(vol.good());
  }
  {
    std::ofstream vgi(vgiFile);
    REQUIRE(vgi.is_open());
    vgi << "[file1]\nSize = 4 1 1\nName = " << volFile.filename().string() << '\n';
    if(declareType)
    {
      vgi << "Datatype = float\nBitsPerElement = 32\n";
    }
    vgi << "[geometry]\nresolution = 1 1 1\n[ end ]\n";
    REQUIRE(vgi.good());
  }
  const ReadVolumeGraphicsFileFilter filter;
  DataStructure dataStructure;
  Arguments args;
  args.insertOrAssign(ReadVolumeGraphicsFileFilter::k_VGHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(vgiFile));
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"VolumeGraphics", "CT Data", "Density"})));
  const auto& density = dataStructure.getDataRefAs<Float32ArrayType>(DataPath({"VolumeGraphics", "CT Data", "Density"}));
  REQUIRE(density.getSize() == values.size());
  for(usize index = 0; index < values.size(); index++)
  {
    CHECK(density[index] == values[index]);
  }
}

TEST_CASE("NXImageProcessing::ReadVolumeGraphicsFileFilter: override missing, zero or out-of-range resolution", "[NXImageProcessing][ReadVolumeGraphicsFileFilter]")
{
  const std::string_view resolution = GENERATE("", "0 1 1", "1e40 3 4");
  const auto dir = k_VgiDestFile.parent_path();
  const auto header = dir / "OverrideVolume.vgi";
  const auto volume = dir / "OverrideVolume.vol";
  {
    std::ofstream stream(header);
    stream << R"({volume1}
[representation]
size = 2 2 1
[file1]
Size = 2 2 1
Name = OverrideVolume.vol
Datatype = float
BitsPerElement = 32
{volumeprimitive1}
[geometry]
unit = mm
)";
    if(!resolution.empty())
    {
      stream << "resolution = " << resolution << '\n';
    }
    REQUIRE(stream.good());
  }
  {
    const std::array<float32, 4> values{1, 2, 3, 4};
    std::ofstream bytes(volume, std::ios::binary);
    bytes.write(reinterpret_cast<const char*>(values.data()), sizeof(values));
    REQUIRE(bytes.good());
  }

  const ReadVolumeGraphicsFileFilter filter;
  Arguments args;
  args.insertOrAssign(ReadVolumeGraphicsFileFilter::k_VGHeaderFile_Key, header);
  CAPTURE(resolution);
  if(resolution == "1e40 3 4")
  {
    args.insertOrAssign(image_io::k_ChangeOrigin, true);
    DataStructure dataStructure;
    const auto strict = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(strict.output_actions);
    const auto& diagnostic = strict.output_actions.errors().front();
    INFO(diagnostic.message);
    CHECK(diagnostic.code == -91503);
    CHECK(diagnostic.message.find(header.string()) != std::string::npos);
    CHECK(diagnostic.message.find("resolution") != std::string::npos);
    CHECK(diagnostic.message.find("1e40") != std::string::npos);
    CHECK(diagnostic.message.find("Enable 'Set Spacing' to use your own values.") != std::string::npos);
  }
  else
  {
    unit_test::RequireBadReaderMetadata(filter, args, header, true);
  }
  unit_test::SetReaderGeometryOverrides(args, {4, -3, 8}, {2, 3, 4});
  unit_test::RequireReaderGeometryExecution(filter, args, DataPath({"VolumeGraphics"}), {4, -3, 8}, {2, 3, 4});
}
