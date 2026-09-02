#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <atomic>
#include <vector>

namespace nx::core::FeatureRemovalUtilities
{
/**
 * @brief Inputs describing where the feature data lives and how the removal should behave.
 */
struct SIMPLNXCORE_EXPORT RemovalArgs
{
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  std::vector<DataPath> IgnoredDataArrayPaths;
  bool FillRemovedFeatures = false;
};

/**
 * @brief Removes the flagged features from a segmented Image Geometry.
 *
 * Zeroes the Cell level FeatureIds for every flagged feature, optionally fills the resulting gaps by
 * iteratively dilating the surviving neighbors, then compacts the feature Attribute Matrix and
 * renumbers the surviving features contiguously starting at 1.
 *
 * @param dataStructure The DataStructure to modify in place.
 * @param flaggedFeatures true means flagged for removal. Index 0 is ignored; feature 0 is not a feature.
 * @param args Paths and options describing the removal.
 * @param messageHandler Receives progress messages.
 * @param shouldCancel Polled to abort the long running loops.
 * @return Valid on success, with warning -45438 when the Feature Ids array was listed in
 * IgnoredDataArrayPaths (it is always copied because it is the array being filled). Invalid with:
 * - -45433 when every feature is flagged, or the flag vector has fewer than two entries. Nothing is modified.
 * - -45434 when the feature group could not be compacted.
 * - -45435 when a FeatureId is negative or not less than the flag count. Nothing is modified.
 * - -45436 when fill is enabled and a pass fills no remaining vacated cell. Removed cells are left at -1.
 * - -45437 when the Feature Ids tuple count differs from the geometry cell count. Nothing is modified.
 */
SIMPLNXCORE_EXPORT Result<> removeFlaggedFeatures(DataStructure& dataStructure, const std::vector<bool>& flaggedFeatures, const RemovalArgs& args, const IFilter::MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel);
} // namespace nx::core::FeatureRemovalUtilities
