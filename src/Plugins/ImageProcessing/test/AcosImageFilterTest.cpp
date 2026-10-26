#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/AcosImageFilter.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Core/Preferences.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/PointwiseEngine.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <any>
#include <atomic>
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

using namespace nx::core;

namespace
{
DataPath runMappedIntegerAcos(DataStructure& dataStructure, const DataPath& input)
{
  const AcosImageFilter filter;
  Arguments args;
  args.insertOrAssign(AcosImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(AcosImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AcosImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  // A mapping regression must fail before invalid integer values can reach a NaN-to-integer cast.
  REQUIRE(actionPtr->type() == DataType::float32);
  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  return input.replaceName("Output");
}

// Test-only transfer failures use the engine's existing forced-buffer option. These stores are resident;
// the separate SimplnxOoc test supplies the actual disk witness.
template <class T>
class AcosTransferStore : public DataStore<T>
{
public:
  explicit AcosTransferStore(T initialValue)
  : DataStore<T>(ShapeType{1, 1, 4}, ShapeType{1}, initialValue)
  {
  }
  ~AcosTransferStore() noexcept override = default;
  AcosTransferStore(const AcosTransferStore&) = delete;
  AcosTransferStore(AcosTransferStore&&) = delete;
  AcosTransferStore& operator=(const AcosTransferStore&) = delete;
  AcosTransferStore& operator=(AcosTransferStore&&) = delete;

  Result<> copyIntoBuffer(usize start, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    if(m_FailRead)
    {
      return MakeErrorResult(-187401, "Acos test input transfer failed.");
    }
    return DataStore<T>::copyIntoBuffer(start, buffer);
  }
  Result<> copyFromBuffer(usize start, nonstd::span<const T> buffer) override
  {
    ++m_WriteCount;
    if(m_FailWrite)
    {
      return MakeErrorResult(-187402, "Acos test output transfer failed.");
    }
    return DataStore<T>::copyFromBuffer(start, buffer);
  }

  void setFailRead(bool failRead) noexcept
  {
    m_FailRead = failRead;
  }
  void setFailWrite(bool failWrite) noexcept
  {
    m_FailWrite = failWrite;
  }
  usize readCount() const noexcept
  {
    return m_ReadCount;
  }
  usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

private:
  bool m_FailRead = false;
  bool m_FailWrite = false;
  mutable usize m_ReadCount = 0;
  usize m_WriteCount = 0;
};
} // namespace

// An angle at zero is pi/2 radians, independently of the input's integer width or signedness.
// Keeping the input at zero makes the unfixed integer-output implementation safe to execute.
TEMPLATE_TEST_CASE("ImageProcessing::AcosImageFilter: integer zero produces float32 radians", "[ImageProcessing][Issue1874][AcosImageFilter]", int8, uint8, int16, uint16, int32, uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {0});
  const DataPath output = input.replaceName("Output");
  Arguments args;
  args.insertOrAssign(AcosImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
  args.insertOrAssign(AcosImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AcosImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));

  const AcosImageFilter filter;
  const auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.value().actions.size() == 1);
  const auto* actionPtr = dynamic_cast<const CreateArrayAction*>(preflightResult.outputActions.value().actions.front().get());
  REQUIRE(actionPtr != nullptr);
  REQUIRE(actionPtr->type() == DataType::float32);
  REQUIRE(actionPtr->dims() == ShapeType{1, 1, 1});
  REQUIRE(actionPtr->componentDims() == ShapeType{1});

  const auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 1});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float32Array* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32Array>(output));
  REQUIRE(anglesPtr->getDataStoreRef().getValue(0) == Approx(1.5707963267948966).epsilon(0).margin(1.0e-7));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::AcosImageFilter: floating input retains type and propagates domain NaN", "[ImageProcessing][Issue1874][AcosImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(
      dataStructure, {-1, 0, 1, 2, -2, std::numeric_limits<TestType>::quiet_NaN(), std::numeric_limits<TestType>::infinity(), -std::numeric_limits<TestType>::infinity()});
  const DataPath output = ip_test::RunPointwiseFilter<AcosImageFilter>(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 8});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = anglesPtr->getDataStoreRef();
  constexpr float64 angleTolerance = std::is_same_v<TestType, float32> ? 1.0e-7 : 1.0e-15;
  REQUIRE(store.getValue(0) == Approx(3.14159265358979323846).epsilon(0).margin(angleTolerance));
  REQUIRE(store.getValue(1) == Approx(1.57079632679489661923).epsilon(0).margin(angleTolerance));
  REQUIRE(store.getValue(2) == 0);
  for(usize index = 3; index < 8; ++index)
  {
    REQUIRE(std::isnan(store.getValue(index)));
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::AcosImageFilter: integer endpoints and invalid values produce floating angles", "[ImageProcessing][Issue1874][AcosImageFilter]", int8, uint8, int16, uint16, int32,
                   uint32, int64, uint64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  std::vector<TestType> values = {0, 1, 2, std::numeric_limits<TestType>::max()};
  if constexpr(std::is_signed_v<TestType>)
  {
    values.push_back(-1);
    values.push_back(std::numeric_limits<TestType>::lowest());
  }

  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, values);
  const DataPath output = runMappedIntegerAcos(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, values.size()});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const Float32Array* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32Array>(output));
  const auto& store = anglesPtr->getDataStoreRef();
  REQUIRE(store.getValue(0) == Approx(1.57079632679489661923).epsilon(0).margin(1.0e-7));
  REQUIRE(store.getValue(1) == 0.0f);
  REQUIRE(std::isnan(store.getValue(2)));
  REQUIRE(std::isnan(store.getValue(3)));
  if constexpr(std::is_signed_v<TestType>)
  {
    REQUIRE(store.getValue(4) == Approx(3.14159265358979323846).epsilon(0).margin(1.0e-7));
    REQUIRE(std::isnan(store.getValue(5)));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEMPLATE_TEST_CASE("ImageProcessing::AcosImageFilter: adjacent domain boundaries remain distinct", "[ImageProcessing][Issue1874][AcosImageFilter]", float32, float64)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  const TestType inside = std::nextafter(TestType{1}, TestType{0});
  const TestType outside = std::nextafter(TestType{1}, std::numeric_limits<TestType>::infinity());
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildScalarValuesImage<TestType>(dataStructure, {inside, -inside, outside, -outside});
  const DataPath output = ip_test::RunPointwiseFilter<AcosImageFilter>(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == GetDataType<TestType>());
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{1, 1, 4});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{1});
  const DataArray<TestType>* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<DataArray<TestType>>(output));
  const auto& store = anglesPtr->getDataStoreRef();
  // The two interior values must give finite, non-endpoint angles; exterior values must not be clamped.
  REQUIRE(store.getValue(0) > 0);
  REQUIRE(store.getValue(0) < 1.0e-3);
  REQUIRE(store.getValue(1) > 3.14);
  REQUIRE(store.getValue(1) < 3.14159265358979323846);
  REQUIRE(std::isnan(store.getValue(2)));
  REQUIRE(std::isnan(store.getValue(3)));
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::AcosImageFilter: mapped multi-component output preserves tuple shape", "[ImageProcessing][Issue1874][AcosImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);
  DataStructure dataStructure;
  const DataPath input = ip_test::BuildRampImage<int16>(dataStructure, 2, 0, 0);
  Int16Array* inputArrayPtr = nullptr;
  REQUIRE_NOTHROW(inputArrayPtr = &dataStructure.getDataRefAs<Int16Array>(input));
  auto store = DataStoreUtilities::CreateDataStore<int16>(dataStructure, input, ShapeType{2, 2, 2}, ShapeType{2});
  REQUIRE(store != nullptr);
  const auto replaceResult = inputArrayPtr->setDataStore(store);
  SIMPLNX_RESULT_REQUIRE_VALID(replaceResult);
  for(usize index = 0; index < 16; ++index)
  {
    store->setValue(index, static_cast<int16>(static_cast<int32>(index % 4) - 1));
  }
  const DataPath output = runMappedIntegerAcos(dataStructure, input);
  const IDataArray* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &dataStructure.getDataRefAs<IDataArray>(output));
  REQUIRE(outputArrayPtr->getDataType() == DataType::float32);
  REQUIRE(outputArrayPtr->getTupleShape() == ShapeType{2, 2, 2});
  REQUIRE(outputArrayPtr->getComponentShape() == ShapeType{2});
  const Float32Array* anglesPtr = nullptr;
  REQUIRE_NOTHROW(anglesPtr = &dataStructure.getDataRefAs<Float32Array>(output));
  const auto& result = anglesPtr->getDataStoreRef();
  for(usize index = 0; index < 16; index += 4)
  {
    REQUIRE(result.getValue(index) == Approx(3.14159265358979323846).epsilon(0).margin(1.0e-7));
    REQUIRE(result.getValue(index + 1) == Approx(1.57079632679489661923).epsilon(0).margin(1.0e-7));
    REQUIRE(result.getValue(index + 2) == 0.0f);
    REQUIRE(std::isnan(result.getValue(index + 3)));
  }
  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("ImageProcessing::AcosImageFilter: mapped buffered engine preserves cancellation and I/O failures", "[ImageProcessing][Issue1874][AcosImageFilter]")
{
  UnitTest::LoadPlugins();
  using OutputType = ImageProcessing::IntegerToFloat32Type<int16>;
  AcosTransferStore<int16> input(0);
  AcosTransferStore<OutputType> output(-7.0f);
  std::atomic_bool cancel = false;
  ImageProcessing::detail::PointwiseExecutionOptions options;
  options.forceBuffered = true;
  const auto operation = [](int16 value) -> OutputType { return static_cast<OutputType>(std::acos(static_cast<float64>(value))); };
  SECTION("Precancel performs no transfers")
  {
    cancel = true;
    const auto result = ImageProcessing::detail::ApplyPointwiseImpl(input, output, operation, cancel, IFilter::MessageHandler{}, options);
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    REQUIRE(input.readCount() == 0);
    REQUIRE(output.writeCount() == 0);
  }
  SECTION("Read failure returns unchanged and performs no write")
  {
    input.setFailRead(true);
    const auto result = ImageProcessing::detail::ApplyPointwiseImpl(input, output, operation, cancel, IFilter::MessageHandler{}, options);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -187401);
    REQUIRE(input.readCount() == 1);
    REQUIRE(output.writeCount() == 0);
  }
  SECTION("Write failure returns unchanged")
  {
    output.setFailWrite(true);
    const auto result = ImageProcessing::detail::ApplyPointwiseImpl(input, output, operation, cancel, IFilter::MessageHandler{}, options);
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -187402);
    REQUIRE(input.readCount() == 1);
    REQUIRE(output.writeCount() == 1);
  }
  for(usize index = 0; index < 4; ++index)
  {
    REQUIRE(output.getValue(index) == -7.0f);
  }
}

// PART A domain edge (documents ITK-faithful behavior; does NOT "fix" it): acos is undefined for |x|>1. The op is
// static_cast<U>(std::acos(static_cast<double>(x))) -- exactly ITK's equally-unguarded Acos functor -- so acos(2)
// is IEEE NaN. This pins that observed output.
TEST_CASE("ImageProcessing::AcosImageFilter: acos of |x|>1 is NaN (domain edge)", "[ImageProcessing][AcosImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  DataStructure ds;
  const DataPath input = ip_test::BuildScalarValuesImage<float32>(ds, {2.0f});
  const DataPath output = ip_test::RunPointwiseFilter<AcosImageFilter>(ds, input);

  const Float32Array* outputArrayPtr = nullptr;
  REQUIRE_NOTHROW(outputArrayPtr = &ds.getDataRefAs<Float32Array>(output));
  const auto& out = outputArrayPtr->getDataStoreRef();
  REQUIRE(std::isnan(out.getValue(0)));

  UnitTest::CheckArraysInheritTupleDims(ds);
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image golden (plan Sec.3/Sec.6). Mirrors ITKAcosImageTest.cpp(defaults): read
// Ramp-Zero-One-Float.nrrd (float32) through OUR ITK-free reader, run OUR AcosImageFilter with the ITK case's
// params (defaults), then compare our output to ITK's committed baseline .nrrd (read through the SAME reader) at
// the ITK test's tolerance 0.01 via ip_golden::CompareImages. CompareImages dispatches on the runtime type and --
// unlike CompareDataArrays -- tolerates the non-finite tails these functors can emit.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::AcosImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][AcosImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath input = geom.createChildPath("CellData").createChildPath("Input");
  const DataPath output = geom.createChildPath("CellData").createChildPath("Output");

  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("Ramp-Zero-One-Float.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  Arguments args;
  args.insertOrAssign(AcosImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(AcosImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(AcosImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  AcosImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // DURABLE golden: baseline compare @ 0.01 (SameAsInput output owned by 'geom').
  const DataPath bGeom({"Baseline Geometry"});
  const auto readBaseline = ip_golden::ReadInputImage(ds, ip_golden::BaselinePath("BasicFilters_AcosImageFilter_defaults.nrrd"), bGeom, "CellData", "Baseline");
  SIMPLNX_RESULT_REQUIRE_VALID(readBaseline);
  const DataPath baseline = bGeom.createChildPath("CellData").createChildPath("Baseline");
  const auto compareResult = ip_golden::CompareImages(ds, bGeom, baseline, geom, output, 0.01);
  SIMPLNX_RESULT_REQUIRE_VALID(compareResult);

  UnitTest::CheckArraysInheritTupleDims(ds);
}
