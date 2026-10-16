#include "AbaqusElementSets.hpp"

#include "simplnx/DataStructure/IO/Generic/IExternalSort.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <limits>
#include <memory>
#include <ostream>
#include <string_view>
#include <type_traits>

namespace nx::core::AbaqusElementSets
{
Result<int32> findMaximumGrainId(const Int32AbstractDataStore& featureIdsRef, const std::atomic_bool& shouldCancel)
{
  const usize totalElements = featureIdsRef.getSize();
  if(totalElements == 0)
  {
    return {int32{0}};
  }

  auto buffer = std::make_unique<int32[]>(std::min(k_ChunkSize, totalElements));
  int32 maximum = std::numeric_limits<int32>::lowest();
  for(usize offset = 0; offset < totalElements; offset += k_ChunkSize)
  {
    if(shouldCancel)
    {
      return {int32{0}};
    }
    const usize count = std::min(k_ChunkSize, totalElements - offset);
    if(Result<> result = featureIdsRef.copyIntoBuffer(offset, nonstd::span<int32>(buffer.get(), count)); result.invalid())
    {
      return ConvertResultTo<int32>(std::move(result), int32{});
    }
    maximum = std::max(maximum, *std::max_element(buffer.get(), buffer.get() + count));
  }
  // Preflight cannot read array contents, so this limit is checked during execution.
  if(maximum > k_MaxFeatureId)
  {
    return MakeErrorResult<int32>(-1120, fmt::format("Maximum feature ID {} exceeds the limit {}. Per-grain tables cost tens of bytes per ID through the maximum, and the deck writes one element set, "
                                                     "section and material per ID. Renumber the features to be contiguous.",
                                                     maximum, k_MaxFeatureId));
  }
  return {maximum};
}

namespace
{
constexpr std::array<std::string_view, 5> k_FileSuffixes = {"_nodes.inp", "_elems.inp", "_sects.inp", "_elset.inp", ".inp"};

/**
 * @brief Groups resident element indexes by positive grain ID.
 * @param progressThrottle Receives progress messages.
 * @param featureIdsRef Supplies cell Feature IDs.
 * @param maxGrainId Largest Feature ID and final bucket index.
 * @param shouldCancel Signals cancellation between chunks.
 * @return One ascending element-index list per ID, or empty after cancellation.
 *
 * Abaqus requires each grain's element IDs in one contiguous ELSET. One source
 * pass avoids a grain-by-cell rescan. The tradeoff is O(maximum ID plus positive
 * cell count) resident bucket memory.
 */
Result<std::vector<std::vector<usize>>> groupElementsByGrain(ThrottledMessageHandler& progressThrottle, const Int32AbstractDataStore& featureIdsRef, int32 maxGrainId,
                                                             const std::atomic_bool& shouldCancel)
{
  const usize elsetCount = maxGrainId > 0 ? static_cast<usize>(maxGrainId) + 1 : 0;
  std::vector<std::vector<usize>> elementsByGrain(elsetCount);
  if(elsetCount == 0)
  {
    return {std::move(elementsByGrain)};
  }

  const usize totalElements = featureIdsRef.getSize();
  auto chunkBuffer = std::make_unique<int32[]>(k_ChunkSize);

  progressThrottle.reset(totalElements, "Grouping Element Sets");
  for(usize offset = 0; offset < totalElements; offset += k_ChunkSize)
  {
    if(shouldCancel)
    {
      return {std::vector<std::vector<usize>>{}};
    }

    const usize count = std::min(k_ChunkSize, totalElements - offset);
    if(Result<> result = featureIdsRef.copyIntoBuffer(offset, nonstd::span<int32>(chunkBuffer.get(), count)); result.invalid())
    {
      return ConvertResultTo<std::vector<std::vector<usize>>>(std::move(result), {});
    }
    for(usize i = 0; i < count; i++)
    {
      const int32 grainId = chunkBuffer[i];
      if(grainId > 0 && grainId <= maxGrainId)
      {
        elementsByGrain[static_cast<usize>(grainId)].push_back(offset + i);
      }
    }

    progressThrottle.updatePercent(offset + count);
  }

  return {std::move(elementsByGrain)};
}

/**
 * @brief Appends one element ID with 16 values per line.
 * @param output Receives bounded element-set text.
 * @param elementId One-based Abaqus element ID.
 * @param elementsOnSet Receives the current grain's written count.
 */
void writeElementId(std::ostream& output, uint64 elementId, usize& elementsOnSet)
{
  if(elementsOnSet != 0)
  {
    output << ((elementsOnSet % 16) != 0u ? ", " : ",\n");
  }
  output << fmt::format("{}", elementId);
  elementsOnSet++;
}

/**
 * @brief Writes resident grain ELSETs from one grouped source pass.
 * @param progressThrottle Receives progress messages.
 * @param output Receives bounded element-set text.
 * @param header Produces the header, including its leading newline.
 * @param featureIdsRef Supplies cell Feature IDs.
 * @param maxGrainId Largest grain ID to emit.
 * @param shouldCancel Signals cancellation between grouping chunks.
 * @return Grouping source-read result, or success after cancellation.
 */
Result<> writeDirectElsets(ThrottledMessageHandler& progressThrottle, std::ostream& output, const HeaderType& header, const Int32AbstractDataStore& featureIdsRef, int32 maxGrainId,
                           const std::atomic_bool& shouldCancel)
{
  auto groupedResult = groupElementsByGrain(progressThrottle, featureIdsRef, maxGrainId, shouldCancel);
  if(groupedResult.invalid())
  {
    return ConvertResult(std::move(groupedResult));
  }
  if(shouldCancel)
  {
    return {};
  }
  const auto& elementsByGrain = groupedResult.value();
  for(int32 grainId = 1; grainId <= maxGrainId; ++grainId)
  {
    if(shouldCancel || !output)
    {
      return {};
    }
    output << header(grainId);
    usize elementsOnSet = 0;
    for(usize elementId : elementsByGrain[static_cast<usize>(grainId)])
    {
      if(elementsOnSet % k_ChunkSize == 0 && (shouldCancel || !output))
      {
        return {};
      }
      writeElementId(output, elementId + 1, elementsOnSet);
    }
  }
  return {};
}

/**
 * @brief Writes ELSETs by rescanning Feature IDs for every grain.
 * @param output Receives bounded element-set text.
 * @param header Produces the header, including its leading newline.
 * @param featureIdsRef Supplies cell Feature IDs.
 * @param maxGrainId Largest grain ID to emit.
 * @param shouldCancel Signals cancellation between read chunks.
 * @return Source bulk-read result, or success after cancellation.
 *
 * This exact bounded fallback is for forced OOC tests on resident data when no
 * sorter exists. Actual OOC input rejects this O(grains times cells) I/O path.
 */
Result<> writeRepeatedScanElsets(std::ostream& output, const HeaderType& header, const Int32AbstractDataStore& featureIdsRef, int32 maxGrainId, const std::atomic_bool& shouldCancel)
{
  const usize totalElements = featureIdsRef.getSize();
  auto featurePage = std::make_unique<int32[]>(std::min(k_ChunkSize, totalElements));
  for(int32 grainId = 1; grainId <= maxGrainId; ++grainId)
  {
    if(shouldCancel || !output)
    {
      return {};
    }
    output << header(grainId);
    usize elementsOnSet = 0;
    for(usize offset = 0; offset < totalElements; offset += k_ChunkSize)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize count = std::min(k_ChunkSize, totalElements - offset);
      if(Result<> result = featureIdsRef.copyIntoBuffer(offset, nonstd::span<int32>(featurePage.get(), count)); result.invalid())
      {
        return result;
      }
      for(usize i = 0; i < count; ++i)
      {
        if(featurePage[i] == grainId)
        {
          writeElementId(output, offset + i + 1, elementsOnSet);
        }
      }
    }
  }
  return {};
}

/**
 * @struct ElementGrainRecord
 * @brief Stores one grain and one-based element ID for external sorting.
 */
struct ElementGrainRecord
{
  int32 grainId = 0;
  uint32 reserved = 0;
  uint64 elementId = 0;
};
static_assert(std::is_trivially_copyable_v<ElementGrainRecord>);

/**
 * @brief Writes grain ELSETs through bounded external sorting.
 * @param output Receives bounded element-set text.
 * @param header Produces the header, including its leading newline.
 * @param featureIdsRef Supplies cell Feature IDs.
 * @param maxGrainId Largest grain ID to emit.
 * @param shouldCancel Signals cancellation during source and sort operations.
 * @return Source-I/O, sorter, short-read, or cancellation result.
 *
 * Records sort by grain and one-based element ID. This produces contiguous
 * ascending ELSET runs without cell-scale resident buckets.
 */
Result<> writeExternalSortedElsets(std::ostream& output, const HeaderType& header, const Int32AbstractDataStore& featureIdsRef, int32 maxGrainId, const std::atomic_bool& shouldCancel)
{
  ExternalSortConfig config;
  config.recordSize = sizeof(ElementGrainRecord);
  config.maxRecordsPerBatch = k_ChunkSize;
  config.compare = [](nonstd::span<const std::byte> leftBytes, nonstd::span<const std::byte> rightBytes) {
    ElementGrainRecord left;
    ElementGrainRecord right;
    std::memcpy(&left, leftBytes.data(), sizeof(left));
    std::memcpy(&right, rightBytes.data(), sizeof(right));
    if(left.grainId != right.grainId)
    {
      return left.grainId < right.grainId ? int32{-1} : int32{1};
    }
    if(left.elementId == right.elementId)
    {
      return int32{0};
    }
    return left.elementId < right.elementId ? int32{-1} : int32{1};
  };

  auto sortResult = DataStoreUtilities::GetIOCollection().createExternalSort(config);
  if(sortResult.invalid())
  {
    return ConvertResult(std::move(sortResult));
  }
  std::unique_ptr<IExternalSort> externalSort = std::move(sortResult.value());
  if(externalSort == nullptr)
  {
    return MakeErrorResult(-1119, "The external-sort provider returned a null sorter for Abaqus element sets.");
  }

  const usize totalElements = featureIdsRef.getSize();
  auto featurePage = std::make_unique<int32[]>(std::min(k_ChunkSize, totalElements));
  std::vector<ElementGrainRecord> records;
  records.reserve(std::min(k_ChunkSize, totalElements));
  for(usize offset = 0; offset < totalElements; offset += k_ChunkSize)
  {
    if(shouldCancel)
    {
      return {};
    }
    const usize count = std::min(k_ChunkSize, totalElements - offset);
    if(Result<> result = featureIdsRef.copyIntoBuffer(offset, nonstd::span<int32>(featurePage.get(), count)); result.invalid())
    {
      return result;
    }
    records.clear();
    for(usize i = 0; i < count; ++i)
    {
      if(featurePage[i] > 0 && featurePage[i] <= maxGrainId)
      {
        records.push_back({featurePage[i], 0, offset + i + 1});
      }
    }
    if(records.empty())
    {
      continue;
    }
    auto bytes = nonstd::span<const std::byte>(reinterpret_cast<const std::byte*>(records.data()), records.size() * sizeof(ElementGrainRecord));
    if(Result<> result = externalSort->append(records.size(), bytes, shouldCancel, {}); result.invalid())
    {
      return result;
    }
  }
  if(Result<> result = externalSort->finish(shouldCancel, {}); result.invalid())
  {
    return result;
  }

  int32 currentGrain = 1;
  usize elementsOnSet = 0;
  if(maxGrainId > 0)
  {
    output << header(currentGrain);
  }
  std::vector<ElementGrainRecord> sortedPage(k_ChunkSize);
  for(uint64 offset = 0; offset < externalSort->recordCount(); offset += k_ChunkSize)
  {
    if(shouldCancel || !output)
    {
      return {};
    }
    const uint64 count = std::min<uint64>(k_ChunkSize, externalSort->recordCount() - offset);
    auto bytes = nonstd::span<std::byte>(reinterpret_cast<std::byte*>(sortedPage.data()), static_cast<usize>(count) * sizeof(ElementGrainRecord));
    Result<uint64> readResult = externalSort->read(offset, count, bytes, shouldCancel);
    if(readResult.invalid())
    {
      return ConvertResult(std::move(readResult));
    }
    if(readResult.value() != count)
    {
      return MakeErrorResult(-1117, "Abaqus element-set external sort returned a short read.");
    }
    for(uint64 i = 0; i < count; ++i)
    {
      const ElementGrainRecord& record = sortedPage[static_cast<usize>(i)];
      while(currentGrain < record.grainId)
      {
        if(shouldCancel || !output)
        {
          return {};
        }
        currentGrain++;
        elementsOnSet = 0;
        output << header(currentGrain);
      }
      writeElementId(output, record.elementId, elementsOnSet);
    }
  }
  while(currentGrain < maxGrainId)
  {
    if(shouldCancel || !output)
    {
      return {};
    }
    currentGrain++;
    output << header(currentGrain);
  }
  return {};
}

} // namespace

Result<> write(ThrottledMessageHandler& progressThrottle, std::ostream& output, const HeaderType& header, const Int32AbstractDataStore& featureIdsRef, int32 maxGrainId, bool useOocAlgorithm,
               const std::atomic_bool& shouldCancel)
{
  Result<> result;
  if(!useOocAlgorithm)
  {
    result = writeDirectElsets(progressThrottle, output, header, featureIdsRef, maxGrainId, shouldCancel);
  }
  else if(DataStoreUtilities::GetIOCollection().hasExternalSortCapability())
  {
    result = writeExternalSortedElsets(output, header, featureIdsRef, maxGrainId, shouldCancel);
  }
  else if(featureIdsRef.getStoreType() != IDataStore::StoreType::OutOfCore)
  {
    result = writeRepeatedScanElsets(output, header, featureIdsRef, maxGrainId, shouldCancel);
  }
  else
  {
    result = MakeErrorResult(-1118, "The OOC Abaqus element-set algorithm requires a registered bounded external-sort provider.");
  }
  if(!output)
  {
    return MakeErrorResult(-12013, "Writing Abaqus element sets failed. Check available disk space.");
  }
  return result;
}

Result<> validateFilePrefix(const std::string& prefix, int32 errorCode)
{
  if(prefix.empty() || prefix == "." || prefix == ".." || prefix.find_first_of("/\\") != std::string::npos || std::filesystem::path(prefix).filename().string() != prefix ||
     std::any_of(prefix.begin(), prefix.end(), [](unsigned char character) { return std::isspace(character) != 0; }))
  {
    return MakeErrorResult(
        errorCode,
        fmt::format("The output file prefix '{}' must be a nonempty plain file name without spaces, other whitespace, or path separators, and cannot be '.' or '..'. Choose a valid prefix.", prefix));
  }
  return {};
}

Result<> validateOutputTargets(const std::filesystem::path& outputPath, const std::string& prefix, int32 errorCode)
{
  for(const auto suffix : k_FileSuffixes)
  {
    const std::filesystem::path targetPath = outputPath / fmt::format("{}{}", prefix, suffix);
    if(std::filesystem::exists(targetPath) && std::filesystem::is_directory(targetPath))
    {
      return MakeErrorResult(errorCode,
                             fmt::format("The output file target '{}' is an existing directory. Choose a different output path or file prefix, or remove the directory.", targetPath.string()));
    }
  }
  return {};
}

Result<> validateMeshSize(const SizeVec3& dimensions)
{
  constexpr uint64 k_MaxLabel = 999'999'999;
  for(const bool nodes : {false, true})
  {
    uint64 count = 1;
    for(usize dimension : dimensions)
    {
      if(dimension > k_MaxLabel)
      {
        count = k_MaxLabel + 1;
        break;
      }
      const uint64 factor = dimension + (nodes ? 1 : 0);
      if(factor != 0 && count > k_MaxLabel / factor)
      {
        count = k_MaxLabel + 1;
        break;
      }
      // uint64 cannot overflow: count <= 999,999,999 and factor <= 1,000,000,000 before multiplication.
      // A zero element dimension makes the count zero; node factors are always positive.
      count *= factor;
    }
    if(count > k_MaxLabel)
    {
      return MakeErrorResult(-1121, fmt::format("Abaqus {} count exceeds the node/element label limit {} for cell dimensions ({}, {}, {}).", nodes ? "node" : "element", k_MaxLabel, dimensions[0],
                                                dimensions[1], dimensions[2]));
    }
  }
  return {};
}
} // namespace nx::core::AbaqusElementSets
