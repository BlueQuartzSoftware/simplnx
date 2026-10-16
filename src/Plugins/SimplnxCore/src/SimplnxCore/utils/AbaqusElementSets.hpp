#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <atomic>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <string>

/**
 * @namespace nx::core::AbaqusElementSets
 * @brief Shared internal grouping and size checks for Abaqus writers.
 */
namespace nx::core::AbaqusElementSets
{
inline constexpr usize k_ChunkSize = 64 * 1024;
inline constexpr int32 k_MaxFeatureId = 100'000'000;
/** @brief Produces an element-set header with a leading and trailing newline. */
using HeaderType = std::function<std::string(int32)>;

/**
 * @brief Finds the maximum Feature ID through bounded reads and checks the table-size limit.
 * @param featureIdsRef Supplies scalar cell Feature IDs.
 * @param shouldCancel Signals cancellation between chunks.
 * @return Maximum ID, read or size error, or zero after cancellation.
 */
SIMPLNXCORE_EXPORT Result<int32> findMaximumGrainId(const Int32AbstractDataStore& featureIdsRef, const std::atomic_bool& shouldCancel);

/**
 * @brief Streams ordered element sets through resident grouping or bounded external sorting.
 * @param progressThrottle Receives grouping progress messages.
 * @param output Receives text through the caller's bounded stream buffer.
 * @param header Produces each grain header with a leading and trailing newline.
 * @param featureIdsRef Supplies scalar cell Feature IDs.
 * @param maxGrainId Largest ID to emit, including gaps.
 * @param useOocAlgorithm Selects external sorting; resident stores can fall back to repeated scans.
 * @param shouldCancel Signals cancellation between bounded batches.
 * @return Source, sorter, or write error; success on completion or cancellation.
 * @pre maxGrainId passed findMaximumGrainId's size check.
 */
SIMPLNXCORE_EXPORT Result<> write(ThrottledMessageHandler& progressThrottle, std::ostream& output, const HeaderType& header, const Int32AbstractDataStore& featureIdsRef, int32 maxGrainId,
                                  bool useOocAlgorithm, const std::atomic_bool& shouldCancel);

/**
 * @brief Checks that the prefix is a nonempty plain file name without whitespace or path separators.
 * @param prefix Output file prefix to validate; neither "." nor ".." is allowed.
 * @param errorCode Error code to return for an invalid prefix.
 * @return Success for a valid prefix, or an error that describes the naming rule.
 */
SIMPLNXCORE_EXPORT Result<> validateFilePrefix(const std::string& prefix, int32 errorCode);

/**
 * @brief Checks that none of the five output file targets is an existing directory.
 * @param outputPath Directory for the Abaqus input files.
 * @param prefix Validated output file prefix.
 * @param errorCode Error code to return for a directory target.
 * @return Success when no target is a directory, or an error that identifies the conflicting path.
 * @pre prefix passed validateFilePrefix.
 */
SIMPLNXCORE_EXPORT Result<> validateOutputTargets(const std::filesystem::path& outputPath, const std::string& prefix, int32 errorCode);

/**
 * @brief Checks cell and node counts without overflowing integer products.
 * @param dimensions Cell dimensions in X, Y, and Z order.
 * @return Error when either count exceeds the Abaqus label limit.
 */
SIMPLNXCORE_EXPORT Result<> validateMeshSize(const SizeVec3& dimensions);
} // namespace nx::core::AbaqusElementSets
