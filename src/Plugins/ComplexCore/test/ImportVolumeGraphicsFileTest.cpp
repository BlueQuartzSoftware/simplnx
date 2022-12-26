/**
 * This file is auto generated from the original Plugins/ImportVolumeGraphicsFileFilter
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[ImportVolumeGraphicsFileFilter][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */


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
