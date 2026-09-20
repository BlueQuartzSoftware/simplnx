#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <atomic>
#include <optional>
#include <vector>

namespace nx::core
{
class DataStructure;

/**
 * @brief Iteratively replaces negative Feature IDs from nonnegative face neighbors.
 *
 * The most common neighboring Feature ID supplies the complete source tuple.
 * Ties keep the first direction in -Z, -Y, -X, +X, +Y, +Z order. Each
 * iteration fills one boundary layer.
 *
 * Arrays process sequentially with three Feature ID slices and three data
 * slices. Feature IDs update last, so all arrays use one assignment snapshot.
 * Scratch scales with XY slice size and one array's component count.
 *
 * @param dataStructure DataStructure containing the cell arrays.
 * @param featureIdsPath Path to the int32 cell FeatureIds array.
 * @param dimensions XYZ dimensions of the owning ImageGeom.
 * @param ignoredArrayPaths Cell arrays that must not be transferred.
 * @param maxFeatureCount Optional exclusive upper bound for nonnegative Feature IDs.
 * @param messageHandler Receives validation and unfillable-region messages.
 * @param shouldCancel Signals cancellation between slices and arrays.
 * @return Success, or a Feature ID, unfillable-region, or bulk-transfer error.
 * @pre dimensions match Feature ID and selected sibling tuple counts.
 * @pre Slice-size and component products fit usize.
 *
 * Cancellation returns success. If it occurs between sibling arrays, some
 * arrays can contain the new boundary layer while Feature IDs retain the prior
 * iteration. Callers must discard output after cancellation.
 */
SIMPLNXCORE_EXPORT Result<> FillBadVoxels(DataStructure& dataStructure, const DataPath& featureIdsPath, const SizeVec3& dimensions, const std::vector<DataPath>& ignoredArrayPaths,
                                          std::optional<usize> maxFeatureCount, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel);

} // namespace nx::core
