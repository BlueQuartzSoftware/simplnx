#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

namespace nx::core
{
/**
 * @struct SilhouetteInputValues
 * @brief Stores metric, mask, cluster, input, and output selections.
 */
struct SIMPLNXCORE_EXPORT SilhouetteInputValues
{
  ClusterUtilities::DistanceMetric DistanceMetric;
  bool UseMask = false;
  DataPath ClusteringArrayPath;
  DataPath MaskArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath SilhouetteArrayPath;
};

/**
 * @class Silhouette
 * @brief Dispatches per-tuple silhouette scores by participating storage.
 *
 * For each enabled tuple, a is its mean distance to the complete enabled own
 * cluster, including its zero self-distance. b is the least mean distance to a
 * different positive cluster. The score is (b - a) divided by max(a, b).
 * Disabled tuples receive zero. The code does not guard a zero or nonfinite
 * denominator, so a score can be NaN or infinite.
 *
 * Resident execution retains a tuple-by-cluster distance table. Scanline
 * execution rereads bounded tuple tiles and retains only one outer tile's
 * feature accumulators. Both paths perform an exact all-pairs calculation with
 * O(N squared) distance work. Storage test overrides can force either path.
 */
class SIMPLNXCORE_EXPORT Silhouette
{
public:
  /**
   * @brief Initializes the silhouette dispatcher.
   * @param dataStructure Contains input, cluster, mask, and output arrays.
   * @param mesgHandler Preserves the common algorithm constructor signature.
   * @param shouldCancel Signals scanline cancellation between tiles.
   * @param inputValues Selects metric and array paths.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  Silhouette(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SilhouetteInputValues* inputValues);
  /**
   * @brief Destroys the silhouette dispatcher.
   */
  ~Silhouette() noexcept;

  Silhouette(const Silhouette&) = delete;
  Silhouette(Silhouette&&) noexcept = delete;
  Silhouette& operator=(const Silhouette&) = delete;
  Silhouette& operator=(Silhouette&&) noexcept = delete;

  /**
   * @brief Computes silhouette scores with the selected storage path.
   * @return Mask, Feature ID, size, or bulk-I/O result.
   * @pre Input, Feature ID, optional mask, and output arrays have equal tuple counts.
   * @pre Feature IDs are nonnegative.
   *
   * The direct path ignores cancellation. Scanline cancellation returns success
   * and can retain output tiles written before the checkpoint.
   */
  Result<> operator()();

  /**
   * @brief Sends an information message for compatible worker interfaces.
   * @param message Message to send.
   *
   * The current direct and scanline implementations do not call this method.
   */
  void updateProgress(const std::string& message);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SilhouetteInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
