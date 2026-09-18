#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ReadImageFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <tiffio.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <system_error>
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

TEST_CASE("SimplnxCore::ReadImageFilter: Tiled_Tiff_Photometric_Orientation", "[SimplnxCore][ReadImageFilter]")
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

TEST_CASE("SimplnxCore::ReadImageFilter: Read_Basic", "[SimplnxCore][ReadImageFilter]")
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

TEST_CASE("SimplnxCore::ReadImageFilter: Override_Origin", "[SimplnxCore][ReadImageFilter]")
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

TEST_CASE("SimplnxCore::ReadImageFilter: Centering_Origin", "[SimplnxCore][ReadImageFilter]")
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

TEST_CASE("SimplnxCore::ReadImageFilter: Override_Spacing", "[SimplnxCore][ReadImageFilter]")
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

TEST_CASE("SimplnxCore::ReadImageFilter: OriginSpacing_Preprocessed", "[SimplnxCore][ReadImageFilter]")
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

TEST_CASE("SimplnxCore::ReadImageFilter: OriginSpacing_Postprocessed", "[SimplnxCore][ReadImageFilter]")
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

TEST_CASE("SimplnxCore::ReadImageFilter: DataType_Conversion", "[SimplnxCore][ReadImageFilter]")
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

TEST_CASE("SimplnxCore::ReadImageFilter: Interaction_Crop_DataType", "[SimplnxCore][ReadImageFilter]")
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

TEST_CASE("SimplnxCore::ReadImageFilter: Interaction_All", "[SimplnxCore][ReadImageFilter]")
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
