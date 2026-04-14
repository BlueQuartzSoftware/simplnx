#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @brief Input values for the ComputeAvgCAxes algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeAvgCAxesInputValues
{
  DataPath QuatsArrayPath;             ///< Cell-level Float32 quaternions (4 components)
  DataPath FeatureIdsArrayPath;        ///< Cell-level Int32 feature ID per voxel
  DataPath CellPhasesArrayPath;        ///< Cell-level Int32 phase index per voxel
  DataPath CellFeatureDataPath;        ///< Feature-level AttributeMatrix path
  DataPath AvgCAxesArrayPath;          ///< Output: Feature-level Float32 average c-axis (3 components)
  DataPath CrystalStructuresArrayPath; ///< Ensemble-level UInt32 crystal structure Laue classes
};

/**
 * @class ComputeAvgCAxes
 * @brief Computes the average crystallographic c-axis direction for each Feature
 *        in the sample reference frame.
 *
 * For each voxel belonging to a Feature, the quaternion is converted to an
 * orientation matrix, transposed (passive to active), and multiplied by the
 * <001> c-axis direction to obtain the c-axis in the sample frame. A running
 * average is maintained per Feature with sign flipping to keep the accumulated
 * directions in the same hemisphere.
 *
 * Only Hexagonal-High (6/mmm) and Hexagonal-Low (6/m) Laue classes are
 * supported; non-hexagonal phases produce NaN output values.
 *
 * ## OOC Optimization
 *
 * Cell-level arrays (featureIds, phases, quats) are read in chunks of 4096
 * tuples via `copyIntoBuffer()`. Ensemble-level crystal structures and
 * feature-level avgCAxes are cached entirely in local `std::vector`s.
 * The final averaged result is written back to the DataStore in a single
 * `copyFromBuffer()` call. This eliminates per-element virtual dispatch
 * overhead that causes severe performance degradation when data is stored
 * out-of-core in chunked format.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeAvgCAxes
{
public:
  ComputeAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeAvgCAxesInputValues* inputValues);
  ~ComputeAvgCAxes() noexcept;

  ComputeAvgCAxes(const ComputeAvgCAxes&) = delete;
  ComputeAvgCAxes(ComputeAvgCAxes&&) noexcept = delete;
  ComputeAvgCAxes& operator=(const ComputeAvgCAxes&) = delete;
  ComputeAvgCAxes& operator=(ComputeAvgCAxes&&) noexcept = delete;

  /**
   * @brief Executes the average c-axis computation using chunked bulk I/O.
   * @return Result<> with any errors or warnings (e.g., non-hexagonal phases).
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeAvgCAxesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
