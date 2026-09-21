#include <catch2/catch.hpp>

#include "ImageProcessing/Filters/ReadImageFilter.hpp"
#include "ImageProcessing/ImageProcessing_test_dirs.hpp"
#include "ImageProcessing/utils/NrrdUtilities.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <fmt/format.h>

#include <tiffio.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <numeric>
#include <system_error>
#include <type_traits>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_TestDataDirName = "itk_image_reader_test_v3";
const fs::path k_TestDataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_TestDataDirName;
const fs::path k_ExemplarFile = k_TestDataDir / "itk_image_reader_test_v3.dream3d";
const fs::path k_InputImageFile = k_TestDataDir / "200x200_0.tif";
const std::string k_ImageGeometryName = "[ImageGeometry]";
const std::string k_ImageCellDataName = "Cell Data";
const std::string k_ImageDataName = "ImageData";

// Values for ReadImageFilter::k_OriginSpacingProcessing_Key
// 0 = Preprocessed, 1 = Postprocessed
constexpr uint64 k_Preprocessed = 0;
constexpr uint64 k_Postprocessed = 1;

class TemporaryTiledFilterFixture
{
public:
  TemporaryTiledFilterFixture()
  : m_Path(fs::path(unit_test::k_BinaryTestOutputDir.view()) / "ReadImageFilter_tiled_orientation.tif")
  {
    std::error_code errorCode;
    fs::remove(m_Path, errorCode);

    using TiffHandle = std::unique_ptr<TIFF, decltype(&TIFFClose)>;
    TiffHandle tiff(TIFFOpen(m_Path.string().c_str(), "w"), TIFFClose);
    REQUIRE(tiff != nullptr);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_IMAGEWIDTH, k_Width) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_IMAGELENGTH, k_Height) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_BITSPERSAMPLE, 8) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_SAMPLESPERPIXEL, 1) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_ORIENTATION, ORIENTATION_BOTRIGHT) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_TILEWIDTH, k_TileWidth) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_TILELENGTH, k_TileHeight) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) == 1);
    REQUIRE(TIFFSetField(tiff.get(), TIFFTAG_COMPRESSION, COMPRESSION_NONE) == 1);

    std::vector<uint8> tile(static_cast<usize>(k_TileWidth) * k_TileHeight, 0);
    for(uint32_t tileY = 0; tileY < k_Height; tileY += k_TileHeight)
    {
      for(uint32_t tileX = 0; tileX < k_Width; tileX += k_TileWidth)
      {
        std::fill(tile.begin(), tile.end(), uint8{0});
        for(uint32_t localY = 0; localY < k_TileHeight; ++localY)
        {
          for(uint32_t localX = 0; localX < k_TileWidth; ++localX)
          {
            const uint32_t sourceX = tileX + localX;
            const uint32_t sourceY = tileY + localY;
            if(sourceX < k_Width && sourceY < k_Height)
            {
              tile[static_cast<usize>(localY) * k_TileWidth + localX] = static_cast<uint8>((sourceX + 3 * sourceY) % 251);
            }
          }
        }
        const ttile_t tileIndex = TIFFComputeTile(tiff.get(), tileX, tileY, 0, 0);
        const auto byteCount = static_cast<tmsize_t>(tile.size());
        REQUIRE(TIFFWriteEncodedTile(tiff.get(), tileIndex, tile.data(), byteCount) == byteCount);
      }
    }
  }

  ~TemporaryTiledFilterFixture() noexcept
  {
    std::error_code errorCode;
    fs::remove(m_Path, errorCode);
  }

  const fs::path& path() const
  {
    return m_Path;
  }

  static constexpr uint32_t k_Width = 35;
  static constexpr uint32_t k_Height = 19;
  static constexpr uint32_t k_TileWidth = 16;
  static constexpr uint32_t k_TileHeight = 16;

private:
  fs::path m_Path;
};
} // namespace

TEST_CASE("ImageProcessing::ReadImageFilter: Tiled_Tiff_Photometric_Orientation", "[ImageProcessing][ReadImageFilter]")
{
  TemporaryTiledFilterFixture fixture;
  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath imageGeometryPath({"Tiled Image"});
  const std::string cellDataName = "Cell Data";
  const std::string imageDataName = "Pixels";
  args.insertOrAssign(ReadImageFilter::k_FileName_Key, fixture.path());
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, imageGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(cellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(imageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const DataPath pixelPath = imageGeometryPath.createChildPath(cellDataName).createChildPath(imageDataName);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(pixelPath));
  const auto& pixels = dataStructure.getDataRefAs<UInt8Array>(pixelPath).getDataStoreRef();
  REQUIRE(pixels.getSize() == static_cast<usize>(TemporaryTiledFilterFixture::k_Width) * TemporaryTiledFilterFixture::k_Height);
  for(usize outputY = 0; outputY < TemporaryTiledFilterFixture::k_Height; ++outputY)
  {
    for(usize outputX = 0; outputX < TemporaryTiledFilterFixture::k_Width; ++outputX)
    {
      const usize sourceX = TemporaryTiledFilterFixture::k_Width - 1 - outputX;
      const usize sourceY = TemporaryTiledFilterFixture::k_Height - 1 - outputY;
      const uint8 raw = static_cast<uint8>((sourceX + 3 * sourceY) % 251);
      REQUIRE(pixels[outputY * TemporaryTiledFilterFixture::k_Width + outputX] == static_cast<uint8>(255 - raw));
    }
  }
}

TEST_CASE("ImageProcessing::ReadImageFilter: Read_Basic", "[ImageProcessing][ReadImageFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({k_ImageGeometryName});

  args.insertOrAssign(ReadImageFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Read_Basic"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Read_Basic"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Read_Basic", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageFilter: Override_Origin", "[ImageProcessing][ReadImageFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::vector<float32> k_Origin{-32.0, -32.0, 0.0};

  const DataPath inputGeometryPath({k_ImageGeometryName});

  args.insertOrAssign(ReadImageFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ReadImageFilter::k_Origin_Key, k_Origin);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Override_Origin"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Override_Origin"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Override_Origin", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageFilter: Centering_Origin", "[ImageProcessing][ReadImageFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({k_ImageGeometryName});

  args.insertOrAssign(ReadImageFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ReadImageFilter::k_CenterOrigin_Key, true);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Centering_Origin"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Centering_Origin"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Centering_Origin", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageFilter: Override_Spacing", "[ImageProcessing][ReadImageFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::vector<float32> k_Spacing{2.5, 3.0, 1.0};

  const DataPath inputGeometryPath({k_ImageGeometryName});

  args.insertOrAssign(ReadImageFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ReadImageFilter::k_Spacing_Key, k_Spacing);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Override_Spacing"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Override_Spacing"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Override_Spacing", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageFilter: OriginSpacing_Preprocessed", "[ImageProcessing][ReadImageFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::vector<float32> k_Origin{10.0, 20.0, 0.0};
  std::vector<float32> k_Spacing{2.0, 2.0, 1.0};

  const DataPath inputGeometryPath({k_ImageGeometryName});

  auto cropOptions = CropGeometryParameter::ValueType();
  cropOptions.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  cropOptions.cropX = true;
  cropOptions.cropY = true;
  cropOptions.cropZ = false;
  cropOptions.xBoundVoxels = {50, 150};
  cropOptions.yBoundVoxels = {50, 150};

  args.insertOrAssign(ReadImageFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ReadImageFilter::k_Origin_Key, k_Origin);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ReadImageFilter::k_Spacing_Key, k_Spacing);
  args.insertOrAssign(ReadImageFilter::k_OriginSpacingProcessing_Key, k_Preprocessed);
  args.insertOrAssign(ReadImageFilter::k_CroppingOptions_Key, cropOptions);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"OriginSpacing_Preprocessed"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"OriginSpacing_Preprocessed"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"OriginSpacing_Preprocessed", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageFilter: OriginSpacing_Postprocessed", "[ImageProcessing][ReadImageFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::vector<float32> k_Origin{10.0, 20.0, 0.0};
  std::vector<float32> k_Spacing{2.0, 2.0, 1.0};

  const DataPath inputGeometryPath({k_ImageGeometryName});

  auto cropOptions = CropGeometryParameter::ValueType();
  cropOptions.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  cropOptions.cropX = true;
  cropOptions.cropY = true;
  cropOptions.cropZ = false;
  cropOptions.xBoundVoxels = {50, 150};
  cropOptions.yBoundVoxels = {50, 150};

  args.insertOrAssign(ReadImageFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ReadImageFilter::k_Origin_Key, k_Origin);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ReadImageFilter::k_Spacing_Key, k_Spacing);
  args.insertOrAssign(ReadImageFilter::k_OriginSpacingProcessing_Key, k_Postprocessed);
  args.insertOrAssign(ReadImageFilter::k_CroppingOptions_Key, cropOptions);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"OriginSpacing_Postprocessed"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"OriginSpacing_Postprocessed"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"OriginSpacing_Postprocessed", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint8>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageFilter: DataType_Conversion", "[ImageProcessing][ReadImageFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({k_ImageGeometryName});

  const uint64 k_DataTypeUInt16 = 1;

  args.insertOrAssign(ReadImageFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, false);
  args.insertOrAssign(ReadImageFilter::k_ChangeDataType_Key, true);
  args.insertOrAssign(ReadImageFilter::k_ImageDataType_Key, k_DataTypeUInt16);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"DataType_Conversion"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"DataType_Conversion"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"DataType_Conversion", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint16>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageFilter: Interaction_Crop_DataType", "[ImageProcessing][ReadImageFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({k_ImageGeometryName});

  auto cropOptions = CropGeometryParameter::ValueType();
  cropOptions.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  cropOptions.cropX = true;
  cropOptions.cropY = true;
  cropOptions.cropZ = false;
  cropOptions.xBoundVoxels = {50, 150};
  cropOptions.yBoundVoxels = {50, 150};

  const uint64 k_DataTypeUInt32 = 2;

  args.insertOrAssign(ReadImageFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, false);
  args.insertOrAssign(ReadImageFilter::k_CroppingOptions_Key, cropOptions);
  args.insertOrAssign(ReadImageFilter::k_ChangeDataType_Key, true);
  args.insertOrAssign(ReadImageFilter::k_ImageDataType_Key, k_DataTypeUInt32);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Interaction_Crop_DataType"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Interaction_Crop_DataType"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Interaction_Crop_DataType", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint32>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ReadImageFilter: Interaction_All", "[ImageProcessing][ReadImageFilter]")
{
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "itk_image_reader_test_v3.tar.gz", k_TestDataDirName, true, true);

  UnitTest::LoadPlugins();
  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  std::vector<float32> k_Origin{5.0, 10.0, 0.0};
  std::vector<float32> k_Spacing{2.0, 2.0, 1.0};
  const DataPath inputGeometryPath({k_ImageGeometryName});

  auto cropOptions = CropGeometryParameter::ValueType();
  cropOptions.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  cropOptions.cropX = true;
  cropOptions.cropY = true;
  cropOptions.cropZ = false;
  cropOptions.xBoundVoxels = {50, 150};
  cropOptions.yBoundVoxels = {50, 150};

  const uint64 k_DataTypeUInt16 = 1;

  args.insertOrAssign(ReadImageFilter::k_FileName_Key, k_InputImageFile);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, true);
  args.insertOrAssign(ReadImageFilter::k_Origin_Key, k_Origin);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, true);
  args.insertOrAssign(ReadImageFilter::k_Spacing_Key, k_Spacing);
  args.insertOrAssign(ReadImageFilter::k_OriginSpacingProcessing_Key, k_Preprocessed);
  args.insertOrAssign(ReadImageFilter::k_CroppingOptions_Key, cropOptions);
  args.insertOrAssign(ReadImageFilter::k_ChangeDataType_Key, true);
  args.insertOrAssign(ReadImageFilter::k_ImageDataType_Key, k_DataTypeUInt16);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  // Compare against exemplar
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath));
  const auto& generatedGeom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Interaction_Crop_OriginSpacing_Preprocessed_DataType"})));
  const auto& exemplarGeom = exemplarDS.getDataRefAs<ImageGeom>(DataPath({"Interaction_Crop_OriginSpacing_Preprocessed_DataType"}));
  UnitTest::CompareImageGeometry(&exemplarGeom, &generatedGeom);

  DataPath generatedDataPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  DataPath exemplarDataPath = DataPath({"Interaction_Crop_OriginSpacing_Preprocessed_DataType", Constants::k_Cell_Data, k_ImageDataName});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(generatedDataPath));
  const auto& generatedArray = dataStructure.getDataRefAs<IDataArray>(generatedDataPath);
  REQUIRE_NOTHROW(exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath));
  const auto& exemplarArray = exemplarDS.getDataRefAs<IDataArray>(exemplarDataPath);
  UnitTest::CompareDataArrays<uint16>(exemplarArray, generatedArray);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// Dimension-aware smoke tests: a multi-page (3D) TIFF read as a Z-stack, and a small NRRD volume
// read through the same filter. These synthesize their inputs at runtime (no committed binaries).
// -----------------------------------------------------------------------------
namespace
{
fs::path ReadImageOutputDir()
{
  fs::path dir = fs::path(std::string(nx::core::unit_test::k_BinaryTestOutputDir.view())) / "ReadImage";
  fs::create_directories(dir);
  return dir;
}

// Writes an N-page uint8 TIFF where every page has the same width/height/1-component layout.
// pagePixels[p] is row-major (top-to-bottom), size == width*height.
void WriteMultiPageTiff(const fs::path& path, uint32_t width, uint32_t height, const std::vector<std::vector<uint8_t>>& pagePixels)
{
  TIFF* tif = TIFFOpen(path.string().c_str(), "w");
  REQUIRE(tif != nullptr);
  for(const auto& pixels : pagePixels)
  {
    REQUIRE(pixels.size() == static_cast<size_t>(width) * height);
    REQUIRE(TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, static_cast<uint16_t>(1)) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0)) != 0);
    for(uint32_t row = 0; row < height; ++row)
    {
      // TIFFWriteScanline takes a non-const void* but does not modify the buffer.
      REQUIRE(TIFFWriteScanline(tif, const_cast<uint8_t*>(pixels.data() + static_cast<size_t>(row) * width), row, 0) >= 0);
    }
    REQUIRE(TIFFWriteDirectory(tif) != 0);
  }
  TIFFClose(tif);
}

// Writes an attached NRRD: ASCII header lines, a blank line, then raw data bytes.
void WriteNrrdRaw(const fs::path& path, const std::vector<std::string>& headerLines, const std::vector<uint8_t>& data)
{
  std::ofstream ofs(path, std::ios::binary);
  REQUIRE(ofs.is_open());
  for(const auto& line : headerLines)
  {
    ofs << line << "\n";
  }
  ofs << "\n"; // blank line terminates the header
  if(!data.empty())
  {
    ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
  }
}
} // namespace

TEST_CASE("ImageProcessing::ReadImageFilter: MultiPage_TIFF_ZStack", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();

  constexpr uint32_t width = 5;
  constexpr uint32_t height = 4;
  constexpr usize numPages = 3;

  // Each pixel encodes its page and in-plane position so both the Z ordering and the X-fastest
  // in-plane ordering can be verified: pixel(page p, row y, col x) = p*100 + (y*width + x).
  std::vector<std::vector<uint8_t>> pagePixels(numPages, std::vector<uint8_t>(static_cast<size_t>(width) * height));
  for(usize p = 0; p < numPages; ++p)
  {
    for(size_t i = 0; i < pagePixels[p].size(); ++i)
    {
      pagePixels[p][i] = static_cast<uint8_t>(p * 100 + i);
    }
  }

  const fs::path tiffPath = ReadImageOutputDir() / "three_page_zstack.tif";
  WriteMultiPageTiff(tiffPath, width, height, pagePixels);

  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({k_ImageGeometryName});
  args.insertOrAssign(ReadImageFilter::k_FileName_Key, tiffPath);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));
  args.insertOrAssign(ReadImageFilter::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ReadImageFilter::k_ChangeSpacing_Key, false);

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto& geom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE(geom.getDimensions()[0] == width);
  REQUIRE(geom.getDimensions()[1] == height);
  REQUIRE(geom.getDimensions()[2] == numPages);

  const DataPath arrayPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  const auto& store = dataStructure.getDataRefAs<DataArray<uint8>>(arrayPath).getDataStoreRef();
  REQUIRE(store.getNumberOfTuples() == static_cast<usize>(width) * height * numPages);
  for(usize z = 0; z < numPages; ++z)
  {
    for(usize i = 0; i < static_cast<usize>(width) * height; ++i)
    {
      const usize flatIndex = z * width * height + i;
      REQUIRE(store.getValue(flatIndex) == pagePixels[z][i]);
    }
  }
}

TEST_CASE("ImageProcessing::ReadImageFilter: Nrrd_Scalar3D", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();

  constexpr usize X = 4;
  constexpr usize Y = 3;
  constexpr usize Z = 2;
  std::vector<int16_t> fileValues(X * Y * Z);
  for(usize i = 0; i < fileValues.size(); ++i)
  {
    fileValues[i] = static_cast<int16_t>(i) - 5;
  }
  std::vector<uint8_t> rawBytes(fileValues.size() * sizeof(int16_t));
  std::memcpy(rawBytes.data(), fileValues.data(), rawBytes.size());

  const fs::path nrrdPath = ReadImageOutputDir() / "read_image_scalar3d.nrrd";
  WriteNrrdRaw(nrrdPath, {"NRRD0004", "type: short", "dimension: 3", "sizes: 4 3 2", "kinds: domain domain domain", "endian: little", "encoding: raw"}, rawBytes);

  ReadImageFilter filter;
  DataStructure dataStructure;
  Arguments args;

  const DataPath inputGeometryPath({k_ImageGeometryName});
  args.insertOrAssign(ReadImageFilter::k_FileName_Key, nrrdPath);
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, inputGeometryPath);
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageCellDataName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, static_cast<DataObjectNameParameter::ValueType>(k_ImageDataName));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  const auto& geom = dataStructure.getDataRefAs<ImageGeom>(inputGeometryPath);
  REQUIRE(geom.getDimensions()[0] == X);
  REQUIRE(geom.getDimensions()[1] == Y);
  REQUIRE(geom.getDimensions()[2] == Z);

  const DataPath arrayPath = inputGeometryPath.createChildPath(k_ImageCellDataName).createChildPath(k_ImageDataName);
  const auto& store = dataStructure.getDataRefAs<DataArray<int16>>(arrayPath).getDataStoreRef();
  REQUIRE(store.getNumberOfTuples() == X * Y * Z);
  // No cropping => destination tuple order equals file order, so store[i] == fileValues[i].
  for(usize i = 0; i < fileValues.size(); ++i)
  {
    REQUIRE(store.getValue(i) == fileValues[i]);
  }
}

// =============================================================================
// NRRD filter-level cases, migrated from the retired standalone NRRD reader.
// ReadImage reads .nrrd/.nhdr through its NRRD backend: preflight builds the
// ImageGeom + Cell DataArray (incl. optional crop delegation to
// CropImageGeometryFilter + the non-axis-aligned 'space directions' warning
// -35750), and execute streams voxels through the ReadNrrdFile algorithm. These
// prove ReadImage's NRRD path == the live-ITK oracle and that its bulk-copy OOC
// path is byte-exact. (Pure header-parse / algorithm-level NRRD unit tests live
// in NrrdUtilitiesTest.cpp.)
// =============================================================================
namespace
{
template <typename T>
std::vector<uint8_t> ToBytes(const std::vector<T>& values)
{
  std::vector<uint8_t> out(values.size() * sizeof(T));
  std::memcpy(out.data(), values.data(), out.size());
  return out;
}

// Compress @p data as a gzip stream (windowBits 15+16). Used to synthesize
// `encoding: gzip` NRRD data segments in tests.
std::vector<uint8_t> GzipCompress(const std::vector<uint8_t>& data)
{
  z_stream zs{};
  REQUIRE(deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) == Z_OK);
  std::vector<uint8_t> out(deflateBound(&zs, static_cast<uLong>(data.size())));
  zs.next_in = const_cast<Bytef*>(data.data());
  zs.avail_in = static_cast<uInt>(data.size());
  zs.next_out = out.data();
  zs.avail_out = static_cast<uInt>(out.size());
  const int rc = deflate(&zs, Z_FINISH);
  REQUIRE(rc == Z_STREAM_END);
  out.resize(zs.total_out);
  deflateEnd(&zs);
  return out;
}

// Builds a ReadImageFilter Arguments set for a NRRD input, mapping the retired NRRD reader's
// keys onto ReadImageFilter's (file_name / cropping_options / output_geometry_path /
// cell_attribute_matrix_name / image_data_array_name). The origin/spacing/data-type override
// params are left at their defaults (the NRRD backend ignores them).
Arguments MakeReadImageNrrdArgs(const fs::path& inputFile, const DataPath& geomPath, const std::string& amName, const std::string& arrName,
                                const CropGeometryParameter::ValueType& crop = CropGeometryParameter::ValueType{})
{
  Arguments args;
  args.insertOrAssign(ReadImageFilter::k_FileName_Key, std::make_any<fs::path>(inputFile));
  args.insertOrAssign(ReadImageFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(crop));
  args.insertOrAssign(ReadImageFilter::k_ImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ReadImageFilter::k_CellDataName_Key, std::make_any<std::string>(amName));
  args.insertOrAssign(ReadImageFilter::k_ImageDataArrayPath_Key, std::make_any<std::string>(arrName));
  return args;
}

template <typename T>
void RequireNrrdArrayEquals(const DataStructure& ds, const DataPath& arrPath, const std::vector<T>& expected)
{
  REQUIRE_NOTHROW(ds.getDataRefAs<DataArray<T>>(arrPath));
  const auto& store = ds.getDataRefAs<DataArray<T>>(arrPath).getDataStoreRef();
  REQUIRE(store.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    if(store[i] != expected[i])
    {
      UNSCOPED_INFO(fmt::format("Mismatch at index {}: expected {}, got {}", i, expected[i], store[i]));
      REQUIRE(store[i] == expected[i]);
    }
  }
}
} // namespace

TEST_CASE("ImageProcessing::ReadImageFilter: NRRD scalar 3D round trip (raw + gzip)", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();

  const std::array<usize, 3> dims = {4, 3, 2}; // X,Y,Z
  const usize numVoxels = dims[0] * dims[1] * dims[2];
  std::vector<int16_t> voxels(numVoxels);
  for(usize i = 0; i < numVoxels; i++)
  {
    voxels[i] = static_cast<int16_t>(i) - 5;
  }
  const std::vector<uint8_t> raw = ToBytes(voxels);

  const DataPath geomPath({"NRRD Image"});
  const std::string amName = "Cell Data";
  const std::string arrName = "ImageData";
  const DataPath arrPath = geomPath.createChildPath(amName).createChildPath(arrName);

  for(bool gzip : {false, true})
  {
    DYNAMIC_SECTION("encoding=" << (gzip ? "gzip" : "raw"))
    {
      const fs::path p = ReadImageOutputDir() / (gzip ? "rt_scalar.gzip.nrrd" : "rt_scalar.raw.nrrd");
      WriteNrrdRaw(p,
                   {"NRRD0004", "type: short", "dimension: 3", "space: left-posterior-superior", "sizes: 4 3 2", "space directions: (0.5,0,0) (0,0.75,0) (0,0,1.25)", "kinds: domain domain domain",
                    "endian: little", gzip ? "encoding: gzip" : "encoding: raw", "space origin: (-1,2,3.5)"},
                   gzip ? GzipCompress(raw) : raw);

      DataStructure ds;
      ReadImageFilter filter;
      const Arguments args = MakeReadImageNrrdArgs(p, geomPath, amName, arrName);
      const auto preflight = filter.preflight(ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
      const auto execute = filter.execute(ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(execute.result);

      const auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
      REQUIRE(geom.getDimensions()[0] == 4);
      REQUIRE(geom.getDimensions()[1] == 3);
      REQUIRE(geom.getDimensions()[2] == 2);
      REQUIRE(geom.getSpacing()[0] == Approx(0.5f));
      REQUIRE(geom.getOrigin()[2] == Approx(3.5f));
      RequireNrrdArrayEquals<int16>(ds, arrPath, voxels);
      UnitTest::CheckArraysInheritTupleDims(ds);
    }
  }
}

TEST_CASE("ImageProcessing::ReadImageFilter: NRRD big-endian uint16 round trip", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();
  const std::array<usize, 3> dims = {3, 3, 2};
  const usize n = dims[0] * dims[1] * dims[2];
  std::vector<uint16_t> voxels(n);
  for(usize i = 0; i < n; i++)
  {
    voxels[i] = static_cast<uint16_t>(0x0100 + i);
  }
  // Byte-swap the payload so the file is big-endian.
  std::vector<uint16_t> swapped = voxels;
  for(auto& v : swapped)
  {
    v = nx::core::byteswap(v);
  }
  const fs::path p = ReadImageOutputDir() / "rt_be_uint16.nrrd";
  WriteNrrdRaw(p, {"NRRD0004", "type: ushort", "dimension: 3", "sizes: 3 3 2", "kinds: domain domain domain", "endian: big", "encoding: raw"}, ToBytes(swapped));

  const DataPath geomPath({"NRRD BE"});
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath("ImageData");
  DataStructure ds;
  ReadImageFilter filter;
  const Arguments args = MakeReadImageNrrdArgs(p, geomPath, "Cell Data", "ImageData");
  // Bind results to locals: SIMPLNX_RESULT_REQUIRE_VALID evaluates its argument
  // multiple times, and preflight()/execute() mutate `ds` (they apply their
  // create-actions), so passing the calls inline would re-run them.
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  RequireNrrdArrayEquals<uint16>(ds, arrPath, voxels);
}

TEST_CASE("ImageProcessing::ReadImageFilter: NRRD RGB (uint8 x3) round trip", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();
  const usize X = 4, Y = 3; // 2D image, 3 components
  std::vector<uint8_t> voxels(X * Y * 3);
  for(usize i = 0; i < voxels.size(); i++)
  {
    voxels[i] = static_cast<uint8_t>(i);
  }
  const fs::path p = ReadImageOutputDir() / "rt_rgb.nrrd";
  WriteNrrdRaw(p,
               {"NRRD0004", "type: unsigned char", "dimension: 3", "space dimension: 2", "sizes: 3 4 3", "space directions: none (1,0) (0,1)", "kinds: vector domain domain", "encoding: raw",
                "space origin: (0,0)"},
               voxels);

  const DataPath geomPath({"NRRD RGB"});
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath("ImageData");
  DataStructure ds;
  ReadImageFilter filter;
  const Arguments args = MakeReadImageNrrdArgs(p, geomPath, "Cell Data", "ImageData");
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto& arr = ds.getDataRefAs<DataArray<uint8>>(arrPath);
  REQUIRE(arr.getNumberOfComponents() == 3);
  const auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
  REQUIRE(geom.getDimensions()[0] == 4);
  REQUIRE(geom.getDimensions()[1] == 3);
  REQUIRE(geom.getDimensions()[2] == 1);
  RequireNrrdArrayEquals<uint8>(ds, arrPath, voxels);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("ImageProcessing::ReadImageFilter: NRRD permuted directions emits warning -35750", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();
  const fs::path p = ReadImageOutputDir() / "rt_permuted.nrrd";
  WriteNrrdRaw(p,
               {"NRRD0004", "type: unsigned char", "dimension: 3", "space: left-posterior-superior", "sizes: 2 2 2", "space directions: (0,-2,0) (0,0,-2) (2.5,0,0)", "kinds: domain domain domain",
                "encoding: raw", "space origin: (0,0,0)"},
               std::vector<uint8_t>(8, 0));
  DataStructure ds;
  ReadImageFilter filter;
  const Arguments args = MakeReadImageNrrdArgs(p, DataPath({"NRRD Perm"}), "Cell Data", "ImageData");
  const auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  bool found = false;
  for(const auto& w : preflight.outputActions.warnings())
  {
    if(w.code == -35750)
    {
      found = true;
    }
  }
  REQUIRE(found);
}

TEST_CASE("ImageProcessing::ReadImageFilter: NRRD VoxelSubvolume crop", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();
  const usize X = 6, Y = 5, Z = 4;
  std::vector<uint16_t> voxels(X * Y * Z);
  for(usize i = 0; i < voxels.size(); i++)
  {
    voxels[i] = static_cast<uint16_t>(i);
  }
  const fs::path p = ReadImageOutputDir() / "rt_crop.nrrd";
  WriteNrrdRaw(p, {"NRRD0004", "type: ushort", "dimension: 3", "sizes: 6 5 4", "kinds: domain domain domain", "endian: little", "encoding: raw"}, ToBytes(voxels));

  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  crop.cropX = crop.cropY = crop.cropZ = true;
  crop.xBoundVoxels = {1, 3};
  crop.yBoundVoxels = {1, 3};
  crop.zBoundVoxels = {1, 2};

  const DataPath geomPath({"NRRD Crop"});
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath("ImageData");
  DataStructure ds;
  ReadImageFilter filter;
  const Arguments args = MakeReadImageNrrdArgs(p, geomPath, "Cell Data", "ImageData", crop);
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
  REQUIRE(geom.getDimensions()[0] == 3);
  REQUIRE(geom.getDimensions()[1] == 3);
  REQUIRE(geom.getDimensions()[2] == 2);
  std::vector<uint16_t> expected;
  for(usize dz = 0; dz < 2; dz++)
  {
    for(usize dy = 0; dy < 3; dy++)
    {
      for(usize dx = 0; dx < 3; dx++)
      {
        const usize src = (1 + dz) * Y * X + (1 + dy) * X + (1 + dx);
        expected.push_back(static_cast<uint16_t>(src));
      }
    }
  }
  RequireNrrdArrayEquals<uint16>(ds, arrPath, expected);
}

// PhysicalSubvolume crop path. This is the one place preflight (delegating to
// CropImageGeometryFilter with physical bounds) and execute (ComputeCropBounds ->
// ImageGeom::getIndex) map the physical box to voxel indices INDEPENDENTLY, so a
// 1-voxel disagreement would silently under-fill the array or trip CopyData -2034.
// Non-unit spacing + non-zero origin exercise that mapping; the chosen bounds land
// mid-voxel so both the float64 (preflight) and float32 (execute) floors agree.
TEST_CASE("ImageProcessing::ReadImageFilter: NRRD PhysicalSubvolume crop", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize X = 6, Y = 5, Z = 4;
  std::vector<uint16_t> voxels(X * Y * Z);
  std::iota(voxels.begin(), voxels.end(), static_cast<uint16_t>(0)); // value == flat file position (X-fastest)

  // spacing {2,3,4}, origin {10,20,30} via axis-aligned space directions + space origin.
  const fs::path p = ReadImageOutputDir() / "rt_crop_physical.nrrd";
  WriteNrrdRaw(p,
               {"NRRD0004", "type: ushort", "dimension: 3", "space: left-posterior-superior", "sizes: 6 5 4", "space directions: (2,0,0) (0,3,0) (0,0,4)", "kinds: domain domain domain",
                "endian: little", "encoding: raw", "space origin: (10,20,30)"},
               ToBytes(voxels));

  // Physical box chosen so each bound floors to a voxel center-region:
  //   X: [13,17] -> voxels floor((13-10)/2)=1 .. floor((17-10)/2)=3
  //   Y: [24.5,30.5] -> floor((24.5-20)/3)=1 .. floor((30.5-20)/3)=3
  //   Z: [36,40] -> floor((36-30)/4)=1 .. floor((40-30)/4)=2
  // => voxel subrange x:1..3, y:1..3, z:1..2, i.e. dims (3,3,2).
  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
  crop.cropX = crop.cropY = crop.cropZ = true;
  crop.xBoundPhysical = {13.0f, 17.0f};
  crop.yBoundPhysical = {24.5f, 30.5f};
  crop.zBoundPhysical = {36.0f, 40.0f};

  const DataPath geomPath({"NRRD PhysCrop"});
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath("ImageData");
  DataStructure ds;
  ReadImageFilter filter;
  const Arguments args = MakeReadImageNrrdArgs(p, geomPath, "Cell Data", "ImageData", crop);
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
  REQUIRE(geom.getDimensions()[0] == 3);
  REQUIRE(geom.getDimensions()[1] == 3);
  REQUIRE(geom.getDimensions()[2] == 2);
  // Cropped origin = min-voxel corner: xMin*sp+origin = {12,23,34}.
  REQUIRE(geom.getOrigin()[0] == Approx(12.0f));
  REQUIRE(geom.getOrigin()[1] == Approx(23.0f));
  REQUIRE(geom.getOrigin()[2] == Approx(34.0f));

  std::vector<uint16_t> expected;
  for(usize dz = 1; dz <= 2; dz++)
  {
    for(usize dy = 1; dy <= 3; dy++)
    {
      for(usize dx = 1; dx <= 3; dx++)
      {
        expected.push_back(static_cast<uint16_t>((dz * Y + dy) * X + dx));
      }
    }
  }
  RequireNrrdArrayEquals<uint16>(ds, arrPath, expected);
}

// Regression for the physical-crop preflight/execute divergence: an out-of-range physical
// box green-preflights (CropImageGeometryFilter clamps the bounds into the volume with a
// warning) but the old execute path hard-errored (ImageGeom::getIndex returns nullopt for
// an OOB coordinate). Execute now clamps identically, so preflight and execute agree.
TEST_CASE("ImageProcessing::ReadImageFilter: NRRD PhysicalSubvolume crop clamps out-of-range bounds", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize X = 6, Y = 5, Z = 4;
  std::vector<uint16_t> voxels(X * Y * Z);
  std::iota(voxels.begin(), voxels.end(), static_cast<uint16_t>(0)); // value == flat file position (X-fastest)

  // spacing {2,3,4}, origin {10,20,30}; far corner = origin + dims*spacing = {22,35,46}.
  // LPS keeps the origin unflipped so this test isolates the crop-clamp behavior.
  const fs::path p = ReadImageOutputDir() / "rt_crop_physical_oob.nrrd";
  WriteNrrdRaw(p,
               {"NRRD0004", "type: ushort", "dimension: 3", "space: left-posterior-superior", "sizes: 6 5 4", "space directions: (2,0,0) (0,3,0) (0,0,4)", "kinds: domain domain domain",
                "endian: little", "encoding: raw", "space origin: (10,20,30)"},
               ToBytes(voxels));

  // Bounds extend far below the origin and far above the far corner on every axis, so both
  // ends clamp to the full extent -> the whole 6x5x4 volume is read.
  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
  crop.cropX = crop.cropY = crop.cropZ = true;
  crop.xBoundPhysical = {-100.0f, 1000.0f};
  crop.yBoundPhysical = {-100.0f, 1000.0f};
  crop.zBoundPhysical = {-100.0f, 1000.0f};

  const DataPath geomPath({"NRRD PhysCropOOB"});
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath("ImageData");
  DataStructure ds;
  ReadImageFilter filter;
  const Arguments args = MakeReadImageNrrdArgs(p, geomPath, "Cell Data", "ImageData", crop);
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
  REQUIRE(geom.getDimensions()[0] == X);
  REQUIRE(geom.getDimensions()[1] == Y);
  REQUIRE(geom.getDimensions()[2] == Z);
  RequireNrrdArrayEquals<uint16>(ds, arrPath, voxels);
}

TEST_CASE("ImageProcessing::ReadImageFilter: NRRD rejects missing file and bad encoding", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();
  SECTION("missing file")
  {
    DataStructure ds;
    ReadImageFilter filter;
    const Arguments args = MakeReadImageNrrdArgs(ReadImageOutputDir() / "does_not_exist_987.nrrd", DataPath({"X"}), "Cell Data", "ImageData");
    REQUIRE(filter.preflight(ds, args).outputActions.invalid());
  }
  SECTION("ascii encoding")
  {
    const fs::path p = ReadImageOutputDir() / "bad_ascii.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 2", "sizes: 2 2", "encoding: ascii"}, {});
    DataStructure ds;
    ReadImageFilter filter;
    const Arguments args = MakeReadImageNrrdArgs(p, DataPath({"X"}), "Cell Data", "ImageData");
    REQUIRE(filter.preflight(ds, args).outputActions.invalid());
  }
}

// -----------------------------------------------------------------------------
// Live-ITK read-parity gate. Reads each real ITK regression .nrrd with BOTH
// ReadImageFilter (NRRD backend) AND the runtime-loaded ITKImageReaderFilter
// (fetched from the FilterList by UUID so no ITK header/link is pulled into
// ImageProcessing) and asserts identical geometry (dims/origin/spacing/type) +
// byte-identical pixel array. Self-skips with a WARN when the ITK plugin is not
// loaded so ITK-free ImageProcessing builds still compile and pass.
// -----------------------------------------------------------------------------
namespace
{
// The real ITK regression .nrrd files are committed under the ITKImageProcessing
// plugin source tree.
fs::path ItkJsonFiltersDir()
{
  return fs::path(std::string(nx::core::unit_test::k_SimplnxSourceDIr.view())) / "src" / "Plugins" / "ImageProcessing" / "data" / "ItkGolden";
}

// ITKImageReaderFilter param-key STRINGS (hard-coded to avoid linking the ITK plugin).
constexpr StringLiteral k_ITK_FileName = "file_name";
constexpr StringLiteral k_ITK_GeometryPath = "output_geometry_path";
constexpr StringLiteral k_ITK_ArrayName = "image_data_array_name";
constexpr StringLiteral k_ITK_CellDataName = "cell_attribute_matrix_name";
constexpr StringLiteral k_ITK_ChangeOrigin = "change_origin";
constexpr StringLiteral k_ITK_ChangeSpacing = "change_spacing";

template <class T>
void RequireStoresEqual(const IDataArray& a, const IDataArray& b)
{
  const auto& sa = dynamic_cast<const DataArray<T>&>(a).getDataStoreRef();
  const auto& sb = dynamic_cast<const DataArray<T>&>(b).getDataStoreRef();
  REQUIRE(sa.getSize() == sb.getSize());
  for(usize i = 0; i < sa.getSize(); i++)
  {
    if(sa[i] != sb[i])
    {
      UNSCOPED_INFO(fmt::format("byte/element mismatch at {}: ours={}, itk={}", i, sa[i], sb[i]));
      REQUIRE(sa[i] == sb[i]);
    }
  }
}

void RequireArraysIdentical(const IDataArray& ours, const IDataArray& itk)
{
  REQUIRE(ours.getDataType() == itk.getDataType());
  REQUIRE(ours.getNumberOfComponents() == itk.getNumberOfComponents());
  switch(ours.getDataType())
  {
  case DataType::int8:
    RequireStoresEqual<int8>(ours, itk);
    break;
  case DataType::uint8:
    RequireStoresEqual<uint8>(ours, itk);
    break;
  case DataType::int16:
    RequireStoresEqual<int16>(ours, itk);
    break;
  case DataType::uint16:
    RequireStoresEqual<uint16>(ours, itk);
    break;
  case DataType::int32:
    RequireStoresEqual<int32>(ours, itk);
    break;
  case DataType::uint32:
    RequireStoresEqual<uint32>(ours, itk);
    break;
  case DataType::int64:
    RequireStoresEqual<int64>(ours, itk);
    break;
  case DataType::uint64:
    RequireStoresEqual<uint64>(ours, itk);
    break;
  case DataType::float32:
    RequireStoresEqual<float32>(ours, itk);
    break;
  case DataType::float64:
    RequireStoresEqual<float64>(ours, itk);
    break;
  default:
    FAIL("unexpected DataType");
  }
}
} // namespace

// -----------------------------------------------------------------------------
// Multi-buffer gzip inflate refill. Reads a gzip-encoded NRRD whose *compressed*
// data segment exceeds nrrd::k_GzReadBufferSize (the 4 MiB compressed-input
// buffer), which forces NrrdDataReader::readBytes to refill that buffer from the
// stream at least once mid-inflate. A high-entropy / incompressible payload keeps
// the gzip stream larger than the buffer -- the exact condition that drives the
// refill branch, which the smaller round-trip tests never reach.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ReadImageFilter: NRRD multi-buffer gzip inflate refill", "[ImageProcessing][ReadImageFilter]")
{
  UnitTest::LoadPlugins();

  // 128^3 int32 == 8 MiB decompressed. A splitmix64-mixed fill is effectively
  // incompressible, so the gzip stream stays above the 4 MiB compressed buffer.
  const usize X = 128, Y = 128, Z = 128;
  std::vector<int32_t> voxels(X * Y * Z);
  uint64_t state = 0x9E3779B97F4A7C15ULL;
  for(usize i = 0; i < voxels.size(); i++)
  {
    state += 0x9E3779B97F4A7C15ULL;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);
    voxels[i] = static_cast<int32_t>(static_cast<uint32_t>(z));
  }

  const std::vector<uint8_t> raw = ToBytes(voxels);
  const std::vector<uint8_t> compressed = GzipCompress(raw);

  // Decompressed segment exceeds 4 MiB...
  REQUIRE(raw.size() > nrrd::k_GzReadBufferSize);
  // ...and, crucially, so does the *compressed* segment, so readBytes must refill
  // its 4 MiB compressed-input buffer at least once while inflating.
  REQUIRE(compressed.size() > nrrd::k_GzReadBufferSize);

  const fs::path p = ReadImageOutputDir() / "multibuf_gzip.nrrd";
  WriteNrrdRaw(p, {"NRRD0004", "type: int", "dimension: 3", "sizes: 128 128 128", "kinds: domain domain domain", "endian: little", "encoding: gzip"}, compressed);

  const DataPath geomPath({"NRRD MultiBuf"});
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath("ImageData");
  DataStructure ds;
  ReadImageFilter filter;
  const Arguments args = MakeReadImageNrrdArgs(p, geomPath, "Cell Data", "ImageData");
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  RequireNrrdArrayEquals<int32>(ds, arrPath, voxels);
}

// =============================================================================
// 3D multi-page TIFF: live-ITK read parity + out-of-core byte-match.
//
// ReadImage reads a multi-page TIFF as a 3D Z-stack (Z == subfile-aware page
// count) through its raster slice-streaming backend. The legacy ITK reader
// (ITKImageReader, UUID d72eaf98-...) reads the same file as a 3D volume too, so
// it is a valid parity oracle -- the Z==3 assertion below confirms that. These
// mirror the NRRD [itk-parity] + OOC idioms above; the input is synthesized at
// runtime (no committed binary). The OOC byte-match extends the NRRD OOC proof to
// the raster backend, showing its bulk-copy path is byte-exact on a disk-backed
// store.
// =============================================================================
namespace
{
// Writes an N-page TIFF of element type T with `numComponents` samples per pixel (contiguous /
// interleaved). Generalizes WriteMultiPageTiff (which is uint8, 1-component) to uint16 and
// multi-component (RGB) stacks so the 3D parity + OOC tests can exercise those layouts.
// pagePixels[p] is row-major (top-to-bottom), X-fastest, component-interleaved;
// size == width*height*numComponents.
template <typename T>
void WriteMultiPageTiffTyped(const fs::path& path, uint32_t width, uint32_t height, uint16_t numComponents, const std::vector<std::vector<T>>& pagePixels)
{
  static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t>, "WriteMultiPageTiffTyped supports only uint8/uint16 sample types");
  const uint16_t bitsPerSample = static_cast<uint16_t>(sizeof(T) * 8);
  const uint16_t photometric = (numComponents == 1) ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB;
  TIFF* tif = TIFFOpen(path.string().c_str(), "w");
  REQUIRE(tif != nullptr);
  for(const auto& pixels : pagePixels)
  {
    REQUIRE(pixels.size() == static_cast<size_t>(width) * height * numComponents);
    REQUIRE(TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, numComponents) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bitsPerSample) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric) != 0);
    REQUIRE(TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0)) != 0);
    for(uint32_t row = 0; row < height; ++row)
    {
      // TIFFWriteScanline takes a non-const void* but does not modify the buffer.
      REQUIRE(TIFFWriteScanline(tif, const_cast<T*>(pixels.data() + static_cast<size_t>(row) * width * numComponents), row, 0) >= 0);
    }
    REQUIRE(TIFFWriteDirectory(tif) != 0);
  }
  TIFFClose(tif);
}
} // namespace
