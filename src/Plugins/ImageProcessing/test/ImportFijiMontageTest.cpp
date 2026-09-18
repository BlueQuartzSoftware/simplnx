#include <catch2/catch.hpp>

#include "ImageProcessing/Filters/ImportFijiMontageFilter.hpp"
#include "ImageProcessing/ImageProcessing_test_dirs.hpp"
#include "ImageProcessing/utils/FijiMontageUtilities.hpp"

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOFactory.hpp"
#include "simplnx/Utilities/ImageIO/ImageMetadata.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
fs::path FijiOutputDir()
{
  fs::path dir = fs::path(std::string(nx::core::unit_test::k_BinaryTestOutputDir.view())) / "ImportFijiMontage";
  fs::create_directories(dir);
  return dir;
}

// Writes a minimal Fiji TileConfiguration file next to (nonexistent) tile paths.
void WriteConfig(const fs::path& path, const std::vector<std::string>& lines)
{
  std::ofstream ofs(path, std::ios::binary);
  REQUIRE(ofs.is_open());
  for(const auto& l : lines)
  {
    ofs << l << "\n";
  }
}

// Writes a tiny grayscale uint8 tile (w*h, single component) via the ImageProcessing image-write path.
void WriteGrayTile(const fs::path& p, usize w, usize h, uint8 fill)
{
  auto ioResult = nx::core::CreateImageIO(p);
  REQUIRE(ioResult.valid());
  nx::core::ImageMetadata md;
  md.width = w;
  md.height = h;
  md.numComponents = 1;
  md.dataType = DataType::uint8;
  std::vector<uint8> buffer(w * h, fill);
  auto writeResult = ioResult.value()->writePixelData(p, buffer, md);
  REQUIRE(writeResult.valid());
}

// Writes a tiny 3-component (RGB) uint8 tile with every pixel set to (r, g, b).
void WriteRgbTile(const fs::path& p, usize w, usize h, uint8 r, uint8 g, uint8 b)
{
  auto ioResult = nx::core::CreateImageIO(p);
  REQUIRE(ioResult.valid());
  nx::core::ImageMetadata md;
  md.width = w;
  md.height = h;
  md.numComponents = 3;
  md.dataType = DataType::uint8;
  std::vector<uint8> buffer;
  buffer.reserve(w * h * 3);
  for(usize i = 0; i < w * h; i++)
  {
    buffer.push_back(r);
    buffer.push_back(g);
    buffer.push_back(b);
  }
  auto writeResult = ioResult.value()->writePixelData(p, buffer, md);
  REQUIRE(writeResult.valid());
}

// Luminosity grayscale of a uniform RGB pixel using ConvertColorToGrayScale's exact math
// (roundf of the weighted sum, then a narrowing cast to uint8), so expected values match bit-for-bit.
uint8 ExpectedLuminosity(uint8 r, uint8 g, uint8 b, const std::vector<float32>& w)
{
  return static_cast<uint8>(static_cast<int32>(std::roundf(r * w[0] + g * w[1] + b * w[2])));
}

// Returns the DataPath in `paths` whose terminal name equals `name`, failing the test if absent.
DataPath FindGeomByName(const std::vector<DataPath>& paths, const std::string& name)
{
  for(const auto& p : paths)
  {
    if(p.getTargetName() == name)
    {
      return p;
    }
  }
  FAIL(fmt::format("no geometry named '{}' was created", name));
  return {};
}

// Builds the filter's default-style args for a given config file.
Arguments MakeFijiArgs(const fs::path& config, bool parentGroup, bool changeOrigin, const std::vector<float32>& origin)
{
  Arguments args;
  args.insertOrAssign(ImportFijiMontageFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(config));
  args.insertOrAssign(ImportFijiMontageFilter::k_LengthUnit_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(IGeometry::LengthUnit::Micrometer)));
  args.insertOrAssign(ImportFijiMontageFilter::k_ChangeOrigin_Key, std::make_any<bool>(changeOrigin));
  args.insertOrAssign(ImportFijiMontageFilter::k_Origin_Key, std::make_any<VectorFloat32Parameter::ValueType>(origin));
  args.insertOrAssign(ImportFijiMontageFilter::k_ConvertToGrayScale_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportFijiMontageFilter::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{0.2125f, 0.7154f, 0.0721f}));
  args.insertOrAssign(ImportFijiMontageFilter::k_ChangeDataType_Key, std::make_any<bool>(false));
  args.insertOrAssign(ImportFijiMontageFilter::k_ImageDataType_Key, std::make_any<ChoicesParameter::ValueType>(0));
  args.insertOrAssign(ImportFijiMontageFilter::k_ParentDataGroup_Key, std::make_any<bool>(parentGroup));
  args.insertOrAssign(ImportFijiMontageFilter::k_DataGroupName_Key, std::make_any<std::string>("Zen DataGroup"));
  args.insertOrAssign(ImportFijiMontageFilter::k_DataContainerPath_Key, std::make_any<std::string>("Mosaic-"));
  args.insertOrAssign(ImportFijiMontageFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("Tile Data"));
  args.insertOrAssign(ImportFijiMontageFilter::k_ImageDataArrayName_Key, std::make_any<std::string>("Image"));
  return args;
}

// Real-data (Task 3) helpers: the shipped Zeiss Zen montage + its exemplar.
const std::string k_SmallZeissZenDir = fmt::format("{}/fiji_montage/small_zeiss_zen", nx::core::unit_test::k_TestFilesDir);
const DataPath k_ZenGroupPath({"Zen DataGroup"});

// Element-for-element compare two DataArrays of identical DataType.
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
      UNSCOPED_INFO(fmt::format("array element mismatch at {}: ours={}, itk={}", i, sa[i], sb[i]));
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

TEST_CASE("ImageProcessing::FijiMontageUtilities: parse 2x2 TileConfiguration", "[ImageProcessing][ImportFijiMontageFilter]")
{
  const fs::path cfg = FijiOutputDir() / "TileConfiguration.txt";
  WriteConfig(cfg, {"# Define the number of dimensions we are working on", "dim = 2", "", "# Define the image coordinates", "a.tif; ; (0.0, 0.0)", "b.tif; ; (700.27, 1.48)",
                    "c.tif; ; (696.0, 700.19)", "d.tif; ; (-0.33, 698.02)"});

  auto result = fiji::ParseTileConfiguration(cfg);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  auto tiles = std::move(result.value());
  REQUIRE(tiles.size() == 4);

  // File paths are resolved relative to the config's parent directory.
  REQUIRE(tiles[0].filePath == cfg.parent_path() / "a.tif");
  REQUIRE(tiles[1].origin[0] == Approx(700.27f));
  REQUIRE(tiles[1].origin[1] == Approx(1.48f));
  REQUIRE(tiles[3].origin[0] == Approx(-0.33f));
  REQUIRE(tiles[0].origin[2] == Approx(0.0f)); // 2D → Z=0
}

TEST_CASE("ImageProcessing::FijiMontageUtilities: name assignment + origin rebase", "[ImageProcessing][ImportFijiMontageFilter]")
{
  std::vector<fiji::FijiTile> tiles;
  tiles.push_back({fs::path("dir") / "a.tif", FloatVec3{10.0f, 20.0f, 0.0f}, ""});
  tiles.push_back({fs::path("dir") / "b.tif", FloatVec3{-5.0f, 40.0f, 0.0f}, ""});

  fiji::AssignTileNames(tiles, "Mosaic-");
  REQUIRE(tiles[0].imageName == "Mosaic-a");
  REQUIRE(tiles[1].imageName == "Mosaic-b");

  // Min corner is (-5, 20). Rebase so the min corner lands on the user origin (100, 100).
  fiji::RebaseOrigins(tiles, FloatVec3{100.0f, 100.0f, 0.0f});
  REQUIRE(tiles[0].origin[0] == Approx(115.0f)); // 10 - (-5-100) = 10 + 105
  REQUIRE(tiles[0].origin[1] == Approx(100.0f)); // 20 - (20-100)  = 20 + 80
  REQUIRE(tiles[1].origin[0] == Approx(100.0f)); // the min-corner tile sits exactly on the user origin
  REQUIRE(tiles[1].origin[1] == Approx(120.0f));
}

TEST_CASE("ImageProcessing::FijiMontageUtilities: rejects bad config", "[ImageProcessing][ImportFijiMontageFilter]")
{
  SECTION("missing file")
  {
    REQUIRE(fiji::ParseTileConfiguration(FijiOutputDir() / "does_not_exist.txt").invalid());
  }
  SECTION("no data header → zero tiles")
  {
    const fs::path cfg = FijiOutputDir() / "no_header.txt";
    WriteConfig(cfg, {"dim = 2", "a.tif; ; (0.0, 0.0)"}); // no "# Define the image coordinates" marker
    REQUIRE(fiji::ParseTileConfiguration(cfg).invalid());
  }
}

TEST_CASE("ImageProcessing::ImportFijiMontageFilter: synthetic 2x2 montage", "[ImageProcessing][ImportFijiMontageFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = FijiOutputDir();
  WriteGrayTile(dir / "a.png", 3, 2, 10);
  WriteGrayTile(dir / "b.png", 3, 2, 20);
  WriteGrayTile(dir / "c.png", 3, 2, 30);
  WriteGrayTile(dir / "d.png", 3, 2, 40);

  const fs::path cfg = dir / "TileConfiguration_synth.txt";
  WriteConfig(cfg, {"dim = 2", "# Define the image coordinates", "a.png; ; (0.0, 0.0)", "b.png; ; (100.0, 0.0)", "c.png; ; (0.0, 50.0)", "d.png; ; (100.0, 50.0)"});

  ImportFijiMontageFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeFijiArgs(cfg, /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath groupPath({"Zen DataGroup"});
  std::vector<DataPath> geoms = GetAllChildDataPaths(dataStructure, groupPath, DataObject::Type::ImageGeom).value();
  REQUIRE(geoms.size() == 4);

  struct Expected
  {
    std::string name;
    FloatVec3 origin;
    uint8 fill;
  };
  const std::vector<Expected> expected = {{"Mosaic-a", {0.0f, 0.0f, 0.0f}, 10}, {"Mosaic-b", {100.0f, 0.0f, 0.0f}, 20}, {"Mosaic-c", {0.0f, 50.0f, 0.0f}, 30}, {"Mosaic-d", {100.0f, 50.0f, 0.0f}, 40}};

  for(const auto& e : expected)
  {
    const DataPath geomPath = groupPath.createChildPath(e.name);
    const auto& geom = dataStructure.getDataRefAs<ImageGeom>(geomPath);
    REQUIRE(geom.getDimensions() == SizeVec3{3, 2, 1});
    REQUIRE(geom.getOrigin()[0] == Approx(e.origin[0]));
    REQUIRE(geom.getOrigin()[1] == Approx(e.origin[1]));

    const auto& arr = dataStructure.getDataRefAs<UInt8Array>(geomPath.createChildPath("Tile Data").createChildPath("Image"));
    REQUIRE(arr.getNumberOfTuples() == 6); // 3*2
    for(usize i = 0; i < arr.getNumberOfTuples(); i++)
    {
      REQUIRE(arr[i] == e.fill);
    }
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ImportFijiMontageFilter: change-origin rebase + no parent group", "[ImageProcessing][ImportFijiMontageFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = FijiOutputDir();
  WriteGrayTile(dir / "a.png", 2, 2, 1);
  WriteGrayTile(dir / "b.png", 2, 2, 2);
  const fs::path cfg = dir / "TileConfiguration_rebase.txt";
  WriteConfig(cfg, {"dim = 2", "# Define the image coordinates", "a.png; ; (10.0, 20.0)", "b.png; ; (-5.0, 40.0)"}); // min corner = (-5, 20)

  ImportFijiMontageFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeFijiArgs(cfg, /*parentGroup=*/false, /*changeOrigin=*/true, {100.0f, 100.0f, 0.0f});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // No parent group → geoms sit at the root.
  const auto& geomA = dataStructure.getDataRefAs<ImageGeom>(DataPath({"Mosaic-a"}));
  const auto& geomB = dataStructure.getDataRefAs<ImageGeom>(DataPath({"Mosaic-b"}));
  REQUIRE(geomA.getOrigin()[0] == Approx(115.0f)); // 10 - (-5-100)
  REQUIRE(geomA.getOrigin()[1] == Approx(100.0f)); // 20 - (20-100)
  REQUIRE(geomB.getOrigin()[0] == Approx(100.0f)); // min-corner tile lands on the user origin
  REQUIRE(geomB.getOrigin()[1] == Approx(120.0f));
}

TEST_CASE("ImageProcessing::ImportFijiMontageFilter: exemplar golden (real Zeiss montage)", "[ImageProcessing][ImportFijiMontageFilter]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "fiji_montage.tar.gz", "fiji_montage");

  DataStructure exemplar = UnitTest::LoadDataStructure(fs::path(fmt::format("{}/fiji_montage/2x2_fiji_montage_test.dream3d", nx::core::unit_test::k_TestFilesDir)));

  ImportFijiMontageFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeFijiArgs(fs::path(k_SmallZeissZenDir) / "TileConfiguration.registered.txt", /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  std::vector<DataPath> generated = GetAllChildDataPaths(dataStructure, k_ZenGroupPath, DataObject::Type::ImageGeom).value();
  std::vector<DataPath> exemplarGeoms = GetAllChildDataPaths(exemplar, k_ZenGroupPath, DataObject::Type::ImageGeom).value();
  REQUIRE(generated.size() == 4); // the shipped montage is a 2x2 (m01/m02/m13/m14) mosaic
  REQUIRE(exemplarGeoms.size() == 4);

  // Pair geometries BY NAME (not bare index), then compare geometry AND every tile's pixel array
  // against the exemplar .dream3d's "Tile Data/Image" arrays -- a full byte-for-byte golden check.
  for(const DataPath& genGeomPath : generated)
  {
    const DataPath exGeomPath = FindGeomByName(exemplarGeoms, genGeomPath.getTargetName());
    UnitTest::CompareImageGeometry(exemplar.getDataAs<ImageGeom>(exGeomPath), dataStructure.getDataAs<ImageGeom>(genGeomPath));

    const DataPath genArr = genGeomPath.createChildPath("Tile Data").createChildPath("Image");
    const DataPath exArr = exGeomPath.createChildPath("Tile Data").createChildPath("Image");
    RequireArraysIdentical(dataStructure.getDataRefAs<IDataArray>(genArr), exemplar.getDataRefAs<IDataArray>(exArr));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ImportFijiMontageFilter: live-ITK read parity", "[ImageProcessing][ImportFijiMontageFilter][itk-parity]")
{
  UnitTest::LoadPlugins();
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "fiji_montage.tar.gz", "fiji_montage");

  auto app = Application::GetOrCreateInstance();
  const Uuid legacyUuid = *Uuid::FromString("4c48ea16-13ef-4281-89cf-315be5fb857d");
  auto legacy = app->getFilterList()->createFilter(legacyUuid);
  if(legacy == nullptr)
  {
    WARN("ITKImageProcessing plugin not loaded (legacy Fiji UUID 4c48ea16... unavailable); skipping live-ITK Fiji parity gate.");
    return;
  }

  // The legacy filter delegates tile reads to the ITK image reader, which cannot write OOC stores.
  const UnitTest::PreferencesSentinel sentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const fs::path config = fs::path(k_SmallZeissZenDir) / "TileConfiguration.registered.txt";

  // Ours.
  DataStructure oursDs;
  ImportFijiMontageFilter ours;
  Arguments oursArgs = MakeFijiArgs(config, /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});
  auto oursPre = ours.preflight(oursDs, oursArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(oursPre.outputActions);
  auto oursExec = ours.execute(oursDs, oursArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(oursExec.result);

  // Legacy ITK oracle — same param keys (verbatim match) drive it.
  DataStructure itkDs;
  Arguments itkArgs = MakeFijiArgs(config, /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});
  auto itkExec = legacy->execute(itkDs, itkArgs); // execute() runs preflight internally
  SIMPLNX_RESULT_REQUIRE_VALID(itkExec.result);

  std::vector<DataPath> oursGeoms = GetAllChildDataPaths(oursDs, k_ZenGroupPath, DataObject::Type::ImageGeom).value();
  std::vector<DataPath> itkGeoms = GetAllChildDataPaths(itkDs, k_ZenGroupPath, DataObject::Type::ImageGeom).value();
  REQUIRE(oursGeoms.size() == itkGeoms.size());
  for(usize i = 0; i < oursGeoms.size(); i++)
  {
    const auto& og = oursDs.getDataRefAs<ImageGeom>(oursGeoms[i]);
    const auto& ig = itkDs.getDataRefAs<ImageGeom>(itkGeoms[i]);
    REQUIRE(og.getDimensions() == ig.getDimensions());
    for(usize k = 0; k < 3; k++)
    {
      REQUIRE(og.getOrigin()[k] == Approx(ig.getOrigin()[k]).margin(1e-4));
    }
    // Byte-identical pixel array parity (both decode the .tif through libtiff).
    const DataPath oursArr = oursGeoms[i].createChildPath("Tile Data").createChildPath("Image");
    const DataPath itkArr = itkGeoms[i].createChildPath("Tile Data").createChildPath("Image");
    RequireArraysIdentical(oursDs.getDataRefAs<IDataArray>(oursArr), itkDs.getDataRefAs<IDataArray>(itkArr));
  }
}

TEST_CASE("ImageProcessing::ImportFijiMontageFilter: grayscale conversion", "[ImageProcessing][ImportFijiMontageFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = FijiOutputDir();
  // Uniform RGB tiles so every output pixel collapses to a single known luminosity value.
  WriteRgbTile(dir / "gray_a.png", 3, 2, 200, 100, 50);
  WriteRgbTile(dir / "gray_b.png", 3, 2, 0, 0, 0);
  WriteRgbTile(dir / "gray_c.png", 3, 2, 255, 255, 255);
  WriteRgbTile(dir / "gray_d.png", 3, 2, 10, 20, 30);

  const fs::path cfg = dir / "TileConfiguration_gray.txt";
  WriteConfig(cfg, {"dim = 2", "# Define the image coordinates", "gray_a.png; ; (0.0, 0.0)", "gray_b.png; ; (100.0, 0.0)", "gray_c.png; ; (0.0, 50.0)", "gray_d.png; ; (100.0, 50.0)"});

  const std::vector<float32> weights{0.2125f, 0.7154f, 0.0721f};

  ImportFijiMontageFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeFijiArgs(cfg, /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});
  args.insertOrAssign(ImportFijiMontageFilter::k_ConvertToGrayScale_Key, std::make_any<bool>(true));
  args.insertOrAssign(ImportFijiMontageFilter::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>(weights));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath groupPath({"Zen DataGroup"});
  struct Expected
  {
    std::string name;
    uint8 gray;
  };
  const std::vector<Expected> expected = {{"Mosaic-gray_a", ExpectedLuminosity(200, 100, 50, weights)},
                                          {"Mosaic-gray_b", ExpectedLuminosity(0, 0, 0, weights)},
                                          {"Mosaic-gray_c", ExpectedLuminosity(255, 255, 255, weights)},
                                          {"Mosaic-gray_d", ExpectedLuminosity(10, 20, 30, weights)}};

  for(const auto& e : expected)
  {
    const DataPath arrPath = groupPath.createChildPath(e.name).createChildPath("Tile Data").createChildPath("Image");
    // After grayscale the color array is deleted and gray<name> renamed back: final array is 1-component uint8 "Image".
    const auto& arr = dataStructure.getDataRefAs<UInt8Array>(arrPath);
    REQUIRE(arr.getNumberOfComponents() == 1);
    REQUIRE(arr.getNumberOfTuples() == 6); // 3*2
    for(usize i = 0; i < arr.getNumberOfTuples(); i++)
    {
      REQUIRE(arr[i] == e.gray);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ImportFijiMontageFilter: set image data type (uint8 -> uint16)", "[ImageProcessing][ImportFijiMontageFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = FijiOutputDir();
  WriteGrayTile(dir / "dt_a.png", 2, 2, 10);
  WriteGrayTile(dir / "dt_b.png", 2, 2, 255);
  const fs::path cfg = dir / "TileConfiguration_datatype.txt";
  WriteConfig(cfg, {"dim = 2", "# Define the image coordinates", "dt_a.png; ; (0.0, 0.0)", "dt_b.png; ; (50.0, 0.0)"});

  ImportFijiMontageFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeFijiArgs(cfg, /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});
  args.insertOrAssign(ImportFijiMontageFilter::k_ChangeDataType_Key, std::make_any<bool>(true));
  args.insertOrAssign(ImportFijiMontageFilter::k_ImageDataType_Key, std::make_any<ChoicesParameter::ValueType>(1)); // uint16

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath groupPath({"Zen DataGroup"});
  // Reader rescales by (clamp(src,0,srcMax)/srcMax)*destMax with srcMax=255, destMax=65535, so src*257.
  struct Expected
  {
    std::string name;
    uint16 value;
  };
  const std::vector<Expected> expected = {{"Mosaic-dt_a", static_cast<uint16>(10 * 257)}, {"Mosaic-dt_b", static_cast<uint16>(65535)}};
  for(const auto& e : expected)
  {
    const DataPath arrPath = groupPath.createChildPath(e.name).createChildPath("Tile Data").createChildPath("Image");
    const auto& arr = dataStructure.getDataRefAs<UInt16Array>(arrPath);
    REQUIRE(arr.getNumberOfComponents() == 1);
    REQUIRE(arr.getNumberOfTuples() == 4); // 2*2
    for(usize i = 0; i < arr.getNumberOfTuples(); i++)
    {
      REQUIRE(arr[i] == e.value);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Fiji has no ForceOutOfCore coverage (NRRD/MHA both do). Drive the montage-level array juggling --
// per-tile CreateArray, the ConvertColorToGrayScale sub-filter, and the removeData + rename dance --
// with storage forced out-of-core. In an OOC super-build the stores are disk-backed; in an OOC-free
// build ForceOutOfCore is inert and this degrades to in-memory, but the logic is still validated.
TEST_CASE("ImageProcessing::ImportFijiMontageFilter: forced out-of-core grayscale montage", "[ImageProcessing][ImportFijiMontageFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = FijiOutputDir();
  WriteRgbTile(dir / "ooc_a.png", 4, 3, 200, 100, 50);
  WriteRgbTile(dir / "ooc_b.png", 4, 3, 10, 20, 30);
  WriteRgbTile(dir / "ooc_c.png", 4, 3, 255, 128, 64);
  WriteRgbTile(dir / "ooc_d.png", 4, 3, 30, 60, 90);
  const fs::path cfg = dir / "TileConfiguration_ooc.txt";
  WriteConfig(cfg, {"dim = 2", "# Define the image coordinates", "ooc_a.png; ; (0.0, 0.0)", "ooc_b.png; ; (100.0, 0.0)", "ooc_c.png; ; (0.0, 50.0)", "ooc_d.png; ; (100.0, 50.0)"});

  const std::vector<float32> weights{0.2125f, 0.7154f, 0.0721f};

  // Small large-data threshold + ForceOutOfCore routes the created stores to disk when available.
  const UnitTest::PreferencesSentinel sentinel(nx::core::DataStorageMode::ForceOutOfCore, 1024);

  ImportFijiMontageFilter filter;
  DataStructure dataStructure;
  Arguments args = MakeFijiArgs(cfg, /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});
  args.insertOrAssign(ImportFijiMontageFilter::k_ConvertToGrayScale_Key, std::make_any<bool>(true));
  args.insertOrAssign(ImportFijiMontageFilter::k_ColorWeights_Key, std::make_any<VectorFloat32Parameter::ValueType>(weights));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath groupPath({"Zen DataGroup"});
  std::vector<DataPath> geoms = GetAllChildDataPaths(dataStructure, groupPath, DataObject::Type::ImageGeom).value();
  REQUIRE(geoms.size() == 4);

  struct Expected
  {
    std::string name;
    uint8 gray;
  };
  const std::vector<Expected> expected = {{"Mosaic-ooc_a", ExpectedLuminosity(200, 100, 50, weights)},
                                          {"Mosaic-ooc_b", ExpectedLuminosity(10, 20, 30, weights)},
                                          {"Mosaic-ooc_c", ExpectedLuminosity(255, 128, 64, weights)},
                                          {"Mosaic-ooc_d", ExpectedLuminosity(30, 60, 90, weights)}};
  for(const auto& e : expected)
  {
    const DataPath arrPath = groupPath.createChildPath(e.name).createChildPath("Tile Data").createChildPath("Image");
    const auto& arr = dataStructure.getDataRefAs<UInt8Array>(arrPath);
    REQUIRE(arr.getNumberOfComponents() == 1);
    REQUIRE(arr.getNumberOfTuples() == 12); // 4*3
    for(usize i = 0; i < arr.getNumberOfTuples(); i++)
    {
      REQUIRE(arr[i] == e.gray);
    }
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// Exercises the per-instance metadata cache: a second preflight/execute on the SAME filter instance
// with unchanged inputs must produce identical, correct output (a stale cache would corrupt it), and
// a parameter edit (the image-geometry prefix) between runs must be honored (the cache must not serve
// stale, prefix-dependent names -- only the file-derived parse + metadata are cached).
TEST_CASE("ImageProcessing::ImportFijiMontageFilter: metadata cache correctness across repeated runs", "[ImageProcessing][ImportFijiMontageFilter]")
{
  UnitTest::LoadPlugins();

  const fs::path dir = FijiOutputDir();
  WriteGrayTile(dir / "cache_a.png", 3, 2, 11);
  WriteGrayTile(dir / "cache_b.png", 3, 2, 22);
  const fs::path cfg = dir / "TileConfiguration_cache.txt";
  WriteConfig(cfg, {"dim = 2", "# Define the image coordinates", "cache_a.png; ; (0.0, 0.0)", "cache_b.png; ; (100.0, 0.0)"});

  ImportFijiMontageFilter filter; // one instance -> its cache persists across the calls below

  auto runInto = [&](DataStructure& ds, Arguments& args) {
    auto pre = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(pre.outputActions);
    auto exec = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(exec.result);
  };

  // First run (cold cache).
  DataStructure ds1;
  Arguments args1 = MakeFijiArgs(cfg, /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});
  runInto(ds1, args1);

  // Second run (warm cache), identical inputs -> identical output.
  DataStructure ds2;
  Arguments args2 = MakeFijiArgs(cfg, /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});
  runInto(ds2, args2);

  const DataPath groupPath({"Zen DataGroup"});
  for(const std::string& name : {std::string("Mosaic-cache_a"), std::string("Mosaic-cache_b")})
  {
    const DataPath arrPath = groupPath.createChildPath(name).createChildPath("Tile Data").createChildPath("Image");
    const auto& a1 = ds1.getDataRefAs<UInt8Array>(arrPath);
    const auto& a2 = ds2.getDataRefAs<UInt8Array>(arrPath);
    REQUIRE(a1.getNumberOfTuples() == a2.getNumberOfTuples());
    for(usize i = 0; i < a1.getNumberOfTuples(); i++)
    {
      REQUIRE(a1[i] == a2[i]);
    }
  }

  // Third run on the SAME instance with a different prefix: the cache must not serve stale names.
  DataStructure ds3;
  Arguments args3 = MakeFijiArgs(cfg, /*parentGroup=*/true, /*changeOrigin=*/false, {0.0f, 0.0f, 0.0f});
  args3.insertOrAssign(ImportFijiMontageFilter::k_DataContainerPath_Key, std::make_any<std::string>("Tile-"));
  runInto(ds3, args3);

  std::vector<DataPath> geoms = GetAllChildDataPaths(ds3, groupPath, DataObject::Type::ImageGeom).value();
  REQUIRE(geoms.size() == 2);
  (void)FindGeomByName(geoms, "Tile-cache_a"); // new prefix honored despite the warm cache
  (void)FindGeomByName(geoms, "Tile-cache_b");
  const auto& renamed = ds3.getDataRefAs<UInt8Array>(groupPath.createChildPath("Tile-cache_a").createChildPath("Tile Data").createChildPath("Image"));
  for(usize i = 0; i < renamed.getNumberOfTuples(); i++)
  {
    REQUIRE(renamed[i] == 11);
  }
}
