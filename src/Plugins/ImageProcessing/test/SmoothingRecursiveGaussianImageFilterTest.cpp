#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/SmoothingRecursiveGaussianImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/UnitTest/AlgorithmTestScope.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/PointwiseEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/RecursiveGaussianEngine.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using SmoothFilter = SmoothingRecursiveGaussianImageFilter;

Arguments makeSmoothingArguments(const DataPath& inputPath, const std::vector<float64>& sigma)
{
  Arguments args;
  args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(sigma));
  args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
  return args;
}

void requireSmoothingInput(const DataStructure& dataStructure, const DataPath& inputPath, usize dimZ)
{
  const IDataArray* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(inputPath));
  REQUIRE(inputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(inputArrayPtr->getTupleShape() == ShapeType{dimZ, 4, 4});
  REQUIRE(inputArrayPtr->getComponentShape() == ShapeType{1});
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

struct SmoothingTransferFaults
{
  usize ReadAt = 0;
  usize WriteAt = 0;
  std::atomic_bool* CancelOnWritePtr = nullptr;
};

// Resident endpoints with real data and controlled transfer outcomes. These are not disk-storage witnesses.
template <class T>
class SmoothingTransferStore : public DataStore<T>
{
public:
  explicit SmoothingTransferStore(usize size, T value, SmoothingTransferFaults faults = {})
  : DataStore<T>(ShapeType{size}, ShapeType{1}, value)
  , m_FailReadAt(faults.ReadAt)
  , m_FailWriteAt(faults.WriteAt)
  , m_CancelOnWritePtr(faults.CancelOnWritePtr)
  {
  }
  ~SmoothingTransferStore() noexcept override = default;
  SmoothingTransferStore(const SmoothingTransferStore&) = delete;
  SmoothingTransferStore(SmoothingTransferStore&&) = delete;
  SmoothingTransferStore& operator=(const SmoothingTransferStore&) = delete;
  SmoothingTransferStore& operator=(SmoothingTransferStore&&) = delete;

  Result<> copyIntoBuffer(usize offset, nonstd::span<T> values) const override
  {
    ++m_ReadCount;
    if(m_ReadCount == m_FailReadAt)
    {
      return MakeErrorResult(-23690, "Injected smoothing read failure.");
    }
    Result<> result = DataStore<T>::copyIntoBuffer(offset, values);
    result.warnings().push_back({.code = -23692, .message = "Injected smoothing read warning."});
    return result;
  }
  Result<> copyFromBuffer(usize offset, nonstd::span<const T> values) override
  {
    ++m_WriteCount;
    if(m_WriteCount == m_FailWriteAt)
    {
      return MakeErrorResult(-23691, "Injected smoothing write failure.");
    }
    Result<> result = DataStore<T>::copyFromBuffer(offset, values);
    result.warnings().push_back({.code = -23693, .message = "Injected smoothing write warning."});
    if(m_CancelOnWritePtr != nullptr)
    {
      m_CancelOnWritePtr->store(true);
    }
    return result;
  }

private:
  usize m_FailReadAt = 0;
  usize m_FailWriteAt = 0;
  std::atomic_bool* m_CancelOnWritePtr = nullptr;
  mutable usize m_ReadCount = 0;
  usize m_WriteCount = 0;
};

usize countSmoothingWarning(const Result<>& result, int32 code)
{
  return static_cast<usize>(std::count_if(result.warnings().begin(), result.warnings().end(), [code](const auto& warning) { return warning.code == code; }));
}
// NOTE: the tests below use analytic oracles and ITK's committed baseline image. No live ITK filter runs here.

// A centrally-symmetric radial blob (for the symmetry-preservation check).
template <class T>
std::vector<T> MakeRadial(usize dx, usize dy, usize dz)
{
  std::vector<T> f(dx * dy * dz);
  const double cx = (dx - 1) / 2.0;
  const double cy = (dy - 1) / 2.0;
  const double cz = (dz - 1) / 2.0;
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        const double r2 = (x - cx) * (x - cx) + (y - cy) * (y - cy) + (z - cz) * (z - cz);
        f[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>(100.0 * std::exp(-r2 / 8.0));
      }
    }
  }
  return f;
}

template <class T>
std::vector<T> MakeRamp(usize dx, usize dy, usize dz)
{
  std::vector<T> f(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        f[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>((3 * x + 2 * y + z) % 17);
      }
    }
  }
  return f;
}

void RunSmooth(IFilter& filter, DataStructure& ds, const DataPath& inputPath, const std::vector<float64>& sigma, bool normalize, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(sigma));
  args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(normalize));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

template <class T>
std::vector<T> ReadArray(const DataStructure& ds, const DataPath& path)
{
  const auto& store = ds.getDataRefAs<DataArray<T>>(path).getDataStoreRef();
  std::vector<T> out(store.getSize());
  for(usize i = 0; i < out.size(); ++i)
  {
    out[i] = store.getValue(i);
  }
  return out;
}
} // namespace

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: nonfinite active sigma is rejected before execution", "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const usize dimZ = GENERATE(usize{1}, usize{4});
  const std::vector<usize> activeAxes = (dimZ == 1) ? std::vector<usize>{0, 1} : std::vector<usize>{0, 1, 2};
  const usize axis = GENERATE_COPY(from_range(activeAxes));
  const float64 invalidSigma = GENERATE(std::numeric_limits<float64>::infinity(), -std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::quiet_NaN());
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 4, 4, dimZ, std::vector<float32>(16 * dimZ, 1.0F));
  requireSmoothingInput(dataStructure, inputPath, dimZ);
  std::vector<float64> sigma{1.0, 1.0, 1.0};
  sigma.at(axis) = invalidSigma;
  const SmoothFilter filter;
  const auto preflight = filter.preflight(dataStructure, makeSmoothingArguments(inputPath, sigma));
  // This deliberately stops at preflight. Invalid coefficients must never reach an unchecked output conversion.
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: nonfinite active spacing is rejected before execution", "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const usize dimZ = GENERATE(usize{1}, usize{4});
  const std::vector<usize> activeAxes = (dimZ == 1) ? std::vector<usize>{0, 1} : std::vector<usize>{0, 1, 2};
  const usize axis = GENERATE_COPY(from_range(activeAxes));
  const float32 invalidSpacing = GENERATE(std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity(), std::numeric_limits<float32>::quiet_NaN());
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 4, 4, dimZ, std::vector<float32>(16 * dimZ, 1.0F));
  requireSmoothingInput(dataStructure, inputPath, dimZ);
  const DataPath geometryPath = inputPath.getParent().getParent();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geometryPath));
  std::array<float32, 3> spacing{1.0F, 1.0F, 1.0F};
  spacing.at(axis) = invalidSpacing;
  dataStructure.getDataRefAs<ImageGeom>(geometryPath).setSpacing({spacing.at(0), spacing.at(1), spacing.at(2)});
  const SmoothFilter filter;
  const auto preflight = filter.preflight(dataStructure, makeSmoothingArguments(inputPath, {1.0, 1.0, 1.0}));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: invalid effective active scale is rejected before execution", "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const usize dimZ = GENERATE(usize{1}, usize{4});
  const std::vector<usize> activeAxes = (dimZ == 1) ? std::vector<usize>{0, 1} : std::vector<usize>{0, 1, 2};
  const usize axis = GENERATE_COPY(from_range(activeAxes));
  const bool underflow = GENERATE(false, true);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 4, 4, dimZ, std::vector<float32>(16 * dimZ, 1.0F));
  requireSmoothingInput(dataStructure, inputPath, dimZ);
  const DataPath geometryPath = inputPath.getParent().getParent();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geometryPath));
  std::array<float32, 3> spacing{1.0F, 1.0F, 1.0F};
  spacing.at(axis) = underflow ? std::numeric_limits<float32>::max() : 0.5F;
  dataStructure.getDataRefAs<ImageGeom>(geometryPath).setSpacing({spacing.at(0), spacing.at(1), spacing.at(2)});
  std::vector<float64> sigma{1.0, 1.0, 1.0};
  sigma.at(axis) = underflow ? std::numeric_limits<float64>::denorm_min() : std::numeric_limits<float64>::max();
  const SmoothFilter filter;
  const auto preflight = filter.preflight(dataStructure, makeSmoothingArguments(inputPath, sigma));
  // Positive finite operands can produce zero or infinity for sigma / abs(spacing). No cutoff on finite scales is imposed.
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: finite negative spacing and unused Z remain accepted", "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const float64 unusedSigma = GENERATE(0.0, -1.0, std::numeric_limits<float64>::infinity(), std::numeric_limits<float64>::quiet_NaN());
  const float32 unusedSpacing = GENERATE(0.0F, -1.0F, std::numeric_limits<float32>::infinity(), std::numeric_limits<float32>::quiet_NaN());
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 4, 4, 1, std::vector<float32>(16, 1.0F));
  requireSmoothingInput(dataStructure, inputPath, 1);
  const DataPath geometryPath = inputPath.getParent().getParent();
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(geometryPath));
  dataStructure.getDataRefAs<ImageGeom>(geometryPath).setSpacing({-1.0F, -2.0F, unusedSpacing});
  const SmoothFilter filter;
  const auto preflight = filter.preflight(dataStructure, makeSmoothingArguments(inputPath, {1.0, 1.0, unusedSigma}));
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: each selected validation branch has a diagnostic", "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const usize failure = GENERATE(usize{0}, usize{1}, usize{2}, usize{3}, usize{4});
  const std::array<int32, 5> expectedCodes{ImageProcessing::k_SmoothingGaussianInvalidSigma, ImageProcessing::k_SmoothingGaussianInvalidSpacing, ImageProcessing::k_SmoothingGaussianInvalidScale,
                                           ImageProcessing::k_SmoothingGaussianInvalidCoefficients, ImageProcessing::k_SmoothingGaussianInvalidCoefficients};
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(dataStructure, 4, 4, 1, std::vector<float32>(16, 1.0F));
  requireSmoothingInput(dataStructure, inputPath, 1);
  std::vector<float64> sigma{1.0, 1.0, 0.0};
  if(failure == 0)
  {
    sigma.at(0) = 0.0;
  }
  if(failure == 1)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()));
    dataStructure.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()).setSpacing({1e-9F, 1.0F, 1.0F});
  }
  if(failure == 2)
  {
    sigma.at(0) = std::numeric_limits<float64>::max();
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()));
    dataStructure.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()).setSpacing({0.5F, 1.0F, 1.0F});
  }
  // At this finite scale exp(L/scale) and cos(W/scale) round to 1, giving 1-4+6-4+1 == 0 in coefficient denominators.
  if(failure == 3)
  {
    sigma.at(0) = 1e20;
  }
  // This quotient stays positive, but the trigonometric coefficient arguments overflow.
  if(failure == 4)
  {
    sigma.at(0) = std::numeric_limits<float64>::denorm_min();
  }
  const SmoothFilter filter;
  const auto preflight = filter.preflight(dataStructure, makeSmoothingArguments(inputPath, sigma));
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  REQUIRE(preflight.outputActions.errors().front().code == expectedCodes.at(failure));
  REQUIRE(preflight.outputActions.errors().front().message.find("Image Geometry") != std::string::npos);
}

TEMPLATE_TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: constants retain type shape and selected endpoint behavior",
                   "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]", int8, int16, int32, int64, float32, float64)
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope algorithmScope(scenario);
  const usize dimZ = GENERATE(usize{1}, usize{4});
  const usize endpoint = GENERATE(usize{0}, usize{1}, usize{2});
  TestType value = TestType{0};
  if constexpr(std::is_integral_v<TestType>)
  {
    if(endpoint == 1)
    {
      value = std::numeric_limits<TestType>::max();
    }
    if(endpoint == 2)
    {
      value = std::numeric_limits<TestType>::lowest();
    }
  }
  const bool clips = endpoint == 1 && std::is_integral_v<TestType> && sizeof(TestType) >= sizeof(int32);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 4, 4, dimZ, std::vector<TestType>(16 * dimZ, value));
  const SmoothFilter filter;
  const Arguments args = makeSmoothingArguments(inputPath, {1.0, 1.0, 1.0});
  const auto preflight = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflight.outputActions);
  REQUIRE(preflight.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflight.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<TestType>());
  REQUIRE(actionPtr->dims() == ShapeType{dimZ, 4, 4});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});
  const auto executed = algorithmScope.executeFilter(filter, dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executed.result);
  REQUIRE(countSmoothingWarning(executed.result, ImageProcessing::k_SmoothingGaussianClippedIntegerOutput) == (clips ? 1 : 0));
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  const auto& output = dataStructure.getDataRefAs<IDataArray>(outputPath);
  REQUIRE(output.getDataType() == GetDataType<TestType>());
  REQUIRE(output.getTupleShape() == ShapeType{dimZ, 4, 4});
  REQUIRE(output.getComponentShape() == ShapeType{1});
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const auto& values = dataStructure.getDataRefAs<DataArray<TestType>>(outputPath).getDataStoreRef();
  for(usize index = 0; index < values.getSize(); ++index)
  {
    REQUIRE(values.getValue(index) == value);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: float64 output retains float32 interpass precision", "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  UnitTest::AlgorithmTestScope algorithmScope(scenario);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<float64>(dataStructure, 4, 4, 1, std::vector<float64>(16, 16777216.25));
  const SmoothFilter filter;
  const auto result = algorithmScope.executeFilter(filter, dataStructure, makeSmoothingArguments(inputPath, {1.0, 1.0, 0.0}));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(dataStructure.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::float64);
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<Float64Array>(outputPath));
  const auto& values = dataStructure.getDataRefAs<Float64Array>(outputPath).getDataStoreRef();
  // Unit DC gain preserves the constant until the required Float32 materialization rounds it to 2^24.
  for(usize index = 0; index < values.getSize(); ++index)
  {
    REQUIRE(values.getValue(index) == 16777216.0);
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: direct and buffered terminal conversion follow literal expectations",
          "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const bool buffered = GENERATE(false, true);
  const std::array<float32, 5> input{93.75F, 127.75F, 128.0F, -128.75F, -129.0F};
  const std::array<int8, 5> expected{93, 127, 127, -128, -128};
  const usize fixture = GENERATE(usize{0}, usize{1}, usize{2}, usize{3}, usize{4});
  const DataStore<float32> source(ShapeType{1}, ShapeType{1}, input.at(fixture));
  DataStore<int8> output(ShapeType{1}, ShapeType{1}, int8{0});
  ImageProcessing::detail::PointwiseExecutionOptions options;
  options.forceBuffered = buffered;
  options.targetScratchBytes = 5;
  const std::atomic_bool shouldCancel{false};
  const Result<> result = ImageProcessing::CastFloat32StoreTo<int8>(source, output, shouldCancel, options);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(countSmoothingWarning(result, ImageProcessing::k_SmoothingGaussianClippedIntegerOutput) == (fixture == 2 || fixture == 4 ? 1 : 0));
  REQUIRE(output.getValue(0) == expected.at(fixture));
}

TEMPLATE_TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: floating terminal values retain nonfinite behavior", "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]",
                   float32, float64)
{
  UnitTest::LoadPlugins();
  const bool buffered = GENERATE(false, true);
  DataStore<float32> source(ShapeType{4}, ShapeType{1}, 93.75F);
  source.setValue(1, std::numeric_limits<float32>::infinity());
  source.setValue(2, -std::numeric_limits<float32>::infinity());
  source.setValue(3, std::numeric_limits<float32>::quiet_NaN());
  DataStore<TestType> output(ShapeType{4}, ShapeType{1}, TestType{0});
  ImageProcessing::detail::PointwiseExecutionOptions options;
  options.forceBuffered = buffered;
  options.targetScratchBytes = 5;
  const std::atomic_bool shouldCancel{false};
  const Result<> result = ImageProcessing::CastFloat32StoreTo<TestType>(source, output, shouldCancel, options);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.warnings().empty());
  REQUIRE(output.getValue(0) == TestType{93.75});
  REQUIRE(std::isinf(output.getValue(1)));
  REQUIRE(output.getValue(1) > TestType{0});
  REQUIRE(std::isinf(output.getValue(2)));
  REQUIRE(output.getValue(2) < TestType{0});
  REQUIRE(std::isnan(output.getValue(3)));
}

TEMPLATE_TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: terminal nonfinite integer errors and binary64 boundaries",
                   "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]", int32, int64)
{
  UnitTest::LoadPlugins();
  const bool buffered = GENERATE(false, true);
  const bool nonfinite = GENERATE(false, true);
  const float32 invalidValue = GENERATE(std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity(), std::numeric_limits<float32>::quiet_NaN());
  const float32 upper = std::is_same_v<TestType, int32> ? 2147483648.0F : 9223372036854775808.0F;
  DataStore<float32> source(ShapeType{2}, ShapeType{1}, upper);
  source.setValue(1, nonfinite ? invalidValue : -upper);
  DataStore<TestType> output(ShapeType{2}, ShapeType{1}, TestType{0});
  ImageProcessing::detail::PointwiseExecutionOptions options;
  options.forceBuffered = buffered;
  options.targetScratchBytes = 1;
  const std::atomic_bool shouldCancel{false};
  const Result<> result = ImageProcessing::CastFloat32StoreTo<TestType>(source, output, shouldCancel, options);
  REQUIRE(countSmoothingWarning(result, ImageProcessing::k_SmoothingGaussianClippedIntegerOutput) == 1);
  if(nonfinite)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == ImageProcessing::k_SmoothingGaussianNonFiniteIntegerOutput);
    REQUIRE(result.errors().front().message.find("index 1") != std::string::npos);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(output.getValue(0) == std::numeric_limits<TestType>::max());
    REQUIRE(output.getValue(1) == std::numeric_limits<TestType>::lowest());
  }
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: clipping warning survives later transfer failure and cancellation",
          "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const usize outcome = GENERATE(usize{0}, usize{1}, usize{2});
  std::atomic_bool shouldCancel{false};
  const SmoothingTransferStore<float32> source(8, 128.0F, {.ReadAt = outcome == 0 ? usize{2} : usize{0}});
  SmoothingTransferStore<int8> output(8, int8{0}, {.WriteAt = outcome == 1 ? usize{2} : usize{0}, .CancelOnWritePtr = outcome == 2 ? &shouldCancel : nullptr});
  ImageProcessing::detail::PointwiseExecutionOptions options;
  options.forceBuffered = true;
  options.targetScratchBytes = 5;
  const Result<> result = ImageProcessing::CastFloat32StoreTo<int8>(source, output, shouldCancel, options);
  REQUIRE(countSmoothingWarning(result, ImageProcessing::k_SmoothingGaussianClippedIntegerOutput) == 1);
  REQUIRE(countSmoothingWarning(result, -23692) >= 1);
  REQUIRE(countSmoothingWarning(result, -23693) >= 1);
  REQUIRE(output.getValue(0) == 127);
  if(outcome < 2)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == (outcome == 0 ? -23690 : -23691));
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(shouldCancel.load());
  }
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: selected plane sinks retain successful warnings before error or cancellation",
          "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const SmoothingTransferStore<float32> source(64, 0.0F);
  std::atomic_bool shouldCancel{false};
  const bool cancel = GENERATE(false, true);
  usize calls = 0;
  const ImageProcessing::RecursiveGaussianPlaneAxis first{.axis = 0, .sigma = 1.0, .spacing = 1.0, .order = 0, .normalizeAcrossScale = false, .CheckedSmoothing = true};
  const ImageProcessing::RecursiveGaussianPlaneAxis second{.axis = 1, .sigma = 1.0, .spacing = 1.0, .order = 0, .normalizeAcrossScale = false, .CheckedSmoothing = true};
  const auto sink = [&](usize, nonstd::span<float32>) -> Result<> {
    ++calls;
    if(calls == 2)
    {
      return MakeErrorResult(-23694, "Injected later smoothing sink failure.");
    }
    if(cancel)
    {
      shouldCancel.store(true);
    }
    return MakeWarningVoidResult(-23695, "Injected smoothing sink warning.");
  };
  const Result<> result = ImageProcessing::RecursiveGaussianXYPlaneCascadeToSink<float32>(source, SizeVec3{4, 4, 4}, first, second, shouldCancel, {}, sink);
  REQUIRE(countSmoothingWarning(result, -23695) == 1);
  REQUIRE(countSmoothingWarning(result, -23692) >= 1);
  if(cancel)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(calls == 1);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -23694);
  }
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: selected tiled transfers keep prior row warnings on failure", "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const bool write = GENERATE(false, true);
  SmoothingTransferStore<float32> store(8, 0.0F, {.ReadAt = write ? usize{0} : usize{2}, .WriteAt = write ? usize{2} : usize{0}});
  std::array<float32, 2> values{1.0F, 2.0F};
  const Result<> result = write ? ImageProcessing::WriteRecursiveGaussian2DBlock<float32>(store, 4, 0, 0, 2, 1, values, true) :
                                  ImageProcessing::detail::ReadRecursiveGaussian2DBlock<float32>(store, 4, 0, 2, 0, 1, values, true);
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == (write ? -23691 : -23690));
  REQUIRE(countSmoothingWarning(result, write ? -23693 : -23692) == 1);
}

TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: checked axis materializations narrow finite overflow explicitly",
          "[ImageProcessing][Issue1874][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const uint32 axis = GENERATE(uint32{0}, uint32{1}, uint32{2});
  const bool fast = GENERATE(false, true);
  const DataStore<float64> source(ShapeType{5, 5, 5}, ShapeType{1}, 2.0 * static_cast<float64>(std::numeric_limits<float32>::max()));
  DataStore<float32> output(ShapeType{5, 5, 5}, ShapeType{1}, 0.0F);
  ImageProcessing::RecursiveGaussianPassOptions options;
  options.CheckedSmoothing = true;
  options.useInMemoryFastPath = fast;
  options.stagingByteBudget = 256;
  const std::atomic_bool shouldCancel{false};
  const Result<> result = ImageProcessing::RecursiveGaussianAxisPass<float64>(source, output, SizeVec3{5, 5, 5}, axis, 1.0, 1.0, 0, false, shouldCancel, {}, options);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  for(usize index = 0; index < output.getSize(); ++index)
  {
    REQUIRE(std::isinf(output.getValue(index)));
    REQUIRE(output.getValue(index) > 0.0F);
  }
}

// -----------------------------------------------------------------------------
// (1) Smoothing preserves a constant image (all signed types). A normalized (DC-gain-1) Gaussian of a constant is the
//     constant, incl. at the edge-extended boundary. Margin 1.0 tolerates the final float32->int static_cast truncation.
// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: constant image preserved", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]", int8, int16, int32, int64, float32,
                   float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 12;
  const std::vector<T> field(d * d * d, static_cast<T>(20));
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<T>(ds, d, d, d, field);
  SmoothFilter filter;
  RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false);
  const std::vector<T> out = ReadArray<T>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
  for(T v : out)
  {
    REQUIRE(std::abs(static_cast<double>(v) - 20.0) <= 1.0);
  }
}

// -----------------------------------------------------------------------------
// (2) Smoothing preserves central symmetry (float64 radial blob) and reduces the peak (variance reduction).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: symmetric blob stays symmetric", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 13; // odd so there is a true center voxel
  const std::vector<float64> field = MakeRadial<float64>(d, d, d);
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, d, field);
  SmoothFilter filter;
  RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false);
  const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));

  auto at = [&](usize x, usize y, usize z) { return out[(z * d + y) * d + x]; };
  const usize c = d / 2;
  for(usize y = 0; y < d; ++y)
  {
    for(usize x = 0; x < d; ++x)
    {
      REQUIRE(at(x, y, c) == Approx(at(d - 1 - x, y, c)).margin(1e-3)); // mirror across X (float32 intermediates -> ~1e-3)
      REQUIRE(at(x, y, c) == Approx(at(x, d - 1 - y, c)).margin(1e-3)); // mirror across Y
    }
  }
  REQUIRE(at(c, c, c) < 100.0); // peak reduced by smoothing
  REQUIRE(at(c, c, c) > 0.0);
}

// -----------------------------------------------------------------------------
// (4) Preflight rejects unsigned input (SignedScalar policy) and too-small dims.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: preflight rejections", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  SECTION("unsigned input rejected")
  {
    DataStructure ds;
    const std::vector<uint16> field(8 * 8 * 8, uint16{3});
    const DataPath inputPath = rt::BuildImageFromPattern<uint16>(ds, 8, 8, 8, field);
    SmoothFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  }
  SECTION("too-few-pixels rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(3 * 8 * 8, int16{1}); // X = 3 < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 3, 8, 8, field);
    SmoothFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  // The "too-few-pixels" arm above trips via X == 3. ValidateSeparableImageDims separately rejects a genuinely 3D image
  // (Z > 1) whose Z < 4 (`zIs2D || dims[2] >= 4`, zIs2D == (dims[2]==1)); a Z of 2 or 3 (NOT treated as 2D) hits that
  // arm, while Z == 1 is accepted as a 2D image. All other params here are valid so the dims check is the only failure.
  SECTION("3D image with Z==2 rejected (too few Z pixels)")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 2, int16{1}); // Z == 2 (> 1, so not 2D) and < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 2, field);
    SmoothFilter filter;
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  SECTION("3D image with Z==3 rejected (too few Z pixels)")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 3, int16{1}); // Z == 3 (> 1, so not 2D) and < 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 3, field);
    SmoothFilter filter;
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_RecursiveGaussianTooFewPixels);
  }
  SECTION("Z==1 accepted as a 2D image")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 1, int16{1}); // Z == 1 -> treated as 2D, so Z is not required to be >= 4
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 1, field);
    SmoothFilter filter;
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  }
  SECTION("non-positive sigma rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 8, int16{1}); // valid dims + signed type; only sigma is invalid
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 8, field);
    SmoothFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{0.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_SmoothingGaussianInvalidSigma);
  }
  SECTION("degenerate spacing rejected")
  {
    DataStructure ds;
    const std::vector<int16> field(8 * 8 * 8, int16{1}); // valid dims/type/sigma; only the X spacing is degenerate
    const DataPath inputPath = rt::BuildImageFromPattern<int16>(ds, 8, 8, 8, field);
    ds.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()).setSpacing(FloatVec3{0.0f, 1.0f, 1.0f});
    SmoothFilter filter;
    const DataPath geomPath = inputPath.getParent().getParent();
    Arguments args;
    args.insertOrAssign(SmoothFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(SmoothFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(SmoothFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(SmoothFilter::k_Sigma_Key, std::make_any<std::vector<float64>>(std::vector<float64>{1.0, 1.0, 1.0}));
    args.insertOrAssign(SmoothFilter::k_NormalizeAcrossScale_Key, std::make_any<bool>(false));
    const auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
    REQUIRE(preflightResult.outputActions.errors().front().code == nx::core::ImageProcessing::k_SmoothingGaussianInvalidSpacing);
  }
}

// -----------------------------------------------------------------------------
// (5) FromSIMPLJson: the per-axis Sigma vector (legacy FloatVec3 object shape), the NormalizeAcrossScale bool, and the
//     geometry/array/name DataPaths. Sigma uses DoubleVec3FilterParameterConverter -> std::vector<float64>.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: FromSIMPLJson", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const nlohmann::json json = {
      {"Sigma", {{"x", 2.0}, {"y", 3.0}, {"z", 4.0}}},
      {"NormalizeAcrossScale", true},
      {"SelectedCellArrayPath", {{"Data Container Name", "ImageDC"}, {"Attribute Matrix Name", "CellAM"}, {"Data Array Name", "InArray"}}},
      {"NewCellArrayName", "SmoothOut"},
  };
  const Result<Arguments> result = SmoothingRecursiveGaussianImageFilter::FromSIMPLJson(json);
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  const Arguments args = result.value();
  REQUIRE(args.value<VectorFloat64Parameter::ValueType>(SmoothFilter::k_Sigma_Key) == std::vector<float64>{2.0, 3.0, 4.0});
  REQUIRE(args.value<bool>(SmoothFilter::k_NormalizeAcrossScale_Key) == true);
  REQUIRE(args.value<DataPath>(SmoothFilter::k_InputImageGeomPath_Key) == DataPath({"ImageDC"}));
  REQUIRE(args.value<DataPath>(SmoothFilter::k_InputImageDataPath_Key) == DataPath({"ImageDC", "CellAM", "InArray"}));
  REQUIRE(args.value<std::string>(SmoothFilter::k_OutputImageArrayName_Key) == "SmoothOut");
}

// -----------------------------------------------------------------------------
// (6) Anisotropic per-axis sigma: a larger sigma on an axis spreads a central impulse MORE along that axis. This guards
//     the per-axis sigma[axis] indexing in SmoothingRecursiveGaussianExecuteFn's custom "last-axis-first" cascade order
//     -- a symmetric-sigma test cannot catch a sigma/axis swap. Smoothing has no live-ITK oracle, so this is an
//     analytic directional check (the Gz(0) factor cancels, so the X-vs-Y comparison reduces to exp(2/sy^2 - 2/sx^2)).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: anisotropic sigma spreads along the larger-sigma axis", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 15;
  std::vector<float64> field(d * d * d, 0.0);
  const usize c = d / 2;
  field[(c * d + c) * d + c] = 1.0; // central impulse
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, d, field);
  SmoothFilter filter;
  RunSmooth(filter, ds, input, {3.0, 1.0, 1.0}, false); // strong blur along X, weak along Y/Z
  const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
  auto at = [&](usize x, usize y, usize z) { return out[(z * d + y) * d + x]; };
  REQUIRE(at(c + 2, c, c) > at(c, c + 2, c));                       // larger sigma_x -> more spread along X than Y
  REQUIRE(at(c, c + 2, c) == Approx(at(c, c, c + 2)).margin(1e-6)); // sigma_y == sigma_z
  REQUIRE(at(c + 2, c, c) == Approx(at(c - 2, c, c)).margin(1e-6)); // symmetric along X
}

// -----------------------------------------------------------------------------
// (7) Anisotropic spacing: a larger spacing on an axis shrinks that axis's voxel-domain sigma (sigmad = sigma/spacing),
//     so the impulse spreads LESS along the larger-spacing axis. Guards the per-axis spacing[axis] indexing.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: anisotropic spacing scales the per-axis blur", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 15;
  std::vector<float64> field(d * d * d, 0.0);
  const usize c = d / 2;
  field[(c * d + c) * d + c] = 1.0;
  DataStructure ds;
  const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, d, field);
  ds.getDataRefAs<ImageGeom>(input.getParent().getParent()).setSpacing(FloatVec3{2.0f, 1.0f, 1.0f}); // coarser X spacing
  SmoothFilter filter;
  RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false); // isotropic physical sigma -> sigmad_x=1 < sigmad_y=sigmad_z=2
  const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
  auto at = [&](usize x, usize y, usize z) { return out[(z * d + y) * d + x]; };
  REQUIRE(at(c + 2, c, c) < at(c, c + 2, c));                       // coarser X spacing -> smaller sigmad_x -> less spread along X
  REQUIRE(at(c, c + 2, c) == Approx(at(c, c, c + 2)).margin(1e-6)); // Y and Z share spacing + sigma
}

// -----------------------------------------------------------------------------
// (8) 2D (Z==1) smoothing exercises the effDim==2 cascade path in SmoothingRecursiveGaussianExecuteFn: a constant is
//     preserved and a symmetric blob stays symmetric.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: 2D smoothing preserves a constant and stays symmetric", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 13;
  SECTION("constant preserved")
  {
    const std::vector<float64> field(d * d * 1, 20.0);
    DataStructure ds;
    const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, 1, field);
    SmoothFilter filter;
    RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false);
    const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
    for(float64 v : out)
    {
      REQUIRE(v == Approx(20.0).margin(1e-4));
    }
  }
  SECTION("symmetric blob stays symmetric")
  {
    const std::vector<float64> field = MakeRadial<float64>(d, d, 1);
    DataStructure ds;
    const DataPath input = rt::BuildImageFromPattern<float64>(ds, d, d, 1, field);
    SmoothFilter filter;
    RunSmooth(filter, ds, input, {2.0, 2.0, 2.0}, false);
    const std::vector<float64> out = ReadArray<float64>(ds, DataPath({"Image Geometry", "CellData", "Output"}));
    auto at = [&](usize x, usize y) { return out[y * d + x]; };
    for(usize y = 0; y < d; ++y)
    {
      for(usize x = 0; x < d; ++x)
      {
        REQUIRE(at(x, y) == Approx(at(d - 1 - x, y)).margin(1e-3)); // mirror across X
        REQUIRE(at(x, y) == Approx(at(x, d - 1 - y)).margin(1e-3)); // mirror across Y
      }
    }
  }
}

// -----------------------------------------------------------------------------
// (9) NormalizeAcrossScale is a no-op for order-0 smoothing (ITK applies it only to derivative orders 1/2). Pin the
//     documented invariant: normalize on/off produce byte-identical output.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: NormalizeAcrossScale is a no-op for smoothing", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize d = 12;
  const std::vector<float64> field = MakeRadial<float64>(d, d, d);
  DataStructure dsOff;
  const DataPath inOff = rt::BuildImageFromPattern<float64>(dsOff, d, d, d, field);
  SmoothFilter fOff;
  RunSmooth(fOff, dsOff, inOff, {2.0, 2.0, 2.0}, false);
  DataStructure dsOn;
  const DataPath inOn = rt::BuildImageFromPattern<float64>(dsOn, d, d, d, field);
  SmoothFilter fOn;
  RunSmooth(fOn, dsOn, inOn, {2.0, 2.0, 2.0}, true);
  const std::vector<float64> a = ReadArray<float64>(dsOff, DataPath({"Image Geometry", "CellData", "Output"}));
  const std::vector<float64> b = ReadArray<float64>(dsOn, DataPath({"Image Geometry", "CellData", "Output"}));
  REQUIRE(a == b); // byte-identical: order-0 ignores normalizeAcrossScale
}

// -----------------------------------------------------------------------------
TEMPLATE_TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: fused plane path matches direct path", "[ImageProcessing][SmoothingRecursiveGaussianImageFilter]", int16, float64)
{
  using T = TestType;
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const usize dimZ = GENERATE(1ULL, 8ULL);
  const FloatVec3 spacing{0.6f, 1.4f, 2.2f};
  const std::vector<T> field = MakeRamp<T>(12, 11, dimZ);
  const std::vector<float64> sigma{1.25, 2.0, 1.6};
  const DataPath outputPath({"Image Geometry", "CellData", "Output"});

  DataStructure directDs;
  const DataPath directInput = rt::BuildImageFromPattern<T>(directDs, 12, 11, dimZ, field);
  directDs.getDataRefAs<ImageGeom>(directInput.getParent().getParent()).setSpacing(spacing);
  {
    const ForceInCoreAlgorithmGuard guard;
    ResetAlgorithmPathExecutionCounts();
    SmoothFilter filter;
    RunSmooth(filter, directDs, directInput, sigma, true);
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    REQUIRE(counts.InCoreOnInMemoryStore == 1);
    REQUIRE(counts.OutOfCore == 0);
  }

  DataStructure fusedDs;
  const DataPath fusedInput = rt::BuildImageFromPattern<T>(fusedDs, 12, 11, dimZ, field);
  fusedDs.getDataRefAs<ImageGeom>(fusedInput.getParent().getParent()).setSpacing(spacing);
  {
    const ForceOocAlgorithmGuard guard(true);
    ResetAlgorithmPathExecutionCounts();
    SmoothFilter filter;
    RunSmooth(filter, fusedDs, fusedInput, sigma, true);
    const AlgorithmPathExecutionCounts counts = GetAlgorithmPathExecutionCounts();
    REQUIRE(counts.OutOfCoreOnInMemoryStore == 1);
    REQUIRE(counts.InCore == 0);
  }

  REQUIRE(ReadArray<T>(directDs, outputPath) == ReadArray<T>(fusedDs, outputPath));
}

// ITK-sourced real-image golden -- "default" case. Duplicates ITKSmoothingRecursiveGaussianImageTest.cpp(default) on
// OUR ITK-free filter: read RA-Float.nrrd (float32), run OUR filter with the ITK case's params (all defaults; per-axis
// Sigma {1,1,1}, NormalizeAcrossScale false), then compare to ITK's committed baseline at tol 0.0001.
//
// The durable golden is the committed baseline image on disk. The (rgb_image) RGB case (VM1111Shrink-RGB.png) is
// SKIPPED (RGB, out of scope).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::SmoothingRecursiveGaussianImageFilter: ITK real-image golden (default)", "[ImageProcessing][ItkGolden][SmoothingRecursiveGaussianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // NOTE: bind every side-effecting Result to a named local before SIMPLNX_RESULT_REQUIRE_VALID (that macro double-
  // evaluates its argument; inlining a ds-mutating read/execute would run it twice and the second would fail).
  DataStructure ds;
  const Result<> readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("RA-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(SmoothingRecursiveGaussianImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  SmoothingRecursiveGaussianImageFilter filter;
  const auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  const auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: compare to ITK's committed baseline at the ITK test's tolerance 0.0001.
  const DataPath bGeom({"Baseline Geometry"});
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const Result<> readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_SmoothingRecursiveGaussianImageFilter_default.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const Result<> compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.0001);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
