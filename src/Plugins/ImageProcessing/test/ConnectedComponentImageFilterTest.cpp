#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ConnectedComponentImageFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
// Legacy ITKConnectedComponentImageFilter, created at runtime by UUID (so this target does not link
// ITKImageProcessing). It is ENABLED in the ITKImageProcessing build, so it is the live-ITK reference for the
// streaming scanline union-find engine's bit-exact label-VALUE parity gate.
const Uuid k_LegacyCCUuid = *Uuid::FromString("905354c1-d55b-4436-b9f7-f4a6e80e5c0f");

using CCFilter = ConnectedComponentImageFilter;

// Sets the standard geom/input/output keys + the FullyConnected flag on a shared Arguments, runs preflight +
// execute, and requires both succeed. Reuses CCFilter::k_*_Key for BOTH the new and the legacy ITK filter --
// correct only because the new filter deliberately reuses the legacy key strings ("input_image_geometry_path",
// "input_image_data_path", "output_array_name", "fully_connected").
void RunCC(IFilter& filter, DataStructure& ds, const DataPath& inputPath, bool fullyConnected, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(CCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(CCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(CCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(CCFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic MULTI-BLOB field with several disjoint blobs, one diagonal-touching pair (exercises both
// connectivities), and (for dimZ>1) one component spanning >=2 z-planes (exercises the cross-plane union). Every
// foreground voxel is 1, so the pattern is representable in EVERY integer type incl. int8/uint8. Requires
// dimX,dimY >= 10.
template <class T>
std::vector<T> MakeBlobField(usize dimX, usize dimY, usize dimZ)
{
  std::vector<T> v(dimX * dimY * dimZ, T{0});
  auto set = [&](usize x, usize y, usize z) {
    if(x < dimX && y < dimY && z < dimZ)
    {
      v[rt::FlatIndex(x, y, z, dimX, dimY)] = T{1};
    }
  };

  // Blob A: 2x2 block at (1,1) on z=0.
  set(1, 1, 0);
  set(1, 2, 0);
  set(2, 1, 0);
  set(2, 2, 0);
  // Blob B: single voxel that touches Blob A's (2,2) corner ONLY diagonally -> merges with A iff fullyConnected.
  set(3, 3, 0);
  // Blob C: a separate 2x2 block, far from A/B under any connectivity.
  set(6, 6, 0);
  set(6, 7, 0);
  set(7, 6, 0);
  set(7, 7, 0);
  // Blob D: an isolated single voxel, far from everything else.
  set(9, 1, 0);
  // Blob E: a 2-voxel column across z (only when dimZ > 1) -> one component spanning >=2 z-planes.
  if(dimZ > 1)
  {
    set(9, 9, 0);
    set(9, 9, 1);
  }
  return v;
}

// Run new + legacy on the same field/params and require the uint32 label outputs match EXACTLY (byte-identical
// label VALUES -- the first live-ITK gate on the connected-component engine).
template <class T>
void RequireParity(const std::vector<T>& field, usize dx, usize dy, usize dz, bool fullyConnected)
{
  DataStructure newDs;
  const DataPath newInput = rt::BuildImageFromPattern<T>(newDs, dx, dy, dz, field);
  CCFilter newFilter;
  RunCC(newFilter, newDs, newInput, fullyConnected);

  IFilter::UniquePointer legacyFilter = Application::Instance()->getFilterList()->createFilter(k_LegacyCCUuid);
  REQUIRE(legacyFilter != nullptr);
  DataStructure legacyDs;
  const DataPath legacyInput = rt::BuildImageFromPattern<T>(legacyDs, dx, dy, dz, field);
  RunCC(*legacyFilter, legacyDs, legacyInput, fullyConnected);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  const auto& newOut = newDs.getDataRefAs<IDataArray>(outputPath);
  const auto& legacyOut = legacyDs.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(newOut.getDataType() == legacyOut.getDataType());
  UnitTest::CompareDataArrays<uint32>(newOut, legacyOut); // EXACT -- label values must byte-match live ITK
}
} // namespace

// -----------------------------------------------------------------------------
// (1) Live-ITK EXACT parity, FULL integer type grid x {FullyConnected on/off} x {3D, 2D}. Connected-component
//     labeling is a fixed uint32 output; the new filter must reproduce legacy ITK's label VALUES EXACTLY (no
//     tolerance -- see the Parity model). This is the primary gate on the streaming scanline union-find engine.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::ConnectedComponentImageFilter: Live-ITK parity", "[ImageProcessing][ConnectedComponentImageFilter]", uint8, int8, uint16, int16, uint32, int32, uint64, int64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  REQUIRE(Application::Instance()->getFilterList()->createFilter(k_LegacyCCUuid) != nullptr);

  const bool fullyConnected = GENERATE(false, true);
  CAPTURE(fullyConnected);

  SECTION("3D")
  {
    constexpr usize DX = 12, DY = 12, DZ = 4;
    RequireParity<T>(MakeBlobField<T>(DX, DY, DZ), DX, DY, DZ, fullyConnected);
  }
  SECTION("2D (Z==1)")
  {
    constexpr usize DX = 12, DY = 12, DZ = 1;
    RequireParity<T>(MakeBlobField<T>(DX, DY, DZ), DX, DY, DZ, fullyConnected);
  }
}

// -----------------------------------------------------------------------------
// (2) Preflight guards: a multi-component input is rejected (requireScalar); a non-integer (float32) input is
//     rejected (IntegerOnly); a valid integer scalar input produces a FIXED uint32 output array (AlwaysUInt32).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ConnectedComponentImageFilter: preflight guards", "[ImageProcessing][ConnectedComponentImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("multi-component rejected")
  {
    DataStructure ds;
    const DataPath scalarPath = rt::BuildImageFromPattern<int32>(ds, 6, 6, 1, std::vector<int32>(36, 0));
    const DataPath vecPath({"Image Geometry", "CellData", "Vec"});
    auto vecStore = DataStoreUtilities::CreateDataStore<int32>(ds, vecPath, {1, 6, 6}, {3});
    const auto& cellAM = ds.getDataRefAs<AttributeMatrix>(DataPath({"Image Geometry", "CellData"}));
    DataArray<int32>::Create(ds, "Vec", vecStore, cellAM.getId());
    const DataPath geomPath = vecPath.getParent().getParent();
    CCFilter filter;
    Arguments args;
    args.insertOrAssign(CCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(CCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(CCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CCFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("float input rejected")
  {
    DataStructure ds;
    const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, 6, 6, 1, std::vector<float32>(36, 1.0f));
    const DataPath geomPath = inputPath.getParent().getParent();
    CCFilter filter;
    Arguments args;
    args.insertOrAssign(CCFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(CCFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(CCFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CCFilter::k_FullyConnected_Key, std::make_any<bool>(false));
    const auto result = filter.preflight(ds, args);
    REQUIRE(result.outputActions.invalid()); // float32 is not an IntegerOnly type
  }
  SECTION("scalar integer input creates a uint32 output")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeBlobField<uint8>(12, 12, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 12, 12, 1, field);
    CCFilter filter;
    RunCC(filter, ds, inputPath, /*fullyConnected=*/false);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint32);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKConnectedComponentImageTest.cpp on OUR ITK-free filter. Output is a
// FIXED uint32 label image (AlwaysUInt32); (A) DURABLE golden = md5-validity-first (plan Sec.4), (B) LIVE-ITK parity =
// BIT-EXACT (integer labels, CompareImages@0.0 -- our established parity class). Helper pins ForceInCore.
//
// NOTE (recorded): the simplnx ITK test has THREE real-image cases, and NONE uses an actual mask/second-input array --
// the case named "(mask)" is simply 2th_cthead1.png run with defaults (the mask/second-input variant the plan alluded
// to is an upstream SimpleITK-only artifact that does not exist here). So no ConnectedComponent case is skipped; all
// three are ported below (the plan's Sec.6 table listed only the first two -- the (fullyconnected) case is added here
// for faithful reproduction, since our filter exposes k_FullyConnected_Key).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::ConnectedComponentImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][ConnectedComponentImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ConnectedComponentImageFilter>("WhiteDots.png", "548f5184428db10d93e3bf377dee5253");
}

// The ITK "(mask)" case: 2th_cthead1.png with DEFAULT parameters (no mask input is set -- name is a misnomer).
TEST_CASE("ImageProcessing::ConnectedComponentImageFilter: ITK real-image golden (2th_cthead1)", "[ImageProcessing][ItkGolden][ConnectedComponentImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ConnectedComponentImageFilter>("2th_cthead1.png", "769315132e427a391edd779191db446d");
}

// The ITK "(fullyconnected)" case: WhiteDots.png with FullyConnected = true.
TEST_CASE("ImageProcessing::ConnectedComponentImageFilter: ITK real-image golden (fullyconnected)", "[ImageProcessing][ItkGolden][ConnectedComponentImageFilter]")
{
  rt::RunReconstructionMd5ItkGolden<ConnectedComponentImageFilter>("WhiteDots.png", "e40b7cdfc1b34ae2e6b13660d626cc29",
                                                                   [](Arguments& args) { args.insertOrAssign(ConnectedComponentImageFilter::k_FullyConnected_Key, std::make_any<bool>(true)); });
}
