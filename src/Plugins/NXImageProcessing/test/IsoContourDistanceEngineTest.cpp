#include "simplnx/Utilities/ImageProcessing/IsoContourDistanceEngine.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nx::core;
using namespace nx::core::ImageProcessing;

namespace
{
usize FlatIndex(usize x, usize y, usize z, usize dimX, usize dimY)
{
  return (z * dimY + y) * dimX + x;
}

template <class T>
class CountingDataStore : public DataStore<T>
{
public:
  using TransferRange = std::pair<usize, usize>;
  using DataStore<T>::DataStore;

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    ++m_ReadCount;
    m_ReadValues += buffer.size();
    m_ReadRanges.emplace_back(startIndex, buffer.size());
    return DataStore<T>::copyIntoBuffer(startIndex, buffer);
  }

  Result<> copyFromBuffer(usize startIndex, nonstd::span<const T> buffer) override
  {
    ++m_WriteCount;
    m_WrittenValues += buffer.size();
    m_MaxWriteValues = std::max(m_MaxWriteValues, buffer.size());
    m_WriteRanges.emplace_back(startIndex, buffer.size());
    return DataStore<T>::copyFromBuffer(startIndex, buffer);
  }

  [[nodiscard]] usize readCount() const noexcept
  {
    return m_ReadCount;
  }

  [[nodiscard]] usize readValues() const noexcept
  {
    return m_ReadValues;
  }

  [[nodiscard]] usize writeCount() const noexcept
  {
    return m_WriteCount;
  }

  [[nodiscard]] usize writtenValues() const noexcept
  {
    return m_WrittenValues;
  }

  [[nodiscard]] usize maxWriteValues() const noexcept
  {
    return m_MaxWriteValues;
  }

  [[nodiscard]] const std::vector<TransferRange>& readRanges() const noexcept
  {
    return m_ReadRanges;
  }

  [[nodiscard]] const std::vector<TransferRange>& writeRanges() const noexcept
  {
    return m_WriteRanges;
  }

private:
  mutable usize m_ReadCount = 0;
  mutable usize m_ReadValues = 0;
  mutable std::vector<TransferRange> m_ReadRanges;
  usize m_WriteCount = 0;
  usize m_WrittenValues = 0;
  usize m_MaxWriteValues = 0;
  std::vector<TransferRange> m_WriteRanges;
};

template <class T>
class OocReportingDataStore : public CountingDataStore<T>
{
public:
  using CountingDataStore<T>::CountingDataStore;

  IDataStore::StoreType getStoreType() const override
  {
    return IDataStore::StoreType::OutOfCore;
  }
};

template <class T>
class CancelOnReadDataStore : public OocReportingDataStore<T>
{
public:
  CancelOnReadDataStore(ShapeType tupleShape, ShapeType componentShape, T initValue, std::atomic_bool& shouldCancel)
  : OocReportingDataStore<T>(std::move(tupleShape), std::move(componentShape), initValue)
  , m_ShouldCancel(shouldCancel)
  {
  }

  Result<> copyIntoBuffer(usize startIndex, nonstd::span<T> buffer) const override
  {
    Result<> result = OocReportingDataStore<T>::copyIntoBuffer(startIndex, buffer);
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
class FailingReadDataStore : public OocReportingDataStore<T>
{
public:
  static constexpr int32 k_ErrorCode = -8630;
  using OocReportingDataStore<T>::OocReportingDataStore;

  Result<> copyIntoBuffer(usize, nonstd::span<T>) const override
  {
    return MakeErrorResult(k_ErrorCode, "The test input store rejected a bulk read.");
  }
};

template <class T>
class FailingWriteDataStore : public OocReportingDataStore<T>
{
public:
  static constexpr int32 k_ErrorCode = -8631;
  using OocReportingDataStore<T>::OocReportingDataStore;

  Result<> copyFromBuffer(usize, nonstd::span<const T>) override
  {
    return MakeErrorResult(k_ErrorCode, "The test output store rejected a bulk write.");
  }
};

// ScopedBudget sets the shared working-memory budget for one test.
// The destructor restores the previous budget after normal completion or a failed assertion.
class ScopedBudget
{
public:
  explicit ScopedBudget(uint64 budgetBytes)
  : m_Manager(CacheMemoryBudgetManager::instance())
  , m_PreviousBudget(m_Manager.budgetBytes())
  {
    m_Manager.clear();
    m_Manager.setBudgetBytes(budgetBytes);
  }
  ~ScopedBudget()
  {
    m_Manager.setBudgetBytes(m_PreviousBudget);
  }
  ScopedBudget(const ScopedBudget&) = delete;
  ScopedBudget(ScopedBudget&&) = delete;
  ScopedBudget& operator=(const ScopedBudget&) = delete;
  ScopedBudget& operator=(ScopedBudget&&) = delete;

private:
  CacheMemoryBudgetManager& m_Manager;
  uint64 m_PreviousBudget;
};

template <class T>
std::vector<float32> ReferenceIsoContour(const std::vector<T>& field, usize dimX, usize dimY, usize dimZ, float64 levelSet, float64 farValue, FloatVec3 spacing)
{
  using RealT = ImageProcessing::detail::IsoRealType<T>;
  const RealT level = static_cast<RealT>(levelSet);
  const float32 positiveFar = static_cast<float32>(farValue);
  const float32 negativeFar = -positiveFar;
  const std::array<double, 3> physicalSpacing = {static_cast<double>(spacing[0]), static_cast<double>(spacing[1]), static_cast<double>(spacing[2])};
  const int64 maxX = static_cast<int64>(dimX) - 1;
  const int64 maxY = static_cast<int64>(dimY) - 1;
  const int64 maxZ = static_cast<int64>(dimZ) - 1;
  const auto sampleIndex = [&](int64 x, int64 y, int64 z) {
    const usize clampedX = static_cast<usize>(std::clamp(x, int64{0}, maxX));
    const usize clampedY = static_cast<usize>(std::clamp(y, int64{0}, maxY));
    const usize clampedZ = static_cast<usize>(std::clamp(z, int64{0}, maxZ));
    return FlatIndex(clampedX, clampedY, clampedZ, dimX, dimY);
  };
  const auto sampleReal = [&](int64 x, int64 y, int64 z) -> RealT { return static_cast<RealT>(field[sampleIndex(x, y, z)]); };
  const auto sampleFloat = [&](int64 x, int64 y, int64 z) -> float32 { return static_cast<float32>(field[sampleIndex(x, y, z)]); };

  std::vector<float32> output(field.size());
  for(usize zIndex = 0; zIndex < dimZ; ++zIndex)
  {
    const int64 z = static_cast<int64>(zIndex);
    for(usize yIndex = 0; yIndex < dimY; ++yIndex)
    {
      const int64 y = static_cast<int64>(yIndex);
      for(usize xIndex = 0; xIndex < dimX; ++xIndex)
      {
        const int64 x = static_cast<int64>(xIndex);
        const RealT center = sampleReal(x, y, z);
        float32 best = center > level ? positiveFar : (center < level ? negativeFar : 0.0f);

        const auto evaluateEdge = [&](int64 edgeX, int64 edgeY, int64 edgeZ, int32 axis, bool targetIsFirst) {
          const int64 neighborX = edgeX + (axis == 0 ? 1 : 0);
          const int64 neighborY = edgeY + (axis == 1 ? 1 : 0);
          const int64 neighborZ = edgeZ + (axis == 2 ? 1 : 0);
          const RealT val0 = sampleReal(edgeX, edgeY, edgeZ) - level;
          const RealT val1 = sampleReal(neighborX, neighborY, neighborZ) - level;
          const bool sign = val0 > RealT{0};
          if(sign == (val1 > RealT{0}))
          {
            return;
          }

          std::array<RealT, 3> gradient0{};
          std::array<RealT, 3> gradient1{};
          for(int32 gradientAxis = 0; gradientAxis < 3; ++gradientAxis)
          {
            const int64 gradientX = gradientAxis == 0 ? 1 : 0;
            const int64 gradientY = gradientAxis == 1 ? 1 : 0;
            const int64 gradientZ = gradientAxis == 2 ? 1 : 0;
            gradient0[gradientAxis] = sampleReal(edgeX + gradientX, edgeY + gradientY, edgeZ + gradientZ) - sampleReal(edgeX - gradientX, edgeY - gradientY, edgeZ - gradientZ);
            const float32 positive = sampleFloat(neighborX + gradientX, neighborY + gradientY, neighborZ + gradientZ);
            const float32 negative = sampleFloat(neighborX - gradientX, neighborY - gradientY, neighborZ - gradientZ);
            gradient1[gradientAxis] = static_cast<RealT>(positive - negative);
          }

          const RealT difference = sign ? val0 - val1 : val1 - val0;
          if(difference < std::numeric_limits<RealT>::min())
          {
            return;
          }
          std::array<RealT, 3> gradient{};
          RealT norm = RealT{0};
          for(int32 gradientAxis = 0; gradientAxis < 3; ++gradientAxis)
          {
            gradient[gradientAxis] = (gradient0[gradientAxis] * RealT{0.5} + gradient1[gradientAxis] * RealT{0.5}) / (RealT{2} * physicalSpacing[static_cast<usize>(gradientAxis)]);
            norm += gradient[gradientAxis] * gradient[gradientAxis];
          }
          norm = std::sqrt(norm);
          if(norm < std::numeric_limits<RealT>::min())
          {
            return;
          }

          const usize edgeAxis = static_cast<usize>(axis);
          const RealT scale = std::abs(gradient[edgeAxis]) * physicalSpacing[edgeAxis] / norm / difference;
          const RealT candidate = (targetIsFirst ? val0 : val1) * scale;
          if(std::abs(static_cast<double>(candidate)) < std::abs(static_cast<double>(best)))
          {
            best = static_cast<float32>(candidate);
          }
        };

        if(z > 0)
        {
          evaluateEdge(x, y, z - 1, 2, false);
        }
        if(y > 0)
        {
          evaluateEdge(x, y - 1, z, 1, false);
        }
        if(x > 0)
        {
          evaluateEdge(x - 1, y, z, 0, false);
        }
        if(x + 1 < static_cast<int64>(dimX))
        {
          evaluateEdge(x, y, z, 0, true);
        }
        if(y + 1 < static_cast<int64>(dimY))
        {
          evaluateEdge(x, y, z, 1, true);
        }
        if(z + 1 < static_cast<int64>(dimZ))
        {
          evaluateEdge(x, y, z, 2, true);
        }
        output[FlatIndex(xIndex, yIndex, zIndex, dimX, dimY)] = best;
      }
    }
  }
  return output;
}

enum class IsoFieldMode
{
  Dense,
  Band,
  Flat,
  Level
};

template <class T>
std::vector<T> MakeIsoField(usize dimX, usize dimY, usize dimZ, uint32 seed, IsoFieldMode mode)
{
  constexpr float64 k_LevelSet = 10.0;
  std::vector<T> field(dimX * dimY * dimZ);
  if(mode == IsoFieldMode::Flat)
  {
    std::fill(field.begin(), field.end(), static_cast<T>(15));
    return field;
  }
  if(mode == IsoFieldMode::Band)
  {
    const double centerX = (static_cast<double>(dimX) - 1.0) * 0.5;
    const double centerY = (static_cast<double>(dimY) - 1.0) * 0.5;
    const double centerZ = (static_cast<double>(dimZ) - 1.0) * 0.5;
    const double radius = static_cast<double>(std::min({dimX, dimY, dimZ})) / 3.0;
    const double radiusSquared = radius * radius;
    for(usize z = 0; z < dimZ; ++z)
    {
      for(usize y = 0; y < dimY; ++y)
      {
        for(usize x = 0; x < dimX; ++x)
        {
          const double deltaX = static_cast<double>(x) - centerX;
          const double deltaY = static_cast<double>(y) - centerY;
          const double deltaZ = static_cast<double>(z) - centerZ;
          field[FlatIndex(x, y, z, dimX, dimY)] = static_cast<T>(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ <= radiusSquared ? 15 : 3);
        }
      }
    }
    return field;
  }

  std::mt19937 generator(seed);
  if constexpr(std::is_floating_point_v<T>)
  {
    std::uniform_real_distribution<float64> distribution(0.0, 20.0);
    for(T& value : field)
    {
      value = static_cast<T>(distribution(generator));
    }
  }
  else
  {
    std::uniform_int_distribution<int32> distribution(0, 20);
    for(T& value : field)
    {
      value = static_cast<T>(distribution(generator));
    }
  }
  if(mode == IsoFieldMode::Level)
  {
    for(usize index = 0; index < field.size(); index += 3)
    {
      field[index] = static_cast<T>(k_LevelSet);
    }
  }
  return field;
}

template <class T>
std::vector<float32> RunDirect(const std::vector<T>& field, const SizeVec3& dims, float64 levelSet, float64 farValue, FloatVec3 spacing)
{
  DataStore<T> inputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, T{});
  for(usize index = 0; index < field.size(); ++index)
  {
    inputStore.setValue(index, field[index]);
  }
  DataStore<float32> outputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, 0.0f);
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  IsoContourDistance<T> engine(inputStore, outputStore, dims, levelSet, farValue, spacing, shouldCancel, messageHandler);
  const Result<> engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);
  std::vector<float32> output(field.size());
  for(usize index = 0; index < output.size(); ++index)
  {
    output[index] = outputStore.getValue(index);
  }
  return output;
}

struct IsoContourCountingResult
{
  using TransferRange = std::pair<usize, usize>;

  std::vector<float32> output;
  usize readCount = 0;
  usize readValues = 0;
  usize writeCount = 0;
  usize writtenValues = 0;
  usize maxWriteValues = 0;
  std::vector<TransferRange> readRanges;
  std::vector<TransferRange> writeRanges;
};

template <class T>
IsoContourCountingResult RunSlabCounting(const std::vector<T>& field, const SizeVec3& dims, uint64 budgetBytes, float64 levelSet, float64 farValue, FloatVec3 spacing)
{
  ScopedBudget budget(budgetBytes);
  OocReportingDataStore<T> inputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, T{});
  OocReportingDataStore<float32> outputStore(ShapeType{dims[2], dims[1], dims[0]}, ShapeType{1}, -999.0f);
  for(usize index = 0; index < field.size(); ++index)
  {
    inputStore.setValue(index, field[index]);
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  IsoContourDistance<T> engine(inputStore, outputStore, dims, levelSet, farValue, spacing, shouldCancel, messageHandler);
  const Result<> engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);
  std::vector<float32> output(field.size());
  for(usize index = 0; index < output.size(); ++index)
  {
    output[index] = outputStore.getValue(index);
  }
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  return {std::move(output),           inputStore.readCount(),       inputStore.readValues(), outputStore.writeCount(),
          outputStore.writtenValues(), outputStore.maxWriteValues(), inputStore.readRanges(), outputStore.writeRanges()};
}

void RequireBitIdentical(const std::vector<float32>& actual, const std::vector<float32>& expected)
{
  REQUIRE(actual.size() == expected.size());
  for(usize index = 0; index < actual.size(); ++index)
  {
    CAPTURE(index);
    REQUIRE(std::bit_cast<uint32>(actual[index]) == std::bit_cast<uint32>(expected[index]));
  }
}

template <class T>
std::vector<float32> RunEngine(const std::vector<T>& field, usize dx, usize dy, usize dz, float64 levelSet, float64 farValue, FloatVec3 spacing)
{
  return RunDirect(field, SizeVec3{dx, dy, dz}, levelSet, farValue, spacing);
}
} // namespace

TEMPLATE_TEST_CASE("ImageProcessing::IsoContourDistanceEngine: direct route matches the serial gather", "[ImageProcessing][IsoContourDistanceEngine]", uint8, int16, float32, float64)
{
  using T = TestType;
  constexpr float64 k_LevelSet = 10.0;
  constexpr float64 k_FarValue = 100.0;
  const FloatVec3 k_UnitSpacing{1.0f, 1.0f, 1.0f};
  const std::array<SizeVec3, 10> dimensions = {SizeVec3{1, 1, 1}, SizeVec3{1, 1, 7}, SizeVec3{7, 1, 1},    SizeVec3{1, 5, 1},  SizeVec3{2, 2, 2},
                                               SizeVec3{3, 3, 3}, SizeVec3{7, 6, 5}, SizeVec3{17, 13, 11}, SizeVec3{33, 9, 7}, SizeVec3{5, 40, 6}};
  constexpr std::array<IsoFieldMode, 4> k_Modes = {IsoFieldMode::Dense, IsoFieldMode::Band, IsoFieldMode::Flat, IsoFieldMode::Level};

  for(usize dimensionsIndex = 0; dimensionsIndex < dimensions.size(); ++dimensionsIndex)
  {
    const SizeVec3& dims = dimensions[dimensionsIndex];
    DYNAMIC_SECTION(dims[0] << "x" << dims[1] << "x" << dims[2])
    {
      for(const IsoFieldMode mode : k_Modes)
      {
        CAPTURE(mode);
        const std::vector<T> field = MakeIsoField<T>(dims[0], dims[1], dims[2], uint32{0x15C00001u} + static_cast<uint32>(dimensionsIndex), mode);
        const std::vector<float32> expected = ReferenceIsoContour(field, dims[0], dims[1], dims[2], k_LevelSet, k_FarValue, k_UnitSpacing);
        const std::vector<float32> actual = RunDirect(field, dims, k_LevelSet, k_FarValue, k_UnitSpacing);
        RequireBitIdentical(actual, expected);
      }
    }
  }

  if constexpr(std::is_same_v<T, uint8> || std::is_same_v<T, float64>)
  {
    const SizeVec3 k_Dims{7, 6, 5};
    const FloatVec3 k_Spacing{0.5f, 2.0f, 1.25f};
    const std::vector<T> field = MakeIsoField<T>(k_Dims[0], k_Dims[1], k_Dims[2], uint32{0x15C0A115u}, IsoFieldMode::Dense);
    const std::vector<float32> expected = ReferenceIsoContour(field, k_Dims[0], k_Dims[1], k_Dims[2], k_LevelSet, k_FarValue, k_Spacing);
    const std::vector<float32> actual = RunDirect(field, k_Dims, k_LevelSet, k_FarValue, k_Spacing);
    RequireBitIdentical(actual, expected);
  }
}

TEMPLATE_TEST_CASE("ImageProcessing::IsoContourDistanceEngine: slab route matches the serial gather at every slab depth", "[ImageProcessing][IsoContourDistanceEngine][WorkingMemory]", uint8, float32)
{
  using T = TestType;
  constexpr float64 k_LevelSet = 10.0;
  constexpr float64 k_FarValue = 100.0;
  const FloatVec3 k_Spacing{1.0f, 1.0f, 1.0f};
  const std::array<SizeVec3, 5> dimensions = {SizeVec3{7, 6, 5}, SizeVec3{17, 13, 11}, SizeVec3{5, 40, 6}, SizeVec3{9, 8, 2}, SizeVec3{9, 8, 3}};
  constexpr std::array<IsoFieldMode, 2> k_Modes = {IsoFieldMode::Dense, IsoFieldMode::Band};

  for(usize dimensionsIndex = 0; dimensionsIndex < dimensions.size(); ++dimensionsIndex)
  {
    const SizeVec3& dims = dimensions[dimensionsIndex];
    const uint64 sliceValues = dims[0] * dims[1];
    const uint64 planeInputBytes = sliceValues * sizeof(T);
    const uint64 planeOutputBytes = sliceValues * sizeof(float32);
    const uint64 usefulBytes = dims[2] * (planeInputBytes + planeOutputBytes);
    const uint64 partialGrant = std::min(5 * planeInputBytes + planeOutputBytes - 1, usefulBytes - 1);
    std::vector<std::pair<uint64, usize>> budgets = {{4 * partialGrant, 1}};
    const uint64 twoCoreGrant = 6 * planeInputBytes + 2 * planeOutputBytes;
    if(twoCoreGrant < usefulBytes)
    {
      budgets.emplace_back(4 * twoCoreGrant, 2);
    }
    budgets.emplace_back(4 * usefulBytes, dims[2]);

    for(const IsoFieldMode mode : k_Modes)
    {
      const std::vector<T> field = MakeIsoField<T>(dims[0], dims[1], dims[2], uint32{0x15C05A81u} + static_cast<uint32>(dimensionsIndex), mode);
      const std::vector<float32> expected = ReferenceIsoContour(field, dims[0], dims[1], dims[2], k_LevelSet, k_FarValue, k_Spacing);
      for(const auto& [budgetBytes, expectedCorePlanes] : budgets)
      {
        DYNAMIC_SECTION(dims[0] << "x" << dims[1] << "x" << dims[2] << " mode " << static_cast<int32>(mode) << " core " << expectedCorePlanes)
        {
          const auto plan = ImageProcessing::detail::PlanIsoContourSlab<T>(dims, budgetBytes / 4);
          REQUIRE(plan.corePlanes == expectedCorePlanes);
          const IsoContourCountingResult result = RunSlabCounting(field, dims, budgetBytes, k_LevelSet, k_FarValue, k_Spacing);
          RequireBitIdentical(result.output, expected);
          REQUIRE(result.readValues == dims[2] * sliceValues);
          const usize numberOfSlabs = (dims[2] + expectedCorePlanes - 1) / expectedCorePlanes;
          std::vector<IsoContourCountingResult::TransferRange> expectedWriteRanges;
          for(usize z0 = 0; z0 < dims[2]; z0 += expectedCorePlanes)
          {
            expectedWriteRanges.emplace_back(z0 * sliceValues, std::min(expectedCorePlanes, dims[2] - z0) * sliceValues);
          }
          REQUIRE(result.readCount <= 2 * numberOfSlabs);
          REQUIRE(result.writeCount == numberOfSlabs);
          REQUIRE(result.writeRanges == expectedWriteRanges);
          REQUIRE(result.maxWriteValues == plan.corePlanes * sliceValues);
          if(plan.corePlanes == dims[2])
          {
            REQUIRE(result.readCount == 1);
          }
        }
      }
    }
  }
}

// A pure-axis unit-slope ramp has one level-set crossing between the two planes that bracket the level set.
// The sub-pixel formula gives the exact signed physical distance for those two planes.
// Every other voxel stays at the signed far value. This test checks each axis, sign, and spacing weight.
TEMPLATE_TEST_CASE("ImageProcessing::IsoContourDistanceEngine: axis ramp == analytic signed distance", "[ImageProcessing][IsoContourDistanceEngine]", uint8, int16, int32, float32, float64)
{
  using T = TestType;
  const int axis = GENERATE(0, 1, 2);
  const bool aniso = GENERATE(false, true);
  const FloatVec3 spacing = aniso ? FloatVec3{2.0f, 1.5f, 3.0f} : FloatVec3{1.0f, 1.0f, 1.0f};
  constexpr float64 kLevel = 2.5;
  constexpr float32 kFar = 10.0f;
  CAPTURE(axis, aniso);

  constexpr usize DX = 6, DY = 5, DZ = 4;
  std::vector<T> field(DX * DY * DZ, T{0});
  for(usize z = 0; z < DZ; ++z)
  {
    for(usize y = 0; y < DY; ++y)
    {
      for(usize x = 0; x < DX; ++x)
      {
        const usize coord = (axis == 0) ? x : (axis == 1) ? y : z;
        field[FlatIndex(x, y, z, DX, DY)] = static_cast<T>(coord);
      }
    }
  }

  const std::vector<float32> out = RunEngine<T>(field, DX, DY, DZ, kLevel, kFar, spacing);
  const float32 sp = spacing[static_cast<usize>(axis)];

  for(usize z = 0; z < DZ; ++z)
  {
    for(usize y = 0; y < DY; ++y)
    {
      for(usize x = 0; x < DX; ++x)
      {
        const int64 coord = static_cast<int64>((axis == 0) ? x : (axis == 1) ? y : z);
        const float32 got = out[FlatIndex(x, y, z, DX, DY)];
        INFO("axis " << axis << " coord " << coord << " aniso " << aniso);
        if(coord == 2 || coord == 3)
        {
          REQUIRE(got == Approx(static_cast<float32>((static_cast<float64>(coord) - kLevel)) * sp).epsilon(1e-5));
        }
        else if(coord < 2)
        {
          REQUIRE(got == -kFar);
        }
        else
        {
          REQUIRE(got == kFar);
        }
      }
    }
  }
}

// A field entirely above or below the level set has no crossing.
// Each voxel receives the applicable signed far value. A voxel on the level set receives zero.
TEST_CASE("ImageProcessing::IsoContourDistanceEngine: init and far regions", "[ImageProcessing][IsoContourDistanceEngine]")
{
  constexpr usize DX = 5, DY = 4, DZ = 3;
  const float64 levelSet = GENERATE(0.0, 3.0, -1.0);
  const float64 farValue = GENERATE(10.0, 4.0);
  CAPTURE(levelSet, farValue);

  SECTION("all above -> +far")
  {
    std::vector<int32> field(DX * DY * DZ, static_cast<int32>(levelSet) + 5);
    const std::vector<float32> out = RunEngine<int32>(field, DX, DY, DZ, levelSet, farValue, FloatVec3{1.0f, 1.0f, 1.0f});
    for(float32 v : out)
    {
      REQUIRE(v == static_cast<float32>(farValue));
    }
  }
  SECTION("all below -> -far")
  {
    std::vector<int32> field(DX * DY * DZ, static_cast<int32>(levelSet) - 5);
    const std::vector<float32> out = RunEngine<int32>(field, DX, DY, DZ, levelSet, farValue, FloatVec3{1.0f, 1.0f, 1.0f});
    for(float32 v : out)
    {
      REQUIRE(v == -static_cast<float32>(farValue));
    }
  }
  SECTION("exactly on level set -> 0")
  {
    std::vector<float32> field(DX * DY * DZ, static_cast<float32>(levelSet));
    const std::vector<float32> out = RunEngine<float32>(field, DX, DY, DZ, levelSet, farValue, FloatVec3{1.0f, 1.0f, 1.0f});
    for(float32 v : out)
    {
      REQUIRE(v == 0.0f);
    }
  }
}

// Radial field: interior of the contour is negative, exterior positive, and all outputs are finite.
TEST_CASE("ImageProcessing::IsoContourDistanceEngine: radial sign + finiteness", "[ImageProcessing][IsoContourDistanceEngine]")
{
  constexpr usize D = 15;
  const float64 c = 7.0;
  const float64 radius = 4.0;
  std::vector<float32> field(D * D * D);
  for(usize z = 0; z < D; ++z)
  {
    for(usize y = 0; y < D; ++y)
    {
      for(usize x = 0; x < D; ++x)
      {
        const float64 dxc = x - c, dyc = y - c, dzc = z - c;
        field[FlatIndex(x, y, z, D, D)] = static_cast<float32>(std::sqrt(dxc * dxc + dyc * dyc + dzc * dzc));
      }
    }
  }
  const std::vector<float32> out = RunEngine<float32>(field, D, D, D, radius, 100.0, FloatVec3{1.0f, 1.0f, 1.0f});
  for(float32 v : out)
  {
    REQUIRE(std::isfinite(v));
  }
  REQUIRE(out[FlatIndex(7, 7, 7, D, D)] < 0.0f); // center: deep interior
  REQUIRE(out[FlatIndex(0, 0, 0, D, D)] > 0.0f); // corner: far exterior
}

// Determinism: the same input yields byte-identical output on repeated runs.
TEST_CASE("ImageProcessing::IsoContourDistanceEngine: deterministic", "[ImageProcessing][IsoContourDistanceEngine]")
{
  constexpr usize D = 12;
  std::vector<int16> field(D * D * D);
  for(usize i = 0; i < field.size(); ++i)
  {
    field[i] = static_cast<int16>((i * 7 + 3) % 11);
  }
  const std::vector<float32> a = RunEngine<int16>(field, D, D, D, 5.0, 10.0, FloatVec3{1.0f, 1.0f, 1.0f});
  const std::vector<float32> b = RunEngine<int16>(field, D, D, D, 5.0, 10.0, FloatVec3{1.0f, 1.0f, 1.0f});
  REQUIRE(a.size() == b.size());
  for(usize i = 0; i < a.size(); ++i)
  {
    REQUIRE(a[i] == b[i]);
  }
}

// A small grant makes the ring wrap across many slabs.
// The output must reproduce the analytic Z-ramp result.
TEST_CASE("ImageProcessing::IsoContourDistanceEngine: tall-volume streaming z-ramp", "[ImageProcessing][IsoContourDistanceEngine]")
{
  constexpr usize DX = 9, DY = 8, DZ = 40;
  constexpr float32 kFar = 50.0f;
  std::vector<int32> field(DX * DY * DZ);
  for(usize z = 0; z < DZ; ++z)
  {
    for(usize y = 0; y < DY; ++y)
    {
      for(usize x = 0; x < DX; ++x)
      {
        field[FlatIndex(x, y, z, DX, DY)] = static_cast<int32>(z);
      }
    }
  }
  constexpr uint64 k_PlaneInputBytes = DX * DY * sizeof(int32);
  constexpr uint64 k_PlaneOutputBytes = DX * DY * sizeof(float32);
  constexpr uint64 k_BudgetBytes = 4 * (5 * k_PlaneInputBytes + k_PlaneOutputBytes - 1);
  const IsoContourCountingResult result = RunSlabCounting(field, SizeVec3{DX, DY, DZ}, k_BudgetBytes, 2.5, kFar, FloatVec3{1.0f, 1.0f, 1.0f});
  const std::vector<float32>& out = result.output;
  REQUIRE(result.readValues == field.size());
  REQUIRE(result.readCount <= 2 * DZ);
  for(usize z = 0; z < DZ; ++z)
  {
    const float32 got = out[FlatIndex(4, 4, z, DX, DY)];
    INFO("z " << z);
    if(z == 2 || z == 3)
    {
      REQUIRE(got == Approx(static_cast<float32>(z) - 2.5f).epsilon(1e-5));
    }
    else if(z < 2)
    {
      REQUIRE(got == -kFar);
    }
    else
    {
      REQUIRE(got == kFar);
    }
  }
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: validates stores and cancellation", "[ImageProcessing][IsoContourDistanceEngine]")
{
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  SECTION("rejects mismatched stores")
  {
    DataStore<uint8> shortInput(ShapeType{63}, ShapeType{1}, uint8{0});
    DataStore<uint8> fullInput(ShapeType{64}, ShapeType{1}, uint8{0});
    DataStore<float32> shortOutput(ShapeType{63}, ShapeType{1}, 0.0f);
    DataStore<float32> fullOutput(ShapeType{64}, ShapeType{1}, 0.0f);

    IsoContourDistance<uint8> inputMismatch(shortInput, fullOutput, SizeVec3{4, 4, 4}, 0.0, 10.0, FloatVec3{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
    Result<> result = inputMismatch();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8622);

    IsoContourDistance<uint8> outputMismatch(fullInput, shortOutput, SizeVec3{4, 4, 4}, 0.0, 10.0, FloatVec3{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
    result = outputMismatch();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == -8623);
  }

  SECTION("pre-cancel preserves output")
  {
    constexpr float32 kPoison = -12345.0f;
    DataStore<uint8> inputStore(ShapeType{4, 4, 4}, ShapeType{1}, uint8{1});
    DataStore<float32> outputStore(ShapeType{4, 4, 4}, ShapeType{1}, kPoison);
    shouldCancel = true;
    IsoContourDistance<uint8> engine(inputStore, outputStore, SizeVec3{4, 4, 4}, 0.0, 10.0, FloatVec3{1.0f, 1.0f, 1.0f}, shouldCancel, messageHandler);
    const Result<> result = engine();
    REQUIRE(result.valid());
    for(const float32 value : outputStore)
    {
      REQUIRE(value == kPoison);
    }
  }
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: slab plan derives the plane count from the grant", "[ImageProcessing][IsoContourDistanceEngine]")
{
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 k_LargeDims{512, 512, 128};

  const auto uint8Partial = ImageProcessing::detail::PlanIsoContourSlab<uint8>(k_LargeDims, 128 * k_MiB);
  REQUIRE(uint8Partial.corePlanes == 101);
  REQUIRE(uint8Partial.inputPlanes == 105);
  REQUIRE(uint8Partial.inputBytes == 105 * 256 * 1024);
  REQUIRE(uint8Partial.outputBytes == 101 * k_MiB);

  const auto uint8Complete = ImageProcessing::detail::PlanIsoContourSlab<uint8>(k_LargeDims, 160 * k_MiB);
  REQUIRE(uint8Complete.corePlanes == 128);
  REQUIRE(uint8Complete.inputPlanes == 128);
  REQUIRE(uint8Complete.inputBytes == 32 * k_MiB);
  REQUIRE(uint8Complete.outputBytes == 128 * k_MiB);

  const auto float64Partial = ImageProcessing::detail::PlanIsoContourSlab<float64>(k_LargeDims, 128 * k_MiB);
  REQUIRE(float64Partial.corePlanes == 40);
  REQUIRE(float64Partial.inputPlanes == 44);
  REQUIRE(float64Partial.inputBytes == 44 * 2 * k_MiB);
  REQUIRE(float64Partial.outputBytes == 40 * k_MiB);

  const auto float64Thin = ImageProcessing::detail::PlanIsoContourSlab<float64>(SizeVec3{1024, 1024, 2}, 20 * k_MiB);
  REQUIRE(float64Thin.corePlanes == 1);
  REQUIRE(float64Thin.inputPlanes == 2);
  REQUIRE(float64Thin.inputBytes == 16 * k_MiB);
  REQUIRE(float64Thin.outputBytes == 4 * k_MiB);

  const SizeVec3 k_ThreePlaneDims{9, 8, 3};
  constexpr uint64 k_ThreePlaneUsefulBytes = 3 * 9 * 8 * (sizeof(uint8) + sizeof(float32));
  const auto uint8Thin = ImageProcessing::detail::PlanIsoContourSlab<uint8>(k_ThreePlaneDims, k_ThreePlaneUsefulBytes - 1);
  REQUIRE(uint8Thin.corePlanes <= 2);
  REQUIRE(uint8Thin.inputPlanes == 3);

  const SizeVec3 k_SmallDims{5, 6, 7};
  const auto smallPartial = ImageProcessing::detail::PlanIsoContourSlab<uint8>(k_SmallDims, 512);
  REQUIRE(smallPartial.corePlanes == 2);
  REQUIRE(smallPartial.inputPlanes == 6);
  REQUIRE(smallPartial.inputBytes == 180);
  REQUIRE(smallPartial.outputBytes == 240);

  const auto smallComplete = ImageProcessing::detail::PlanIsoContourSlab<uint8>(k_SmallDims, 2048);
  REQUIRE(smallComplete.corePlanes == 7);
  REQUIRE(smallComplete.inputPlanes == 7);
  REQUIRE(smallComplete.inputBytes == 210);
  REQUIRE(smallComplete.outputBytes == 840);

  auto uint8Useful = ImageProcessing::detail::IsoContourSlabUsefulBytes<uint8>(k_LargeDims);
  SIMPLNX_RESULT_REQUIRE_VALID(uint8Useful);
  REQUIRE(uint8Useful.value() == 160 * k_MiB);
  auto float64Useful = ImageProcessing::detail::IsoContourSlabUsefulBytes<float64>(k_LargeDims);
  SIMPLNX_RESULT_REQUIRE_VALID(float64Useful);
  REQUIRE(float64Useful.value() == 384 * k_MiB);
  auto overflow = ImageProcessing::detail::IsoContourSlabUsefulBytes<uint8>(SizeVec3{std::numeric_limits<usize>::max(), 2, 2});
  SIMPLNX_RESULT_REQUIRE_INVALID(overflow);
  REQUIRE(overflow.errors().front().code == -8626);
  auto zeroDimension = ImageProcessing::detail::IsoContourSlabUsefulBytes<uint8>(SizeVec3{4, 4, 0});
  SIMPLNX_RESULT_REQUIRE_VALID(zeroDimension);
  REQUIRE(zeroDimension.value() == 0);
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: slab reservation follows the grant", "[ImageProcessing][IsoContourDistanceEngine][WorkingMemory]")
{
  constexpr uint64 k_MiB = 1024ULL * 1024ULL;
  const SizeVec3 k_Dims{512, 512, 128};
  auto usefulResult = ImageProcessing::detail::IsoContourSlabUsefulBytes<uint8>(k_Dims);
  SIMPLNX_RESULT_REQUIRE_VALID(usefulResult);
  const usize usefulBytes = usefulResult.value();

  {
    ScopedBudget budget(512 * k_MiB);
    {
      auto reservation = ReserveWorkingMemory(usefulBytes, usefulBytes);
      REQUIRE(reservation.sizeBytes() == 128 * k_MiB);
      const auto plan = ImageProcessing::detail::PlanIsoContourSlab<uint8>(k_Dims, reservation.sizeBytes());
      REQUIRE(plan.corePlanes == 101);
      REQUIRE(plan.inputPlanes == 105);
    }
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);

  {
    ScopedBudget budget(1024 * k_MiB);
    {
      auto reservation = ReserveWorkingMemory(usefulBytes, usefulBytes);
      REQUIRE(reservation.sizeBytes() == 160 * k_MiB);
      const auto plan = ImageProcessing::detail::PlanIsoContourSlab<uint8>(k_Dims, reservation.sizeBytes());
      REQUIRE(plan.corePlanes == 128);
      REQUIRE(plan.inputPlanes == 128);
    }
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: slab route transfers follow the plan", "[ImageProcessing][IsoContourDistanceEngine][WorkingMemory]")
{
  const SizeVec3 k_Dims{5, 6, 7};
  const FloatVec3 k_Spacing{1.0f, 1.0f, 1.0f};
  const std::vector<uint8> field = MakeIsoField<uint8>(k_Dims[0], k_Dims[1], k_Dims[2], uint32{0x15C071A9u}, IsoFieldMode::Dense);
  const std::vector<float32> expected = ReferenceIsoContour(field, k_Dims[0], k_Dims[1], k_Dims[2], 10.0, 100.0, k_Spacing);

  const IsoContourCountingResult partial = RunSlabCounting(field, k_Dims, 2048, 10.0, 100.0, k_Spacing);
  RequireBitIdentical(partial.output, expected);
  const std::vector<IsoContourCountingResult::TransferRange> expectedPartialReads = {{0, 120}, {120, 60}, {180, 30}};
  const std::vector<IsoContourCountingResult::TransferRange> expectedPartialWrites = {{0, 60}, {60, 60}, {120, 60}, {180, 30}};
  REQUIRE(partial.readRanges == expectedPartialReads);
  REQUIRE(partial.writeRanges == expectedPartialWrites);
  REQUIRE(partial.writeCount == 4);
  REQUIRE(partial.readValues == 210);
  REQUIRE(partial.maxWriteValues == 60);

  const IsoContourCountingResult complete = RunSlabCounting(field, k_Dims, 8192, 10.0, 100.0, k_Spacing);
  RequireBitIdentical(complete.output, expected);
  REQUIRE(complete.readCount == 1);
  REQUIRE(complete.writeCount == 1);
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: slab route reads each input plane once", "[ImageProcessing][IsoContourDistanceEngine][WorkingMemory]")
{
  const SizeVec3 k_Dims{5, 6, 7};
  const FloatVec3 k_Spacing{1.0f, 1.0f, 1.0f};
  const std::vector<uint8> field = MakeIsoField<uint8>(k_Dims[0], k_Dims[1], k_Dims[2], uint32{0x15C091A9u}, IsoFieldMode::Dense);
  const IsoContourCountingResult result = RunSlabCounting(field, k_Dims, 2048, 10.0, 100.0, k_Spacing);
  REQUIRE(result.readValues == 7 * 30);
  REQUIRE(result.writtenValues == 7 * 30);
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: direct route performs no bulk transfers", "[ImageProcessing][IsoContourDistanceEngine]")
{
  const SizeVec3 k_Dims{17, 13, 11};
  const FloatVec3 k_Spacing{1.0f, 1.0f, 1.0f};
  const std::vector<uint8> field = MakeIsoField<uint8>(k_Dims[0], k_Dims[1], k_Dims[2], uint32{0x15C0D17Eu}, IsoFieldMode::Dense);
  const std::vector<float32> expected = ReferenceIsoContour(field, k_Dims[0], k_Dims[1], k_Dims[2], 10.0, 100.0, k_Spacing);
  CountingDataStore<uint8> inputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, uint8{0});
  CountingDataStore<float32> outputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, -999.0f);
  for(usize index = 0; index < field.size(); ++index)
  {
    inputStore.setValue(index, field[index]);
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  IsoContourDistance<uint8> engine(inputStore, outputStore, k_Dims, 10.0, 100.0, k_Spacing, shouldCancel, messageHandler);
  const Result<> engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);
  std::vector<float32> actual(field.size());
  for(usize index = 0; index < actual.size(); ++index)
  {
    actual[index] = outputStore.getValue(index);
  }
  REQUIRE(inputStore.readCount() == 0);
  REQUIRE(outputStore.writeCount() == 0);
  RequireBitIdentical(actual, expected);
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: an OOC store overrides the forced in-core path", "[ImageProcessing][IsoContourDistanceEngine][WorkingMemory]")
{
  ScopedBudget budget(2048);
  const SizeVec3 k_Dims{5, 6, 7};
  const FloatVec3 k_Spacing{1.0f, 1.0f, 1.0f};
  const std::vector<uint8> field = MakeIsoField<uint8>(k_Dims[0], k_Dims[1], k_Dims[2], uint32{0x15C0F04Cu}, IsoFieldMode::Dense);
  OocReportingDataStore<uint8> inputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, uint8{0});
  OocReportingDataStore<float32> outputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, -999.0f);
  for(usize index = 0; index < field.size(); ++index)
  {
    inputStore.setValue(index, field[index]);
  }

  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};
  ForceInCoreAlgorithmGuard forceInCore;
  IsoContourDistance<uint8> engine(inputStore, outputStore, k_Dims, 10.0, 100.0, k_Spacing, shouldCancel, messageHandler);
  const AlgorithmPathExecutionCounts countsBefore = GetAlgorithmPathExecutionCounts();
  const Result<> engineResult = engine();
  SIMPLNX_RESULT_REQUIRE_VALID(engineResult);
  const AlgorithmPathExecutionCounts countsAfter = GetAlgorithmPathExecutionCounts();
  REQUIRE(inputStore.readCount() > 0);
  REQUIRE(outputStore.writeCount() > 0);
  REQUIRE(countsAfter.InCore == countsBefore.InCore);
  REQUIRE(countsAfter.OutOfCore == countsBefore.OutOfCore + 1);
  REQUIRE(countsAfter.OutOfCoreOnOutOfCoreStore == countsBefore.OutOfCoreOnOutOfCoreStore + 1);
  REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: pre-cancelled run leaves the output untouched", "[ImageProcessing][IsoContourDistanceEngine][WorkingMemory]")
{
  const SizeVec3 k_Dims{9, 8, 7};
  constexpr float32 k_Poison = -999.0f;
  const FloatVec3 k_Spacing{1.0f, 1.0f, 1.0f};
  const std::vector<uint8> field = MakeIsoField<uint8>(k_Dims[0], k_Dims[1], k_Dims[2], uint32{0x15C0CA11u}, IsoFieldMode::Dense);
  std::atomic_bool shouldCancel{true};
  IFilter::MessageHandler messageHandler{};

  SECTION("direct route")
  {
    CountingDataStore<uint8> inputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, uint8{0});
    CountingDataStore<float32> outputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, k_Poison);
    for(usize index = 0; index < field.size(); ++index)
    {
      inputStore.setValue(index, field[index]);
    }
    IsoContourDistance<uint8> engine(inputStore, outputStore, k_Dims, 10.0, 100.0, k_Spacing, shouldCancel, messageHandler);
    const Result<> result = engine();
    REQUIRE(result.valid());
    REQUIRE(inputStore.readCount() == 0);
    REQUIRE(outputStore.writeCount() == 0);
    for(const float32 value : outputStore)
    {
      REQUIRE(value == k_Poison);
    }
  }

  SECTION("slab route")
  {
    ScopedBudget budget(8192);
    OocReportingDataStore<uint8> inputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, uint8{0});
    OocReportingDataStore<float32> outputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, k_Poison);
    for(usize index = 0; index < field.size(); ++index)
    {
      inputStore.setValue(index, field[index]);
    }
    IsoContourDistance<uint8> engine(inputStore, outputStore, k_Dims, 10.0, 100.0, k_Spacing, shouldCancel, messageHandler);
    const Result<> result = engine();
    REQUIRE(result.valid());
    REQUIRE(inputStore.readCount() == 0);
    REQUIRE(outputStore.writeCount() == 0);
    for(const float32 value : outputStore)
    {
      REQUIRE(value == k_Poison);
    }
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: slab cancellation and transfer failures release the reservation", "[ImageProcessing][IsoContourDistanceEngine][WorkingMemory]")
{
  ScopedBudget budget(2048);
  const SizeVec3 k_Dims{5, 6, 7};
  constexpr float32 k_Poison = -999.0f;
  const FloatVec3 k_Spacing{1.0f, 1.0f, 1.0f};
  std::atomic_bool shouldCancel{false};
  IFilter::MessageHandler messageHandler{};

  SECTION("cancellation after the first read prevents output writes")
  {
    CancelOnReadDataStore<uint8> inputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, uint8{1}, shouldCancel);
    OocReportingDataStore<float32> outputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, k_Poison);
    IsoContourDistance<uint8> engine(inputStore, outputStore, k_Dims, 10.0, 100.0, k_Spacing, shouldCancel, messageHandler);
    const Result<> result = engine();
    REQUIRE(result.valid());
    REQUIRE(inputStore.readCount() == 1);
    REQUIRE(outputStore.writeCount() == 0);
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }

  SECTION("read failure is returned")
  {
    FailingReadDataStore<uint8> inputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, uint8{1});
    OocReportingDataStore<float32> outputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, k_Poison);
    IsoContourDistance<uint8> engine(inputStore, outputStore, k_Dims, 10.0, 100.0, k_Spacing, shouldCancel, messageHandler);
    const Result<> result = engine();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == FailingReadDataStore<uint8>::k_ErrorCode);
    REQUIRE(outputStore.writeCount() == 0);
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }

  SECTION("write failure is returned")
  {
    OocReportingDataStore<uint8> inputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, uint8{1});
    FailingWriteDataStore<float32> outputStore(ShapeType{k_Dims[2], k_Dims[1], k_Dims[0]}, ShapeType{1}, k_Poison);
    IsoContourDistance<uint8> engine(inputStore, outputStore, k_Dims, 10.0, 100.0, k_Spacing, shouldCancel, messageHandler);
    const Result<> result = engine();
    REQUIRE(result.invalid());
    REQUIRE(result.errors().front().code == FailingWriteDataStore<float32>::k_ErrorCode);
    REQUIRE(CacheMemoryBudgetManager::instance().reservedWorkingMemoryBytes() == 0);
  }
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: 2D planner bounds row blocks and overwide tiles", "[ImageProcessing][IsoContourDistanceEngine]")
{
  const auto benchmark = ImageProcessing::detail::BuildIsoContour2DBufferPlan(5888, 5888, sizeof(uint8));
  REQUIRE(benchmark.valid);
  REQUIRE(benchmark.coreCols == 5888);
  REQUIRE(benchmark.coreRows > 1);
  REQUIRE(benchmark.residentBytes <= ImageProcessing::detail::k_IsoContour2DResidentLimit);

  for(const usize inputBytes : {sizeof(uint8), sizeof(uint64)})
  {
    const auto stress = ImageProcessing::detail::BuildIsoContour2DBufferPlan(16385, 1025, inputBytes);
    CAPTURE(inputBytes);
    REQUIRE(stress.valid);
    REQUIRE(stress.coreCols == 16385);
    REQUIRE(stress.coreRows > 0);
    REQUIRE(stress.residentBytes <= ImageProcessing::detail::k_IsoContour2DResidentLimit);
  }

  const auto overwide = ImageProcessing::detail::BuildIsoContour2DBufferPlan(100000000, 2, sizeof(uint8));
  REQUIRE(overwide.valid);
  REQUIRE(overwide.coreCols < 100000000);
  REQUIRE(overwide.coreRows == 1);
  REQUIRE(overwide.residentBytes <= ImageProcessing::detail::k_IsoContour2DResidentLimit);

  constexpr usize kSmallLimit = 4995;
  const auto forcedTiled = ImageProcessing::detail::BuildIsoContour2DBufferPlan(100, 8, sizeof(uint8), kSmallLimit);
  REQUIRE(forcedTiled.valid);
  REQUIRE(forcedTiled.coreCols < 100);
  REQUIRE(forcedTiled.coreRows == 1);
  REQUIRE(forcedTiled.residentBytes <= kSmallLimit);

  const auto oneCell = ImageProcessing::detail::BuildIsoContour2DBufferPlan(1, 1, sizeof(uint64));
  REQUIRE(oneCell.valid);
  REQUIRE(oneCell.residentBytes <= ImageProcessing::detail::k_IsoContour2DResidentLimit);
}

TEST_CASE("ImageProcessing::IsoContourDistanceEngine: 2D planner rejects invalid and overflow dimensions", "[ImageProcessing][IsoContourDistanceEngine]")
{
  const auto overflow = ImageProcessing::detail::BuildIsoContour2DBufferPlan(std::numeric_limits<usize>::max(), std::numeric_limits<usize>::max(), sizeof(uint64));
  const auto zeroX = ImageProcessing::detail::BuildIsoContour2DBufferPlan(0, 1, sizeof(uint8));
  const auto zeroY = ImageProcessing::detail::BuildIsoContour2DBufferPlan(1, 0, sizeof(uint8));
  REQUIRE_FALSE(overflow.valid);
  REQUIRE(overflow.overflow);
  REQUIRE_FALSE(zeroX.valid);
  REQUIRE(zeroX.overflow);
  REQUIRE_FALSE(zeroY.valid);
  REQUIRE(zeroY.overflow);
}
