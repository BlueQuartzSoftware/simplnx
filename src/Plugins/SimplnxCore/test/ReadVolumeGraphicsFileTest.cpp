#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ReadVolumeGraphicsFileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

const fs::path k_VgiSrcFile = fs::path(nx::core::unit_test::k_DREAM3DDataDir.str()) / "TestFiles" / "volume_graphics_test.vgi";
const fs::path k_VolFile = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.str()) / "VolumeGraphicsTest.vol";
const fs::path k_VgiDestFile = fs::path(nx::core::unit_test::k_BinaryTestOutputDir.str()) / "VolumeGraphicsTest.vgi";
const SizeVec3 k_Dimensions = {50, 20, 80};

// -----------------------------------------------------------------------------
void writeVolTestFile()
{
  // Write out the data file
  FILE* outputFile = fopen(k_VolFile.string().c_str(), "wb");
  REQUIRE(outputFile != nullptr);
  const size_t count = k_Dimensions[0] * k_Dimensions[1] * k_Dimensions[2];
  std::vector<float> data(count, 2.0F);
  const usize bytesWritten = fwrite(data.data(), sizeof(float), count, outputFile);
  REQUIRE(bytesWritten == count);
  REQUIRE(fclose(outputFile) == 0);

  // Copy the vgi file
  const auto copyOptions = fs::copy_options::skip_existing;
  REQUIRE_NOTHROW(fs::copy(k_VgiSrcFile, k_VgiDestFile, copyOptions)); // copy vgi file
}

TEST_CASE("SimplnxCore::ReadVolumeGraphicsFileFilter - Valid filter execution", "[Plugins][ReadVolumeGraphicsFileFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "volume_graphics_test.tar.gz", "volume_graphics_test.vgi");

  // Instantiate the filter, a DataStructure object and an Arguments Object
  const ReadVolumeGraphicsFileFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Write the .vol test file
  writeVolTestFile();

  {
    args.insertOrAssign(ReadVolumeGraphicsFileFilter::k_VGHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_VgiDestFile));
    auto preflightResult = filter.preflight(dataStructure, args);
    REQUIRE(preflightResult.outputActions.valid());

    auto executeResult = filter.execute(dataStructure, args);
    REQUIRE(executeResult.result.valid());

    DataPath dap = DataPath({"VolumeGraphics"});
    const ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(dap);
    SizeVec3 dims = imageGeom.getDimensions();
    REQUIRE(dims[0] == 50);
    REQUIRE(dims[1] == 20);
    REQUIRE(dims[2] == 80);

    dap = dap.createChildPath("CT Data");
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<AttributeMatrix>(dap));

    dap = dap.createChildPath("Density");
    const Float32Array& data = dataStructure.getDataRefAs<Float32Array>(dap);

    const size_t numTuples = data.getNumberOfTuples();
    for(size_t i = 0; i < numTuples; i++)
    {
      REQUIRE(data[i] == 2.0F);
    }
  }
}
