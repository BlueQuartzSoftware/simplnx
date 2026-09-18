#include <catch2/catch.hpp>

#include "ImageProcessing/Filters/Algorithms/ReadMhaFile.hpp"
#include "ImageProcessing/Filters/ReadMhaFileFilter.hpp"
#include "ImageProcessing/utils/MetaImageUtilities.hpp"

#include "ImageProcessing/ImageProcessing_test_dirs.hpp"

#include "simplnx/Common/Bit.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <fmt/format.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <vector>

namespace fs = std::filesystem;

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
fs::path MhaOutputDir()
{
  fs::path dir = fs::path(std::string(nx::core::unit_test::k_BinaryTestOutputDir.view())) / "ReadMhaFile";
  fs::create_directories(dir);
  return dir;
}

// Writes an attached MetaImage: ASCII "Key = Value" header lines, then the
// binary data appended immediately after the ElementDataFile line's newline.
// `headerLines` must already end with an "ElementDataFile = LOCAL" line.
void WriteMhaAttached(const fs::path& path, const std::vector<std::string>& headerLines, const std::vector<uint8_t>& data)
{
  std::ofstream ofs(path, std::ios::binary);
  REQUIRE(ofs.is_open());
  for(const auto& line : headerLines)
  {
    ofs << line << "\n";
  }
  if(!data.empty())
  {
    ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
  }
}

template <typename T>
std::vector<uint8_t> ToBytes(const std::vector<T>& values)
{
  std::vector<uint8_t> out(values.size() * sizeof(T));
  std::memcpy(out.data(), values.data(), out.size());
  return out;
}

Arguments MakeMhaArgs(const fs::path& inputFile, const DataPath& geomPath, const std::string& amName, const std::string& arrName,
                      const CropGeometryParameter::ValueType& crop = CropGeometryParameter::ValueType{})
{
  Arguments args;
  args.insertOrAssign(ReadMhaFileFilter::k_InputFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(inputFile));
  args.insertOrAssign(ReadMhaFileFilter::k_CroppingOptions_Key, std::make_any<CropGeometryParameter::ValueType>(crop));
  args.insertOrAssign(ReadMhaFileFilter::k_CreatedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(ReadMhaFileFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>(amName));
  args.insertOrAssign(ReadMhaFileFilter::k_ImageDataArrayName_Key, std::make_any<std::string>(arrName));
  args.insertOrAssign(ReadMhaFileFilter::k_ApplyImageTransformation_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadMhaFileFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL));
  args.insertOrAssign(ReadMhaFileFilter::k_TransposeTransformMatrix_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadMhaFileFilter::k_SaveImageTransformation_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadMhaFileFilter::k_TransformationMatrixPath_Key, std::make_any<ArrayCreationParameter::ValueType>(geomPath.createChildPath("TransformationMatrix")));
  return args;
}

template <typename T>
void RequireMhaArrayEquals(const DataStructure& ds, const DataPath& arrPath, const std::vector<T>& expected)
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

TEST_CASE("ImageProcessing::MetaImageUtilities: ElementType string -> DataType map", "[ImageProcessing][ReadMhaFileFilter]")
{
  using mhd::MetTypeToSimplnx;
  REQUIRE(MetTypeToSimplnx("MET_CHAR").value().first == DataType::int8);
  REQUIRE(MetTypeToSimplnx("MET_UCHAR").value().first == DataType::uint8);
  REQUIRE(MetTypeToSimplnx("MET_SHORT").value().first == DataType::int16);
  REQUIRE(MetTypeToSimplnx("MET_USHORT").value().first == DataType::uint16);
  REQUIRE(MetTypeToSimplnx("MET_INT").value().first == DataType::int32);
  REQUIRE(MetTypeToSimplnx("MET_UINT").value().first == DataType::uint32);
  REQUIRE(MetTypeToSimplnx("MET_LONG").value().first == DataType::int32); // MetaIO MET_LONG == 4 bytes
  REQUIRE(MetTypeToSimplnx("MET_LONG").value().second == 4);
  REQUIRE(MetTypeToSimplnx("MET_ULONG").value().first == DataType::uint32);
  REQUIRE(MetTypeToSimplnx("MET_LONG_LONG").value().first == DataType::int64);
  REQUIRE(MetTypeToSimplnx("MET_ULONG_LONG").value().first == DataType::uint64);
  REQUIRE(MetTypeToSimplnx("MET_FLOAT").value().first == DataType::float32);
  REQUIRE(MetTypeToSimplnx("MET_DOUBLE").value().first == DataType::float64);
  REQUIRE(MetTypeToSimplnx("MET_DOUBLE").value().second == 8);
  REQUIRE_FALSE(MetTypeToSimplnx("MET_STRING").has_value());
  REQUIRE_FALSE(MetTypeToSimplnx("nonsense").has_value());
}

TEST_CASE("ImageProcessing::MetaImageUtilities: parse scalar 2D header (compressed)", "[ImageProcessing][ReadMhaFileFilter]")
{
  const fs::path p = MhaOutputDir() / "hdr_scalar2d.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "BinaryDataByteOrderMSB = False", "CompressedData = True", "TransformMatrix = 1 0 0 1", "Offset = -1 2",
                    "CenterOfRotation = 0 0", "ElementSpacing = 0.5 0.75", "DimSize = 4 3", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"},
                   {});

  const auto result = mhd::ReadMetaImageHeader(p);
  REQUIRE(result.valid());
  const auto md = result.value();
  REQUIRE(md.nDims == 2);
  REQUIRE(md.dataType == DataType::uint8);
  REQUIRE(md.componentCount == 1);
  REQUIRE(md.dimensions == std::array<usize, 3>{4, 3, 1}); // Z=1 for 2D
  REQUIRE(md.spacing[0] == Approx(0.5f));
  REQUIRE(md.spacing[1] == Approx(0.75f));
  REQUIRE(md.origin[0] == Approx(-1.0f));
  REQUIRE(md.origin[1] == Approx(2.0f));
  REQUIRE(md.encoding == mhd::Encoding::Compressed);
  REQUIRE(md.byteSwapRequired == false); // little-endian on a little-endian host
  REQUIRE(md.detached == false);
  REQUIRE(md.dataStartOffset > 0);
  REQUIRE(md.transformMatrix == std::vector<float64>{1, 0, 0, 1});
}

TEST_CASE("ImageProcessing::MetaImageUtilities: parse 3D multi-component raw header", "[ImageProcessing][ReadMhaFileFilter]")
{
  const fs::path p = MhaOutputDir() / "hdr_3d_multicomp.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 3", "BinaryData = True", "CompressedData = False", "ElementNumberOfChannels = 2", "ElementSpacing = 1 2 3", "Offset = 0 0 0", "DimSize = 5 4 3",
                    "ElementType = MET_FLOAT", "ElementDataFile = LOCAL"},
                   {});
  const auto result = mhd::ReadMetaImageHeader(p);
  REQUIRE(result.valid());
  const auto md = result.value();
  REQUIRE(md.nDims == 3);
  REQUIRE(md.dimensions == std::array<usize, 3>{5, 4, 3});
  REQUIRE(md.componentCount == 2);
  REQUIRE(md.dataType == DataType::float32);
  REQUIRE(md.spacing[2] == Approx(3.0f));
  REQUIRE(md.encoding == mhd::Encoding::Raw);
}

TEST_CASE("ImageProcessing::MetaImageUtilities: big-endian flag sets byteSwapRequired", "[ImageProcessing][ReadMhaFileFilter]")
{
  const fs::path p = MhaOutputDir() / "hdr_bigendian.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "BinaryDataByteOrderMSB = True", "CompressedData = False", "ElementSpacing = 1 1", "DimSize = 2 2",
                    "ElementType = MET_SHORT", "ElementDataFile = LOCAL"},
                   {});
  const auto result = mhd::ReadMetaImageHeader(p);
  REQUIRE(result.valid());
  const auto md = result.value();
  // On the (little-endian) CI/dev host, a big-endian file requires a swap.
  const bool hostIsLittle = (nx::core::checkEndian() == nx::core::endian::little);
  REQUIRE(md.byteSwapRequired == hostIsLittle);
}

TEST_CASE("ImageProcessing::MetaImageUtilities: detached .mhd resolves sibling .raw", "[ImageProcessing][ReadMhaFileFilter]")
{
  // Create the sibling raw file, then a .mhd that points at it by name.
  const fs::path raw = MhaOutputDir() / "vol_detached.raw";
  {
    std::ofstream ofs(raw, std::ios::binary);
    REQUIRE(ofs.is_open());
    const std::vector<uint8_t> bytes(2 * 2, 0);
    ofs.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  const fs::path mhd = MhaOutputDir() / "vol_detached.mhd";
  {
    std::ofstream ofs(mhd, std::ios::binary);
    REQUIRE(ofs.is_open());
    for(const char* line :
        {"ObjectType = Image", "NDims = 2", "BinaryData = True", "CompressedData = False", "ElementSpacing = 1 1", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = vol_detached.raw"})
    {
      ofs << line << "\n";
    }
  }
  const auto result = mhd::ReadMetaImageHeader(mhd);
  REQUIRE(result.valid());
  const auto md = result.value();
  REQUIRE(md.detached == true);
  REQUIRE(md.dataStartOffset == 0);
  REQUIRE(md.dataFilePath.filename() == fs::path("vol_detached.raw"));
}

TEST_CASE("ImageProcessing::MetaImageUtilities: rejects bad headers", "[ImageProcessing][ReadMhaFileFilter]")
{
  SECTION("unsupported ElementType")
  {
    const fs::path p = MhaOutputDir() / "hdr_bad_type.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "DimSize = 2 2", "ElementType = MET_STRING", "ElementDataFile = LOCAL"}, {});
    REQUIRE(mhd::ReadMetaImageHeader(p).invalid());
  }
  SECTION("NDims not 2 or 3")
  {
    const fs::path p = MhaOutputDir() / "hdr_4d.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 4", "DimSize = 2 2 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    REQUIRE(mhd::ReadMetaImageHeader(p).invalid());
  }
  SECTION("DimSize count != NDims")
  {
    const fs::path p = MhaOutputDir() / "hdr_dim_mismatch.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 3", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    REQUIRE(mhd::ReadMetaImageHeader(p).invalid());
  }
  SECTION("ObjectType not Image")
  {
    const fs::path p = MhaOutputDir() / "hdr_not_image.mha";
    WriteMhaAttached(p, {"ObjectType = Tube", "NDims = 2", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    REQUIRE(mhd::ReadMetaImageHeader(p).invalid());
  }
  SECTION("BinaryData = False (ASCII data)")
  {
    const fs::path p = MhaOutputDir() / "hdr_ascii.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "BinaryData = False", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    REQUIRE(mhd::ReadMetaImageHeader(p).invalid());
  }
  SECTION("no ElementDataFile tag")
  {
    const fs::path p = MhaOutputDir() / "hdr_no_datafile.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "DimSize = 2 2", "ElementType = MET_UCHAR"}, {});
    REQUIRE(mhd::ReadMetaImageHeader(p).invalid());
  }
}

TEST_CASE("ImageProcessing::MetaImageUtilities: rejects nonzero HeaderSize", "[ImageProcessing][ReadMhaFileFilter]")
{
  // MetaIO honors HeaderSize (metaImage.cxx ~2352/2367); this reader cannot, so a nonzero
  // value is rejected rather than silently dropped (which would read the wrong voxels).
  SECTION("HeaderSize = 8 -> -35814")
  {
    const fs::path p = MhaOutputDir() / "hdr_headersize8.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "BinaryData = True", "HeaderSize = 8", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35814);
  }
  SECTION("HeaderSize = -1 (seek-from-end sentinel) -> -35814")
  {
    const fs::path p = MhaOutputDir() / "hdr_headersize_neg1.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "BinaryData = True", "HeaderSize = -1", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35814);
  }
  SECTION("HeaderSize = 0 is a no-op and accepted")
  {
    const fs::path p = MhaOutputDir() / "hdr_headersize0.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "BinaryData = True", "HeaderSize = 0", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"},
                     std::vector<uint8_t>(2 * 2, 0));
    REQUIRE(mhd::ReadMetaImageHeader(p).valid());
  }
}

TEST_CASE("ImageProcessing::MetaImageUtilities: numeric booleans and multi-alias precedence", "[ImageProcessing][ReadMhaFileFilter]")
{
  const bool hostIsLittle = (nx::core::checkEndian() == nx::core::endian::little);

  SECTION("CompressedData = 1 (numeric) is treated as compressed")
  {
    // MetaIO decides booleans on the first char only, so "1" == true. The old whole-string
    // "== true" check mis-read this as raw.
    const fs::path p = MhaOutputDir() / "hdr_numeric_compressed.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "BinaryData = True", "CompressedData = 1", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.valid());
    REQUIRE(result.value().encoding == mhd::Encoding::Compressed);
  }
  SECTION("BinaryDataByteOrderMSB = T (numeric-form true) sets the byteswap flag")
  {
    const fs::path p = MhaOutputDir() / "hdr_msb_t.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "BinaryData = True", "BinaryDataByteOrderMSB = T", "DimSize = 2 2", "ElementType = MET_SHORT", "ElementDataFile = LOCAL"}, {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.valid());
    REQUIRE(result.value().byteSwapRequired == hostIsLittle); // big-endian file on a little-endian host
  }
  SECTION("origin precedence: Origin wins over Offset and Position regardless of file order")
  {
    // MetaIO reads Position, then Offset, then Origin into m_Offset (last wins).
    const fs::path p = MhaOutputDir() / "hdr_origin_precedence.mha";
    WriteMhaAttached(
        p, {"ObjectType = Image", "NDims = 2", "BinaryData = True", "Origin = 3 3", "Offset = 2 2", "Position = 1 1", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.valid());
    REQUIRE(result.value().origin[0] == Approx(3.0f));
    REQUIRE(result.value().origin[1] == Approx(3.0f));
  }
  SECTION("origin precedence: ImagePosition wins over Origin")
  {
    const fs::path p = MhaOutputDir() / "hdr_imageposition.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "BinaryData = True", "Origin = 3 3", "ImagePosition = 7 7", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.valid());
    REQUIRE(result.value().origin[0] == Approx(7.0f));
    REQUIRE(result.value().origin[1] == Approx(7.0f));
  }
  SECTION("byte-order precedence: BinaryDataByteOrderMSB overrides ElementByteOrderMSB")
  {
    // ElementByteOrderMSB says big-endian, BinaryDataByteOrderMSB says little-endian; the
    // latter wins, so on any host no swap is required for a little-endian file.
    const fs::path p = MhaOutputDir() / "hdr_byteorder_precedence.mha";
    WriteMhaAttached(
        p,
        {"ObjectType = Image", "NDims = 2", "BinaryData = True", "ElementByteOrderMSB = True", "BinaryDataByteOrderMSB = False", "DimSize = 2 2", "ElementType = MET_SHORT", "ElementDataFile = LOCAL"},
        {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.valid());
    REQUIRE(result.value().byteSwapRequired == !hostIsLittle); // little-endian file wins
  }
  SECTION("transform precedence: TransformMatrix wins over Rotation and Orientation")
  {
    const fs::path p = MhaOutputDir() / "hdr_transform_precedence.mha";
    WriteMhaAttached(p,
                     {"ObjectType = Image", "NDims = 2", "BinaryData = True", "Orientation = 1 0 0 1", "Rotation = 0 1 1 0", "TransformMatrix = 2 0 0 2", "DimSize = 2 2", "ElementType = MET_UCHAR",
                      "ElementDataFile = LOCAL"},
                     {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.valid());
    REQUIRE(result.value().transformMatrix == std::vector<float64>{2, 0, 0, 2});
  }
}

TEST_CASE("ImageProcessing::MetaImageUtilities: ElementDataFile LIST vs single filename classification", "[ImageProcessing][ReadMhaFileFilter]")
{
  SECTION("a name whose first 4 chars are LIST is rejected (-35807)")
  {
    // MetaIO treats first-4-chars == "LIST" as a multi-file list (metaImage.cxx ~1230); a
    // literal "LISTfoo.raw" must be rejected, not mis-opened as a data file.
    const fs::path p = MhaOutputDir() / "hdr_list_prefix.mha";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "BinaryData = True", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LISTfoo.raw"}, {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35807);
  }
  SECTION("a single detached filename containing spaces is accepted, not misread as multi-file")
  {
    const fs::path raw = MhaOutputDir() / "my scan data.raw";
    {
      std::ofstream ofs(raw, std::ios::binary);
      REQUIRE(ofs.is_open());
      const std::vector<uint8_t> bytes(2 * 2, 0);
      ofs.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    const fs::path p = MhaOutputDir() / "hdr_spaced_filename.mhd";
    WriteMhaAttached(p, {"ObjectType = Image", "NDims = 2", "BinaryData = True", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = my scan data.raw"}, {});
    const auto result = mhd::ReadMetaImageHeader(p);
    REQUIRE(result.valid());
    REQUIRE(result.value().detached == true);
    REQUIRE(result.value().dataFilePath.filename() == fs::path("my scan data.raw"));
  }
}

namespace
{
// Compress @p data as a raw ZLIB stream (windowBits 15) -- the MetaImage
// CompressedData wire format. (The reader's 15+32 auto-detect also accepts gzip,
// but real .mha payloads are zlib, so tests exercise the zlib path.)
std::vector<uint8_t> ZlibCompress(const std::vector<uint8_t>& data)
{
  z_stream zs{};
  REQUIRE(deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) == Z_OK);
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
} // namespace

TEST_CASE("ImageProcessing::MetaImageDataReader: raw + zlib readBytes", "[ImageProcessing][ReadMhaFileFilter]")
{
  std::vector<int16_t> values(4 * 3 * 2);
  for(size_t i = 0; i < values.size(); i++)
  {
    values[i] = static_cast<int16_t>(i) - 5;
  }
  const std::vector<uint8_t> raw = ToBytes(values);

  for(bool compressed : {false, true})
  {
    DYNAMIC_SECTION("compressed=" << (compressed ? "true" : "false"))
    {
      const fs::path p = MhaOutputDir() / (compressed ? "reader_zlib.mha" : "reader_raw.mha");
      WriteMhaAttached(p,
                       {"ObjectType = Image", "NDims = 3", "BinaryData = True", "BinaryDataByteOrderMSB = False", compressed ? "CompressedData = True" : "CompressedData = False",
                        "ElementSpacing = 1 1 1", "DimSize = 4 3 2", "ElementType = MET_SHORT", "ElementDataFile = LOCAL"},
                       compressed ? ZlibCompress(raw) : raw);

      const auto mdResult = mhd::ReadMetaImageHeader(p);
      REQUIRE(mdResult.valid());
      auto readerResult = mhd::MetaImageDataReader::Create(mdResult.value());
      REQUIRE(readerResult.valid());
      auto& reader = *readerResult.value();

      std::vector<int16_t> readBack(values.size());
      const auto r = reader.readBytes(readBack.data(), readBack.size() * sizeof(int16_t));
      REQUIRE(r.valid());
      REQUIRE(readBack == values);

      // Reading past the end must fail with a short-read error.
      int16_t extra = 0;
      REQUIRE(reader.readBytes(&extra, sizeof(int16_t)).invalid());
    }
  }
}

namespace
{
const DataPath k_AlgoGeomPath({"ImageGeometry"});
const std::string k_AlgoCellDataName = "CellData";
const std::string k_AlgoArrayName = "ImageData";

DataPath AlgoImageDataPath()
{
  return k_AlgoGeomPath.createChildPath(k_AlgoCellDataName).createChildPath(k_AlgoArrayName);
}

// Mirrors what ReadMhaFileFilter::preflight creates: ImageGeom + cell AM + typed
// DataArray sized to destDims (X,Y,Z) tuples with `comp` components. The cell AM
// tuple dims are [Z, Y, X].
DataStructure BuildMhaTarget(DataType dataType, const std::array<usize, 3>& destDims, usize comp)
{
  DataStructure ds;
  const CreateImageGeometryAction::DimensionType dims = {destDims[0], destDims[1], destDims[2]};
  const CreateImageGeometryAction::OriginType origin = {0.0f, 0.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType spacing = {1.0f, 1.0f, 1.0f};
  auto geomAction = CreateImageGeometryAction(k_AlgoGeomPath, dims, origin, spacing, k_AlgoCellDataName);
  REQUIRE(geomAction.apply(ds, IDataAction::Mode::Execute).valid());
  const std::vector<usize> tupleDims = {destDims[2], destDims[1], destDims[0]}; // [Z, Y, X]
  auto arrayAction = CreateArrayAction(dataType, tupleDims, {comp}, AlgoImageDataPath());
  REQUIRE(arrayAction.apply(ds, IDataAction::Mode::Execute).valid());
  return ds;
}

Result<> RunReadMhaFile(DataStructure& ds, const fs::path& inputFile, const CropGeometryParameter::ValueType& crop = {})
{
  ReadMhaFileInputValues inputValues;
  inputValues.InputFilePath = inputFile;
  inputValues.ImageGeometryPath = k_AlgoGeomPath;
  inputValues.CellAttributeMatrixName = k_AlgoCellDataName;
  inputValues.ImageDataArrayName = k_AlgoArrayName;
  inputValues.CroppingOptions = crop;
  const IFilter::MessageHandler messageHandler{};
  const std::atomic_bool shouldCancel{false};
  ReadMhaFile algorithm(ds, messageHandler, shouldCancel, &inputValues);
  return algorithm();
}
} // namespace

TEST_CASE("ImageProcessing::ReadMhaFile: scalar 3D streams X-fastest [Z,Y,X]", "[ImageProcessing][ReadMhaFileFilter]")
{
  constexpr usize X = 4, Y = 3, Z = 2;
  std::vector<int16_t> fileValues(X * Y * Z);
  std::iota(fileValues.begin(), fileValues.end(), static_cast<int16_t>(-5));
  const fs::path p = MhaOutputDir() / "algo_scalar3d.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 3", "BinaryData = True", "BinaryDataByteOrderMSB = False", "CompressedData = False", "ElementSpacing = 1 1 1", "DimSize = 4 3 2",
                    "ElementType = MET_SHORT", "ElementDataFile = LOCAL"},
                   ToBytes(fileValues));

  DataStructure ds = BuildMhaTarget(DataType::int16, {X, Y, Z}, 1);
  REQUIRE(RunReadMhaFile(ds, p).valid());
  const auto& store = ds.getDataRefAs<DataArray<int16>>(AlgoImageDataPath()).getDataStoreRef();
  REQUIRE(store.getNumberOfTuples() == X * Y * Z);
  for(usize i = 0; i < fileValues.size(); i++)
  {
    REQUIRE(store.getValue(i) == fileValues[i]);
  }
}

TEST_CASE("ImageProcessing::ReadMhaFile: big-endian data is byteswapped", "[ImageProcessing][ReadMhaFileFilter]")
{
  constexpr usize X = 4, Y = 3, Z = 1;
  std::vector<int16_t> logical(X * Y * Z);
  for(usize i = 0; i < logical.size(); i++)
  {
    logical[i] = static_cast<int16_t>(static_cast<int>(i) * 257 - 1000);
  }
  // Serialize big-endian, independent of host endianness.
  std::vector<uint8_t> data(logical.size() * sizeof(int16_t));
  for(usize i = 0; i < logical.size(); i++)
  {
    const auto u = static_cast<uint16_t>(logical[i]);
    data[i * 2 + 0] = static_cast<uint8_t>((u >> 8) & 0xFFu);
    data[i * 2 + 1] = static_cast<uint8_t>(u & 0xFFu);
  }
  const fs::path p = MhaOutputDir() / "algo_bigendian.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "BinaryDataByteOrderMSB = True", "CompressedData = False", "ElementSpacing = 1 1", "DimSize = 4 3",
                    "ElementType = MET_SHORT", "ElementDataFile = LOCAL"},
                   data);
  DataStructure ds = BuildMhaTarget(DataType::int16, {X, Y, Z}, 1);
  REQUIRE(RunReadMhaFile(ds, p).valid());
  const auto& store = ds.getDataRefAs<DataArray<int16>>(AlgoImageDataPath()).getDataStoreRef();
  for(usize i = 0; i < logical.size(); i++)
  {
    REQUIRE(store.getValue(i) == logical[i]);
  }
}

TEST_CASE("ImageProcessing::ReadMhaFile: numeric CompressedData = 1 round-trips", "[ImageProcessing][ReadMhaFileFilter]")
{
  // Regression for the ParseBool fix: "CompressedData = 1" must decode as zlib, not be
  // read raw (which short-read errored before).
  constexpr usize X = 4, Y = 3, Z = 2;
  std::vector<int16_t> fileValues(X * Y * Z);
  std::iota(fileValues.begin(), fileValues.end(), static_cast<int16_t>(-5));
  const fs::path p = MhaOutputDir() / "algo_numeric_compressed.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 3", "BinaryData = True", "BinaryDataByteOrderMSB = False", "CompressedData = 1", "ElementSpacing = 1 1 1", "DimSize = 4 3 2",
                    "ElementType = MET_SHORT", "ElementDataFile = LOCAL"},
                   ZlibCompress(ToBytes(fileValues)));
  DataStructure ds = BuildMhaTarget(DataType::int16, {X, Y, Z}, 1);
  REQUIRE(RunReadMhaFile(ds, p).valid());
  const auto& store = ds.getDataRefAs<DataArray<int16>>(AlgoImageDataPath()).getDataStoreRef();
  REQUIRE(store.getNumberOfTuples() == X * Y * Z);
  for(usize i = 0; i < fileValues.size(); i++)
  {
    REQUIRE(store.getValue(i) == fileValues[i]);
  }
}

TEST_CASE("ImageProcessing::ReadMhaFile: BinaryDataByteOrderMSB = T byteswaps to correct values", "[ImageProcessing][ReadMhaFileFilter]")
{
  // Regression for the ParseBool fix: the numeric-form true "T" must engage the byteswap
  // just like "True"; the old whole-string check left it off and produced wrong values.
  constexpr usize X = 4, Y = 3, Z = 1;
  std::vector<int16_t> logical(X * Y * Z);
  for(usize i = 0; i < logical.size(); i++)
  {
    logical[i] = static_cast<int16_t>(static_cast<int>(i) * 257 - 1000);
  }
  // Serialize big-endian, independent of host endianness.
  std::vector<uint8_t> data(logical.size() * sizeof(int16_t));
  for(usize i = 0; i < logical.size(); i++)
  {
    const auto u = static_cast<uint16_t>(logical[i]);
    data[i * 2 + 0] = static_cast<uint8_t>((u >> 8) & 0xFFu);
    data[i * 2 + 1] = static_cast<uint8_t>(u & 0xFFu);
  }
  const fs::path p = MhaOutputDir() / "algo_msb_t.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "BinaryDataByteOrderMSB = T", "CompressedData = False", "ElementSpacing = 1 1", "DimSize = 4 3", "ElementType = MET_SHORT",
                    "ElementDataFile = LOCAL"},
                   data);
  DataStructure ds = BuildMhaTarget(DataType::int16, {X, Y, Z}, 1);
  REQUIRE(RunReadMhaFile(ds, p).valid());
  const auto& store = ds.getDataRefAs<DataArray<int16>>(AlgoImageDataPath()).getDataStoreRef();
  for(usize i = 0; i < logical.size(); i++)
  {
    REQUIRE(store.getValue(i) == logical[i]);
  }
}

TEST_CASE("ImageProcessing::ReadMhaFile: multi-component interleaves channels", "[ImageProcessing][ReadMhaFileFilter]")
{
  constexpr usize comp = 2, X = 4, Y = 2, Z = 1;
  std::vector<float32> fileValues(comp * X * Y);
  std::iota(fileValues.begin(), fileValues.end(), 0.0f);
  const fs::path p = MhaOutputDir() / "algo_multicomp.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "BinaryDataByteOrderMSB = False", "CompressedData = False", "ElementNumberOfChannels = 2", "ElementSpacing = 1 1",
                    "DimSize = 4 2", "ElementType = MET_FLOAT", "ElementDataFile = LOCAL"},
                   ToBytes(fileValues));
  DataStructure ds = BuildMhaTarget(DataType::float32, {X, Y, Z}, comp);
  REQUIRE(RunReadMhaFile(ds, p).valid());
  const auto& store = ds.getDataRefAs<DataArray<float32>>(AlgoImageDataPath()).getDataStoreRef();
  REQUIRE(store.getNumberOfTuples() == X * Y);
  REQUIRE(store.getNumberOfComponents() == comp);
  for(usize i = 0; i < fileValues.size(); i++)
  {
    REQUIRE(store.getValue(i) == fileValues[i]);
  }
}

TEST_CASE("ImageProcessing::ReadMhaFile: voxel subvolume crop keeps only the requested region", "[ImageProcessing][ReadMhaFileFilter]")
{
  constexpr usize X = 4, Y = 3, Z = 2;
  std::vector<int16_t> fileValues(X * Y * Z);
  std::iota(fileValues.begin(), fileValues.end(), static_cast<int16_t>(0)); // value == flat file position (X-fastest)
  const fs::path p = MhaOutputDir() / "algo_crop.mha";
  WriteMhaAttached(
      p, {"ObjectType = Image", "NDims = 3", "BinaryData = True", "CompressedData = False", "ElementSpacing = 1 1 1", "DimSize = 4 3 2", "ElementType = MET_SHORT", "ElementDataFile = LOCAL"},
      ToBytes(fileValues));
  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  crop.cropX = true;
  crop.xBoundVoxels = {1, 2};
  crop.cropY = true;
  crop.yBoundVoxels = {0, 1};
  crop.cropZ = true;
  crop.zBoundVoxels = {1, 1};
  DataStructure ds = BuildMhaTarget(DataType::int16, {2, 2, 1}, 1);
  REQUIRE(RunReadMhaFile(ds, p, crop).valid());
  const auto& store = ds.getDataRefAs<DataArray<int16>>(AlgoImageDataPath()).getDataStoreRef();
  // file pos = (z*Y + y)*X + x; z=1,y=0,x=1..2 then z=1,y=1,x=1..2.
  const std::vector<int16_t> expected = {13, 14, 17, 18};
  REQUIRE(store.getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    REQUIRE(store.getValue(i) == expected[i]);
  }
}

TEST_CASE("ImageProcessing::ReadMhaFileFilter: scalar 2D round trip (raw + zlib)", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();

  const usize X = 4, Y = 3;
  std::vector<uint8_t> voxels(X * Y);
  for(usize i = 0; i < voxels.size(); i++)
  {
    voxels[i] = static_cast<uint8_t>(i * 3 + 1);
  }

  const DataPath geomPath({"MHA Image"});
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath("ImageData");

  for(bool compressed : {false, true})
  {
    DYNAMIC_SECTION("compressed=" << (compressed ? "true" : "false"))
    {
      const fs::path p = MhaOutputDir() / (compressed ? "rt_scalar.zlib.mha" : "rt_scalar.raw.mha");
      WriteMhaAttached(p,
                       {"ObjectType = Image", "NDims = 2", "BinaryData = True", "BinaryDataByteOrderMSB = False", compressed ? "CompressedData = True" : "CompressedData = False",
                        "TransformMatrix = 1 0 0 1", "Offset = -1 2", "CenterOfRotation = 0 0", "ElementSpacing = 0.5 0.75", "DimSize = 4 3", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"},
                       compressed ? ZlibCompress(voxels) : voxels);

      DataStructure ds;
      ReadMhaFileFilter filter;
      const Arguments args = MakeMhaArgs(p, geomPath, "Cell Data", "ImageData");
      const auto preflightResult = filter.preflight(ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
      const auto executeResult = filter.execute(ds, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

      const auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
      REQUIRE(geom.getDimensions()[0] == 4);
      REQUIRE(geom.getDimensions()[1] == 3);
      REQUIRE(geom.getDimensions()[2] == 1);
      REQUIRE(geom.getSpacing()[0] == Approx(0.5f));
      REQUIRE(geom.getOrigin()[1] == Approx(2.0f));
      RequireMhaArrayEquals<uint8>(ds, arrPath, voxels);
      UnitTest::CheckArraysInheritTupleDims(ds);
    }
  }
}

TEST_CASE("ImageProcessing::ReadMhaFileFilter: multi-component float32 round trip", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();
  const usize X = 4, Y = 3, comp = 2;
  std::vector<float32> voxels(X * Y * comp);
  std::iota(voxels.begin(), voxels.end(), 0.5f);
  const fs::path p = MhaOutputDir() / "rt_multicomp.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "CompressedData = False", "ElementNumberOfChannels = 2", "ElementSpacing = 1 1", "DimSize = 4 3", "ElementType = MET_FLOAT",
                    "ElementDataFile = LOCAL"},
                   ToBytes(voxels));
  const DataPath geomPath({"MHA MC"});
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath("ImageData");
  DataStructure ds;
  ReadMhaFileFilter filter;
  const Arguments args = MakeMhaArgs(p, geomPath, "Cell Data", "ImageData");
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const auto& arr = ds.getDataRefAs<DataArray<float32>>(arrPath);
  REQUIRE(arr.getNumberOfComponents() == 2);
  RequireMhaArrayEquals<float32>(ds, arrPath, voxels);
  UnitTest::CheckArraysInheritTupleDims(ds);
}

// Regression for the physical-crop preflight/execute divergence: an out-of-range physical
// box green-preflights (CropImageGeometryFilter clamps the bounds into the volume with a
// warning) but the old execute path hard-errored (ImageGeom::getIndex returns nullopt for
// an OOB coordinate). Execute now clamps identically, so preflight and execute agree.
TEST_CASE("ImageProcessing::ReadMhaFileFilter: PhysicalSubvolume crop clamps out-of-range bounds", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize X = 6, Y = 5, Z = 4;
  std::vector<uint16_t> voxels(X * Y * Z);
  std::iota(voxels.begin(), voxels.end(), static_cast<uint16_t>(0)); // value == flat file position (X-fastest)

  // spacing {2,3,4}, origin {10,20,30}; far corner = origin + dims*spacing = {22,35,46}.
  const fs::path p = MhaOutputDir() / "rt_crop_physical_oob.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 3", "BinaryData = True", "CompressedData = False", "ElementSpacing = 2 3 4", "Offset = 10 20 30", "DimSize = 6 5 4", "ElementType = MET_USHORT",
                    "ElementDataFile = LOCAL"},
                   ToBytes(voxels));

  // Bounds extend far below the origin and far above the far corner on every axis, so both
  // ends clamp to the full extent -> the whole 6x5x4 volume is read.
  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
  crop.cropX = crop.cropY = crop.cropZ = true;
  crop.xBoundPhysical = {-100.0f, 1000.0f};
  crop.yBoundPhysical = {-100.0f, 1000.0f};
  crop.zBoundPhysical = {-100.0f, 1000.0f};

  const DataPath geomPath({"MHA PhysCropOOB"});
  const DataPath arrPath = geomPath.createChildPath("Cell Data").createChildPath("ImageData");
  DataStructure ds;
  ReadMhaFileFilter filter;
  const Arguments args = MakeMhaArgs(p, geomPath, "Cell Data", "ImageData", crop);
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const auto& geom = ds.getDataRefAs<ImageGeom>(geomPath);
  REQUIRE(geom.getDimensions()[0] == X);
  REQUIRE(geom.getDimensions()[1] == Y);
  REQUIRE(geom.getDimensions()[2] == Z);
  RequireMhaArrayEquals<uint16>(ds, arrPath, voxels);
}

TEST_CASE("ImageProcessing::ReadMhaFileFilter: rejects missing file and bad ObjectType", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();
  SECTION("missing file")
  {
    DataStructure ds;
    ReadMhaFileFilter filter;
    const Arguments args = MakeMhaArgs(MhaOutputDir() / "does_not_exist_654.mha", DataPath({"X"}), "Cell Data", "ImageData");
    const auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.invalid());
  }
  SECTION("not an image")
  {
    const fs::path p = MhaOutputDir() / "rt_not_image.mha";
    WriteMhaAttached(p, {"ObjectType = Tube", "NDims = 2", "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"}, {});
    DataStructure ds;
    ReadMhaFileFilter filter;
    const Arguments args = MakeMhaArgs(p, DataPath({"X"}), "Cell Data", "ImageData");
    const auto preflightResult = filter.preflight(ds, args);
    REQUIRE(preflightResult.outputActions.invalid());
  }
}

TEST_CASE("ImageProcessing::ReadMhaFileFilter: apply-transform preflights against staged geometry", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();

  // A 2D 90-degree rotation ("0 -1 1 0", determinant 1). With apply_image_transformation = true the
  // filter must preflight ApplyTransformationToGeometry against the geometry it queues, not the
  // (empty) incoming DataStructure. Before the staging fix, that downstream filter could not see the
  // queued ImageGeom and took its "not an Image Geometry" branch: it emitted the spurious -5555
  // warning and declared none of the transform's output actions -- a misleading preflight result.
  std::vector<uint8_t> pixels(4 * 3);
  std::iota(pixels.begin(), pixels.end(), static_cast<uint8_t>(1));
  const fs::path p = MhaOutputDir() / "xform_apply_preflight.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "CompressedData = False", "TransformMatrix = 0 -1 1 0", "Offset = 0 0", "CenterOfRotation = 0 0", "ElementSpacing = 1 1",
                    "DimSize = 4 3", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"},
                   pixels);

  const DataPath geomPath({"MHA Apply"});
  DataStructure ds;
  ReadMhaFileFilter filter;
  Arguments args = MakeMhaArgs(p, geomPath, "Cell Data", "ImageData");
  args.insertOrAssign(ReadMhaFileFilter::k_ApplyImageTransformation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadMhaFileFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL)); // nearest neighbor

  const auto preflight = filter.preflight(ds, args);
  // Post-fix: the downstream transform sees the staged geometry, so preflight is VALID.
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  // Post-fix: the geometry is found, so the spurious "-5555 not an Image Geometry" warning is gone.
  for(const auto& w : preflight.outputActions.warnings())
  {
    REQUIRE(w.code != -5555);
  }
}

namespace
{
// The real ITK regression .mha files live under the ITKImageProcessing source tree.
fs::path ItkMhaInputDir()
{
  return fs::path(std::string(nx::core::unit_test::k_SimplnxSourceDIr.view())) / "src" / "Plugins" / "ITKImageProcessing" / "data" / "JSONFilters";
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

TEST_CASE("ImageProcessing::ReadMhaFileFilter: live-ITK read parity", "[ImageProcessing][ReadMhaFileFilter][itk-parity]")
{
  UnitTest::LoadPlugins();

  // The legacy ITK reader adopts its buffer into the store via a dynamic_cast to the in-core
  // DataStore<T> (throws bad_cast on an OOC store). Read-value parity is storage-independent, so
  // pin BOTH readers in-core here; our reader's OOC path is proven separately (Task 7).
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto app = Application::GetOrCreateInstance();
  const Uuid itkReaderUuid = *Uuid::FromString("d72eaf98-9b1d-44c9-88f2-a5c3cf57b4f2");
  auto itkReader = app->getFilterList()->createFilter(itkReaderUuid);
  if(itkReader == nullptr)
  {
    WARN("ITKImageProcessing plugin not loaded (ITK reader UUID d72eaf98... unavailable); skipping live-ITK MHA parity gate.");
    return;
  }

  const fs::path dir = ItkMhaInputDir();
  // Axis-aligned, identity-transform inputs = the clean read-parity set:
  //   scalar compressed (uint8 + float32), and raw multi-component (float32 + float64).
  const std::vector<fs::path> files = {
      dir / "Input" / "2th_cthead1.mha",            // MET_UCHAR, compressed, 2D
      dir / "Input" / "cthead1-Float.mha",          // MET_FLOAT, compressed, 2D
      dir / "Input" / "displacement.mha",           // MET_FLOAT, raw, 2-component, 2D
      dir / "Baseline" / "displacement_13x17y.mha", // MET_DOUBLE, raw, 2-component, 2D
  };

  for(const auto& file : files)
  {
    DYNAMIC_SECTION("file=" << file.filename().string())
    {
      REQUIRE(fs::exists(file));

      // Ours.
      DataStructure oursDs;
      ReadMhaFileFilter ours;
      const DataPath oursGeom({"OursGeom"});
      const DataPath oursArr = oursGeom.createChildPath("Cell Data").createChildPath("ImageData");
      const Arguments oursArgs = MakeMhaArgs(file, oursGeom, "Cell Data", "ImageData");
      const auto oursPreflight = ours.preflight(oursDs, oursArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(oursPreflight.outputActions);
      const auto oursExecute = ours.execute(oursDs, oursArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(oursExecute.result);

      // ITK oracle.
      DataStructure itkDs;
      const DataPath itkGeom({"ItkGeom"});
      const DataPath itkArr = itkGeom.createChildPath("Cell Data").createChildPath("ImageData");
      Arguments itkArgs;
      itkArgs.insertOrAssign(k_ITK_FileName, std::make_any<fs::path>(file));
      itkArgs.insertOrAssign(k_ITK_GeometryPath, std::make_any<DataPath>(itkGeom));
      itkArgs.insertOrAssign(k_ITK_CellDataName, std::make_any<std::string>("Cell Data"));
      itkArgs.insertOrAssign(k_ITK_ArrayName, std::make_any<std::string>("ImageData"));
      itkArgs.insertOrAssign(k_ITK_ChangeOrigin, std::make_any<bool>(false));
      itkArgs.insertOrAssign(k_ITK_ChangeSpacing, std::make_any<bool>(false));
      const auto itkExec = itkReader->execute(itkDs, itkArgs); // execute() runs preflight internally
      SIMPLNX_RESULT_REQUIRE_VALID(itkExec.result);

      // Geometry parity.
      const auto& og = oursDs.getDataRefAs<ImageGeom>(oursGeom);
      const auto& ig = itkDs.getDataRefAs<ImageGeom>(itkGeom);
      REQUIRE(og.getDimensions() == ig.getDimensions());
      for(usize k = 0; k < 3; k++)
      {
        REQUIRE(og.getOrigin()[k] == Approx(ig.getOrigin()[k]).margin(1e-4));
        REQUIRE(og.getSpacing()[k] == Approx(ig.getSpacing()[k]).margin(1e-4));
      }

      // Byte-identical pixel array parity.
      RequireArraysIdentical(oursDs.getDataRefAs<IDataArray>(oursArr), itkDs.getDataRefAs<IDataArray>(itkArr));
    }
  }
}

// Broadens the live-ITK read-parity gate (fixes M1 byteswap + integer types) with SYNTHETIC
// inputs the committed real .mha set does not cover: an integer MET type (MET_SHORT) and a
// big-endian multibyte volume (BinaryDataByteOrderMSB = True). Both are read by our reader and
// the generic runtime ITK reader (ITKImageReader d72eaf98-...) and asserted byte-identical.
// Self-skips (WARN) when the ITK plugin is absent so ITK-free ImageProcessing builds still pass.
TEST_CASE("ImageProcessing::ReadMhaFileFilter: live-ITK parity for integer + big-endian volumes", "[ImageProcessing][ReadMhaFileFilter][itk-parity]")
{
  UnitTest::LoadPlugins();

  // The legacy ITK reader adopts its buffer via a dynamic_cast to the in-core DataStore<T>
  // (throws bad_cast on an OOC store); read-value parity is storage-independent, so pin both in-core.
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto app = Application::GetOrCreateInstance();
  const Uuid itkReaderUuid = *Uuid::FromString("d72eaf98-9b1d-44c9-88f2-a5c3cf57b4f2");
  auto itkReader = app->getFilterList()->createFilter(itkReaderUuid);
  if(itkReader == nullptr)
  {
    WARN("ITKImageProcessing plugin not loaded (ITK reader UUID d72eaf98... unavailable); skipping broadened live-ITK MHA parity.");
    return;
  }

  // Serialize int16 values with an explicit byte order so the file is unambiguous on any host.
  auto serialize = [](const std::vector<int16_t>& vals, bool bigEndian) {
    std::vector<uint8_t> out(vals.size() * sizeof(int16_t));
    for(usize i = 0; i < vals.size(); i++)
    {
      const auto u = static_cast<uint16_t>(vals[i]);
      out[i * 2 + (bigEndian ? 0 : 1)] = static_cast<uint8_t>((u >> 8) & 0xFFu);
      out[i * 2 + (bigEndian ? 1 : 0)] = static_cast<uint8_t>(u & 0xFFu);
    }
    return out;
  };

  constexpr usize X = 5, Y = 4, Z = 3;
  std::vector<int16_t> voxels(X * Y * Z);
  for(usize i = 0; i < voxels.size(); i++)
  {
    voxels[i] = static_cast<int16_t>(static_cast<int>(i) * 137 - 3000); // spans negative + positive
  }

  for(bool bigEndian : {false, true})
  {
    DYNAMIC_SECTION("MET_SHORT " << (bigEndian ? "big-endian" : "little-endian"))
    {
      const fs::path p = MhaOutputDir() / (bigEndian ? "parity_short_be.mha" : "parity_short_le.mha");
      WriteMhaAttached(p,
                       {"ObjectType = Image", "NDims = 3", "BinaryData = True", bigEndian ? "BinaryDataByteOrderMSB = True" : "BinaryDataByteOrderMSB = False", "CompressedData = False",
                        "ElementSpacing = 1 1 1", "DimSize = 5 4 3", "ElementType = MET_SHORT", "ElementDataFile = LOCAL"},
                       serialize(voxels, bigEndian));

      // Ours.
      DataStructure oursDs;
      ReadMhaFileFilter ours;
      const DataPath oursGeom({"OursGeom"});
      const DataPath oursArr = oursGeom.createChildPath("Cell Data").createChildPath("ImageData");
      const Arguments oursArgs = MakeMhaArgs(p, oursGeom, "Cell Data", "ImageData");
      const auto oursPreflight = ours.preflight(oursDs, oursArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(oursPreflight.outputActions);
      const auto oursExecute = ours.execute(oursDs, oursArgs);
      SIMPLNX_RESULT_REQUIRE_VALID(oursExecute.result);

      // ITK oracle.
      DataStructure itkDs;
      const DataPath itkGeom({"ItkGeom"});
      const DataPath itkArr = itkGeom.createChildPath("Cell Data").createChildPath("ImageData");
      Arguments itkArgs;
      itkArgs.insertOrAssign(k_ITK_FileName, std::make_any<fs::path>(p));
      itkArgs.insertOrAssign(k_ITK_GeometryPath, std::make_any<DataPath>(itkGeom));
      itkArgs.insertOrAssign(k_ITK_CellDataName, std::make_any<std::string>("Cell Data"));
      itkArgs.insertOrAssign(k_ITK_ArrayName, std::make_any<std::string>("ImageData"));
      itkArgs.insertOrAssign(k_ITK_ChangeOrigin, std::make_any<bool>(false));
      itkArgs.insertOrAssign(k_ITK_ChangeSpacing, std::make_any<bool>(false));
      const auto itkExec = itkReader->execute(itkDs, itkArgs); // execute() runs preflight internally
      SIMPLNX_RESULT_REQUIRE_VALID(itkExec.result);

      // Geometry parity.
      const auto& og = oursDs.getDataRefAs<ImageGeom>(oursGeom);
      const auto& ig = itkDs.getDataRefAs<ImageGeom>(itkGeom);
      REQUIRE(og.getDimensions() == ig.getDimensions());
      for(usize k = 0; k < 3; k++)
      {
        REQUIRE(og.getOrigin()[k] == Approx(ig.getOrigin()[k]).margin(1e-4));
        REQUIRE(og.getSpacing()[k] == Approx(ig.getSpacing()[k]).margin(1e-4));
      }

      // Byte-identical pixel array parity (proves the byteswap + integer decode match ITK).
      RequireArraysIdentical(oursDs.getDataRefAs<IDataArray>(oursArr), itkDs.getDataRefAs<IDataArray>(itkArr));
    }
  }
}

namespace
{
// Reads the .mha at @p p twice through ReadMhaFileFilter -- once forced in-core, once forced
// out-of-core with a tiny large-data threshold so the output store is disk-backed -- then asserts
// the OOC store equals the in-core read AND equals @p expected, element-by-element (byte-exact:
// the reader does a byte-exact bulk copy, so even float values are bit-identical). In an OOC-free
// build ForceOutOfCore is inert and both reads are in-memory; the comparison still holds.
template <typename T>
void RequireMhaOocMatchesInCore(const fs::path& p, const std::vector<T>& expected)
{
  auto readInto = [&](DataStructure& ds) {
    ReadMhaFileFilter filter;
    const DataPath geomPath({"MHA OOC"});
    const Arguments args = MakeMhaArgs(p, geomPath, "Cell Data", "ImageData");
    const auto preflight = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
    const auto execute = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
    return geomPath.createChildPath("Cell Data").createChildPath("ImageData");
  };

  DataStructure inCoreDs;
  DataPath arrPath;
  {
    const UnitTest::PreferencesSentinel sentinel(nx::core::DataStorageMode::ForceInCore, 0);
    arrPath = readInto(inCoreDs);
  }
  DataStructure oocDs;
  {
    const UnitTest::PreferencesSentinel sentinel(nx::core::DataStorageMode::ForceOutOfCore, 1024);
    readInto(oocDs);
  }

  const auto& inCore = inCoreDs.getDataRefAs<DataArray<T>>(arrPath).getDataStoreRef();
  const auto& ooc = oocDs.getDataRefAs<DataArray<T>>(arrPath).getDataStoreRef();
  REQUIRE(inCore.getSize() == ooc.getSize());
  REQUIRE(inCore.getSize() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    REQUIRE(ooc[i] == inCore[i]);
    REQUIRE(ooc[i] == expected[i]);
  }
}
} // namespace

TEST_CASE("ImageProcessing::ReadMhaFileFilter: OOC output matches in-core (scalar + multi-component)", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();

  SECTION("float32 scalar (compressed)")
  {
    // 40x40x20 == 32000 tuples (128 KiB); large enough for a forced-OOC store to span chunks.
    constexpr usize X = 40, Y = 40, Z = 20;
    std::vector<float32> voxels(X * Y * Z);
    for(usize i = 0; i < voxels.size(); i++)
    {
      voxels[i] = static_cast<float32>(i) * 0.25f - 1234.5f; // finite (no NaN/inf) so == is exact
    }
    const fs::path p = MhaOutputDir() / "ooc_float32.mha";
    WriteMhaAttached(p,
                     {"ObjectType = Image", "NDims = 3", "BinaryData = True", "BinaryDataByteOrderMSB = False", "CompressedData = True", "ElementSpacing = 1 1 1", "DimSize = 40 40 20",
                      "ElementType = MET_FLOAT", "ElementDataFile = LOCAL"},
                     ZlibCompress(ToBytes(voxels)));
    RequireMhaOocMatchesInCore<float32>(p, voxels);
  }

  SECTION("uint8 multi-component (ElementNumberOfChannels=3, raw)")
  {
    // 2D image (Z=1): 48x32 == 1536 tuples x 3 components == 4608 bytes.
    constexpr usize X = 48, Y = 32;
    std::vector<uint8> voxels(3 * X * Y);
    for(usize i = 0; i < voxels.size(); i++)
    {
      voxels[i] = static_cast<uint8>((i * 3 + 7) % 251);
    }
    const fs::path p = MhaOutputDir() / "ooc_rgb_uint8.mha";
    WriteMhaAttached(p,
                     {"ObjectType = Image", "NDims = 2", "BinaryData = True", "CompressedData = False", "ElementNumberOfChannels = 3", "ElementSpacing = 1 1", "DimSize = 48 32",
                      "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"},
                     voxels);
    RequireMhaOocMatchesInCore<uint8>(p, voxels);
  }
}

namespace
{
// Reads a synthetic .mha and returns the 16 saved transform-matrix floats (row-major 4x4).
std::vector<float32> ReadSavedTransform(const fs::path& p, bool transpose)
{
  const DataPath geomPath({"MHA T"});
  const DataPath tMatrixPath = geomPath.createChildPath("TransformationMatrix");
  DataStructure ds;
  ReadMhaFileFilter filter;
  Arguments args = MakeMhaArgs(p, geomPath, "Cell Data", "ImageData");
  args.insertOrAssign(ReadMhaFileFilter::k_SaveImageTransformation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadMhaFileFilter::k_TransposeTransformMatrix_Key, std::make_any<bool>(transpose));
  args.insertOrAssign(ReadMhaFileFilter::k_TransformationMatrixPath_Key, std::make_any<ArrayCreationParameter::ValueType>(tMatrixPath));
  const auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  const auto& store = ds.getDataRefAs<DataArray<float32>>(tMatrixPath).getDataStoreRef();
  std::vector<float32> out(store.getSize());
  for(usize i = 0; i < out.size(); i++)
  {
    out[i] = store[i];
  }
  return out;
}
} // namespace

TEST_CASE("ImageProcessing::ReadMhaFileFilter: transform matrix 2D parse + save-as-array", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();
  // 2D 90-degree rotation: [[0,-1],[1,0]] (row-major "0 -1 1 0"), determinant 1.
  std::vector<uint8_t> pixels(4 * 3, 0);
  const fs::path p = MhaOutputDir() / "xform_2d.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "CompressedData = False", "TransformMatrix = 0 -1 1 0", "Offset = 0 0", "CenterOfRotation = 0 0", "ElementSpacing = 1 1",
                    "DimSize = 4 3", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"},
                   pixels);

  const std::vector<float32> saved = ReadSavedTransform(p, /*transpose=*/false);
  const std::vector<float32> expected = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  REQUIRE(saved.size() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    REQUIRE(saved[i] == Approx(expected[i]));
  }

  // Transpose of a pure rotation is allowed (determinant 1) and swaps off-diagonals.
  const std::vector<float32> transposed = ReadSavedTransform(p, /*transpose=*/true);
  const std::vector<float32> expectedT = {0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  for(usize i = 0; i < expectedT.size(); i++)
  {
    REQUIRE(transposed[i] == Approx(expectedT[i]));
  }
}

TEST_CASE("ImageProcessing::ReadMhaFileFilter: transform matrix 3D parse + save-as-array", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();
  // 3D 90-degree rotation about z: row-major "0 -1 0 1 0 0 0 0 1".
  std::vector<uint8_t> pixels(2 * 2 * 2, 0);
  const fs::path p = MhaOutputDir() / "xform_3d.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 3", "BinaryData = True", "CompressedData = False", "TransformMatrix = 0 -1 0 1 0 0 0 0 1", "Offset = 0 0 0", "CenterOfRotation = 0 0 0",
                    "ElementSpacing = 1 1 1", "DimSize = 2 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"},
                   pixels);
  const std::vector<float32> saved = ReadSavedTransform(p, /*transpose=*/false);
  const std::vector<float32> expected = {0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
  REQUIRE(saved.size() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    REQUIRE(saved[i] == Approx(expected[i]));
  }
}

TEST_CASE("ImageProcessing::ReadMhaFileFilter: transpose of a non-rotation matrix errors -35852", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();
  // A 2D scale matrix (determinant 4) is not a pure rotation: transpose must error.
  std::vector<uint8_t> pixels(2 * 2, 0);
  const fs::path p = MhaOutputDir() / "xform_scale.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "CompressedData = False", "TransformMatrix = 2 0 0 2", "Offset = 0 0", "CenterOfRotation = 0 0", "ElementSpacing = 1 1",
                    "DimSize = 2 2", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"},
                   pixels);
  DataStructure ds;
  ReadMhaFileFilter filter;
  Arguments args = MakeMhaArgs(p, DataPath({"MHA Bad"}), "Cell Data", "ImageData");
  args.insertOrAssign(ReadMhaFileFilter::k_TransposeTransformMatrix_Key, std::make_any<bool>(true));
  const auto preflight = filter.preflight(ds, args);
  REQUIRE(preflight.outputActions.invalid());
  bool found = false;
  for(const auto& e : preflight.outputActions.errors())
  {
    if(e.code == -35852)
    {
      found = true;
    }
  }
  REQUIRE(found);
}

TEST_CASE("ImageProcessing::ReadMhaFileFilter: apply transformation to geometry executes", "[ImageProcessing][ReadMhaFileFilter]")
{
  UnitTest::LoadPlugins();
  // 2D 90-degree rotation applied to a small image via ApplyTransformationToGeometry.
  std::vector<uint8_t> pixels(4 * 3);
  std::iota(pixels.begin(), pixels.end(), static_cast<uint8_t>(1));
  const fs::path p = MhaOutputDir() / "xform_apply.mha";
  WriteMhaAttached(p,
                   {"ObjectType = Image", "NDims = 2", "BinaryData = True", "CompressedData = False", "TransformMatrix = 0 -1 1 0", "Offset = 0 0", "CenterOfRotation = 0 0", "ElementSpacing = 1 1",
                    "DimSize = 4 3", "ElementType = MET_UCHAR", "ElementDataFile = LOCAL"},
                   pixels);
  const DataPath geomPath({"MHA Apply"});
  DataStructure ds;
  ReadMhaFileFilter filter;
  Arguments args = MakeMhaArgs(p, geomPath, "Cell Data", "ImageData");
  args.insertOrAssign(ReadMhaFileFilter::k_ApplyImageTransformation_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadMhaFileFilter::k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(0ULL)); // nearest neighbor
  const auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  const auto execute = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(execute.result);
  // The geometry still exists after the transform resample.
  REQUIRE_NOTHROW(ds.getDataRefAs<ImageGeom>(geomPath));
}

TEST_CASE("ImageProcessing::ReadMhaFileFilter: live-ITK transform-matrix parity (downloaded)", "[ImageProcessing][ReadMhaFileFilter][itk-parity]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  // The 2x2/3x3 transform-matrix .mha fixtures are downloaded, not committed in-tree.
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "ITKMhaFileReaderTest_v3.tar.gz", "ITKMhaFileReaderTest_v3");
  const fs::path file = fs::path(nx::core::unit_test::k_TestFilesDir.view()) / "ITKMhaFileReaderTest_v3" / "SmallIN100_073.mha";
  if(!fs::exists(file))
  {
    WARN("SmallIN100_073.mha not available (download skipped); skipping live-ITK MHA transform parity.");
    return;
  }

  auto app = Application::GetOrCreateInstance();
  const Uuid itkMhaUuid = *Uuid::FromString("41c33a08-0052-4915-8d53-d503f85f30d9");
  auto itkMha = app->getFilterList()->createFilter(itkMhaUuid);
  if(itkMha == nullptr)
  {
    WARN("ITKImageProcessing plugin not loaded (ITK MHA reader UUID 41c33a08... unavailable); skipping live-ITK MHA transform parity.");
    return;
  }

  // Ours: save the transform as an array.
  DataStructure oursDs;
  ReadMhaFileFilter ours;
  const DataPath oursGeom({"OursGeom"});
  const DataPath oursTMat = oursGeom.createChildPath("TransformationMatrix");
  Arguments oursArgs = MakeMhaArgs(file, oursGeom, "Cell Data", "ImageData");
  oursArgs.insertOrAssign(ReadMhaFileFilter::k_SaveImageTransformation_Key, std::make_any<bool>(true));
  oursArgs.insertOrAssign(ReadMhaFileFilter::k_TransformationMatrixPath_Key, std::make_any<ArrayCreationParameter::ValueType>(oursTMat));
  const auto oursPreflight = ours.preflight(oursDs, oursArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(oursPreflight.outputActions);
  const auto oursExecute = ours.execute(oursDs, oursArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(oursExecute.result);

  // ITK oracle: ITKMhaFileReaderFilter with save-transformation on. Keys per the ITK filter.
  DataStructure itkDs;
  const DataPath itkGeom({"ItkGeom"});
  const DataPath itkTMat = itkGeom.createChildPath("TransformationMatrix");
  Arguments itkArgs;
  itkArgs.insertOrAssign("file_name", std::make_any<fs::path>(file));
  itkArgs.insertOrAssign("output_geometry_path", std::make_any<DataPath>(itkGeom));
  itkArgs.insertOrAssign("cell_attribute_matrix_name", std::make_any<std::string>("Cell Data"));
  itkArgs.insertOrAssign("image_data_array_name", std::make_any<std::string>("ImageData"));
  itkArgs.insertOrAssign("apply_image_transformation", std::make_any<bool>(false));
  itkArgs.insertOrAssign("interpolation_type_index", std::make_any<ChoicesParameter::ValueType>(0ULL));
  itkArgs.insertOrAssign("transpose_transform_matrix", std::make_any<bool>(false));
  itkArgs.insertOrAssign("save_image_transformation", std::make_any<bool>(true));
  itkArgs.insertOrAssign("output_transformation_matrix_path", std::make_any<ArrayCreationParameter::ValueType>(itkTMat));
  const auto itkExec = itkMha->execute(itkDs, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkExec.result);

  const auto& ours16 = oursDs.getDataRefAs<DataArray<float32>>(oursTMat).getDataStoreRef();
  const auto& itk16 = itkDs.getDataRefAs<DataArray<float32>>(itkTMat).getDataStoreRef();
  REQUIRE(ours16.getSize() == 16);
  REQUIRE(itk16.getSize() == 16);
  for(usize i = 0; i < 16; i++)
  {
    REQUIRE(ours16[i] == Approx(itk16[i]).margin(1e-4));
  }
}
