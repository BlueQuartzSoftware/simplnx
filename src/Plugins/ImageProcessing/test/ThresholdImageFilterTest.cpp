#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/FilterList.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "ImageProcessing/Filters/ThresholdImageFilter.hpp"

#include "ItkGoldenTestUtils.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <iostream>
#include <memory>

using namespace nx::core;

namespace
{
// Routes filter messages (including the engine's throttled progress) to stdout so long-running OOC
// runs show live activity instead of appearing to hang.
IFilter::MessageHandler MakeConsoleMessageHandler(std::string label)
{
  return IFilter::MessageHandler{[messageLabel = std::move(label)](const IFilter::Message& msg) { std::cout << "  [" << messageLabel << "] " << msg.message << std::endl; }};
}

// Builds an ImageGeom "Image Geometry" with a cell AttributeMatrix "CellData" and a float32 cell
// array "Input" filled with the ramp value (tupleIndex), returning the input array DataPath.
// Fills via bulk copyFromBuffer in bounded chunks so it is fast both in-core and out-of-core (a
// per-element setValue loop makes millions of individual OOC writes and appears to hang). Allocates
// only a fixed chunk buffer, never one proportional to the array size.
DataPath BuildRampImage(DataStructure& ds, usize dimX, usize dimY, usize dimZ)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<float32>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();

  const usize total = ref.getSize();
  std::cout << "  [build] filling " << dimX << "x" << dimY << "x" << dimZ << " ramp (" << total << " values) via bulk I/O..." << std::endl;
  constexpr usize k_ChunkValues = 65536;
  auto buffer = std::make_unique<float32[]>(k_ChunkValues);
  for(usize start = 0; start < total; start += k_ChunkValues)
  {
    const usize count = std::min(k_ChunkValues, total - start);
    for(usize i = 0; i < count; ++i)
    {
      buffer[i] = static_cast<float32>(start + i);
    }
    Result<> writeResult = ref.copyFromBuffer(start, nonstd::span<const float32>(buffer.get(), count));
    SIMPLNX_RESULT_REQUIRE_VALID(writeResult);
  }
  std::cout << "  [build] done" << std::endl;
  return inputPath;
}
} // namespace

TEST_CASE("ImageProcessing::ThresholdImageFilter: Small Correctness", "[ImageProcessing][ThresholdImageFilter]")
{
  UnitTest::LoadPlugins();
  // The out-of-core store path is retired to the generic SimplnxOoc store tests; this keeps the unique
  // independent computed-expected oracle (every voxel checked against the threshold semantics) as a plain
  // in-memory ForceInCore case.
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  std::cout << "\n[TEST] ThresholdImageFilter: Small Correctness (in-memory)" << std::endl;

  constexpr usize kDim = 10;
  DataStructure ds;
  const DataPath inputPath = BuildRampImage(ds, kDim, kDim, kDim);

  const float64 lower = 100.0;
  const float64 upper = 500.0;
  const float64 outside = -1.0;

  ThresholdImageFilter filter;
  Arguments args;
  args.insertOrAssign(ThresholdImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(ThresholdImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(ThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(ThresholdImageFilter::k_Lower_Key, std::make_any<float64>(lower));
  args.insertOrAssign(ThresholdImageFilter::k_Upper_Key, std::make_any<float64>(upper));
  args.insertOrAssign(ThresholdImageFilter::k_OutsideValue_Key, std::make_any<float64>(outside));

  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args, nullptr, MakeConsoleMessageHandler("threshold"));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(outputPath));
  auto& outStore = ds.getDataRefAs<Float32Array>(outputPath).getDataStoreRef();
  std::cout << "  [verify] checking " << outStore.getSize() << " output values..." << std::endl;
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    const auto v = static_cast<float32>(i);
    const float32 expected = (v < static_cast<float32>(lower) || v > static_cast<float32>(upper)) ? static_cast<float32>(outside) : v;
    REQUIRE(outStore.getValue(i) == expected);
  }
  UnitTest::CheckArraysInheritTupleDims(ds);
}

TEST_CASE("ImageProcessing::ThresholdImageFilter: Legacy parity", "[ImageProcessing][ThresholdImageFilter]")
{
  UnitTest::LoadPlugins();
  // Force in-memory storage for BOTH filters: this test runs the legacy ITK filter, which hard-errors
  // on out-of-core data (error -2002). ForceInCore keeps arrays in RAM in every build configuration,
  // including the DREAM3D-NX OOC build (whose default routes arrays to disk). The new filter handles
  // OOC natively and is exercised on real OOC stores by the other two Threshold test cases.
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  std::cout << "\n[TEST] ThresholdImageFilter: Legacy parity (new vs ITK, in-memory)" << std::endl;

  constexpr usize kDim = 12;
  const float64 lower = 30.0;
  const float64 upper = 900.0;
  const float64 outside = 0.0;

  auto runThreshold = [&](IFilter& filter, DataStructure& ds, const DataPath& inputPath) {
    Arguments args;
    args.insertOrAssign(ThresholdImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(ThresholdImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(ThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(ThresholdImageFilter::k_Lower_Key, std::make_any<float64>(lower));
    args.insertOrAssign(ThresholdImageFilter::k_Upper_Key, std::make_any<float64>(upper));
    args.insertOrAssign(ThresholdImageFilter::k_OutsideValue_Key, std::make_any<float64>(outside));
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(ds, args, nullptr, MakeConsoleMessageHandler("threshold"));
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };

  // The new (ImageProcessing) filter and the legacy (ITK) filter share identical parameter keys.
  DataStructure newDs;
  const DataPath newInput = BuildRampImage(newDs, kDim, kDim, kDim);
  ThresholdImageFilter newFilter;
  runThreshold(newFilter, newDs, newInput);

  // Legacy ITK filter — created at runtime by UUID (no link dependency on ITKImageProcessing).
  const Uuid k_ItkThresholdUuid = *Uuid::FromString("ddf222f3-4af2-4583-967d-3eb9b86e77b4");
  IFilter::UniquePointer itkFilter = Application::Instance()->getFilterList()->createFilter(k_ItkThresholdUuid);
  REQUIRE(itkFilter != nullptr);
  DataStructure itkDs;
  const DataPath itkInput = BuildRampImage(itkDs, kDim, kDim, kDim);
  runThreshold(*itkFilter, itkDs, itkInput);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  UnitTest::CompareDataArrays<float32>(itkDs.getDataRefAs<Float32Array>(outputPath), newDs.getDataRefAs<Float32Array>(outputPath));
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the three ITK-sourced real-image md5 golden cases below. Mirrors ITKThresholdImageTest.cpp:
// read the ITK case's input through OUR ITK-free reader, run OUR ThresholdImageFilter (SameAsInput -- int16 for the
// .nrrd input, uint16 for the 16-bit .png inputs) with the ITK case's params, then (B) live-ITK parity FIRST
// (bit-exact) and (A) md5-validity-first (plan Sec.4). Pinned ForceInCore: the legacy ITK filter bad_casts an OOC
// store (reading via OUR reader is OOC-safe).
// -----------------------------------------------------------------------------
void RunThresholdItkGolden(const std::string& inputFile, const std::function<void(Arguments&)>& setParams, const std::string& committedMd5)
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
  args.insertOrAssign(ThresholdImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(ThresholdImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args);
  ThresholdImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (B) LIVE-ITK parity: legacy ITK filter on the SAME input into the SAME ds, bit-exact (integer).
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<ThresholdImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  Arguments itkArgs = args;
  itkArgs.insertOrAssign(ThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("ITK Output"));
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, ds, itkArgs);
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  const DataPath itkOutput = geom.createChildPath("CellData").createChildPath("ITK Output");
  const auto parityResult = ip_golden::CompareImages(ds, geom, itkOutput, geom, output, 0.0);
  SIMPLNX_RESULT_REQUIRE_VALID(parityResult);

  // (A) DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("ThresholdImageFilter md5: ours='{}' committed='{}'", ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

TEST_CASE("ImageProcessing::ThresholdImageFilter: ITK real-image golden (Default)", "[ImageProcessing][ItkGolden][ThresholdImageFilter]")
{
  RunThresholdItkGolden("RA-Short.nrrd", [](Arguments&) {}, "59071590099d21dd439896592338bf95");
}

TEST_CASE("ImageProcessing::ThresholdImageFilter: ITK real-image golden (Threshold1)", "[ImageProcessing][ItkGolden][ThresholdImageFilter]")
{
  RunThresholdItkGolden(
      "RA-Slice-Short.png",
      [](Arguments& args) {
        args.insertOrAssign(ThresholdImageFilter::k_Lower_Key, std::make_any<float64>(25000.0));
        args.insertOrAssign(ThresholdImageFilter::k_Upper_Key, std::make_any<float64>(65535.0));
      },
      "f70a31938657e0163b82521af4f8e3b0");
}

TEST_CASE("ImageProcessing::ThresholdImageFilter: ITK real-image golden (Threshold2)", "[ImageProcessing][ItkGolden][ThresholdImageFilter]")
{
  RunThresholdItkGolden(
      "RA-Slice-Short.png",
      [](Arguments& args) {
        args.insertOrAssign(ThresholdImageFilter::k_Lower_Key, std::make_any<float64>(0.0));
        args.insertOrAssign(ThresholdImageFilter::k_Upper_Key, std::make_any<float64>(25000.0));
        args.insertOrAssign(ThresholdImageFilter::k_OutsideValue_Key, std::make_any<float64>(25000.0));
      },
      "898743e4ec2e75c0169d025fb29b7ead");
}
