#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/ConnectedComponentImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <any>
#include <atomic>
#include <cctype>
#include <limits>
#include <string>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using CCFilter = ConnectedComponentImageFilter;

// Sets the geometry, input, and output keys plus the FullyConnected flag on the Arguments. The helper runs
// preflight and execute, and requires that both steps succeed.
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
} // namespace

TEST_CASE("ImageProcessing::ConnectedComponentImageFilter: provisional label limit policy is explicit and defaults to Error", "[ImageProcessing][Issue1874][ConnectedComponentImageFilter]")
{
  UnitTest::LoadPlugins();
  const ConnectedComponentImageFilter filter;
  REQUIRE(filter.parametersVersion() == 2);
  const Parameters parameters = filter.parameters();
  const std::string policyKey = "provisional_label_limit_handling_index";

  // Check the public schema before any lookup. This is a safe baseline failure with no label allocation or execution.
  REQUIRE(parameters.contains(policyKey));
  const auto* policyPtr = dynamic_cast<const ChoicesParameter*>(parameters.at(policyKey).get());
  REQUIRE(policyPtr != nullptr);
  REQUIRE(policyPtr->humanName() == "Provisional Label Limit Handling");
  REQUIRE(policyPtr->choices() == ChoicesParameter::Choices{"Error", "Warn and Continue (Legacy Behavior)"});
  REQUIRE(policyPtr->defaultIndex() == 0);
  const Arguments defaultArguments = filter.getDefaultArguments();
  REQUIRE(defaultArguments.value<ChoicesParameter::ValueType>(policyKey) == 0);

  // Check required risk content without fixing the complete tooltip prose.
  std::string tooltip = policyPtr->helpText();
  REQUIRE_FALSE(tooltip.empty());
  std::ranges::transform(tooltip, tooltip.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
  REQUIRE((tooltip.find("4,294,967,295") != std::string::npos || tooltip.find("4294967295") != std::string::npos));
  for(const auto* riskTermPtr : {"default", "provisional", "wrap", "background", "reuse", "incorrect", "statistics"})
  {
    INFO(riskTermPtr);
    REQUIRE(tooltip.find(riskTermPtr) != std::string::npos);
  }
}

TEST_CASE("ImageProcessing::ConnectedComponentImageFilter: both policies preserve literal connectivity labels and uint32 shape", "[ImageProcessing][Issue1874][ConnectedComponentImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto handling = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
  const bool fullyConnected = GENERATE(false, true);
  const bool threeDimensional = GENERATE(false, true);
  CAPTURE(handling, fullyConnected, threeDimensional);
  // Opposite corners touch only with full connectivity in both 2D and 3D. Native uint64 max is just foreground.
  const usize dimZ = threeDimensional ? 2 : 1;
  std::vector<uint64> input(4 * dimZ, 0);
  input.at(0) = std::numeric_limits<uint64>::max();
  input.at(input.size() - 1) = 1;
  std::vector<uint32> expected(input.size(), 0);
  expected.at(0) = 1;
  expected.at(expected.size() - 1) = fullyConnected ? 1 : 2;
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint64>(dataStructure, 2, 2, dimZ, input);
  const ConnectedComponentImageFilter filter;
  Arguments arguments;
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_FullyConnected_Key, std::make_any<bool>(fullyConnected));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_ProvisionalLabelLimitHandling_Key, std::make_any<ChoicesParameter::ValueType>(handling));
  const auto preflightResult = filter.preflight(dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == DataType::uint32);
  REQUIRE(actionPtr->dims() == ShapeType{dimZ, 2, 2});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});
  const auto executeResult = filter.execute(dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(executeResult.result.warnings().empty());
  const DataPath outputPath = inputPath.replaceName("Output");
  const IDataArray* arrayPtr = nullptr;
  REQUIRE_NOTHROW(arrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(arrayPtr->getDataType() == DataType::uint32);
  REQUIRE(arrayPtr->getTupleShape() == ShapeType{dimZ, 2, 2});
  REQUIRE(arrayPtr->getComponentShape() == ShapeType{1});
  const UInt32Array* labelsPtr = nullptr;
  REQUIRE_NOTHROW(labelsPtr = &dataStructure.getDataRefAs<UInt32Array>(outputPath));
  for(usize index = 0; index < expected.size(); ++index)
  {
    REQUIRE(labelsPtr->getDataStoreRef().getValue(index) == expected.at(index));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ConnectedComponentImageFilter: many provisional runs can form one exact component", "[ImageProcessing][Issue1874][ConnectedComponentImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto handling = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
  const std::vector<int8> input{1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1};
  const std::vector<uint32> expected{1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1};
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int8>(dataStructure, 5, 3, 1, input);
  Arguments arguments;
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_ProvisionalLabelLimitHandling_Key, std::make_any<ChoicesParameter::ValueType>(handling));
  const ConnectedComponentImageFilter filter;
  const auto executeResult = filter.execute(dataStructure, arguments);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(executeResult.result.warnings().empty());
  const IDataArray* arrayPtr = nullptr;
  REQUIRE_NOTHROW(arrayPtr = &dataStructure.getDataRefAs<IDataArray>(inputPath.replaceName("Output")));
  REQUIRE(arrayPtr->getDataType() == DataType::uint32);
  REQUIRE(arrayPtr->getTupleShape() == ShapeType{1, 3, 5});
  REQUIRE(arrayPtr->getComponentShape() == ShapeType{1});
  const UInt32Array* labelsPtr = nullptr;
  REQUIRE_NOTHROW(labelsPtr = &dataStructure.getDataRefAs<UInt32Array>(inputPath.replaceName("Output")));
  for(usize index = 0; index < expected.size(); ++index)
  {
    REQUIRE(labelsPtr->getDataStoreRef().getValue(index) == expected.at(index));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::ConnectedComponentImageFilter: execution cancellation keeps the filter cancellation result", "[ImageProcessing][Issue1874][ConnectedComponentImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const auto handling = GENERATE(ChoicesParameter::ValueType{0}, ChoicesParameter::ValueType{1});
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<int8>(dataStructure, 3, 2, 1, std::vector<int8>(6, 1));
  Arguments arguments;
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  arguments.insertOrAssign(ConnectedComponentImageFilter::k_ProvisionalLabelLimitHandling_Key, std::make_any<ChoicesParameter::ValueType>(handling));
  std::atomic_bool shouldCancel{false};
  const IFilter::MessageHandler handler{[&shouldCancel](const IFilter::Message& message) {
    if(message.type == IFilter::Message::Type::Info && message.message == "Labeling Connected Components")
    {
      shouldCancel = true;
    }
  }};
  const ConnectedComponentImageFilter filter;
  const auto result = filter.execute(dataStructure, arguments, nullptr, handler, shouldCancel);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(shouldCancel);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors().front().code == -1);
  REQUIRE(result.result.warnings().empty());
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

// -----------------------------------------------------------------------------
// (1) Preflight guards: a multi-component input is rejected (requireScalar); a non-integer (float32) input is
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
// ITK-sourced real-image golden -- duplicates ITKConnectedComponentImageTest.cpp on our ITK-free filter. The output is
// a fixed uint32 label image (AlwaysUInt32). The durable golden is an md5 pin. The helper pins ForceInCore.
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
