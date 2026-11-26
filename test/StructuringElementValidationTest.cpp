#include "simplnx/Common/Result.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ObjectMorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
void RequireError(const Result<>& result, int32 code, const std::string& phrase)
{
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().size() == 1);
  REQUIRE(result.errors().front().code == code);
  REQUIRE(result.errors().front().message.find(phrase) != std::string::npos);
}

void RequireOffsets(KernelType type, const std::array<uint32, 3>& radius, const std::vector<SEOffset>& expected)
{
  auto result = MakeCheckedStructuringElement(type, radius);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const auto& se = result.value();
  REQUIRE(se.offsets.size() == expected.size());
  for(usize axis = 0; axis < 3; ++axis)
  {
    REQUIRE(se.radius[axis] == radius[axis]);
  }
  for(usize i = 0; i < expected.size(); ++i)
  {
    CAPTURE(i);
    REQUIRE(se.offsets[i].dx == expected[i].dx);
    REQUIRE(se.offsets[i].dy == expected[i].dy);
    REQUIRE(se.offsets[i].dz == expected[i].dz);
  }
}
} // namespace

TEST_CASE("simplnx::StructuringElement: Kernel Radius diagnostics", "[simplnx][StructuringElement]")
{
  SECTION("Length")
  {
    const std::array<uint32, 2> radius = {0, 0};
    auto result = ValidateKernelRadius(radius);
    RequireError(result, k_KernelRadiusLengthError, "2");
  }
  SECTION("Every axis at first invalid")
  {
    for(usize axis = 0; axis < 3; ++axis)
    {
      std::array<uint32, 3> radius = {0, 0, 0};
      radius[axis] = 2147483647u;
      auto result = ValidateKernelRadius(radius);
      RequireError(result, k_KernelRadiusRangeError, std::array<std::string, 3>{"X", "Y", "Z"}[axis]);
      REQUIRE(result.errors().front().message.find("2147483647") != std::string::npos);
      REQUIRE(result.errors().front().message.find("2147483646") != std::string::npos);
    }
  }
  SECTION("Bounding-box product")
  {
    const std::array<uint32, 3> radius = {2147483646u, 2147483646u, 2147483646u};
    auto result = ValidateKernelRadius(radius);
    RequireError(result, k_KernelRadiusCountOverflowError, "(2*X+1)*(2*Y+1)*(2*Z+1)");
    REQUIRE(result.errors().front().message.find("2147483646") != std::string::npos);
  }
  SECTION("Vector element capacity")
  {
    const std::array<uint32, 3> radius = {1048576, 1048576, 1048576};
    auto result = ValidateKernelRadius(radius);
    RequireError(result, k_KernelRadiusCapacityError, "element count");
    REQUIRE(result.errors().front().message.find("1048576") != std::string::npos);
  }
  SECTION("Large representable element count has no workload cap")
  {
    const std::array<uint32, 3> radius = {200000000, 200000000, 0};
    auto result = ValidateKernelRadius(radius);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  SECTION("Zero and last valid axis")
  {
    for(usize axis = 0; axis < 3; ++axis)
    {
      std::array<uint32, 3> radius = {0, 0, 0};
      auto result = ValidateKernelRadius(radius);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
      radius[axis] = k_MaxKernelRadius;
      result = ValidateKernelRadius(radius);
      SIMPLNX_RESULT_REQUIRE_VALID(result);
    }
  }
}

TEST_CASE("simplnx::StructuringElement: Checked construction failures", "[simplnx][StructuringElement]")
{
  const std::array<uint32, 3> radius = {1, 1, 0};
  SECTION("bad_alloc")
  {
    auto result = ImageProcessing::detail::MakeCheckedStructuringElementWith(KernelType::Box, radius, [](KernelType, nonstd::span<const uint32>) -> StructuringElement { throw std::bad_alloc{}; });
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == k_KernelConstructionError);
    REQUIRE(result.errors().front().message.find("Kernel Radius (X=1, Y=1, Z=0)") != std::string::npos);
  }
  SECTION("length_error")
  {
    auto result = ImageProcessing::detail::MakeCheckedStructuringElementWith(
        KernelType::Box, radius, [](KernelType, nonstd::span<const uint32>) -> StructuringElement { throw std::length_error("injected construction failure"); });
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == k_KernelConstructionError);
  }
}

TEST_CASE("simplnx::StructuringElement: Ordered offset oracles", "[simplnx][StructuringElement]")
{
  SECTION("Unit cube")
  {
    const std::vector<SEOffset> box = {{.dx = -1, .dy = -1, .dz = -1}, {.dx = 0, .dy = -1, .dz = -1}, {.dx = 1, .dy = -1, .dz = -1}, {.dx = -1, .dy = 0, .dz = -1}, {.dx = 0, .dy = 0, .dz = -1},
                                       {.dx = 1, .dy = 0, .dz = -1},   {.dx = -1, .dy = 1, .dz = -1}, {.dx = 0, .dy = 1, .dz = -1},  {.dx = 1, .dy = 1, .dz = -1},  {.dx = -1, .dy = -1, .dz = 0},
                                       {.dx = 0, .dy = -1, .dz = 0},   {.dx = 1, .dy = -1, .dz = 0},  {.dx = -1, .dy = 0, .dz = 0},  {.dx = 0, .dy = 0, .dz = 0},   {.dx = 1, .dy = 0, .dz = 0},
                                       {.dx = -1, .dy = 1, .dz = 0},   {.dx = 0, .dy = 1, .dz = 0},   {.dx = 1, .dy = 1, .dz = 0},   {.dx = -1, .dy = -1, .dz = 1}, {.dx = 0, .dy = -1, .dz = 1},
                                       {.dx = 1, .dy = -1, .dz = 1},   {.dx = -1, .dy = 0, .dz = 1},  {.dx = 0, .dy = 0, .dz = 1},   {.dx = 1, .dy = 0, .dz = 1},   {.dx = -1, .dy = 1, .dz = 1},
                                       {.dx = 0, .dy = 1, .dz = 1},    {.dx = 1, .dy = 1, .dz = 1}};
    const std::vector<SEOffset> ball = {{.dx = 0, .dy = -1, .dz = -1}, {.dx = -1, .dy = 0, .dz = -1}, {.dx = 0, .dy = 0, .dz = -1}, {.dx = 1, .dy = 0, .dz = -1}, {.dx = 0, .dy = 1, .dz = -1},
                                        {.dx = -1, .dy = -1, .dz = 0}, {.dx = 0, .dy = -1, .dz = 0},  {.dx = 1, .dy = -1, .dz = 0}, {.dx = -1, .dy = 0, .dz = 0}, {.dx = 0, .dy = 0, .dz = 0},
                                        {.dx = 1, .dy = 0, .dz = 0},   {.dx = -1, .dy = 1, .dz = 0},  {.dx = 0, .dy = 1, .dz = 0},  {.dx = 1, .dy = 1, .dz = 0},  {.dx = 0, .dy = -1, .dz = 1},
                                        {.dx = -1, .dy = 0, .dz = 1},  {.dx = 0, .dy = 0, .dz = 1},   {.dx = 1, .dy = 0, .dz = 1},  {.dx = 0, .dy = 1, .dz = 1}};
    const std::vector<SEOffset> annulus = {{.dx = 0, .dy = -1, .dz = -1}, {.dx = -1, .dy = 0, .dz = -1}, {.dx = 0, .dy = 0, .dz = -1}, {.dx = 1, .dy = 0, .dz = -1}, {.dx = 0, .dy = 1, .dz = -1},
                                           {.dx = -1, .dy = -1, .dz = 0}, {.dx = 0, .dy = -1, .dz = 0},  {.dx = 1, .dy = -1, .dz = 0}, {.dx = -1, .dy = 0, .dz = 0}, {.dx = 1, .dy = 0, .dz = 0},
                                           {.dx = -1, .dy = 1, .dz = 0},  {.dx = 0, .dy = 1, .dz = 0},   {.dx = 1, .dy = 1, .dz = 0},  {.dx = 0, .dy = -1, .dz = 1}, {.dx = -1, .dy = 0, .dz = 1},
                                           {.dx = 0, .dy = 0, .dz = 1},   {.dx = 1, .dy = 0, .dz = 1},   {.dx = 0, .dy = 1, .dz = 1}};
    RequireOffsets(KernelType::Box, {1, 1, 1}, box);
    RequireOffsets(KernelType::Ball, {1, 1, 1}, ball);
    RequireOffsets(KernelType::Annulus, {1, 1, 1}, annulus);
    RequireOffsets(KernelType::Cross, {1, 1, 1},
                   {{.dx = 0, .dy = 0, .dz = -1},
                    {.dx = 0, .dy = -1, .dz = 0},
                    {.dx = -1, .dy = 0, .dz = 0},
                    {.dx = 0, .dy = 0, .dz = 0},
                    {.dx = 1, .dy = 0, .dz = 0},
                    {.dx = 0, .dy = 1, .dz = 0},
                    {.dx = 0, .dy = 0, .dz = 1}});
  }
  SECTION("Unit square")
  {
    const std::vector<SEOffset> square = {{.dx = -1, .dy = -1, .dz = 0}, {.dx = 0, .dy = -1, .dz = 0}, {.dx = 1, .dy = -1, .dz = 0}, {.dx = -1, .dy = 0, .dz = 0}, {.dx = 0, .dy = 0, .dz = 0},
                                          {.dx = 1, .dy = 0, .dz = 0},   {.dx = -1, .dy = 1, .dz = 0}, {.dx = 0, .dy = 1, .dz = 0},  {.dx = 1, .dy = 1, .dz = 0}};
    RequireOffsets(KernelType::Box, {1, 1, 0}, square);
    RequireOffsets(KernelType::Ball, {1, 1, 0}, square);
    RequireOffsets(KernelType::Annulus, {1, 1, 0},
                   {{.dx = -1, .dy = -1, .dz = 0},
                    {.dx = 0, .dy = -1, .dz = 0},
                    {.dx = 1, .dy = -1, .dz = 0},
                    {.dx = -1, .dy = 0, .dz = 0},
                    {.dx = 1, .dy = 0, .dz = 0},
                    {.dx = -1, .dy = 1, .dz = 0},
                    {.dx = 0, .dy = 1, .dz = 0},
                    {.dx = 1, .dy = 1, .dz = 0}});
    RequireOffsets(KernelType::Cross, {1, 1, 0}, {{.dx = 0, .dy = -1, .dz = 0}, {.dx = -1, .dy = 0, .dz = 0}, {.dx = 0, .dy = 0, .dz = 0}, {.dx = 1, .dy = 0, .dz = 0}, {.dx = 0, .dy = 1, .dz = 0}});
  }
  SECTION("Asymmetric plane")
  {
    RequireOffsets(KernelType::Box, {2, 1, 0},
                   {{.dx = -2, .dy = -1, .dz = 0},
                    {.dx = -1, .dy = -1, .dz = 0},
                    {.dx = 0, .dy = -1, .dz = 0},
                    {.dx = 1, .dy = -1, .dz = 0},
                    {.dx = 2, .dy = -1, .dz = 0},
                    {.dx = -2, .dy = 0, .dz = 0},
                    {.dx = -1, .dy = 0, .dz = 0},
                    {.dx = 0, .dy = 0, .dz = 0},
                    {.dx = 1, .dy = 0, .dz = 0},
                    {.dx = 2, .dy = 0, .dz = 0},
                    {.dx = -2, .dy = 1, .dz = 0},
                    {.dx = -1, .dy = 1, .dz = 0},
                    {.dx = 0, .dy = 1, .dz = 0},
                    {.dx = 1, .dy = 1, .dz = 0},
                    {.dx = 2, .dy = 1, .dz = 0}});
    RequireOffsets(KernelType::Ball, {2, 1, 0},
                   {{.dx = -1, .dy = -1, .dz = 0},
                    {.dx = 0, .dy = -1, .dz = 0},
                    {.dx = 1, .dy = -1, .dz = 0},
                    {.dx = -2, .dy = 0, .dz = 0},
                    {.dx = -1, .dy = 0, .dz = 0},
                    {.dx = 0, .dy = 0, .dz = 0},
                    {.dx = 1, .dy = 0, .dz = 0},
                    {.dx = 2, .dy = 0, .dz = 0},
                    {.dx = -1, .dy = 1, .dz = 0},
                    {.dx = 0, .dy = 1, .dz = 0},
                    {.dx = 1, .dy = 1, .dz = 0}});
    RequireOffsets(KernelType::Annulus, {2, 1, 0},
                   {{.dx = -1, .dy = -1, .dz = 0},
                    {.dx = 0, .dy = -1, .dz = 0},
                    {.dx = 1, .dy = -1, .dz = 0},
                    {.dx = -2, .dy = 0, .dz = 0},
                    {.dx = 2, .dy = 0, .dz = 0},
                    {.dx = -1, .dy = 1, .dz = 0},
                    {.dx = 0, .dy = 1, .dz = 0},
                    {.dx = 1, .dy = 1, .dz = 0}});
    RequireOffsets(KernelType::Cross, {2, 1, 0},
                   {{.dx = 0, .dy = -1, .dz = 0},
                    {.dx = -2, .dy = 0, .dz = 0},
                    {.dx = -1, .dy = 0, .dz = 0},
                    {.dx = 0, .dy = 0, .dz = 0},
                    {.dx = 1, .dy = 0, .dz = 0},
                    {.dx = 2, .dy = 0, .dz = 0},
                    {.dx = 0, .dy = 1, .dz = 0}});
  }
}

TEST_CASE("simplnx::StructuringElement: Legacy signed radius", "[simplnx][StructuringElement]")
{
  for(const int32 value : std::array<int32, 2>{-1, std::numeric_limits<int32>::min()})
  {
    auto se = MakeStructuringElement(KernelType::Box, {value, 0, 0});
    REQUIRE(se.offsets.empty());
  }
  const auto se = MakeStructuringElement(KernelType::Box, {0, 0, 0});
  REQUIRE(se.offsets.size() == 1);
  REQUIRE(se.offsets[0].dx == 0);
  REQUIRE(se.offsets[0].dy == 0);
  REQUIRE(se.offsets[0].dz == 0);
}

TEST_CASE("simplnx::MorphologyLayout: Representation boundaries", "[simplnx][StructuringElement]")
{
  constexpr usize k_SignedMax = static_cast<usize>(std::numeric_limits<int64>::max());
  struct LayoutCase
  {
    SizeVec3 dims;
    std::array<uint32, 3> radius;
    bool safe_border;
    usize bytes;
    int32 code;
    std::string phrase;
  };
  const std::vector<LayoutCase> cases = {
      {.dims = {std::numeric_limits<usize>::max() - 1, 1, 1}, .radius = {1, 0, 0}, .safe_border = true, .bytes = 1, .code = k_MorphologyPaddedExtentError, .phrase = "padded extent"},
      {.dims = {k_SignedMax - 2, 1, 1}, .radius = {2, 0, 0}, .safe_border = false, .bytes = 1, .code = k_MorphologyCoordinateError, .phrase = "coordinate"},
      {.dims = {3, 3, 1024}, .radius = {100000000, 100000000, 0}, .safe_border = true, .bytes = 1, .code = k_MorphologyVolumeError, .phrase = "volume"},
      {.dims = {usize{1} << 32, usize{1} << 31, 1}, .radius = {0, 0, 0}, .safe_border = false, .bytes = 1, .code = k_MorphologyPlaneError, .phrase = "plane"},
      {.dims = {usize{1} << 31, usize{1} << 31, 1}, .radius = {0, 0, 0}, .safe_border = false, .bytes = 8, .code = k_MorphologyByteCountError, .phrase = "byte"}};
  for(const auto& item : cases)
  {
    CAPTURE(item.phrase);
    auto result = ValidateMorphologyLayout(item.dims, item.radius, item.safe_border, item.bytes);
    RequireError(result, item.code, item.phrase);
    REQUIRE(result.errors().front().message.find("Kernel Radius") != std::string::npos);
    REQUIRE(result.errors().front().message.find(std::to_string(item.dims[0])) != std::string::npos);
    REQUIRE(result.errors().front().message.find(std::to_string(item.radius[0])) != std::string::npos);
  }
  for(const usize axis : std::array<usize, 2>{0, 1})
  {
    SizeVec3 dims = {1, 1, 1};
    std::array<uint32, 3> radius = {0, 0, 0};
    dims[axis] = k_SignedMax - 3;
    radius[axis] = 2;
    auto result = ValidateMorphologyLayout(dims, radius, false, 1);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    ++dims[axis];
    result = ValidateMorphologyLayout(dims, radius, false, 1);
    RequireError(result, k_MorphologyCoordinateError, "coordinate");
  }
  const std::vector<LayoutCase> controls = {{.dims = {k_SignedMax - 1, 1, 1}, .radius = {0, 0, 0}, .safe_border = false, .bytes = 1, .code = 0, .phrase = "large X"},
                                            {.dims = {1, k_SignedMax - 1, 1}, .radius = {0, 0, 0}, .safe_border = false, .bytes = 1, .code = 0, .phrase = "large Y"},
                                            {.dims = {3, 3, 1024}, .radius = {100000000, 100000000, 0}, .safe_border = false, .bytes = 1, .code = 0, .phrase = "Safe Border off"},
                                            {.dims = {3, 3, 1}, .radius = {0, 0, 2147483646u}, .safe_border = true, .bytes = 1, .code = 0, .phrase = "2D ignores Z padding"},
                                            {.dims = {131072, 131072, 2}, .radius = {0, 0, 1073741823}, .safe_border = false, .bytes = 1, .code = 0, .phrase = "unrepresentable flat delta"},
                                            {.dims = {usize{1} << 30, usize{1} << 30, 4}, .radius = {0, 0, 4}, .safe_border = false, .bytes = 1, .code = 0, .phrase = "flat delta plus volume"}};
  for(const auto& item : controls)
  {
    CAPTURE(item.phrase);
    auto result = ValidateMorphologyLayout(item.dims, item.radius, item.safe_border, item.bytes);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
}

TEST_CASE("simplnx::MorphologyLayout: Packed word endpoint", "[simplnx][StructuringElement]")
{
  constexpr int64 k_SignedMax = std::numeric_limits<int64>::max();
  REQUIRE(ImageProcessing::detail::MorphologyPackedWordEnd(k_SignedMax - 8, k_SignedMax - 1) == k_SignedMax - 1);
  REQUIRE(ImageProcessing::detail::MorphologyPackedWordEnd(0, 3) == 3);
  REQUIRE(ImageProcessing::detail::MorphologyPackedWordEnd(-10, 3) == 3);
  REQUIRE(ImageProcessing::detail::MorphologyPackedWordEnd(-70, 3) == -6);
  const std::array<ImageProcessing::detail::BinaryMorphWord, 1> word = {5};
  ImageProcessing::detail::PackedBinaryPlanesView view;
  view.words = word.data();
  view.zLo = 0;
  view.depth = 1;
  view.capacity = 1;
  view.layout.wordsPerRow = 1;
  view.layout.planeWords = 1;
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(view, {3, 1, 1}, 0, 0, 0, false) == 5);
  REQUIRE(ImageProcessing::detail::ExtractPackedBinaryWord(view, {3, 1, 1}, k_SignedMax - 8, 0, 0, false) == 0);
}

TEST_CASE("simplnx::MorphologyLayout: Direct engine guards", "[simplnx][StructuringElement]")
{
  // Invalid metadata must return before dispatch, reading values, or allocating scratch.
  DataStructure ds;
  auto* inputPtr = UnitTest::CreateTestDataArray<uint8>(ds, "Input", {1}, {1});
  auto* outputPtr = UnitTest::CreateTestDataArray<uint8>(ds, "Output", {1}, {1});
  REQUIRE(inputPtr != nullptr);
  REQUIRE(outputPtr != nullptr);
  const auto& in = inputPtr->getDataStoreRef();
  auto& out = outputPtr->getDataStoreRef();
  const SizeVec3 dims = {static_cast<usize>(std::numeric_limits<int64>::max()) - 2, 1, 1};
  const auto se = MakeStructuringElement(KernelType::Box, {2, 0, 0});
  const auto box = ImageProcessing::detail::MakeFullBoxNeighborOffsets();
  const std::atomic_bool cancel = false;
  const IFilter::MessageHandler handler;
  std::vector<Result<>> results;
  results.push_back(MorphDirect<uint8>(in, out, dims, se, MorphOp::Dilate, cancel, handler)());
  results.push_back(MorphScanline<uint8>(in, out, dims, se, MorphOp::Dilate, cancel, handler)());
  results.push_back(MorphWorkingMemory<uint8>(in, out, dims, se, MorphOp::Dilate, cancel, handler)());
  results.push_back(MorphGradientDirect<uint8>(in, out, dims, se, cancel, handler)());
  results.push_back(MorphGradientScanline<uint8>(in, out, dims, se, cancel, handler)());
  results.push_back(MorphGradientWorkingMemory<uint8>(in, out, dims, se, cancel, handler)());
  results.push_back(BinaryMorphDirect<uint8>(in, out, dims, se, MorphOp::Dilate, 5, 0, false, cancel, handler)());
  results.push_back(BinaryMorphScanline<uint8>(in, out, dims, se, MorphOp::Dilate, 5, 0, false, cancel, handler)());
  results.push_back(BinaryMorphWorkingMemory<uint8>(in, out, dims, se, MorphOp::Dilate, 5, 0, false, cancel, handler)());
  results.push_back(ObjectMorphScatter<uint8>(in, out, dims, se, box, ObjectMorphOp::Dilate, 5, 0, cancel, handler)());
  results.push_back(ObjectMorphGather<uint8>(in, out, dims, se, box, ObjectMorphOp::Dilate, 5, 0, cancel, handler)());
  results.push_back(ApplyMorphology<uint8>(in, out, dims, se, MorphOp::Dilate, *inputPtr, *outputPtr, cancel, handler));
  results.push_back(ApplyMorphologyGradient<uint8>(in, out, dims, se, *inputPtr, *outputPtr, cancel, handler));
  results.push_back(ApplyBinaryMorphology<uint8>(in, out, dims, se, MorphOp::Dilate, 5, 0, false, *inputPtr, *outputPtr, cancel, handler));
  results.push_back(ApplyObjectMorphology<uint8>(in, out, dims, se, box, ObjectMorphOp::Dilate, 5, 0, *inputPtr, *outputPtr, cancel, handler));
  results.push_back(ApplyMorphologyCompositePipeline<uint8>(in, out, dims, se, MorphOp::Erode, MorphOp::Dilate, false, ImageProcessing::detail::MorphologyCompositeOutputMode::Morphology, cancel, 0));
  results.push_back(ApplyBinaryMorphologyCompositePipeline<uint8>(in, out, dims, se, MorphOp::Erode, MorphOp::Dilate, false, 5, 0, cancel, 0));
  for(const auto& result : results)
  {
    RequireError(result, k_MorphologyCoordinateError, "coordinate");
  }
}

TEST_CASE("simplnx::MorphologyLayout: Selected geometry error context", "[simplnx][StructuringElement]")
{
  const DataPath geometryPath({"Dataset", "Selected Geometry"});
  constexpr int32 k_TestWarning = 37;
  SECTION("Invalid layout keeps its code, message and warnings")
  {
    auto layout = MakeErrorResult(k_MorphologyVolumeError, "Original layout message.");
    layout.warnings().push_back({k_TestWarning, "Original warning."});
    auto result = AddMorphologyLayoutGeometryContext(std::move(layout), geometryPath);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().size() == 1);
    REQUIRE(result.errors().front().code == k_MorphologyVolumeError);
    REQUIRE(result.errors().front().message == "Original layout message. Image Geometry: 'Dataset/Selected Geometry'.");
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings().front().code == k_TestWarning);
    REQUIRE(result.warnings().front().message == "Original warning.");
  }
  SECTION("Valid result keeps its warnings")
  {
    Result<> layout;
    layout.warnings().push_back({k_TestWarning, "Original warning."});
    auto result = AddMorphologyLayoutGeometryContext(std::move(layout), geometryPath);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings().front().code == k_TestWarning);
    REQUIRE(result.warnings().front().message == "Original warning.");
  }
}
