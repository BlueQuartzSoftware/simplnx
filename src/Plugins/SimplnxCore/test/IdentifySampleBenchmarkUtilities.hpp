#pragma once

#include "IdentifySampleBatchTestUtilities.hpp"
#include "SimplnxCore/Filters/Algorithms/IdentifySampleCCL.hpp"
#include "simplnx/Utilities/CacheMemoryBudgetManager.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#ifdef SIMPLNX_ENABLE_MULTICORE
#include <tbb/global_control.h>
#endif
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace identify_sample_benchmark_test
{
using namespace nx::core;
constexpr usize k_Dimension = 200;
constexpr usize k_XyValues = k_Dimension * k_Dimension;
constexpr uint64 k_DefaultBudget = 536870912;
constexpr usize k_RequestedThreads = 8;
using ClockType = std::chrono::steady_clock;
using JsonType = nlohmann::json;

/**
 * @brief Parses a positive uint64 field with at most twenty decimal digits.
 * @param text Complete decimal field.
 * @param name Identifies the field in an error message.
 * @return Parsed value.
 * @throws std::invalid_argument If the field is empty, too long, zero, out of range, or contains another character.
 */
inline uint64 ParsePositiveDecimal(std::string_view text, std::string_view name)
{
  if(text.empty() || text.size() > 20 || !std::all_of(text.begin(), text.end(), [](char value) { return value >= '0' && value <= '9'; }))
  {
    throw std::invalid_argument(std::string(name) + " requires one to twenty decimal digits");
  }
  uint64 value = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if(parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || value == 0)
  {
    throw std::invalid_argument(std::string(name) + " requires a positive uint64 value");
  }
  return value;
}

/**
 * @brief Reads one bounded decimal environment field without changing the environment.
 * @param name Environment-variable name.
 * @param fallback Value used only when the variable is absent.
 * @return Parsed value or the fallback.
 * @throws std::invalid_argument If the present value is not a positive uint64 decimal field.
 */
inline uint64 ReadPositiveEnvironment(const char* name, uint64 fallback)
{
  const char* rawPtr = std::getenv(name);
  if(rawPtr == nullptr)
  {
    return fallback;
  }
  usize length = 0;
  while(length <= 20 && rawPtr[length] != '\0')
  {
    length++;
  }
  if(length > 20)
  {
    throw std::invalid_argument(std::string(name) + " exceeds twenty decimal digits");
  }
  return ParsePositiveDecimal(std::string_view(rawPtr, length), name);
}

inline int64 UtcMilliseconds()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline uint64 ProcessId()
{
#ifdef _WIN32
  return static_cast<uint64>(GetCurrentProcessId());
#else
  return static_cast<uint64>(getpid());
#endif
}

// These are process-wide OS requests. Other files, metadata, and device requests can contribute.
struct ProcessIo
{
  bool available = false;
  uint64 error = 0;
  uint64 read = 0;
  uint64 write = 0;
  uint64 other = 0;
};

inline ProcessIo ReadProcessIo() noexcept
{
  ProcessIo result;
#ifdef _WIN32
  IO_COUNTERS counters{};
  if(GetProcessIoCounters(GetCurrentProcess(), &counters) == FALSE)
  {
    result.error = GetLastError();
    return result;
  }
  result.available = true;
  result.read = counters.ReadTransferCount;
  result.write = counters.WriteTransferCount;
  result.other = counters.OtherTransferCount;
#endif
  return result;
}

inline JsonType ProcessIoDelta(const ProcessIo& before, const ProcessIo& after)
{
  if(!before.available || !after.available)
  {
    return {{"available", false}, {"before_error", before.error}, {"after_error", after.error}, {"scope", "Windows self-process I/O requests; unavailable on other platforms"}};
  }
  REQUIRE(after.read >= before.read);
  REQUIRE(after.write >= before.write);
  REQUIRE(after.other >= before.other);
  return {{"available", true},
          {"read_bytes", after.read - before.read},
          {"write_bytes", after.write - before.write},
          {"other_bytes", after.other - before.other},
          {"scope", "current-process OS requests; includes other files; not mask-only or physical device bytes"}};
}

/**
 * @class BudgetRestore
 * @brief Restores only the current manager budget after test-owned reservations expire.
 */
class BudgetRestore
{
public:
  BudgetRestore()
  : m_Original(CacheMemoryBudgetManager::Instance().budgetBytes())
  {
  }
  ~BudgetRestore()
  {
    CacheMemoryBudgetManager::Instance().setBudgetBytes(m_Original);
  }
  BudgetRestore(const BudgetRestore&) = delete;
  BudgetRestore(BudgetRestore&&) = delete;
  BudgetRestore& operator=(const BudgetRestore&) = delete;
  BudgetRestore& operator=(BudgetRestore&&) = delete;

private:
  uint64 m_Original;
};

inline bool InputValue(usize x, usize y, usize z)
{
  const usize shift = x % 3;
  const bool rectangle = y >= 20 + shift && y <= 179 + shift && z >= 20 && z <= 179;
  return (rectangle && (y != 80 + shift || z != 90)) || (y == 2 && z == 2);
}

inline bool ExpectedValue(usize x, usize y, usize z, bool fillHoles)
{
  const usize shift = x % 3;
  return y >= 20 + shift && y <= 179 + shift && z >= 20 && z <= 179 && (fillHoles || y != 80 + shift || z != 90);
}

template <class T>
DataStructure CreateBenchmarkFixture()
{
  DataStructure data;
  auto* imagePtr = ImageGeom::Create(data, identify_sample_batch_test::k_ImagePath.getTargetName());
  REQUIRE(imagePtr != nullptr);
  imagePtr->setDimensions({k_Dimension, k_Dimension, k_Dimension});
  REQUIRE(imagePtr->setSpacing({1.0F, 1.0F, 1.0F}).valid());
  REQUIRE(imagePtr->setOrigin({0.0F, 0.0F, 0.0F}).valid());
  auto* cellsPtr = AttributeMatrix::Create(data, "CellData", {k_Dimension, k_Dimension, k_Dimension}, imagePtr->getId());
  REQUIRE(cellsPtr != nullptr);
  imagePtr->setCellData(*cellsPtr);
  auto store = data_store_utilities::CreateDataStore<T>(data, identify_sample_batch_test::k_MaskPath, cellsPtr->getShape(), ShapeType{1});
  REQUIRE(store != nullptr);
  auto* maskPtr = DataArray<T>::Create(data, "Mask", store, cellsPtr->getId());
  REQUIRE(maskPtr != nullptr);
  auto xy = std::make_unique<T[]>(k_XyValues);
  for(usize z = 0; z < k_Dimension; z++)
  {
    for(usize y = 0; y < k_Dimension; y++)
    {
      for(usize x = 0; x < k_Dimension; x++)
      {
        xy[y * k_Dimension + x] = static_cast<T>(InputValue(x, y, z));
      }
    }
    const auto result = store->copyFromBuffer(z * k_XyValues, nonstd::span<const T>(xy.get(), k_XyValues));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
  }
  return data;
}

/**
 * @brief Checks every resident output value against the independent large-fixture predicate.
 * @tparam T Bool or UInt8 mask type.
 * @param data Filter output with the original geometry and mask.
 * @param fillHoles Selects the expected filled-hole value.
 */
template <class T>
void RequireResidentOutput(const DataStructure& data, bool fillHoles)
{
  REQUIRE_NOTHROW(data.getDataRefAs<ImageGeom>(identify_sample_batch_test::k_ImagePath));
  REQUIRE_NOTHROW(data.getDataRefAs<DataArray<T>>(identify_sample_batch_test::k_MaskPath));
  const auto& image = data.getDataRefAs<ImageGeom>(identify_sample_batch_test::k_ImagePath);
  const auto& mask = data.getDataRefAs<DataArray<T>>(identify_sample_batch_test::k_MaskPath);
  REQUIRE(image.getDimensions() == SizeVec3Type(k_Dimension, k_Dimension, k_Dimension));
  const ShapeType expectedShape{k_Dimension, k_Dimension, k_Dimension};
  REQUIRE(mask.getTupleShape() == expectedShape);
  REQUIRE(mask.getComponentShape() == ShapeType{1});
  auto xy = std::make_unique<T[]>(k_XyValues);
  uint64 trueCount = 0;
  for(usize z = 0; z < k_Dimension; z++)
  {
    const auto result = mask.getDataStoreRef().copyIntoBuffer(z * k_XyValues, nonstd::span<T>(xy.get(), k_XyValues));
    SIMPLNX_RESULT_REQUIRE_VALID(result);
    for(usize y = 0; y < k_Dimension; y++)
    {
      for(usize x = 0; x < k_Dimension; x++)
      {
        const T expected = static_cast<T>(ExpectedValue(x, y, z, fillHoles));
        if(xy[y * k_Dimension + x] != expected)
        {
          FAIL("Resident value mismatch at " << x << ',' << y << ',' << z);
        }
        trueCount += static_cast<uint64>(xy[y * k_Dimension + x]);
      }
    }
  }
  REQUIRE(trueCount == (fillHoles ? 5120000ULL : 5119800ULL));
  unit_test::CheckArraysInheritTupleDims(data);
}
/**
 * @struct LogicalCounters
 * @brief Accumulates CCL slice observations on the filter execution thread.
 * @note The selected mask identity outlives the callback. No callback runs under storage or HDF5 locks.
 * @note extraAllocatedBytes, extraRequestedBytes, and extraGrantedBytes accumulate across batches.
 * @note minimumBatchWidth remains uint64's maximum until the first batch-width event.
 */
struct LogicalCounters
{
  const IDataArray* selected = nullptr;
  uint64 reads = 0;
  uint64 writes = 0;
  uint64 read_success = 0;
  uint64 write_success = 0;
  uint64 read_values = 0;
  uint64 write_values = 0;
  uint64 maximum_span = 0;
  uint64 active = 0;
  uint64 concurrent = 0;
  uint64 plane_live = 0;
  uint64 xy_live = 0;
  uint64 named_carrier_peak = 0;
  uint64 allocation_events = 0;
  uint64 release_events = 0;
  uint64 reservation_min = std::numeric_limits<uint64>::max();
  uint64 reservation_max = 0;
  bool invalid = false;

  uint64 extra_live_bytes = 0;
  uint64 extra_peak_bytes = 0;
  uint64 extra_allocated_bytes = 0;
  uint64 extra_allocation_count = 0;
  uint64 extra_release_count = 0;
  uint64 extra_request_count = 0;
  uint64 extra_grant_count = 0;
  uint64 extra_retain_count = 0;
  uint64 extra_reservation_release_count = 0;
  uint64 extra_requested_bytes = 0;
  uint64 extra_requested_max_bytes = 0;
  uint64 extra_granted_bytes = 0;
  uint64 extra_granted_max_bytes = 0;
  uint64 extra_retained_bytes = 0;
  uint64 extra_retained_max_bytes = 0;
  uint64 batch_count = 0;
  uint64 batch_planes = 0;
  uint64 minimum_batch_width = std::numeric_limits<uint64>::max();
  uint64 maximum_batch_width = 0;
  bool invalid_extra_accounting = false;

  /**
   * @brief Records transfers and actual buffer lifetimes without allocating memory.
   * @param context Non-null pointer to the live counters for the selected mask.
   * @param mask Identifies the source of the event.
   * @param event Identifies the operation or lifetime boundary.
   * @param values Gives elements, bytes, or planes as defined by the event.
   * @param elementBytes Converts values to bytes; admission and width events use one.
   * @param success Reports transfer completion; lifetime events require true.
   */
  static void Observe(void* context, const IDataArray* mask, IdentifySampleSliceEventForTesting event, usize values, usize elementBytes, bool success) noexcept
  {
    auto& counts = *static_cast<LogicalCounters*>(context);
    if(mask != counts.selected)
    {
      return;
    }
    if(elementBytes == 0 || values > std::numeric_limits<uint64>::max() / elementBytes)
    {
      counts.invalid = true;
      return;
    }
    const uint64 bytes = static_cast<uint64>(values) * elementBytes;
    const uint64 reserved = CacheMemoryBudgetManager::Instance().reservedWorkingMemoryBytes();
    counts.reservation_min = std::min(counts.reservation_min, reserved);
    counts.reservation_max = std::max(counts.reservation_max, reserved);
    switch(event)
    {
    case IdentifySampleSliceEventForTesting::ReadBegin:
    case IdentifySampleSliceEventForTesting::WriteBegin:
      counts.concurrent = std::max(counts.concurrent, ++counts.active);
      counts.maximum_span = std::max(counts.maximum_span, static_cast<uint64>(values));
      if(event == IdentifySampleSliceEventForTesting::ReadBegin)
      {
        counts.reads++;
        counts.read_values += values;
      }
      else
      {
        counts.writes++;
        counts.write_values += values;
      }
      break;
    case IdentifySampleSliceEventForTesting::ReadEnd:
    case IdentifySampleSliceEventForTesting::WriteEnd:
      if(counts.active == 0)
      {
        counts.invalid = true;
        break;
      }
      counts.active--;
      if(event == IdentifySampleSliceEventForTesting::ReadEnd)
      {
        counts.read_success += static_cast<uint64>(success);
      }
      else
      {
        counts.write_success += static_cast<uint64>(success);
      }
      break;
    case IdentifySampleSliceEventForTesting::PlaneAllocated:
    case IdentifySampleSliceEventForTesting::XyAllocated: {
      auto& live = event == IdentifySampleSliceEventForTesting::PlaneAllocated ? counts.plane_live : counts.xy_live;
      counts.invalid = counts.invalid || live != 0;
      live = bytes;
      counts.named_carrier_peak = std::max(counts.named_carrier_peak, counts.plane_live + counts.xy_live);
      counts.allocation_events++;
      break;
    }
    case IdentifySampleSliceEventForTesting::PlaneReleased:
    case IdentifySampleSliceEventForTesting::XyReleased: {
      auto& live = event == IdentifySampleSliceEventForTesting::PlaneReleased ? counts.plane_live : counts.xy_live;
      counts.invalid = counts.invalid || live != bytes;
      live = 0;
      counts.release_events++;
      break;
    }
    case IdentifySampleSliceEventForTesting::ExtraBytesRequested:
      counts.invalid_extra_accounting = counts.invalid_extra_accounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::None || counts.extra_live_bytes != 0;
      counts.m_ExtraPhase = ExtraPhase::Requested;
      counts.m_RequestedBytes = bytes;
      counts.m_GrantedBytes = 0;
      counts.m_ExtraOwnerObserved = false;
      counts.m_BatchWidthObserved = false;
      counts.extra_request_count++;
      counts.addExtraTotal(counts.extra_requested_bytes, bytes);
      counts.extra_requested_max_bytes = std::max(counts.extra_requested_max_bytes, bytes);
      break;
    case IdentifySampleSliceEventForTesting::ExtraBytesGranted:
      counts.invalid_extra_accounting = counts.invalid_extra_accounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::Requested || bytes > counts.m_RequestedBytes;
      counts.m_ExtraPhase = ExtraPhase::Granted;
      counts.m_GrantedBytes = bytes;
      counts.extra_grant_count++;
      counts.addExtraTotal(counts.extra_granted_bytes, bytes);
      counts.extra_granted_max_bytes = std::max(counts.extra_granted_max_bytes, bytes);
      break;
    case IdentifySampleSliceEventForTesting::ExtraBytesRetained:
      counts.invalid_extra_accounting = counts.invalid_extra_accounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::Granted || bytes > counts.m_GrantedBytes;
      counts.invalid_extra_accounting = counts.invalid_extra_accounting || (counts.plane_live == 0 ? bytes != 0 : bytes % counts.plane_live != 0);
      counts.m_ExtraPhase = ExtraPhase::Retained;
      counts.extra_retained_bytes = bytes;
      counts.extra_retain_count++;
      counts.extra_retained_max_bytes = std::max(counts.extra_retained_max_bytes, bytes);
      break;
    case IdentifySampleSliceEventForTesting::ExtraPlanesAllocated:
      counts.invalid_extra_accounting = counts.invalid_extra_accounting || !success || counts.m_ExtraPhase != ExtraPhase::Retained || counts.m_ExtraOwnerObserved || counts.m_BatchWidthObserved ||
                                        counts.extra_live_bytes != 0 || bytes == 0 || bytes != counts.extra_retained_bytes;
      counts.m_ExtraOwnerObserved = true;
      counts.extra_live_bytes = bytes;
      counts.extra_peak_bytes = std::max(counts.extra_peak_bytes, bytes);
      counts.addExtraTotal(counts.extra_allocated_bytes, bytes);
      counts.extra_allocation_count++;
      break;
    case IdentifySampleSliceEventForTesting::ExtraPlanesReleased:
      counts.invalid_extra_accounting = counts.invalid_extra_accounting || !success || counts.m_ExtraPhase != ExtraPhase::Retained || !counts.m_ExtraOwnerObserved || bytes == 0 ||
                                        counts.extra_live_bytes != bytes || bytes > counts.extra_retained_bytes;
      counts.extra_live_bytes = 0;
      counts.extra_release_count++;
      break;
    case IdentifySampleSliceEventForTesting::BatchWidth: {
      const uint64 expectedWidth = counts.plane_live == 0 ? 1 : 1 + counts.extra_retained_bytes / counts.plane_live;
      counts.invalid_extra_accounting = counts.invalid_extra_accounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::Retained || counts.m_BatchWidthObserved || values < 1 ||
                                        values > 8 || values != expectedWidth || counts.extra_live_bytes != counts.extra_retained_bytes;
      counts.m_BatchWidthObserved = true;
      counts.batch_count++;
      counts.addExtraTotal(counts.batch_planes, values);
      counts.minimum_batch_width = std::min(counts.minimum_batch_width, static_cast<uint64>(values));
      counts.maximum_batch_width = std::max(counts.maximum_batch_width, static_cast<uint64>(values));
      break;
    }
    case IdentifySampleSliceEventForTesting::ExtraReservationReleased:
      // The token returns its grant only after the extra array releases its values.
      counts.invalid_extra_accounting =
          counts.invalid_extra_accounting || elementBytes != 1 || !success || counts.m_ExtraPhase != ExtraPhase::Retained || counts.extra_live_bytes != 0 || bytes != counts.extra_retained_bytes;
      counts.extra_retained_bytes = 0;
      counts.m_ExtraPhase = ExtraPhase::None;
      counts.extra_reservation_release_count++;
      break;
    }
  }

  /**
   * @brief Reports observed counts and peaks with separate extra-plane ownership fields.
   * @return Measurements for the selected mask, excluding total process memory.
   */
  JsonType describe() const
  {
    return {{"read_calls", reads},
            {"write_calls", writes},
            {"successful_read_calls", read_success},
            {"successful_write_calls", write_success},
            {"read_values", read_values},
            {"write_values", write_values},
            {"maximum_span_values", maximum_span},
            {"outer_concurrency", concurrent},
            {"original_plane_and_xy_requested_peak_bytes", named_carrier_peak},
            {"carrier_allocation_events", allocation_events},
            {"carrier_release_events", release_events},
            {"working_reservation_min_bytes", reservation_min},
            {"working_reservation_max_bytes", reservation_max},
            {"extra_plane_allocation_observation", "actual extra-array ownership; excludes original carriers, CCL records, and backend memory"},
            {"extra_plane_live_bytes", extra_live_bytes},
            {"extra_plane_peak_bytes", extra_peak_bytes},
            {"extra_plane_allocated_bytes", extra_allocated_bytes},
            {"extra_plane_allocation_events", extra_allocation_count},
            {"extra_plane_release_events", extra_release_count},
            {"extra_request_events", extra_request_count},
            {"extra_grant_events", extra_grant_count},
            {"extra_retain_events", extra_retain_count},
            {"extra_reservation_release_events", extra_reservation_release_count},
            {"extra_requested_bytes", extra_requested_bytes},
            {"extra_requested_max_bytes", extra_requested_max_bytes},
            {"extra_granted_bytes", extra_granted_bytes},
            {"extra_granted_max_bytes", extra_granted_max_bytes},
            {"extra_retained_live_bytes", extra_retained_bytes},
            {"extra_retained_max_bytes", extra_retained_max_bytes},
            {"batch_count", batch_count},
            {"batch_planes", batch_planes},
            {"minimum_batch_width", batch_count == 0 ? 0 : minimum_batch_width},
            {"maximum_batch_width", maximum_batch_width},
            {"extra_accounting_invalid", invalid_extra_accounting}};
  }

private:
  /**
   * @enum ExtraPhase
   * @brief Tracks admission order separately from the manager's global reservation total.
   */
  enum class ExtraPhase
  {
    None,      ///< No admission is active.
    Requested, ///< The next event must report the actual grant.
    Granted,   ///< The next event must report the whole-plane reservation.
    Retained   ///< The token remains active until all extra values are released.
  };
  ExtraPhase m_ExtraPhase = ExtraPhase::None;
  uint64 m_RequestedBytes = 0;
  uint64 m_GrantedBytes = 0;
  bool m_ExtraOwnerObserved = false;
  bool m_BatchWidthObserved = false;

  /**
   * @brief Adds an observation without allowing a wrapped total to appear valid.
   * @param total Accumulates the observed quantity.
   * @param amount Gives the increment in the same units as total.
   */
  void addExtraTotal(uint64& total, uint64 amount) noexcept
  {
    if(amount > std::numeric_limits<uint64>::max() - total)
    {
      invalid_extra_accounting = true;
      return;
    }
    total += amount;
  }
};

/**
 * @brief Checks actual batch widths and extra owners for the 200-cube fixture.
 * @tparam T Bool or UInt8 mask element type.
 * @param counts Contains observations from one successful CCL execution.
 */
template <class T>
void RequireFullWidthYzBatchObservations(const LogicalCounters& counts)
{
  constexpr uint64 k_ExpectedBatches = k_Dimension / 8;
  constexpr uint64 k_ExpectedExtraBytes = 7 * k_XyValues * sizeof(T);
  static_assert(k_Dimension % 8 == 0);
  REQUIRE_FALSE(counts.invalid_extra_accounting);
  REQUIRE(counts.extra_live_bytes == 0);
  REQUIRE(counts.extra_retained_bytes == 0);
  REQUIRE(counts.extra_allocation_count == k_ExpectedBatches);
  REQUIRE(counts.extra_release_count == k_ExpectedBatches);
  REQUIRE(counts.extra_request_count == k_ExpectedBatches);
  REQUIRE(counts.extra_grant_count == k_ExpectedBatches);
  REQUIRE(counts.extra_retain_count == k_ExpectedBatches);
  REQUIRE(counts.extra_reservation_release_count == k_ExpectedBatches);
  REQUIRE(counts.extra_allocated_bytes == k_ExpectedBatches * k_ExpectedExtraBytes);
  REQUIRE(counts.extra_peak_bytes == k_ExpectedExtraBytes);
  REQUIRE(counts.extra_requested_bytes == k_ExpectedBatches * k_ExpectedExtraBytes);
  REQUIRE(counts.extra_requested_max_bytes == k_ExpectedExtraBytes);
  REQUIRE(counts.extra_granted_bytes == k_ExpectedBatches * k_ExpectedExtraBytes);
  REQUIRE(counts.extra_granted_max_bytes == k_ExpectedExtraBytes);
  REQUIRE(counts.extra_retained_max_bytes == k_ExpectedExtraBytes);
  REQUIRE(counts.extra_peak_bytes <= counts.extra_retained_max_bytes);
  REQUIRE(counts.extra_retained_max_bytes <= counts.extra_granted_max_bytes);
  REQUIRE(counts.batch_count == k_ExpectedBatches);
  REQUIRE(counts.batch_planes == k_Dimension);
  REQUIRE(counts.minimum_batch_width == 8);
  REQUIRE(counts.maximum_batch_width == 8);
}

} // namespace identify_sample_benchmark_test
