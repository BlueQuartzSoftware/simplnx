#include <catch2/catch.hpp>

#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ImportVolumeGraphicsFileFilter.hpp"

using namespace complex;

const fs::path k_VgiSrcFile = fs::path(complex::unit_test::k_DREAM3DDataDir.str()) / "TestFiles" / "volume_graphics_test.vgi";
const fs::path k_VolFile = fs::temp_directory_path() / "VolumeGraphicsTest.vol";
const fs::path k_VgiDestFile = k_VolFile.parent_path() / "VolumeGraphicsTest.vgi";
const SizeVec3 k_Dimensions = {50, 20, 80};

// -----------------------------------------------------------------------------
void writeVolTestFile()
{
  // Write out the data file
  FILE* f = fopen(k_VolFile.c_str(), "wb");
  size_t count = k_Dimensions[0] * k_Dimensions[1] * k_Dimensions[2];
  std::vector<float> data(count, 2.0F);
  usize bytesWritten = fwrite(data.data(), sizeof(float), count, f);
  REQUIRE(bytesWritten == count);
  fclose(f);

  // Copy the vgi file
  const auto copyOptions = fs::copy_options::skip_existing;
  REQUIRE_NOTHROW(fs::copy(k_VgiSrcFile, k_VgiDestFile, copyOptions)); // copy vgi file
}

TEST_CASE("Plugins::ImportVolumeGraphicsFileFilter - Valid filter execution", "[Plugins][ImportVolumeGraphicsFileFilter]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ImportVolumeGraphicsFileFilter filter;
  DataStructure dataStructure;
  Arguments args;

  // Write the .vol test file
  writeVolTestFile();

  {
    args.insertOrAssign(ImportVolumeGraphicsFileFilter::k_VGHeaderFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_VgiDestFile));
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
    const AttributeMatrix& am = dataStructure.getDataRefAs<AttributeMatrix>(dap);

    dap = dap.createChildPath("Density");
    const Float32Array& data = dataStructure.getDataRefAs<Float32Array>(dap);

    size_t numTuples = data.getNumberOfTuples();
    for(size_t i = 0; i < numTuples; i++)
    {
      REQUIRE(data[i] == 2.0F);
    }
  }
}
