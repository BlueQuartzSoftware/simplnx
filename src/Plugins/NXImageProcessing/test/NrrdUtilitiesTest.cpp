#include <catch2/catch.hpp>

#include "ImageProcessing/Filters/Algorithms/ReadNrrdFile.hpp"
#include "ImageProcessing/utils/NrrdUtilities.hpp"

#include "ImageProcessing/ImageProcessing_test_dirs.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <fmt/format.h>

#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <numeric>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
fs::path NrrdOutputDir()
{
  fs::path dir = fs::path(std::string(nx::core::unit_test::k_BinaryTestOutputDir.view())) / "ReadNrrdFile";
  fs::create_directories(dir);
  return dir;
}

// Writes an attached NRRD: ASCII header lines, a blank line, then the raw data bytes.
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

template <typename T>
std::vector<uint8_t> ToBytes(const std::vector<T>& values)
{
  std::vector<uint8_t> out(values.size() * sizeof(T));
  std::memcpy(out.data(), values.data(), out.size());
  return out;
}

// Writes a detached NRRD: a .nhdr text header (terminated by a blank line) that
// references a sibling data file, plus the sibling data file itself.
void WriteNhdrDetached(const fs::path& hdrPath, const std::vector<std::string>& headerLines, const fs::path& dataPath, const std::vector<uint8_t>& data)
{
  std::ofstream hdr(hdrPath, std::ios::binary);
  REQUIRE(hdr.is_open());
  for(const auto& line : headerLines)
  {
    hdr << line << "\n";
  }
  hdr << "\n"; // blank line terminates the header
  hdr.close();

  std::ofstream df(dataPath, std::ios::binary);
  REQUIRE(df.is_open());
  if(!data.empty())
  {
    df.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
  }
}

// Writes just a .nhdr text header (no sibling), for the detached error-path tests.
void WriteNhdrHeaderOnly(const fs::path& hdrPath, const std::vector<std::string>& headerLines)
{
  std::ofstream hdr(hdrPath, std::ios::binary);
  REQUIRE(hdr.is_open());
  for(const auto& line : headerLines)
  {
    hdr << line << "\n";
  }
  hdr << "\n";
}
} // namespace

TEST_CASE("ImageProcessing::NrrdUtilities: type string -> DataType map", "[ImageProcessing][NrrdUtilities]")
{
  using nrrd::NrrdTypeToSimplnx;
  REQUIRE(NrrdTypeToSimplnx("unsigned char").value().first == DataType::uint8);
  REQUIRE(NrrdTypeToSimplnx("uchar").value().first == DataType::uint8);
  REQUIRE(NrrdTypeToSimplnx("signed char").value().first == DataType::int8);
  REQUIRE(NrrdTypeToSimplnx("short").value().first == DataType::int16);
  REQUIRE(NrrdTypeToSimplnx("unsigned short").value().first == DataType::uint16);
  REQUIRE(NrrdTypeToSimplnx("int").value().first == DataType::int32);
  REQUIRE(NrrdTypeToSimplnx("unsigned int").value().first == DataType::uint32);
  REQUIRE(NrrdTypeToSimplnx("longlong").value().first == DataType::int64);
  REQUIRE(NrrdTypeToSimplnx("unsigned long long").value().first == DataType::uint64);
  REQUIRE(NrrdTypeToSimplnx("float").value().first == DataType::float32);
  REQUIRE(NrrdTypeToSimplnx("double").value().first == DataType::float64);
  REQUIRE(NrrdTypeToSimplnx("short").value().second == 2); // elementSize bytes
  REQUIRE(NrrdTypeToSimplnx("double").value().second == 8);
  REQUIRE_FALSE(NrrdTypeToSimplnx("block").has_value());
  REQUIRE_FALSE(NrrdTypeToSimplnx("nonsense").has_value());
}

TEST_CASE("ImageProcessing::NrrdUtilities: parse scalar 3D header", "[ImageProcessing][NrrdUtilities]")
{
  const fs::path p = NrrdOutputDir() / "hdr_scalar3d.nrrd";
  WriteNrrdRaw(p,
               {"NRRD0004", "type: short", "dimension: 3", "space: left-posterior-superior", "sizes: 4 3 2", "space directions: (0.5,0,0) (0,0.75,0) (0,0,1.25)", "kinds: domain domain domain",
                "endian: little", "encoding: raw", "space origin: (-1,2,3.5)"},
               std::vector<uint8_t>(4 * 3 * 2 * 2, 0));

  auto result = nrrd::ReadNrrdHeader(p);
  REQUIRE(result.valid());
  const auto md = result.value();
  REQUIRE(md.dataType == DataType::int16);
  REQUIRE(md.componentCount == 1);
  REQUIRE(md.spatialDim == 3);
  REQUIRE(md.dimensions == std::array<usize, 3>{4, 3, 2});
  REQUIRE(md.spacing[0] == Approx(0.5f));
  REQUIRE(md.spacing[1] == Approx(0.75f));
  REQUIRE(md.spacing[2] == Approx(1.25f));
  REQUIRE(md.origin[0] == Approx(-1.0f));
  REQUIRE(md.origin[1] == Approx(2.0f));
  REQUIRE(md.origin[2] == Approx(3.5f));
  REQUIRE(md.encoding == nrrd::Encoding::Raw);
  REQUIRE(md.byteSwapRequired == false); // little endian on a little-endian host
  REQUIRE_FALSE(md.affineHasRotation);
  REQUIRE(md.detached == false);
  REQUIRE(md.dataStartOffset > 0);
}

TEST_CASE("ImageProcessing::NrrdUtilities: parse 2D scalar (Z=1)", "[ImageProcessing][NrrdUtilities]")
{
  const fs::path p = NrrdOutputDir() / "hdr_scalar2d.nrrd";
  WriteNrrdRaw(p,
               {"NRRD0004", "type: unsigned char", "dimension: 2", "space dimension: 2", "sizes: 5 4", "space directions: (1,0) (0,1)", "kinds: domain domain", "encoding: raw", "space origin: (0,0)"},
               std::vector<uint8_t>(5 * 4, 0));
  auto result = nrrd::ReadNrrdHeader(p);
  REQUIRE(result.valid());
  const auto md = result.value();
  REQUIRE(md.spatialDim == 2);
  REQUIRE(md.dimensions == std::array<usize, 3>{5, 4, 1});
  REQUIRE(md.componentCount == 1);
  REQUIRE(md.dataType == DataType::uint8);
}

TEST_CASE("ImageProcessing::NrrdUtilities: parse RGB/vector component axis", "[ImageProcessing][NrrdUtilities]")
{
  const fs::path p = NrrdOutputDir() / "hdr_rgb.nrrd";
  WriteNrrdRaw(p,
               {"NRRD0004", "type: unsigned char", "dimension: 3", "space dimension: 2", "sizes: 3 8 6", "space directions: none (1,0) (0,1)", "kinds: vector domain domain", "encoding: raw",
                "space origin: (0,0)"},
               std::vector<uint8_t>(3 * 8 * 6, 0));
  auto result = nrrd::ReadNrrdHeader(p);
  REQUIRE(result.valid());
  const auto md = result.value();
  REQUIRE(md.componentCount == 3);
  REQUIRE(md.spatialDim == 2);
  REQUIRE(md.dimensions == std::array<usize, 3>{8, 6, 1}); // X=8, Y=6, Z=1
  REQUIRE(md.dataType == DataType::uint8);
  REQUIRE_FALSE(md.affineHasRotation);
}

TEST_CASE("ImageProcessing::NrrdUtilities: permuted directions flag rotation", "[ImageProcessing][NrrdUtilities]")
{
  const fs::path p = NrrdOutputDir() / "hdr_permuted.nrrd";
  WriteNrrdRaw(p,
               {"NRRD0004", "type: short", "dimension: 3", "space: left-posterior-superior", "sizes: 4 4 4", "space directions: (0,-2,0) (0,0,-2) (2.5,0,0)", "kinds: domain domain domain",
                "endian: little", "encoding: raw", "space origin: (0.6,-0.5,254.5)"},
               std::vector<uint8_t>(4 * 4 * 4 * 2, 0));
  auto result = nrrd::ReadNrrdHeader(p);
  REQUIRE(result.valid());
  const auto md = result.value();
  REQUIRE(md.affineHasRotation);          // permutation is not axis-aligned
  REQUIRE(md.spacing[0] == Approx(2.0f)); // ||(0,-2,0)||
  REQUIRE(md.spacing[2] == Approx(2.5f)); // ||(2.5,0,0)||
}

TEST_CASE("ImageProcessing::NrrdUtilities: rejects bad headers", "[ImageProcessing][NrrdUtilities]")
{
  SECTION("unsupported encoding")
  {
    const fs::path p = NrrdOutputDir() / "hdr_ascii.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 2", "sizes: 2 2", "encoding: ascii"}, {});
    REQUIRE(nrrd::ReadNrrdHeader(p).invalid());
  }
  SECTION("unsupported type block")
  {
    const fs::path p = NrrdOutputDir() / "hdr_block.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: block", "dimension: 2", "sizes: 2 2", "encoding: raw"}, {});
    REQUIRE(nrrd::ReadNrrdHeader(p).invalid());
  }
  SECTION("sizes count mismatch")
  {
    const fs::path p = NrrdOutputDir() / "hdr_mismatch.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 3", "sizes: 2 2", "encoding: raw"}, {});
    REQUIRE(nrrd::ReadNrrdHeader(p).invalid());
  }
  SECTION("bad magic")
  {
    const fs::path p = NrrdOutputDir() / "hdr_badmagic.nrrd";
    WriteNrrdRaw(p, {"NOTNRRD", "type: short", "dimension: 2", "sizes: 2 2", "encoding: raw"}, {});
    REQUIRE(nrrd::ReadNrrdHeader(p).invalid());
  }
  SECTION("multi-byte type missing endian (matches ITK, which errors)")
  {
    // teem/ITK's NrrdIO errors ("type (...) and encoding (...) require endian info")
    // for a multi-byte type with a raw/gzip encoding when the 'endian' field is
    // absent, rather than assuming a default byte order. We match that with -35713.
    const fs::path p = NrrdOutputDir() / "hdr_no_endian.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 2", "sizes: 2 2", "encoding: raw"}, std::vector<uint8_t>(2 * 2 * 2, 0));
    const auto result = nrrd::ReadNrrdHeader(p);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35713);
  }
  SECTION("single-byte type does NOT require endian")
  {
    // endian is irrelevant for 1-byte types, so a missing 'endian' field is fine.
    const fs::path p = NrrdOutputDir() / "hdr_uchar_no_endian.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: unsigned char", "dimension: 2", "sizes: 2 2", "encoding: raw"}, std::vector<uint8_t>(2 * 2, 0));
    REQUIRE(nrrd::ReadNrrdHeader(p).valid());
  }
}

TEST_CASE("ImageProcessing::NrrdUtilities: rejects unsupported skip fields and malformed sizes", "[ImageProcessing][NrrdUtilities]")
{
  // teem/ITK honor 'byte skip' / 'line skip'; this reader cannot, so a nonzero value is
  // rejected rather than silently dropped (which would read the wrong voxels).
  SECTION("nonzero byte skip -> -35714")
  {
    const fs::path p = NrrdOutputDir() / "hdr_byteskip.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 3", "sizes: 2 2 2", "endian: little", "encoding: raw", "byte skip: 4"}, std::vector<uint8_t>(2 * 2 * 2 * 2, 0));
    const auto result = nrrd::ReadNrrdHeader(p);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35714);
  }
  SECTION("nonzero line skip -> -35715")
  {
    const fs::path p = NrrdOutputDir() / "hdr_lineskip.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 3", "sizes: 2 2 2", "endian: little", "encoding: raw", "line skip: 1"}, std::vector<uint8_t>(2 * 2 * 2 * 2, 0));
    const auto result = nrrd::ReadNrrdHeader(p);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35715);
  }
  SECTION("byte skip: 0 and line skip: 0 are no-ops and accepted")
  {
    const fs::path p = NrrdOutputDir() / "hdr_skip_zero.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 3", "sizes: 2 2 2", "endian: little", "encoding: raw", "byte skip: 0", "line skip: 0"}, std::vector<uint8_t>(2 * 2 * 2 * 2, 0));
    REQUIRE(nrrd::ReadNrrdHeader(p).valid());
  }
  SECTION("negative size -> -35706 (not a wrapped huge allocation)")
  {
    // std::stoull would silently accept "-1" and wrap it to a huge value; ToUSize now
    // rejects a leading non-digit so the malformed size fails cleanly.
    const fs::path p = NrrdOutputDir() / "hdr_negsize.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 3", "sizes: -1 2 2", "endian: little", "encoding: raw"}, {});
    const auto result = nrrd::ReadNrrdHeader(p);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35706);
  }
}

TEST_CASE("ImageProcessing::NrrdUtilities: named 'space' -> LPS origin sign-flip", "[ImageProcessing][NrrdUtilities]")
{
  // itkNrrdImageIO.cxx converts a named space to LPS on read by sign-flipping the origin:
  // RAS negates X and Y, LAS negates X, LPS is unchanged. Verify our parser matches.
  auto readOrigin = [](const std::string& spaceLine) {
    const fs::path p = NrrdOutputDir() / "hdr_space_origin.nrrd";
    WriteNrrdRaw(p,
                 {"NRRD0004", "type: short", "dimension: 3", spaceLine, "sizes: 2 2 2", "space directions: (1,0,0) (0,1,0) (0,0,1)", "kinds: domain domain domain", "endian: little", "encoding: raw",
                  "space origin: (10,20,30)"},
                 std::vector<uint8_t>(2 * 2 * 2 * 2, 0));
    auto result = nrrd::ReadNrrdHeader(p);
    REQUIRE(result.valid());
    return result.value().origin;
  };

  SECTION("right-anterior-superior (RAS) negates X and Y")
  {
    const auto origin = readOrigin("space: right-anterior-superior");
    REQUIRE(origin[0] == Approx(-10.0f));
    REQUIRE(origin[1] == Approx(-20.0f));
    REQUIRE(origin[2] == Approx(30.0f));
  }
  SECTION("RAS abbreviation is matched case-insensitively")
  {
    const auto origin = readOrigin("space: RAS");
    REQUIRE(origin[0] == Approx(-10.0f));
    REQUIRE(origin[1] == Approx(-20.0f));
    REQUIRE(origin[2] == Approx(30.0f));
  }
  SECTION("left-anterior-superior (LAS) negates X only")
  {
    const auto origin = readOrigin("space: left-anterior-superior");
    REQUIRE(origin[0] == Approx(-10.0f));
    REQUIRE(origin[1] == Approx(20.0f));
    REQUIRE(origin[2] == Approx(30.0f));
  }
  SECTION("left-posterior-superior (LPS) leaves the origin unchanged")
  {
    const auto origin = readOrigin("space: left-posterior-superior");
    REQUIRE(origin[0] == Approx(10.0f));
    REQUIRE(origin[1] == Approx(20.0f));
    REQUIRE(origin[2] == Approx(30.0f));
  }
  SECTION("unrecognized space is rejected (-35716), matching teem")
  {
    const fs::path p = NrrdOutputDir() / "hdr_space_bad.nrrd";
    WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 3", "space: banana", "sizes: 2 2 2", "endian: little", "encoding: raw"}, std::vector<uint8_t>(2 * 2 * 2 * 2, 0));
    const auto result = nrrd::ReadNrrdHeader(p);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35716);
  }
}

namespace
{
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
} // namespace

TEST_CASE("ImageProcessing::NrrdDataReader: raw + gzip readBytes", "[ImageProcessing][NrrdUtilities]")
{
  std::vector<int16_t> values(4 * 3 * 2);
  for(size_t i = 0; i < values.size(); i++)
  {
    values[i] = static_cast<int16_t>(i) - 5;
  }
  const std::vector<uint8_t> raw = ToBytes(values);

  for(bool gzip : {false, true})
  {
    DYNAMIC_SECTION("encoding=" << (gzip ? "gzip" : "raw"))
    {
      const fs::path p = NrrdOutputDir() / (gzip ? "reader_gzip.nrrd" : "reader_raw.nrrd");
      const std::vector<std::string> hdr = {"NRRD0004", "type: short", "dimension: 3", "sizes: 4 3 2", "kinds: domain domain domain", "endian: little", gzip ? "encoding: gzip" : "encoding: raw"};
      WriteNrrdRaw(p, hdr, gzip ? GzipCompress(raw) : raw);

      auto mdResult = nrrd::ReadNrrdHeader(p);
      REQUIRE(mdResult.valid());
      auto readerResult = nrrd::NrrdDataReader::Create(mdResult.value());
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

// -----------------------------------------------------------------------------
// ReadNrrdFile algorithm-level tests. These exercise the scanline streamer,
// per-element byteswap, interleaved-component layout, and voxel cropping by
// driving the algorithm directly against a preflight-shaped DataStructure
// (ImageGeom + cell AttributeMatrix + typed DataArray). Full filter coverage
// (parity vs live ITK, OOC byte-match) lives in ReadImageTest, driving
// ReadImageFilter's NRRD path (which calls this same algorithm).
// -----------------------------------------------------------------------------
namespace
{
const DataPath k_GeomPath({"ImageGeometry"});
const std::string k_CellDataName = "CellData";
const std::string k_ArrayName = "ImageData";

DataPath ImageDataPath()
{
  return k_GeomPath.createChildPath(k_CellDataName).createChildPath(k_ArrayName);
}

// Builds a DataStructure with an ImageGeom + cell AttributeMatrix + a typed
// DataArray sized to hold destDims (X, Y, Z) tuples with `comp` components -
// mirrors what ReadImageFilter's NRRD preflight creates so the algorithm can be
// exercised directly. The cell AttributeMatrix tuple dims are [Z, Y, X].
DataStructure BuildNrrdTarget(DataType dataType, const std::array<usize, 3>& destDims, usize comp)
{
  DataStructure ds;
  const CreateImageGeometryAction::DimensionType dims = {destDims[0], destDims[1], destDims[2]};
  const CreateImageGeometryAction::OriginType origin = {0.0f, 0.0f, 0.0f};
  const CreateImageGeometryAction::SpacingType spacing = {1.0f, 1.0f, 1.0f};
  auto geomAction = CreateImageGeometryAction(k_GeomPath, dims, origin, spacing, k_CellDataName);
  REQUIRE(geomAction.apply(ds, IDataAction::Mode::Execute).valid());

  const std::vector<usize> tupleDims = {destDims[2], destDims[1], destDims[0]}; // [Z, Y, X]
  auto arrayAction = CreateArrayAction(dataType, tupleDims, {comp}, ImageDataPath());
  REQUIRE(arrayAction.apply(ds, IDataAction::Mode::Execute).valid());
  return ds;
}

Result<> RunReadNrrdFile(DataStructure& ds, const fs::path& inputFile, const CropGeometryParameter::ValueType& crop = {})
{
  ReadNrrdFileInputValues inputValues;
  inputValues.InputFilePath = inputFile;
  inputValues.ImageGeometryPath = k_GeomPath;
  inputValues.CellAttributeMatrixName = k_CellDataName;
  inputValues.ImageDataArrayName = k_ArrayName;
  inputValues.CroppingOptions = crop;

  const IFilter::MessageHandler messageHandler{};
  const std::atomic_bool shouldCancel{false};
  ReadNrrdFile algorithm(ds, messageHandler, shouldCancel, &inputValues);
  return algorithm();
}
} // namespace

TEST_CASE("ImageProcessing::ReadNrrdFile: scalar 3D streams X-fastest [Z,Y,X]", "[ImageProcessing][NrrdUtilities]")
{
  constexpr usize X = 4;
  constexpr usize Y = 3;
  constexpr usize Z = 2;
  std::vector<int16_t> fileValues(X * Y * Z);
  std::iota(fileValues.begin(), fileValues.end(), static_cast<int16_t>(-5));

  const fs::path p = NrrdOutputDir() / "algo_scalar3d.nrrd";
  WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 3", "sizes: 4 3 2", "kinds: domain domain domain", "endian: little", "encoding: raw"}, ToBytes(fileValues));

  DataStructure ds = BuildNrrdTarget(DataType::int16, {X, Y, Z}, 1);
  REQUIRE(RunReadNrrdFile(ds, p).valid());

  const auto& store = ds.getDataRefAs<DataArray<int16>>(ImageDataPath()).getDataStoreRef();
  REQUIRE(store.getNumberOfTuples() == X * Y * Z);
  REQUIRE(store.getNumberOfComponents() == 1);
  // No cropping => destination tuple order equals file order, so store[i] == fileValues[i].
  for(usize i = 0; i < fileValues.size(); i++)
  {
    REQUIRE(store.getValue(i) == fileValues[i]);
  }
}

TEST_CASE("ImageProcessing::ReadNrrdFile: big-endian data is byteswapped", "[ImageProcessing][NrrdUtilities]")
{
  constexpr usize X = 4;
  constexpr usize Y = 3;
  constexpr usize Z = 2;
  std::vector<int16_t> logical(X * Y * Z);
  for(usize i = 0; i < logical.size(); i++)
  {
    logical[i] = static_cast<int16_t>(static_cast<int>(i) * 257 - 1000);
  }

  // Serialize `logical` in big-endian byte order, independent of host endianness.
  // The reader will (little-endian host) swap it back, or (big-endian host) read it directly.
  std::vector<uint8_t> data(logical.size() * sizeof(int16_t));
  for(usize i = 0; i < logical.size(); i++)
  {
    const auto u = static_cast<uint16_t>(logical[i]);
    data[i * 2 + 0] = static_cast<uint8_t>((u >> 8) & 0xFFu); // high byte first
    data[i * 2 + 1] = static_cast<uint8_t>(u & 0xFFu);
  }

  const fs::path p = NrrdOutputDir() / "algo_bigendian.nrrd";
  WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 3", "sizes: 4 3 2", "kinds: domain domain domain", "endian: big", "encoding: raw"}, data);

  DataStructure ds = BuildNrrdTarget(DataType::int16, {X, Y, Z}, 1);
  REQUIRE(RunReadNrrdFile(ds, p).valid());

  const auto& store = ds.getDataRefAs<DataArray<int16>>(ImageDataPath()).getDataStoreRef();
  for(usize i = 0; i < logical.size(); i++)
  {
    REQUIRE(store.getValue(i) == logical[i]);
  }
}

TEST_CASE("ImageProcessing::ReadNrrdFile: RGB component axis interleaves components", "[ImageProcessing][NrrdUtilities]")
{
  constexpr usize comp = 3;
  constexpr usize X = 4;
  constexpr usize Y = 2; // Z == 1 (2D image)
  std::vector<uint8_t> fileValues(comp * X * Y);
  std::iota(fileValues.begin(), fileValues.end(), static_cast<uint8_t>(0));

  const fs::path p = NrrdOutputDir() / "algo_rgb.nrrd";
  WriteNrrdRaw(p, {"NRRD0004", "type: unsigned char", "dimension: 3", "space dimension: 2", "sizes: 3 4 2", "space directions: none (1,0) (0,1)", "kinds: vector domain domain", "encoding: raw"},
               fileValues);

  DataStructure ds = BuildNrrdTarget(DataType::uint8, {X, Y, 1}, comp);
  REQUIRE(RunReadNrrdFile(ds, p).valid());

  const auto& store = ds.getDataRefAs<DataArray<uint8>>(ImageDataPath()).getDataStoreRef();
  REQUIRE(store.getNumberOfTuples() == X * Y);
  REQUIRE(store.getNumberOfComponents() == comp);
  // Component axis is fastest in the file (comp, X, Y) which already matches the
  // interleaved simplnx layout, so store[i] == fileValues[i].
  for(usize i = 0; i < fileValues.size(); i++)
  {
    REQUIRE(store.getValue(i) == fileValues[i]);
  }
}

TEST_CASE("ImageProcessing::ReadNrrdFile: voxel subvolume crop keeps only the requested region", "[ImageProcessing][NrrdUtilities]")
{
  constexpr usize X = 4;
  constexpr usize Y = 3;
  constexpr usize Z = 2;
  std::vector<int16_t> fileValues(X * Y * Z);
  std::iota(fileValues.begin(), fileValues.end(), static_cast<int16_t>(0)); // value == flat file position

  const fs::path p = NrrdOutputDir() / "algo_crop.nrrd";
  WriteNrrdRaw(p, {"NRRD0004", "type: short", "dimension: 3", "sizes: 4 3 2", "kinds: domain domain domain", "endian: little", "encoding: raw"}, ToBytes(fileValues));

  CropGeometryParameter::ValueType crop;
  crop.type = CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume;
  crop.cropX = true;
  crop.xBoundVoxels = {1, 2};
  crop.cropY = true;
  crop.yBoundVoxels = {0, 1};
  crop.cropZ = true;
  crop.zBoundVoxels = {1, 1};

  // Cropped extent is X=2, Y=2, Z=1.
  DataStructure ds = BuildNrrdTarget(DataType::int16, {2, 2, 1}, 1);
  REQUIRE(RunReadNrrdFile(ds, p, crop).valid());

  const auto& store = ds.getDataRefAs<DataArray<int16>>(ImageDataPath()).getDataStoreRef();
  // Destination order z=1,y=0,x=1..2 then z=1,y=1,x=1..2; file pos = (z*Y + y)*X + x.
  const std::vector<int16_t> expected = {13, 14, 17, 18};
  REQUIRE(store.getNumberOfTuples() == expected.size());
  for(usize i = 0; i < expected.size(); i++)
  {
    REQUIRE(store.getValue(i) == expected[i]);
  }
}

TEST_CASE("ImageProcessing::ReadNrrdFile: detached .nhdr + sibling data file", "[ImageProcessing][NrrdUtilities]")
{
  constexpr usize X = 4;
  constexpr usize Y = 3;
  constexpr usize Z = 2;
  std::vector<int16_t> voxels(X * Y * Z);
  std::iota(voxels.begin(), voxels.end(), static_cast<int16_t>(-5));
  const std::vector<uint8_t> raw = ToBytes(voxels);

  const std::vector<std::string> commonHdr = {"NRRD0004", "type: short", "dimension: 3", "sizes: 4 3 2", "kinds: domain domain domain", "endian: little"};

  auto readAndCheck = [&](const fs::path& hdr) {
    auto mdResult = nrrd::ReadNrrdHeader(hdr);
    REQUIRE(mdResult.valid());
    REQUIRE(mdResult.value().detached == true);
    DataStructure ds = BuildNrrdTarget(DataType::int16, {X, Y, Z}, 1);
    REQUIRE(RunReadNrrdFile(ds, hdr).valid());
    const auto& store = ds.getDataRefAs<DataArray<int16>>(ImageDataPath()).getDataStoreRef();
    REQUIRE(store.getNumberOfTuples() == X * Y * Z);
    for(usize i = 0; i < voxels.size(); i++)
    {
      REQUIRE(store.getValue(i) == voxels[i]);
    }
  };

  SECTION("raw sibling round trip")
  {
    const fs::path hdr = NrrdOutputDir() / "detached_raw.nhdr";
    std::vector<std::string> lines = commonHdr;
    lines.push_back("encoding: raw");
    lines.push_back("data file: detached_raw.rawdata");
    WriteNhdrDetached(hdr, lines, NrrdOutputDir() / "detached_raw.rawdata", raw);
    readAndCheck(hdr);
  }
  SECTION("gzip sibling round trip")
  {
    const fs::path hdr = NrrdOutputDir() / "detached_gz.nhdr";
    std::vector<std::string> lines = commonHdr;
    lines.push_back("encoding: gzip");
    lines.push_back("data file: detached_gz.rawdata.gz");
    WriteNhdrDetached(hdr, lines, NrrdOutputDir() / "detached_gz.rawdata.gz", GzipCompress(raw));
    readAndCheck(hdr);
  }
  SECTION("single filename containing spaces is accepted (not misread as multi-file)")
  {
    const fs::path hdr = NrrdOutputDir() / "detached_spaces.nhdr";
    std::vector<std::string> lines = commonHdr;
    lines.push_back("encoding: raw");
    lines.push_back("data file: my scan data.rawdata");
    WriteNhdrDetached(hdr, lines, NrrdOutputDir() / "my scan data.rawdata", raw);
    readAndCheck(hdr);
  }
  SECTION("data file: LIST is rejected -> -35709")
  {
    const fs::path hdr = NrrdOutputDir() / "detached_list.nhdr";
    std::vector<std::string> lines = commonHdr;
    lines.push_back("encoding: raw");
    lines.push_back("data file: LIST");
    WriteNhdrHeaderOnly(hdr, lines);
    const auto result = nrrd::ReadNrrdHeader(hdr);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35709);
  }
  SECTION("missing sibling data file is rejected -> -35710")
  {
    const fs::path hdr = NrrdOutputDir() / "detached_missing.nhdr";
    std::vector<std::string> lines = commonHdr;
    lines.push_back("encoding: raw");
    lines.push_back("data file: does_not_exist_zzz.rawdata");
    WriteNhdrHeaderOnly(hdr, lines);
    const auto result = nrrd::ReadNrrdHeader(hdr);
    REQUIRE(result.invalid());
    REQUIRE(result.errors()[0].code == -35710);
  }
}
