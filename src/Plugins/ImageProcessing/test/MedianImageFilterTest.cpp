#include "ItkGoldenTestUtils.hpp"
#include "UnaryFilterTestUtils.hpp"

#include "ImageProcessing/Filters/MedianImageFilter.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/BoxNeighborhoodEngine.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <vector>

using namespace nx::core;

namespace
{
template <class T>
class OutOfCoreTransferCountingDataStore : public DataStore<T>
{
public:
  using DataStore<T>::DataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    m_MaxReadValues = std::max(m_MaxReadValues, buffer.size());
    m_ReadCount++;
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    m_MaxWriteValues = std::max(m_MaxWriteValues, buffer.size());
    m_WrittenValues += buffer.size();
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  usize maxReadValues() const noexcept
  {
    return m_MaxReadValues;
  }

  usize maxWriteValues() const noexcept
  {
    return m_MaxWriteValues;
  }

  usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

  usize readCount() const noexcept
  {
    return m_ReadCount;
  }

protected:
  mutable usize m_MaxReadValues = 0;
  usize m_MaxWriteValues = 0;
  usize m_WrittenValues = 0;
  mutable usize m_ReadCount = 0;
};

template <class T>
class CancelAfterReadDataStore : public OutOfCoreTransferCountingDataStore<T>
{
public:
  CancelAfterReadDataStore(const ShapeType& tupleShape, const ShapeType& componentShape, T initialValue, std::atomic_bool& shouldCancel)
  : OutOfCoreTransferCountingDataStore<T>(tupleShape, componentShape, initialValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = OutOfCoreTransferCountingDataStore<T>::copyIntoBuffer(startIndex, buffer);
    if(result.valid())
    {
      m_ShouldCancel = true;
    }
    return result;
  }

private:
  std::atomic_bool& m_ShouldCancel;
};

template <class T>
std::vector<T> BoxMedian2DOracle(const std::vector<T>& input, usize dimX, usize dimY, const std::array<usize, 3>& radius)
{
  const usize neighborhoodSize = (2 * radius[0] + 1) * (2 * radius[1] + 1) * (2 * radius[2] + 1);
  std::vector<T> output(input.size());
  std::vector<T> scratch(neighborhoodSize);
  for(usize y = 0; y < dimY; ++y)
  {
    for(usize x = 0; x < dimX; ++x)
    {
      usize index = 0;
      for(usize dz = 0; dz <= 2 * radius[2]; ++dz)
      {
        static_cast<void>(dz);
        for(usize dy = 0; dy <= 2 * radius[1]; ++dy)
        {
          const usize neighborY = ImageProcessing::detail::ClampCoord(static_cast<int64>(y) - static_cast<int64>(radius[1]) + static_cast<int64>(dy), dimY);
          for(usize dx = 0; dx <= 2 * radius[0]; ++dx)
          {
            const usize neighborX = ImageProcessing::detail::ClampCoord(static_cast<int64>(x) - static_cast<int64>(radius[0]) + static_cast<int64>(dx), dimX);
            scratch[index++] = input[neighborY * dimX + neighborX];
          }
        }
      }
      auto middle = scratch.begin() + scratch.size() / 2;
      std::nth_element(scratch.begin(), middle, scratch.end());
      output[y * dimX + x] = *middle;
    }
  }
  return output;
}

constexpr usize k_MedianTestDimX = 9;
constexpr usize k_MedianTestDimY = 7;
constexpr usize k_MedianTestValues = k_MedianTestDimX * k_MedianTestDimY;
constexpr std::array<usize, 3> k_MedianTestRadius = {2, 1, 1};
constexpr usize k_MedianTestNeighborhoodValues = 45;

std::vector<int32> MakeMedianTestInput()
{
  std::vector<int32> input(k_MedianTestValues);
  for(usize y = 0; y < k_MedianTestDimY; ++y)
  {
    for(usize x = 0; x < k_MedianTestDimX; ++x)
    {
      input[y * k_MedianTestDimX + x] = static_cast<int32>((x * 17 + y * 11 + x * y * 3) % 67) - 31;
    }
  }
  return input;
}
} // namespace

TEST_CASE("ImageProcessing::BoxNeighborhoodEngine: validates stores and cancellation", "[ImageProcessing][MedianImageFilter][BoxNeighborhoodEngine]")
{
  const auto median = [](nonstd::span<int32> values) {
    auto middle = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), middle, values.end());
    return *middle;
  };
  std::atomic_bool shouldCancel{false};

  SECTION("rejects zero dimensions")
  {
    DataStore<int32> inputStore(ShapeType{1}, ShapeType{1}, 0);
    DataStore<int32> outputStore(ShapeType{1}, ShapeType{1}, 0);
    const Result<> result = ImageProcessing::ApplyBoxNeighborhood(inputStore, outputStore, SizeVec3{0, 1, 1}, std::array<usize, 3>{1, 1, 1}, median, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8610);
  }

  SECTION("rejects mismatched stores")
  {
    DataStore<int32> shortStore(ShapeType{63}, ShapeType{1}, 0);
    DataStore<int32> fullStore(ShapeType{64}, ShapeType{1}, 0);
    Result<> result = ImageProcessing::ApplyBoxNeighborhood(shortStore, fullStore, SizeVec3{4, 4, 4}, std::array<usize, 3>{1, 1, 1}, median, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8613);

    result = ImageProcessing::ApplyBoxNeighborhood(fullStore, shortStore, SizeVec3{4, 4, 4}, std::array<usize, 3>{1, 1, 1}, median, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_INVALID(result);
    REQUIRE(result.errors().front().code == -8614);
  }

  SECTION("pre-cancel preserves output")
  {
    constexpr int32 k_Poison = -12345;
    DataStore<int32> inputStore(ShapeType{4, 4, 4}, ShapeType{1}, 1);
    DataStore<int32> outputStore(ShapeType{4, 4, 4}, ShapeType{1}, k_Poison);
    shouldCancel = true;
    const Result<> result = ImageProcessing::ApplyBoxNeighborhood(inputStore, outputStore, SizeVec3{4, 4, 4}, std::array<usize, 3>{1, 1, 1}, median, shouldCancel, {});
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    for(const int32 value : outputStore)
    {
      REQUIRE(value == k_Poison);
    }
  }
}

TEST_CASE("ImageProcessing::BoxNeighborhoodEngine: bounded 2D blocks and tiles preserve median transfers", "[ImageProcessing][MedianImageFilter][BoxNeighborhoodEngine]")
{
  const usize workerCount = ImageProcessing::detail::BoxNeighborhood2DWorkerCount();
  const usize workerScratchBytes = k_MedianTestNeighborhoodValues * sizeof(int32) * workerCount;
  const std::vector<int32> input = MakeMedianTestInput();
  const std::vector<int32> expected = BoxMedian2DOracle(input, k_MedianTestDimX, k_MedianTestDimY, k_MedianTestRadius);
  const auto median = [](nonstd::span<int32> values) {
    auto middle = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), middle, values.end());
    return *middle;
  };

  struct TransferCase
  {
    const char* label;
    usize bufferBytes;
    bool fullWidth;
    usize coreRows;
    usize coreColumns;
    usize maximumReadValues;
    usize maximumWriteValues;
  };
  const std::array<TransferCase, 2> transferCases = {{{"full-width row blocks", 216, true, 2, 9, 36, 18}, {"overwide X tiles", 80, false, 1, 2, 6, 2}}};

  for(const TransferCase& transferCase : transferCases)
  {
    DYNAMIC_SECTION(transferCase.label)
    {
      const usize targetBytes = transferCase.bufferBytes + workerScratchBytes;
      const auto planResult =
          ImageProcessing::detail::CreateBoxNeighborhood2DPlan(k_MedianTestDimX, k_MedianTestDimY, k_MedianTestRadius[0], k_MedianTestRadius[1], sizeof(int32), targetBytes, workerScratchBytes);
      SIMPLNX_RESULT_REQUIRE_VALID(planResult);
      const auto& plan = planResult.value();
      REQUIRE(plan.fullWidth == transferCase.fullWidth);
      REQUIRE(plan.coreRows == transferCase.coreRows);
      REQUIRE(plan.coreColumns == transferCase.coreColumns);
      REQUIRE(plan.residentBytes == targetBytes);

      OutOfCoreTransferCountingDataStore<int32> inputStore(ShapeType{1, k_MedianTestDimY, k_MedianTestDimX}, ShapeType{1}, int32{0});
      OutOfCoreTransferCountingDataStore<int32> outputStore(ShapeType{1, k_MedianTestDimY, k_MedianTestDimX}, ShapeType{1}, int32{0});
      for(usize index = 0; index < input.size(); ++index)
      {
        inputStore.setValue(index, input[index]);
      }

      std::atomic_bool shouldCancel{false};
      Result<> applyResult = ImageProcessing::ApplyBoxNeighborhood(inputStore, outputStore, SizeVec3{k_MedianTestDimX, k_MedianTestDimY, 1}, k_MedianTestRadius, median, shouldCancel, {}, targetBytes);
      SIMPLNX_RESULT_REQUIRE_VALID(applyResult);

      std::vector<int32> actual(k_MedianTestValues);
      Result<> copyIntoBufferResult = outputStore.copyIntoBuffer(0, nonstd::span<int32>(actual.data(), actual.size()));
      SIMPLNX_RESULT_REQUIRE_VALID(copyIntoBufferResult);
      REQUIRE(actual == expected);
      CAPTURE(inputStore.maxReadValues(), outputStore.maxWriteValues(), outputStore.writtenValues());
      REQUIRE(inputStore.maxReadValues() == transferCase.maximumReadValues);
      REQUIRE(outputStore.maxWriteValues() == transferCase.maximumWriteValues);
      REQUIRE(outputStore.writtenValues() == k_MedianTestValues);
    }
  }

  auto equalScratchPlanResult =
      ImageProcessing::detail::CreateBoxNeighborhood2DPlan(k_MedianTestDimX, k_MedianTestDimY, k_MedianTestRadius[0], k_MedianTestRadius[1], sizeof(int32), workerScratchBytes, workerScratchBytes);
  SIMPLNX_RESULT_REQUIRE_INVALID(equalScratchPlanResult);
  auto overflowDimPlanResult = ImageProcessing::detail::CreateBoxNeighborhood2DPlan(std::numeric_limits<usize>::max(), k_MedianTestDimY, k_MedianTestRadius[0], k_MedianTestRadius[1], sizeof(int32),
                                                                                    216 + workerScratchBytes, workerScratchBytes);
  SIMPLNX_RESULT_REQUIRE_INVALID(overflowDimPlanResult);
}

TEST_CASE("ImageProcessing::BoxNeighborhoodEngine: bounded 2D cancellation after input preserves output", "[ImageProcessing][MedianImageFilter][BoxNeighborhoodEngine]")
{
  constexpr int32 k_Poison = 73;
  const std::vector<int32> input = MakeMedianTestInput();
  const auto median = [](nonstd::span<int32> values) {
    auto middle = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), middle, values.end());
    return *middle;
  };
  std::atomic_bool shouldCancel{false};
  CancelAfterReadDataStore<int32> inputStore(ShapeType{1, k_MedianTestDimY, k_MedianTestDimX}, ShapeType{1}, int32{0}, shouldCancel);
  OutOfCoreTransferCountingDataStore<int32> outputStore(ShapeType{1, k_MedianTestDimY, k_MedianTestDimX}, ShapeType{1}, k_Poison);
  for(usize index = 0; index < input.size(); ++index)
  {
    inputStore.setValue(index, input[index]);
  }

  const usize workerCount = ImageProcessing::detail::BoxNeighborhood2DWorkerCount();
  const usize workerScratchBytes = k_MedianTestNeighborhoodValues * sizeof(int32) * workerCount;
  const usize targetBytes = 216 + workerScratchBytes;
  Result<> applyResult = ImageProcessing::ApplyBoxNeighborhood(inputStore, outputStore, SizeVec3{k_MedianTestDimX, k_MedianTestDimY, 1}, k_MedianTestRadius, median, shouldCancel, {}, targetBytes);
  SIMPLNX_RESULT_REQUIRE_VALID(applyResult);
  REQUIRE(shouldCancel);
  REQUIRE(inputStore.readCount() == 1);
  REQUIRE(outputStore.writtenValues() == 0);
  for(const int32 value : outputStore)
  {
    REQUIRE(value == k_Poison);
  }
}

TEST_CASE("ImageProcessing::MedianImageFilter: Legacy parity 3D (radius 1,1,1)", "[ImageProcessing][MedianImageFilter]")
{
  ip_test::RunLegacyParity<MedianImageFilter>(
      *Uuid::FromString("a60ca165-59ac-486b-b4b4-0f0c24d80af8"),
      [](Arguments& a) { a.insertOrAssign(MedianImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{1, 1, 1})); },
      /*dim=*/12, /*startValue=*/-500.0, /*step=*/1.0);
}

TEST_CASE("ImageProcessing::MedianImageFilter: Legacy parity 3D (radius 2,1,0)", "[ImageProcessing][MedianImageFilter]")
{
  ip_test::RunLegacyParity<MedianImageFilter>(
      *Uuid::FromString("a60ca165-59ac-486b-b4b4-0f0c24d80af8"),
      [](Arguments& a) { a.insertOrAssign(MedianImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{2, 1, 0})); },
      /*dim=*/12, /*startValue=*/0.0, /*step=*/1.0);
}

namespace
{
DataPath Build2DImage(DataStructure& ds, usize dimX, usize dimY)
{
  const ShapeType cellShape = {1, dimY, dimX}; // Z=1
  auto* imageGeom = ImageGeom::Create(ds, "Image Geometry");
  imageGeom->setDimensions({dimX, dimY, 1});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});
  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);
  const DataPath inputPath({"Image Geometry", "CellData", "Input"});
  auto store = DataStoreUtilities::CreateDataStore<float32>(ds, inputPath, cellShape, {1});
  auto* array = DataArray<float32>::Create(ds, "Input", store, cellAM->getId());
  auto& ref = array->getDataStoreRef();
  for(usize i = 0; i < ref.getSize(); ++i)
  {
    ref.setValue(i, static_cast<float32>((i * 7) % 53)); // non-monotonic pattern so the median is non-trivial
  }
  return inputPath;
}
} // namespace

TEST_CASE("ImageProcessing::MedianImageFilter: Legacy parity 2D (Z=1)", "[ImageProcessing][MedianImageFilter]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  auto run = [&](IFilter& filter, DataStructure& ds, const DataPath& inputPath) {
    Arguments args;
    args.insertOrAssign(MedianImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(DataPath({"Image Geometry"})));
    args.insertOrAssign(MedianImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(inputPath));
    args.insertOrAssign(MedianImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
    args.insertOrAssign(MedianImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(std::vector<uint32>{2, 2, 0}));
    auto preflightResult = filter.preflight(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(ds, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  };

  DataStructure newDs;
  const DataPath newInput = Build2DImage(newDs, 20, 16);
  MedianImageFilter newFilter;
  run(newFilter, newDs, newInput);

  IFilter::UniquePointer itkFilter = Application::Instance()->getFilterList()->createFilter(*Uuid::FromString("a60ca165-59ac-486b-b4b4-0f0c24d80af8"));
  REQUIRE(itkFilter != nullptr);
  DataStructure itkDs;
  const DataPath itkInput = Build2DImage(itkDs, 20, 16);
  run(*itkFilter, itkDs, itkInput);

  const DataPath outputPath({"Image Geometry", "CellData", "Output"});
  UnitTest::CompareDataArrays<float32>(itkDs.getDataRefAs<Float32Array>(outputPath), newDs.getDataRefAs<Float32Array>(outputPath));
}

namespace
{
// -----------------------------------------------------------------------------
// Shared body for the two ITK-sourced real-image md5 golden cases below. Mirrors
// ITKImageProcessing/test/ITKMedianImageTest.cpp exactly: read RA-Short.nrrd (int16) through OUR ITK-free reader, run
// OUR MedianImageFilter with the ITK case's radius (unset -> default {1,1,1} for "defaults"; {2,3,0} for "by23"),
// then apply the plan's md5-validity-first procedure (Sec.4):
//   (A) DURABLE golden -- our ITK-free output's md5 must EQUAL ITK's committed hash. If it matches as-committed the
//       durable golden is confirmed byte-for-byte (no re-freeze needed).
//   (B) LIVE-ITK parity -- run the legacy ITK filter (resolved from the loaded plugin's replacement map) on the SAME
//       input and require BIT-EXACT parity; median-of-neighborhood on integer data is deterministic, so exact.
// The whole case is pinned ForceInCore: the legacy ITK filter bad_casts an OOC store (reading via OUR reader is
// OOC-safe, but (B) is not).
// -----------------------------------------------------------------------------
void RunMedianItkGolden(std::optional<std::vector<uint32>> radius, const std::string& committedMd5)
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  const DataPath geom({"Image Geometry"});
  const DataPath cellData = geom.createChildPath("CellData");
  const DataPath input = cellData.createChildPath("Input");
  const DataPath output = cellData.createChildPath("Output");

  // --- read input through OUR ITK-free reader ---
  DataStructure ds;
  const auto readInput = ip_golden::ReadInputImage(ds, ip_golden::InputPath("RA-Short.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readInput);

  // --- run OUR filter with the ITK case's radius ---
  Arguments args;
  args.insertOrAssign(MedianImageFilter::k_InputImageGeomPath_Key, std::make_any<DataPath>(geom));
  args.insertOrAssign(MedianImageFilter::k_InputImageDataPath_Key, std::make_any<DataPath>(input));
  args.insertOrAssign(MedianImageFilter::k_OutputImageArrayName_Key, std::make_any<std::string>("Output"));
  if(radius.has_value())
  {
    args.insertOrAssign(MedianImageFilter::k_Radius_Key, std::make_any<VectorUInt32Parameter::ValueType>(*radius));
  }
  MedianImageFilter filter;
  auto preflightResult = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
  auto executeResult = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // --- (A) DURABLE golden: md5-validity-first (plan Sec.4) -- our output's md5 must equal ITK's committed hash. ---
  const std::string ourMd5 = ip_golden::ComputeMd5Hash(ds, output);
  INFO(fmt::format("Median md5: ours='{}' committed='{}'", ourMd5, committedMd5));
  REQUIRE(ourMd5 == committedMd5);

  // --- (B) LIVE-ITK parity (coexistence only): legacy ITK filter on the SAME input, require bit-exact (int16) ---
  const auto legacyUuid = ip_golden::LegacyUuidFor(FilterTraits<MedianImageFilter>::uuid);
  REQUIRE(legacyUuid.has_value());
  DataStructure itkDs;
  const auto readItkInput = ip_golden::ReadInputImage(itkDs, ip_golden::InputPath("RA-Short.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(readItkInput);
  const auto itkRun = ip_golden::RunLegacyItkFilter(*legacyUuid, itkDs, args); // identical key strings drive both filters
  SIMPLNX_RESULT_REQUIRE_VALID(itkRun);
  UnitTest::CompareDataArrays<int16>(ds.getDataRefAs<IDataArray>(output), itkDs.getDataRefAs<IDataArray>(output));

  UnitTest::CheckArraysInheritTupleDims(ds);
}
} // namespace

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- "defaults" case: RA-Short.nrrd, radius default {1,1,1}. Duplicates
// ITKMedianImageTest.cpp(defaults): (A) md5 == committed hash + (B) live-ITK bit-exact.
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MedianImageFilter: ITK real-image golden (defaults)", "[ImageProcessing][ItkGolden][MedianImageFilter]")
{
  RunMedianItkGolden(/*radius=*/std::nullopt, "cbc59611297961dea9f872282534f3df");
}

// -----------------------------------------------------------------------------
// ITK-sourced real-image md5 golden -- "by23" case: RA-Short.nrrd, radius {2,3,0}. Duplicates
// ITKMedianImageTest.cpp(by23).
// -----------------------------------------------------------------------------
TEST_CASE("ImageProcessing::MedianImageFilter: ITK real-image golden (by23)", "[ImageProcessing][ItkGolden][MedianImageFilter]")
{
  RunMedianItkGolden(/*radius=*/std::vector<uint32>{2, 3, 0}, "4afeba184100773dc279a776b1ae493b");
}
