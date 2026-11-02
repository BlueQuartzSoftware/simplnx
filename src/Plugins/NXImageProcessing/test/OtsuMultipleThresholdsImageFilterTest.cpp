#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/OtsuMultipleThresholdsImageFilter.hpp"

#include <array>

using namespace nx::core;

namespace
{
/**
 * @brief Builds an ImageGeom "Image Geometry" with cell AttributeMatrix "CellData" and a float32 cell
 *        array "Input" holding a deterministic MULTIMODAL distribution: the voxels are split into four
 *        equal blocks whose intensities cluster around 10, 70, 140 and 210 (with a small in-block spread),
 *        so multi-threshold Otsu results are meaningful. The value is a pure function of the flat index,
 *        so two DataStructures built with the same dimension hold byte-identical input.
 */
DataPath BuildMultimodalImage(DataStructure& ds, usize dim)
{
  const ShapeType cellShape = {dim, dim, dim};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dim, dim, dim});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<float32>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();

  const std::array<float32, 4> clusterBase = {10.0f, 70.0f, 140.0f, 210.0f};
  const usize total = ref.getSize();
  const usize quarter = (total >= 4) ? (total / 4) : 1;
  for(usize i = 0; i < total; ++i)
  {
    const usize cluster = std::min<usize>(3, i / quarter);
    ref.setValue(i, clusterBase[cluster] + static_cast<float32>(i % 8)); // clusters at ~10-17, 70-77, 140-147, 210-217
  }
  return inputPath;
}

DataPath BuildCompactInt16Image(DataStructure& ds, usize dim)
{
  const ShapeType cellShape = {dim, dim, dim};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dim, dim, dim});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<int16>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<int16>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();
  for(usize i = 0; i < ref.getSize(); ++i)
  {
    const int32 cluster = static_cast<int32>((i / 37) % 4);
    ref.setValue(i, static_cast<int16>(-360 + cluster * 240 + static_cast<int32>(i % 23)));
  }
  return inputPath;
}

void RunCompactInt16Otsu(DataStructure& ds, const DataPath& inputPath)
{
  OtsuMultipleThresholdsImageFilter filter;
  Arguments args;
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(uint8{3}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(uint8{7}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(uint32{128}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(true));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(true));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}
} // namespace

// -----------------------------------------------------------------------------
// (3) Validation: a data-independent guard must reject a threshold count that is 0 or >= the bin count
//     (which would otherwise make the backend return an empty threshold set and silently collapse the
//     image into a single class). preflight() must return an INVALID result with error code -8311.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: Invalid threshold count (-8311)", "[ImageProcessing][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath inputPath = BuildMultimodalImage(ds, 8);

  uint8 numThresholds = 1;
  uint32 numBins = 128;
  SECTION("thresholds >= bins")
  {
    numThresholds = 200;
    numBins = 128;
  }
  SECTION("thresholds == 0")
  {
    numThresholds = 0;
    numBins = 128;
  }

  OtsuMultipleThresholdsImageFilter filter;
  Arguments args;
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(numThresholds));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(uint8{0}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(numBins));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(false));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(false));

  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors()[0].code == -8311);
}

// -----------------------------------------------------------------------------
// (4) Classify tie rule (regression for the upper_bound -> lower_bound fix). On a CONSTANT image the single
//     Otsu threshold equals the constant value, so every voxel's value is exactly equal to the threshold. ITK's
//     ThresholdLabeler puts a value == threshold in the LOWER class, so every voxel must be labeled labelOffset
//     (0). The pre-fix std::upper_bound put them one class too high (all labeled 1).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: constant image -> lower class at the tie", "[ImageProcessing][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({4, 4, 4});
  const ShapeType cellShape = {4, 4, 4};
  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<float32>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();
  for(usize i = 0; i < ref.getSize(); ++i)
  {
    ref.setValue(i, 42.0f); // constant
  }

  OtsuMultipleThresholdsImageFilter filter;
  Arguments args;
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(uint8{1}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(uint8{0}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(uint32{128}));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(false));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(false));
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& outStore = ds.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    INFO("index=" << i);
    REQUIRE(outStore.getValue(i) == uint8{0}); // value == threshold -> lower class (NOT 1)
  }
}

// -----------------------------------------------------------------------------
// (5) Label offset: the classify op is `offset + idx` (OtsuMultipleThresholdsImageFilter.cpp classify lambda,
//     `return static_cast<U>(offset + idx);`). Every other test pins LabelOffset == 0; this asserts a non-zero
//     LabelOffset shifts EVERY output label by exactly that amount. Running the SAME multimodal image with
//     LabelOffset 0 and LabelOffset 10 must yield outputs that differ by a constant 10 at every voxel (labels
//     0,1,2,3 -> 10,11,12,13). A filter that IGNORED LabelOffset would produce identical outputs and fail here.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: LabelOffset shifts labels", "[ImageProcessing][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr uint8 k_Offset = 10;
  const auto runWithOffset = [](DataStructure& ds, const DataPath& inputPath, uint8 labelOffset) {
    OtsuMultipleThresholdsImageFilter filter;
    Arguments args;
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(uint8{3})); // 4 classes -> labels 0..3
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_LabelOffset_Key, std::make_any<uint8>(labelOffset));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(uint32{128}));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(false));
    args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(false));
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };

  DataStructure baseDs;
  const DataPath baseInput = BuildMultimodalImage(baseDs, 8);
  runWithOffset(baseDs, baseInput, uint8{0});

  DataStructure offsetDs;
  const DataPath offsetInput = BuildMultimodalImage(offsetDs, 8);
  runWithOffset(offsetDs, offsetInput, k_Offset);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& baseStore = baseDs.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  const auto& offsetStore = offsetDs.getDataRefAs<DataArray<uint8>>(outputPath).getDataStoreRef();
  REQUIRE(baseStore.getSize() == offsetStore.getSize());
  bool allShifted = true;
  bool anyNonZeroBase = false; // guard against a degenerate all-zero base making the shift trivially satisfied
  usize badIndex = 0;
  for(usize i = 0; i < baseStore.getSize(); ++i)
  {
    if(baseStore.getValue(i) != 0)
    {
      anyNonZeroBase = true;
    }
    if(static_cast<uint8>(baseStore.getValue(i) + k_Offset) != offsetStore.getValue(i))
    {
      allShifted = false;
      badIndex = i;
      break;
    }
  }
  if(!allShifted)
  {
    UNSCOPED_INFO(fmt::format("index={} base={} offset={} expected={}", badIndex, static_cast<uint32>(baseStore.getValue(badIndex)), static_cast<uint32>(offsetStore.getValue(badIndex)),
                              static_cast<uint32>(static_cast<uint8>(baseStore.getValue(badIndex) + k_Offset))));
  }
  REQUIRE(allShifted);
  REQUIRE(anyNonZeroBase);
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: real OOC int16 compact-range parity", "[.OOC][ImageProcessing][OtsuMultipleThresholdsImageFilter]")
{
  UnitTest::LoadPlugins();
  constexpr usize k_Dim = 19;
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});

  DataStructure inCoreDs;
  {
    const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
    const DataPath inputPath = BuildCompactInt16Image(inCoreDs, k_Dim);
    RunCompactInt16Otsu(inCoreDs, inputPath);
  }

  DataStructure oocDs;
  {
    const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceOutOfCore, 0);
    const DataPath inputPath = BuildCompactInt16Image(oocDs, k_Dim);
    REQUIRE(oocDs.getDataRefAs<IDataArray>(inputPath).getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
    RunCompactInt16Otsu(oocDs, inputPath);
    REQUIRE(oocDs.getDataRefAs<IDataArray>(outputPath).getIDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore);
  }

  const auto& inCoreOutput = inCoreDs.getDataRefAs<IDataArray>(outputPath);
  const auto& oocOutput = oocDs.getDataRefAs<IDataArray>(outputPath);
  UnitTest::CompareDataArrays<uint8>(inCoreOutput, oocOutput);
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the four ITK-sourced real-image md5 golden cases below. Mirrors ITKOtsuMultipleThresholdsImageTest.cpp:
// read the ITK case's input through OUR ITK-free reader, run OUR OtsuMultipleThresholdsImageFilter (AlwaysUInt8 label
// output) with the ITK case's params, then apply the (A) md5-validity-first golden (plan Sec.4). Pinned ForceInCore.
// -----------------------------------------------------------------------------
void RunOtsuItkGolden(const std::string& inputFile, const std::function<void(Arguments&)>& setParams, const std::string& committedMd5)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args);
  OtsuMultipleThresholdsImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("OtsuMultipleThresholdsImageFilter md5: ours='{}' committed='{}'", ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][OtsuMultipleThresholdsImageFilter]")
{
  RunOtsuItkGolden("RA-Short.nrrd", [](Arguments&) {}, "a9c3b0c0971c5cbda12b29db916451c6");
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: ITK real-image golden (two_on_float)", "[ImageProcessing][ItkGolden][OtsuMultipleThresholdsImageFilter]")
{
  RunOtsuItkGolden(
      "Ramp-Zero-One-Float.nrrd",
      [](Arguments& args) {
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(2));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(true));
      },
      "1ab20d3cd9a354b45ac07ec59c0413b3");
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: ITK real-image golden (three_on)", "[ImageProcessing][ItkGolden][OtsuMultipleThresholdsImageFilter]")
{
  RunOtsuItkGolden(
      "cthead1.png",
      [](Arguments& args) {
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(3));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfHistogramBins_Key, std::make_any<uint32>(256));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(true));
      },
      "b61c3f4e063fcdd24dba76227129ae34");
}

TEST_CASE("ImageProcessing::OtsuMultipleThresholdsImageFilter: ITK real-image golden (valley_emphasis)", "[ImageProcessing][ItkGolden][OtsuMultipleThresholdsImageFilter]")
{
  RunOtsuItkGolden(
      "cthead1.png",
      [](Arguments& args) {
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_NumberOfThresholds_Key, std::make_any<uint8>(3));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ValleyEmphasis_Key, std::make_any<bool>(true));
        args.insertOrAssign(OtsuMultipleThresholdsImageFilter::k_ReturnBinMidpoint_Key, std::make_any<bool>(true));
      },
      "fb65e730472c8001185f355fb626ca3f");
}
