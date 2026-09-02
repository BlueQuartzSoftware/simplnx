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
 * @return Invalid (-45435) if a FeatureId is negative or not less than the flag count, before anything
 * is modified; invalid (-45433) if every feature was flagged; invalid (-45436) if fill is enabled and a
 * pass cannot fill any remaining vacated cell; invalid (-45434) if the feature group could not be compacted.
 */
SIMPLNXCORE_EXPORT Result<> removeFlaggedFeatures(DataStructure& dataStructure, const std::vector<bool>& flaggedFeatures, const RemovalArgs& args, const IFilter::MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel);
} // namespace nx::core::FeatureRemovalUtilities
