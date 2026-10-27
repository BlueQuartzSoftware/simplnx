#include "ItkGoldenTestUtils.hpp"
#include "ReconstructionFilterTestUtils.hpp"

#include "ImageProcessing/Filters/CurvatureFlowImageFilter.hpp"

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Extent.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/FiniteDifferenceEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/TerminalImageConversion.hpp"
#include "simplnx/Utilities/ImageProcessing/WorkingMemory.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;
namespace rt = recon_test;

namespace
{
using CurvFilter = CurvatureFlowImageFilter;

// Sets the geometry, input, and output keys plus TimeStep and NumberOfIterations on the Arguments. The helper runs
// preflight and execute, and requires that both steps succeed.
void RunCurvatureFlow(IFilter& filter, DataStructure& ds, const DataPath& inputPath, float64 timeStep, uint32 numberOfIterations, const std::string& outputName = "Output")
{
  const DataPath geomPath = inputPath.getParent().getParent();
  Arguments args;
  args.insertOrAssign(CurvFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(CurvFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(CurvFilter::k_OutputImageArrayName_Key, std::make_any<std::string>(outputName));
  args.insertOrAssign(CurvFilter::k_TimeStep_Key, std::make_any<float64>(timeStep));
  args.insertOrAssign(CurvFilter::k_NumberOfIterations_Key, std::make_any<uint32>(numberOfIterations));
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
}

// A deterministic non-constant pattern with values in [0, 16], representable in EVERY scalar type (incl. uint8/int8)
// so the full 10-type parity grid can reuse it. Varies along all three axes so the curvature-flow update is non-trivial.
template <class T>
std::vector<T> MakeRamp(usize dx, usize dy, usize dz)
{
  std::vector<T> v(dx * dy * dz);
  for(usize z = 0; z < dz; ++z)
  {
    for(usize y = 0; y < dy; ++y)
    {
      for(usize x = 0; x < dx; ++x)
      {
        v[rt::FlatIndex(x, y, z, dx, dy)] = static_cast<T>((3 * x + 2 * y + z) % 17);
      }
    }
  }
  return v;
}
} // namespace

namespace
{
// A real resident forwarding endpoint that is deliberately not a DataStore. It selects the streamed final-cast
// fallback without pretending to be disk-backed. Optional transfer faults exercise the checked no-op I/O contract.
template <class T>
class CurvatureFlowForwardingStore : public AbstractDataStore<T>
{
public:
  explicit CurvatureFlowForwardingStore(const ShapeType& shape, bool failRead = false, bool failWrite = false, bool reportOutOfCore = false, std::atomic_bool* cancelOnReadPtr = nullptr)
  : m_Store(shape, ShapeType{1}, T{})
  , m_FailRead(failRead)
  , m_FailWrite(failWrite)
  , m_ReportOutOfCore(reportOutOfCore)
  , m_CancelOnReadPtr(cancelOnReadPtr)
  {
  }
  ~CurvatureFlowForwardingStore() override = default;
  CurvatureFlowForwardingStore(const CurvatureFlowForwardingStore&) = delete;
  CurvatureFlowForwardingStore(CurvatureFlowForwardingStore&&) = delete;
  CurvatureFlowForwardingStore& operator=(const CurvatureFlowForwardingStore&) = delete;
  CurvatureFlowForwardingStore& operator=(CurvatureFlowForwardingStore&&) = delete;

  usize getNumberOfTuples() const override
  {
    return m_Store.getNumberOfTuples();
  }
  const ShapeType& getTupleShape() const override
  {
    return m_Store.getTupleShape();
  }
  usize getNumberOfComponents() const override
  {
    return 1;
  }
  const ShapeType& getComponentShape() const override
  {
    return m_Store.getComponentShape();
  }
  Result<> resizeTuples(const ShapeType& shape) override
  {
    return m_Store.resizeTuples(shape);
  }
  DataType getDataType() const override
  {
    return m_Store.getDataType();
  }
  IDataStore::StoreType getStoreType() const override
  {
    return m_ReportOutOfCore ? IDataStore::StoreType::OutOfCore : IDataStore::StoreType::InMemory;
  }
  std::map<std::string, std::string> getRecoveryMetadata() const override
  {
    return {};
  }
  usize getTypeSize() const override
  {
    return sizeof(T);
  }
  std::unique_ptr<IDataStore> deepCopy(const std::string& format) const override
  {
    return m_Store.deepCopy(format);
  }
  std::unique_ptr<IDataStore> createNewInstance() const override
  {
    return m_Store.createNewInstance();
  }
  std::pair<int32, std::string> writeBinaryFile(const std::string& path) const override
  {
    return m_Store.writeBinaryFile(path);
  }
  std::pair<int32, std::string> writeBinaryFile(std::ostream& stream) const override
  {
    return m_Store.writeBinaryFile(stream);
  }
  T getValue(usize index) const override
  {
    return m_Store.getValue(index);
  }
  void setValue(usize index, T value) override
  {
    m_Store.setValue(index, value);
  }
  Result<> copyIntoBuffer(usize offset, nonstd::span<T> values) const override
  {
    ++m_ReadCount;
    m_MaxReadValues = std::max(m_MaxReadValues, values.size());
    if(m_CancelOnReadPtr != nullptr)
    {
      m_CancelOnReadPtr->store(true);
    }
    Result<> result;
    if(m_FailRead)
    {
      result = MakeErrorResult(-28890, "Injected Curvature Flow input transfer failure.");
    }
    else if(m_ReadCount == m_FailReadAt)
    {
      result = MakeErrorResult(-28892, "Injected delayed Curvature Flow input transfer failure.");
    }
    else
    {
      result = m_Store.copyIntoBuffer(offset, values);
    }
    if(m_WarnOnRead)
    {
      result.warnings().push_back({.code = -28893, .message = "Injected Curvature Flow read warning."});
    }
    return result;
  }
  Result<> copyFromBuffer(usize offset, nonstd::span<const T> values) override
  {
    ++m_WriteCount;
    m_MaxWriteValues = std::max(m_MaxWriteValues, values.size());
    Result<> result = m_FailWrite ? MakeErrorResult(-28891, "Injected Curvature Flow output transfer failure.") : m_Store.copyFromBuffer(offset, values);
    if(m_WarnOnWrite)
    {
      result.warnings().push_back({.code = -28894, .message = "Injected Curvature Flow write warning."});
    }
    if(m_CancelOnWritePtr != nullptr && m_WriteCount == m_CancelWriteAt)
    {
      m_CancelOnWritePtr->store(true);
    }
    return result;
  }
  Result<std::vector<T>> readExtent(const Extent& extent) const override
  {
    return m_Store.readExtent(extent);
  }
  Result<> readExtentIntoBuffer(const Extent& extent, nonstd::span<T> destination) const override
  {
    return m_Store.readExtentIntoBuffer(extent, destination);
  }
  Result<> writeExtent(const Extent& extent, nonstd::span<const T> values) override
  {
    return m_Store.writeExtent(extent, values);
  }
  T at(usize index) const override
  {
    return m_Store.at(index);
  }
  void add(usize index, T value) override
  {
    m_Store.add(index, value);
  }
  void sub(usize index, T value) override
  {
    m_Store.sub(index, value);
  }
  void mul(usize index, T value) override
  {
    m_Store.mul(index, value);
  }
  void div(usize index, T value) override
  {
    m_Store.div(index, value);
  }
  void rem(usize index, T value) override
  {
    m_Store.rem(index, value);
  }
  void bitwiseAND(usize index, T value) override
  {
    m_Store.bitwiseAND(index, value);
  }
  void bitwiseOR(usize index, T value) override
  {
    m_Store.bitwiseOR(index, value);
  }
  void bitwiseXOR(usize index, T value) override
  {
    m_Store.bitwiseXOR(index, value);
  }
  void bitwiseLShift(usize index, T value) override
  {
    m_Store.bitwiseLShift(index, value);
  }
  void bitwiseRShift(usize index, T value) override
  {
    m_Store.bitwiseRShift(index, value);
  }
  void byteSwap(usize index) override
  {
    m_Store.byteSwap(index);
  }
  void swap(usize left, usize right) override
  {
    m_Store.swap(left, right);
  }
  Result<> readHdf5(const HDF5::DatasetIO& dataset) override
  {
    return m_Store.readHdf5(dataset);
  }
  Result<> writeHdf5(HDF5::DatasetIO& dataset) const override
  {
    return m_Store.writeHdf5(dataset);
  }
  usize readCount() const
  {
    return m_ReadCount;
  }
  usize writeCount() const
  {
    return m_WriteCount;
  }
  usize maxReadValues() const
  {
    return m_MaxReadValues;
  }
  usize maxWriteValues() const
  {
    return m_MaxWriteValues;
  }
  void failReadAt(usize call)
  {
    m_FailReadAt = call;
  }
  void warnOnReads()
  {
    m_WarnOnRead = true;
  }
  void warnOnWrites()
  {
    m_WarnOnWrite = true;
  }
  void cancelOnWrite(usize call, std::atomic_bool& shouldCancel)
  {
    m_CancelWriteAt = call;
    m_CancelOnWritePtr = &shouldCancel;
  }

private:
  DataStore<T> m_Store;
  bool m_FailRead = false;
  bool m_FailWrite = false;
  bool m_ReportOutOfCore = false;
  std::atomic_bool* m_CancelOnReadPtr = nullptr;
  mutable usize m_ReadCount = 0;
  usize m_WriteCount = 0;
  mutable usize m_MaxReadValues = 0;
  usize m_MaxWriteValues = 0;
  usize m_FailReadAt = std::numeric_limits<usize>::max();
  bool m_WarnOnRead = false;
  bool m_WarnOnWrite = false;
  usize m_CancelWriteAt = std::numeric_limits<usize>::max();
  std::atomic_bool* m_CancelOnWritePtr = nullptr;
};

Arguments makeCurvatureFlowArgs(const DataPath& inputPath, float64 timeStep, uint32 iterations)
{
  Arguments args;
  args.insertOrAssign(CurvatureFlowImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(inputPath.getParent().getParent()));
  args.insertOrAssign(CurvatureFlowImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
  args.insertOrAssign(CurvatureFlowImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  args.insertOrAssign(CurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(timeStep));
  args.insertOrAssign(CurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(iterations));
  return args;
}

template <class T>
void requireCurvatureFlowConstant(DataStructure& dataStructure, const DataPath& inputPath, T expected, float64 timeStep, uint32 iterations)
{
  const CurvatureFlowImageFilter filter;
  const Arguments args = makeCurvatureFlowArgs(inputPath, timeStep, iterations);
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == GetDataType<T>());
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, 1});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  REQUIRE(executeResult.result.warnings().empty());
  const DataPath outputPath = inputPath.replaceName("Output");
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<T>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<T>* valuesPtr = nullptr;
  REQUIRE_NOTHROW(valuesPtr = &dataStructure.getDataRefAs<DataArray<T>>(outputPath));
  REQUIRE(valuesPtr->getDataStoreRef().getValue(0) == expected);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
} // namespace

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: invalid time step is rejected before execution", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const float64 timeStep = GENERATE(-0.05, std::numeric_limits<float64>::quiet_NaN(), std::numeric_limits<float64>::infinity(), -std::numeric_limits<float64>::infinity());
  const uint32 iterations = GENERATE(0u, 1u);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 1, 1, 1, {7});
  const CurvatureFlowImageFilter filter;
  const auto result = filter.preflight(dataStructure, makeCurvatureFlowArgs(inputPath, timeStep, iterations));
  // Preflight only: never evolve or cast an invalid-control baseline.
  SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
  REQUIRE(result.outputActions.errors().front().code == ImageProcessing::k_CurvatureFlowInvalidTimeStep);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: invalid active spacing is rejected for no-op requests", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool is3D = GENERATE(false, true);
  const uint32 iterations = GENERATE(0u, 1u);
  const float32 invalidSpacing = GENERATE(0.0F, std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity(), -std::numeric_limits<float32>::infinity());
  const usize activeAxes = is3D ? 3 : 2;
  for(usize axis = 0; axis < activeAxes; ++axis)
  {
    DYNAMIC_SECTION("active axis " << axis)
    {
      DataStructure dataStructure;
      const usize dimZ = is3D ? 2 : 1;
      const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 1, 1, dimZ, std::vector<uint8>(dimZ, 7));
      ImageGeom* geometryPtr = nullptr;
      REQUIRE_NOTHROW(geometryPtr = &dataStructure.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()));
      std::array<float32, 3> spacing = {1.0F, 1.0F, 1.0F};
      spacing.at(axis) = invalidSpacing;
      geometryPtr->setSpacing({spacing.at(0), spacing.at(1), spacing.at(2)});
      const CurvatureFlowImageFilter filter;
      const auto result = filter.preflight(dataStructure, makeCurvatureFlowArgs(inputPath, 0.0, iterations));
      // All executions are deferred: nonfinite spacing can poison even a constant baseline.
      SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
      REQUIRE(result.outputActions.errors().front().code == ImageProcessing::k_CurvatureFlowInvalidSpacing);
      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: finite negative spacing and unused Z remain accepted", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const float32 unusedSpacing = GENERATE(0.0F, std::numeric_limits<float32>::quiet_NaN(), std::numeric_limits<float32>::infinity());
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 1, 1, 1, {7});
  ImageGeom* geometryPtr = nullptr;
  REQUIRE_NOTHROW(geometryPtr = &dataStructure.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()));
  geometryPtr->setSpacing({-1.0F, -2.0F, unusedSpacing});
  const CurvatureFlowImageFilter filter;
  const auto result = filter.preflight(dataStructure, makeCurvatureFlowArgs(inputPath, 0.0, 0));
  SIMPLNX_RESULT_REQUIRE_VALID(result.outputActions);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: constant finite input preserves type and value", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64, float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {TestType{7}});
  // A constant has zero first derivatives, so the independent expected update is exactly zero.
  requireCurvatureFlowConstant(dataStructure, inputPath, TestType{7}, 0.05, 2);
}

TEMPLATE_TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: zero operation copies int64 values exactly", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool zeroIterations = GENERATE(false, true);
  constexpr TestType inputValue = (TestType{1} << 53) + TestType{1};
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {inputValue});
  // The baseline rounds this finite constant to 2^53. Both values are well inside either native integer range.
  // Its constant-image derivative guard returns zero, so zero timestep cannot create a nonfinite update.
  requireCurvatureFlowConstant(dataStructure, inputPath, inputValue, zeroIterations ? 0.05 : 0.0, zeroIterations ? 0u : 1u);
}

TEMPLATE_TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: terminal integer conversion has exact bounds", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]", int8, uint8, int16, uint16,
                   int32, uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  using ImageProcessing::detail::ConvertTerminalImageValue;
  using ImageProcessing::detail::TerminalImageConversionStatus;
  TestType output = 7;
  REQUIRE(ConvertTerminalImageValue(93.75, output) == TerminalImageConversionStatus::Valid);
  REQUIRE(output == TestType{93});
  REQUIRE(ConvertTerminalImageValue(-0.5, output) == TerminalImageConversionStatus::Valid);
  REQUIRE(output == TestType{0});
  const double upper = std::ldexp(1.0, std::numeric_limits<TestType>::digits);
  REQUIRE(ConvertTerminalImageValue(upper, output) == TerminalImageConversionStatus::Clipped);
  REQUIRE(output == std::numeric_limits<TestType>::max());
  if constexpr(std::numeric_limits<TestType>::digits <= 32)
  {
    REQUIRE(ConvertTerminalImageValue(static_cast<double>(std::numeric_limits<TestType>::max()) + 0.5, output) == TerminalImageConversionStatus::Valid);
    REQUIRE(output == std::numeric_limits<TestType>::max());
  }
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(ConvertTerminalImageValue(-upper, output) == TerminalImageConversionStatus::Valid);
    REQUIRE(output == std::numeric_limits<TestType>::lowest());
    REQUIRE(ConvertTerminalImageValue(std::nextafter(-upper - 1.0, -std::numeric_limits<double>::infinity()), output) == TerminalImageConversionStatus::Clipped);
    REQUIRE(output == std::numeric_limits<TestType>::lowest());
    REQUIRE(ConvertTerminalImageValue(-1.5, output) == TerminalImageConversionStatus::Valid);
    REQUIRE(output == TestType{-1});
  }
  else
  {
    REQUIRE(ConvertTerminalImageValue(-1.5, output) == TerminalImageConversionStatus::Clipped);
    REQUIRE(output == TestType{0});
  }
  for(const double value : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()})
  {
    output = 7;
    REQUIRE(ConvertTerminalImageValue(value, output) == TerminalImageConversionStatus::NonFinite);
    REQUIRE(output == TestType{7});
  }
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: floating narrowing retains IEEE values", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  using ImageProcessing::detail::NarrowFloatingImageValue;
  const auto positive = NarrowFloatingImageValue<float32>(std::numeric_limits<float64>::max());
  const auto negative = NarrowFloatingImageValue<float32>(-std::numeric_limits<float64>::max());
  REQUIRE(std::isinf(positive));
  REQUIRE_FALSE(std::signbit(positive));
  REQUIRE(std::isinf(negative));
  REQUIRE(std::signbit(negative));
  REQUIRE(std::isnan(NarrowFloatingImageValue<float32>(std::numeric_limits<float64>::quiet_NaN())));
  REQUIRE(std::signbit(NarrowFloatingImageValue<float32>(-0.0)));
  REQUIRE(NarrowFloatingImageValue<float64>(std::numeric_limits<float32>::max()) == static_cast<float64>(std::numeric_limits<float32>::max()));
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: independent one-step stencil truncates then clips once", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool clip = GENERATE(false, true);
  const bool negativeSpacing = GENERATE(false, true);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 3, 3, 1, {0, 0, 0, 0, 100, 100, 0, 100, 100});
  if(negativeSpacing)
  {
    ImageGeom* geometryPtr = nullptr;
    REQUIRE_NOTHROW(geometryPtr = &dataStructure.getDataRefAs<ImageGeom>(inputPath.getParent().getParent()));
    geometryPtr->setSpacing({-1.0F, -1.0F, std::numeric_limits<float32>::quiet_NaN()});
  }
  const CurvatureFlowImageFilter filter;
  const auto result = filter.execute(dataStructure, makeCurvatureFlowArgs(inputPath, clip ? 10.0 : 0.05, 1));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().size() == (clip ? 1 : 0));
  if(clip)
  {
    REQUIRE(result.result.warnings().front().code == ImageProcessing::k_CurvatureFlowClippedIntegerOutput);
  }
  const IDataArray* outputArrayPtr = nullptr;
  const DataPath outputPath = inputPath.replaceName("Output");
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == DataType::uint8);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 3, 3});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const UInt8Array* outputPtr = nullptr;
  REQUIRE_NOTHROW(outputPtr = &dataStructure.getDataRefAs<UInt8Array>(outputPath));
  // At the center: gradient=(50,50), second derivatives=(-100,-100), mixed derivative=25.
  // The level-set curvature update is -125: 100 + 0.05*(-125) = 93.75; dt=10 yields -1150.
  REQUIRE(outputPtr->getDataStoreRef().getValue(4) == (clip ? uint8{0} : uint8{93}));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: native no-op preserves integer endpoints", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]", int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const TestType value = GENERATE(std::numeric_limits<TestType>::lowest(), std::numeric_limits<TestType>::max());
  const bool zeroIterations = GENERATE(false, true);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 1, 1, 1, {value});
  requireCurvatureFlowConstant(dataStructure, inputPath, value, zeroIterations ? 0.05 : 0.0, zeroIterations ? 0u : 1u);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: nonfinite terminal integer result errors", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath inputPath = rt::BuildImageFromPattern<uint8>(dataStructure, 3, 3, 1, {0, 0, 0, 0, 100, 100, 0, 100, 100});
  const CurvatureFlowImageFilter filter;
  const auto result = filter.execute(dataStructure, makeCurvatureFlowArgs(inputPath, std::numeric_limits<float64>::max(), 1));
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().front().code == ImageProcessing::k_CurvatureFlowNonFiniteIntegerOutput);
  REQUIRE(result.result.errors().front().message.find("index 4") != std::string::npos);
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: streamed and ping-pong terminal routes honor the stencil", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool pingPong = GENERATE(false, true);
  const bool nonFinite = GENERATE(false, true);
  const ShapeType shape{2, 3, 3};
  const SizeVec3 dims{3, 3, 2};
  const std::vector<uint8> field{0, 0, 0, 0, 100, 100, 0, 100, 100, 0, 0, 0, 0, 100, 100, 0, 100, 100};
  DataStore<uint8> input(shape, ShapeType{1}, uint8{0});
  const auto copyResult = input.copyFromBuffer(0, field);
  SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
  CurvatureFlowForwardingStore<uint8> output(shape);
  DataStore<float64> first(shape, ShapeType{1}, 0.0);
  DataStore<float64> second(shape, ShapeType{1}, 0.0);
  const std::atomic_bool shouldCancel{false};
  const float64 step = nonFinite ? std::numeric_limits<float64>::max() : 10.0;
  Result<> result;
  if(pingPong)
  {
    result = ImageProcessing::ApplyFiniteDifference3DPingPong<uint8, ImageProcessing::detail::CurvatureFlowFn, float64, true>(input, output, first, second, dims, {1.0, 1.0, 1.0},
                                                                                                                              ImageProcessing::detail::CurvatureFlowFn{}, step, 1, shouldCancel, {});
  }
  else
  {
    result = ImageProcessing::ApplyFiniteDifference<uint8, ImageProcessing::detail::CurvatureFlowFn, true>(input, output, dims, {1.0F, 1.0F, 1.0F}, ImageProcessing::detail::CurvatureFlowFn{}, step, 1,
                                                                                                           shouldCancel, {});
  }
  if(nonFinite)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == ImageProcessing::k_CurvatureFlowNonFiniteIntegerOutput);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(result.warnings().size() == 1);
    REQUIRE(result.warnings().front().code == ImageProcessing::k_CurvatureFlowClippedIntegerOutput);
    REQUIRE(output.getValue(4) == 0);
    REQUIRE(output.getValue(13) == 0);
    REQUIRE(output.writeCount() == 2);
  }
  REQUIRE(output.getTupleShape() == shape);
  REQUIRE(output.getComponentShape() == ShapeType{1});
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: native no-op propagates transfer failures and cancellation", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const int scenario = GENERATE(0, 1, 2, 3);
  const ShapeType shape{1, 1, 1};
  std::atomic_bool shouldCancel{scenario == 2};
  CurvatureFlowForwardingStore<uint64> input(shape, scenario == 0, false, false, scenario == 3 ? &shouldCancel : nullptr);
  CurvatureFlowForwardingStore<uint64> output(shape, false, scenario == 1);
  input.setValue(0, std::numeric_limits<uint64>::max());
  output.setValue(0, 7);
  const auto result = ImageProcessing::ApplyFiniteDifference<uint64, ImageProcessing::detail::CurvatureFlowFn, true>(input, output, {1, 1, 1}, {1.0F, 1.0F, 1.0F},
                                                                                                                     ImageProcessing::detail::CurvatureFlowFn{}, 0.0, 1, shouldCancel, {});
  if(scenario >= 2)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(input.readCount() == (scenario == 2 ? 0 : 1));
    REQUIRE(output.writeCount() == 0);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == (scenario == 0 ? -28890 : -28891));
  }
  REQUIRE(output.getValue(0) == 7);
  REQUIRE(output.getTupleShape() == shape);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: resident wrapper merges clipping warning with copy failure", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const ImageProcessing::ScopedWorkingMemoryTuningOverride tuning(1024 * 1024);
  const ShapeType shape{2, 3, 3};
  DataStore<uint8> input(shape, ShapeType{1}, uint8{0});
  const std::vector<uint8> field{0, 0, 0, 0, 100, 100, 0, 100, 100, 0, 0, 0, 0, 100, 100, 0, 100, 100};
  const auto copyResult = input.copyFromBuffer(0, field);
  SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
  // This endpoint selects the wrapper and injects its final-copy fault. Real backing is checked in the paired tests.
  CurvatureFlowForwardingStore<uint8> output(shape, false, true, true);
  const std::atomic_bool shouldCancel{false};
  const auto result = ImageProcessing::ApplyFiniteDifference<uint8, ImageProcessing::detail::CurvatureFlowFn, true>(input, output, {3, 3, 2}, {1.0F, 1.0F, 1.0F},
                                                                                                                    ImageProcessing::detail::CurvatureFlowFn{}, 10.0, 1, shouldCancel, {});
  SIMPLNX_RESULT_REQUIRE_INVALID(result);
  REQUIRE(result.errors().front().code == -28891);
  REQUIRE(result.warnings().size() == 1);
  REQUIRE(result.warnings().front().code == ImageProcessing::k_CurvatureFlowClippedIntegerOutput);
  REQUIRE(output.writeCount() == 1);
}

TEMPLATE_TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: floating output retains nonfinite and overflow behavior", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool nanInput = GENERATE(false, true);
  DataStructure dataStructure;
  const std::vector<TestType> field = nanInput ? std::vector<TestType>(9, std::numeric_limits<TestType>::quiet_NaN()) : std::vector<TestType>{0, 0, 0, 0, 100, 100, 0, 100, 100};
  const DataPath inputPath = rt::BuildImageFromPattern<TestType>(dataStructure, 3, 3, 1, field);
  const CurvatureFlowImageFilter filter;
  const auto result = filter.execute(dataStructure, makeCurvatureFlowArgs(inputPath, nanInput ? 0.05 : 1.0e40, 1));
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  REQUIRE(result.result.warnings().empty());
  const DataPath outputPath = inputPath.replaceName("Output");
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(outputPath));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 3, 3});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* outputPtr = nullptr;
  REQUIRE_NOTHROW(outputPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(outputPath));
  const TestType center = outputPtr->getDataStoreRef().getValue(4);
  if(nanInput)
  {
    REQUIRE(std::isnan(center));
  }
  else if constexpr(std::is_same_v<TestType, float32>)
  {
    REQUIRE(std::isinf(center));
    REQUIRE(std::signbit(center));
  }
  else
  {
    REQUIRE(center == Approx(-1.25e42));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: exact no-op uses bounded native transfers", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  constexpr usize valuesPerMiB = (usize{1024} * 1024) / sizeof(uint64);
  constexpr usize count = (2 * valuesPerMiB) + 1;
  const ShapeType shape{1, 1, count};
  CurvatureFlowForwardingStore<uint64> input(shape);
  CurvatureFlowForwardingStore<uint64> output(shape);
  input.setValue(0, std::numeric_limits<uint64>::max());
  input.setValue(count - 1, (uint64{1} << 53) + 1);
  const std::atomic_bool shouldCancel{false};
  const auto result = ImageProcessing::ApplyFiniteDifference<uint64, ImageProcessing::detail::CurvatureFlowFn, true>(input, output, {count, 1, 1}, {1.0F, 1.0F, 1.0F},
                                                                                                                     ImageProcessing::detail::CurvatureFlowFn{}, 0.0, 1, shouldCancel, {});
  SIMPLNX_RESULT_REQUIRE_VALID(result);
  REQUIRE(result.warnings().empty());
  REQUIRE(input.readCount() == 3);
  REQUIRE(output.writeCount() == 3);
  REQUIRE(input.maxReadValues() == valuesPerMiB);
  REQUIRE(output.maxWriteValues() == valuesPerMiB);
  REQUIRE(output.getValue(0) == std::numeric_limits<uint64>::max());
  REQUIRE(output.getValue(count - 1) == (uint64{1} << 53) + 1);
  REQUIRE(output.getTupleShape() == shape);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: synthetic delayed 3D exits retain clipping and transfer warnings", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const int route = GENERATE(0, 1, 2); // ping-pong delayed read, ping-pong cancellation, resident fallback cancellation
  const ShapeType shape{3, 3, 3};
  CurvatureFlowForwardingStore<uint8> input(shape);
  CurvatureFlowForwardingStore<uint8> output(shape);
  const std::array<uint8, 9> plane{0, 0, 0, 0, 100, 100, 0, 100, 100};
  for(usize index = 0; index < 27; ++index)
  {
    input.setValue(index, plane.at(index % plane.size()));
  }
  input.warnOnReads();
  output.warnOnWrites();
  std::atomic_bool shouldCancel{false};
  if(route == 0)
  {
    input.failReadAt(3); // planes 0 and 1 are loaded before the first clipped output; loading plane 2 then fails
  }
  else
  {
    output.cancelOnWrite(1, shouldCancel);
  }
  Result<> result;
  if(route == 2)
  {
    result = ImageProcessing::ApplyFiniteDifference<uint8, ImageProcessing::detail::CurvatureFlowFn, true>(input, output, {3, 3, 3}, {1.0F, 1.0F, 1.0F}, ImageProcessing::detail::CurvatureFlowFn{},
                                                                                                           10.0, 1, shouldCancel, {});
  }
  else
  {
    DataStore<float64> first(shape, ShapeType{1}, 0.0);
    DataStore<float64> second(shape, ShapeType{1}, 0.0);
    result = ImageProcessing::ApplyFiniteDifference3DPingPong<uint8, ImageProcessing::detail::CurvatureFlowFn, float64, true>(input, output, first, second, {3, 3, 3}, {1.0, 1.0, 1.0},
                                                                                                                              ImageProcessing::detail::CurvatureFlowFn{}, 10.0, 1, shouldCancel, {});
  }
  if(route == 0)
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -28892);
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(shouldCancel.load());
  }
  REQUIRE(output.writeCount() == 1);
  REQUIRE(output.getValue(4) == 0);
  const auto countWarnings = [&](int32 code) {
    return static_cast<usize>(std::count_if(result.warnings().begin(), result.warnings().end(), [code](const Warning& warning) { return warning.code == code; }));
  };
  REQUIRE(countWarnings(ImageProcessing::k_CurvatureFlowClippedIntegerOutput) == 1);
  REQUIRE(countWarnings(-28893) == input.readCount());
  REQUIRE(countWarnings(-28894) == output.writeCount());
  REQUIRE(result.warnings().size() == input.readCount() + output.writeCount() + 1);
  REQUIRE(output.getTupleShape() == shape);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: synthetic delayed 2D terminal exits retain warnings", "[ImageProcessing][Issue1874][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel preferences(DataStorageMode::ForceInCore, 0);
  const bool cancel = GENERATE(false, true);
  const ShapeType shape{1, 3, 3};
  DataStore<uint8> input(shape, ShapeType{1}, uint8{0});
  const std::vector<uint8> field{0, 0, 0, 0, 100, 100, 0, 100, 100};
  const auto copyResult = input.copyFromBuffer(0, field);
  SIMPLNX_RESULT_REQUIRE_VALID(copyResult);
  CurvatureFlowForwardingStore<uint8> output(shape);
  CurvatureFlowForwardingStore<float64> first(shape);
  CurvatureFlowForwardingStore<float64> second(shape);
  second.warnOnReads();
  output.warnOnWrites();
  std::atomic_bool shouldCancel{false};
  if(cancel)
  {
    output.cancelOnWrite(2, shouldCancel); // row 1 clips; cancellation follows its output write
  }
  else
  {
    second.failReadAt(3); // final row read follows the clipped center row
  }
  constexpr usize limit = ImageProcessing::detail::k_FiniteDifference2DFixedStateBytes + (12 * sizeof(float64));
  const auto plan = ImageProcessing::detail::BuildFiniteDifference2DBufferPlan(3, 3, 1, sizeof(float64), limit);
  REQUIRE(plan.coreCols == 3);
  REQUIRE(plan.coreRows == 1);
  const auto result = ImageProcessing::ApplyFiniteDifference2DBounded<uint8, ImageProcessing::detail::CurvatureFlowFn, float64, true>(
      input, output, first, second, 3, 3, {1.0, 1.0, 1.0}, ImageProcessing::detail::CurvatureFlowFn{}, 10.0, 1, shouldCancel, {}, limit);
  if(cancel)
  {
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(shouldCancel.load());
  }
  else
  {
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -28892);
  }
  REQUIRE(output.writeCount() == 2);
  REQUIRE(output.getValue(4) == 0);
  const auto countWarnings = [&](int32 code) {
    return static_cast<usize>(std::count_if(result.warnings().begin(), result.warnings().end(), [code](const Warning& warning) { return warning.code == code; }));
  };
  REQUIRE(countWarnings(ImageProcessing::k_CurvatureFlowClippedIntegerOutput) == 1);
  REQUIRE(countWarnings(-28893) == second.readCount());
  REQUIRE(countWarnings(-28894) == output.writeCount());
  REQUIRE(result.warnings().size() == second.readCount() + output.writeCount() + 1);
  REQUIRE(output.getTupleShape() == shape);
}

// -----------------------------------------------------------------------------
// (1) Preflight guards: a multi-component input is rejected (requireScalar); a valid scalar input produces a
//     SameAsInput output array (unlike GradientMagnitude's fixed float32, CurvatureFlow preserves the input type).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: preflight guards", "[ImageProcessing][CurvatureFlowImageFilter]")
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
    CurvFilter filter;
    Arguments args;
    args.insertOrAssign(CurvFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geomPath));
    args.insertOrAssign(CurvFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(vecPath));
    args.insertOrAssign(CurvFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(CurvFilter::k_TimeStep_Key, std::make_any<float64>(0.05));
    args.insertOrAssign(CurvFilter::k_NumberOfIterations_Key, std::make_any<uint32>(5u));
    const auto result = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(result.outputActions);
    REQUIRE(result.outputActions.errors().front().code == nx::core::ImageProcessing::k_NonScalarInput);
  }
  SECTION("scalar input creates a SameAsInput output")
  {
    DataStructure ds;
    const std::vector<uint8> field = MakeRamp<uint8>(6, 6, 1);
    const DataPath inputPath = rt::BuildImageFromPattern<uint8>(ds, 6, 6, 1, field);
    CurvFilter filter;
    RunCurvatureFlow(filter, ds, inputPath, 0.05, 5u);
    const DataPath outputPath({"Image Geometry", "CellData", "Output"});
    REQUIRE(ds.getDataRefAs<IDataArray>(outputPath).getDataType() == DataType::uint8);
  }
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden -- duplicates ITKCurvatureFlowImageTest.cpp on our ITK-free filter (RA-Float.nrrd ->
// float32, SameAsInput). The durable golden is a committed baseline at a tolerance of 0.0001 via
// ip_golden::CompareImages. The helper pins ForceInCore.
//   defaults: the filter defaults (TimeStep 0.05, NumberOfIterations 5). They agree with the ITK defaults.
//   longer:   TimeStep 0.1, NumberOfIterations 10.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][CurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<CurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_CurvatureFlowImageFilter_defaults.nrrd", /*tolerance=*/0.0001);
}

TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: ITK real-image golden (longer)", "[ImageProcessing][ItkGolden][CurvatureFlowImageFilter]")
{
  rt::RunDistanceMapItkGoldenBaseline<CurvatureFlowImageFilter>("RA-Float.nrrd", "BasicFilters_CurvatureFlowImageFilter_longer.nrrd", /*tolerance=*/0.0001, [](Arguments& args) {
    args.insertOrAssign(CurvatureFlowImageFilter::k_TimeStep_Key, std::make_any<float64>(0.1));
    args.insertOrAssign(CurvatureFlowImageFilter::k_NumberOfIterations_Key, std::make_any<uint32>(10u));
  });
}

// -----------------------------------------------------------------------------
// (2) NumberOfIterations == 0 is a no-op: the driver's `for(iter=0; iter<numberOfIterations; ...)` loop never runs, and
//     the final output = cast<T>(accum = cast<Real>(input)) is a lossless float32 round-trip -> output == input EXACTLY.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::CurvatureFlowImageFilter: zero iterations is a no-op", "[ImageProcessing][CurvatureFlowImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  constexpr usize DX = 8, DY = 8, DZ = 8;
  DataStructure ds;
  const DataPath inputPath = rt::BuildImageFromPattern<float32>(ds, DX, DY, DZ, MakeRamp<float32>(DX, DY, DZ));
  CurvFilter filter;
  RunCurvatureFlow(filter, ds, inputPath, /*timeStep=*/0.05, /*numberOfIterations=*/0u);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  rt::RequireExact<float32>(ds.getDataRefAs<IDataArray>(outputPath), ds.getDataRefAs<IDataArray>(inputPath));
}
