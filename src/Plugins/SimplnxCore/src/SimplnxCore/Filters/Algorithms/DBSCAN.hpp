#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

#include <random>

namespace nx::core
{
/**
 * @struct DBSCANInputValues
 * @brief Collects grid-based DBSCAN settings and DataStructure paths.
 */
struct SIMPLNXCORE_EXPORT DBSCANInputValues
{
  DataPath ClusteringArrayPath;
  DataPath MaskArrayPath;
  bool UseMask = false;
  DataPath FeatureIdsArrayPath;
  float32 Epsilon;
  int32 MinPoints;
  ClusterUtilities::DistanceMetric DistanceMetric;
  DataPath FeatureAM;
  ChoicesParameter::ValueType ParseOrder;
  std::mt19937_64::result_type Seed;
};

/**
 * @class DBSCAN
 * @brief Dispatches grid-based DBSCAN by array storage type.
 *
 * This modified GDCF algorithm follows Boonchoo et al. 2019. It bins 2D or 3D
 * points into cells with side length epsilon divided by the square root of the
 * component count. Cells with at least MinPoints seed clusters. Neighbor cells
 * merge when one cross-cell point pair is closer than epsilon.
 *
 * The direct path retains point and grid state in RAM. The OOC scanline path
 * stores point and occupied-grid records in temporary files. Fixed caches and
 * tiles bound its RAM use. Storage overrides can force either path.
 */
class SIMPLNXCORE_EXPORT DBSCAN
{
public:
  /**
   * @brief Initializes the DBSCAN dispatcher.
   * @param dataStructure Contains input and output arrays.
   * @param mesgHandler Receives phase messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects settings and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  DBSCAN(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, DBSCANInputValues* inputValues);
  ~DBSCAN() noexcept;

  DBSCAN(const DBSCAN&) = delete;
  DBSCAN(DBSCAN&&) noexcept = delete;
  DBSCAN& operator=(const DBSCAN&) = delete;
  DBSCAN& operator=(DBSCAN&&) noexcept = delete;

  /**
   * @enum ParseOrder
   * @brief Controls core-grid traversal and border-grid attachment.
   *
   * Both random modes shuffle with DBSCANInputValues::Seed. The caller supplies
   * any policy difference between those modes.
   */
  enum ParseOrder
  {
    LowDensityFirst, // Uses deterministic ascending grid population.
    Random,          // Shuffles with the supplied seed.
    SeededRandom     // Shuffles with the supplied seed.
  };

  /**
   * @brief Executes the selected DBSCAN implementation.
   * @return Success or a no-cluster warning, or a validation, mask, storage, or record-I/O error.
   * @pre Epsilon and MinPoints are positive.
   * @pre The coordinate array has two or three components.
   *
   * Cancellation returns success. Completed Feature ID writes remain, but the
   * feature AttributeMatrix is not resized after an observed cancellation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const DBSCANInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
