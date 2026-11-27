#include <catch2/catch.hpp>

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include "NXImageProcessing/Filters/ThresholdImageFilter.hpp"

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
  return IFilter::MessageHandler{[messageLabel = std::move(label)](const IFilter::Message& msg) { std::cout << "  [" << messageLabel << "] " << msg.message << '\n'; }};
}

// Builds an ImageGeom "Image Geometry" with a cell AttributeMatrix "CellData" and a float32 cell
// array "Input" filled with the ramp value (tupleIndex), returning the input array DataPath.
// Fills via bulk copyFromBuffer in bounded chunks so it is fast both in-core and out-of-core (a
// per-element setValue loop makes millions of individual OOC writes and appears to hang). Allocates
// only a fixed chunk buffer, never one proportional to the array size.
DataPath BuildRampImage(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeomPtr = ImageGeom::Create(dataStructure, "Image Geometry");
  imageGeomPtr->setDimensions({dimX, dimY, dimZ});
  REQUIRE(imageGeomPtr->setSpacing({1.0f, 1.0f, 1.0f}).valid());
  REQUIRE(imageGeomPtr->setOrigin({0.0f, 0.0f, 0.0f}).valid());

  auto* cellAmPtr = AttributeMatrix::Create(dataStructure, "CellData", cellShape, imageGeomPtr->getId());
  imageGeomPtr->setCellData(*cellAmPtr);

  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = data_store_utilities::CreateDataStore<float32>(dataStructure, inputPath, cellShape, {1});
  auto* arrayPtr = DataArray<float32>::Create(dataStructure, "Input", store, cellAmPtr->getId());
  auto& ref = arrayPtr->getDataStoreRef();

  const usize total = ref.getSize();
  std::cout << "  [build] filling " << dimX << "x" << dimY << "x" << dimZ << " ramp (" << total << " values) via bulk I/O..." << '\n';
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
  std::cout << "  [build] done" << '\n';
  return inputPath;
}
} // namespace

TEST_CASE("NXImageProcessing::ThresholdImageFilter: Small Correctness", "[NXImageProcessing][ThresholdImageFilter]")
{
  unit_test::LoadPlugins();
  // The out-of-core store path is retired to the generic SimplnxOoc store tests; this keeps the unique
  // independent computed-expected oracle (every voxel checked against the threshold semantics) as a plain
  // in-memory ForceInCore case.
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  std::cout << "\n[TEST] ThresholdImageFilter: Small Correctness (in-memory)" << '\n';

  constexpr usize k_Dim = 10;
  DataStructure dataStructure;
  const DataPath inputPath = BuildRampImage(dataStructure, k_Dim, k_Dim, k_Dim);

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

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args, nullptr, MakeConsoleMessageHandler("threshold"));
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float32ArrayType>(outputPath));
  auto& outStore = dataStructure.getDataRefAs<Float32ArrayType>(outputPath).getDataStoreRef();
  std::cout << "  [verify] checking " << outStore.getSize() << " output values..." << '\n';
  for(usize i = 0; i < outStore.getSize(); ++i)
  {
    const auto v = static_cast<float32>(i);
    const float32 expected = (v < static_cast<float32>(lower) || v > static_cast<float32>(upper)) ? static_cast<float32>(outside) : v;
    REQUIRE(outStore.getValue(i) == expected);
  }
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the three ITK-sourced real-image md5 golden cases below. Mirrors ITKThresholdImageTest.cpp:
// read the ITK case's input through OUR ITK-free reader, run OUR ThresholdImageFilter (SameAsInput -- int16 for the
// .nrrd input, uint16 for the 16-bit .png inputs) with the ITK case's params, then apply the (A) md5-validity-first
// golden (plan Sec.4). Pinned ForceInCore.
// -----------------------------------------------------------------------------
void RunThresholdItkGolden(const std::string& inputFile, const std::function<void(Arguments&)>& setParams, const std::string& committedMd5)
{
  unit_test::LoadPlugins();
  const unit_test::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure dataStructure;
  const auto readInput = ip_golden::ReadInputImage(dataStructure, ip_golden::InputPath(inputFile), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(ThresholdImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(ThresholdImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(ThresholdImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  setParams(args);
  ThresholdImageFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // (A) DURABLE golden: md5-validity-first (plan Sec.4).
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(dataStructure, output);
  INFO(fmt::format("ThresholdImageFilter md5: ours='{}' committed='{}'", ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("NXImageProcessing::ThresholdImageFilter: ITK real-image golden (Default)", "[NXImageProcessing][ItkGolden][ThresholdImageFilter]")
{
  RunThresholdItkGolden("RA-Short.nrrd", [](Arguments&) {}, "59071590099d21dd439896592338bf95");
}

TEST_CASE("NXImageProcessing::ThresholdImageFilter: ITK real-image golden (Threshold1)", "[NXImageProcessing][ItkGolden][ThresholdImageFilter]")
{
  RunThresholdItkGolden(
      "RA-Slice-Short.png",
      [](Arguments& args) {
        args.insertOrAssign(ThresholdImageFilter::k_Lower_Key, std::make_any<float64>(25000.0));
        args.insertOrAssign(ThresholdImageFilter::k_Upper_Key, std::make_any<float64>(65535.0));
      },
      "f70a31938657e0163b82521af4f8e3b0");
}

TEST_CASE("NXImageProcessing::ThresholdImageFilter: ITK real-image golden (Threshold2)", "[NXImageProcessing][ItkGolden][ThresholdImageFilter]")
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
