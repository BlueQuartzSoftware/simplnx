#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{
/**
 * @struct ComputeCAxisReferenceMisalignmentsInputValues
 * @brief Holds the reference direction, the mask switch, and the array paths.
 *
 * Quaternions, Phases, Mask, and output share one tuple count at any level.
 * Crystal Structures is the ensemble array that Phases indexes.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeCAxisReferenceMisalignmentsInputValues
{
  std::vector<float32> ReferenceDirection;
  bool UseMask = false;
  DataPath QuatsArrayPath;
  DataPath PhasesArrayPath;
  DataPath MaskArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath OutputArrayPath;
};

/**
 * @class ComputeCAxisReferenceMisalignments
 * @brief Computes hexagonal c-axis angles to a sample-frame reference direction.
 *
 * One bounded bulk-I/O traversal serves resident and out-of-core stores.
 * Masked tuples, non-hexagonal tuples, and zero or nonfinite quaternions receive NaN.
 * Finite, nonzero quaternions are normalized before use.
 * Antipodal symmetry folds angles to [0, 90] degrees.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeCAxisReferenceMisalignments
{
public:
  /**
   * @brief An unmasked phase value is not an index into the Crystal Structures array.
   */
  static constexpr int32 k_PhaseOutOfRangeError = -98712;
  /**
   * @brief Warning: some unmasked tuples have a known non-hexagonal crystal structure.
   */
  static constexpr int32 k_NonHexagonalPhasesWarning = -98713;

  /**
   * @brief Initializes the executor. All arguments must outlive this instance.
   * @param dataStructure Provides the selected arrays and pre-created output.
   * @param messageHandler Receives progress updates.
   * @param shouldCancel Signals cancellation at a chunk boundary.
   * @param inputValues Identifies arrays and the reference direction.
   */
  ComputeCAxisReferenceMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                     const ComputeCAxisReferenceMisalignmentsInputValues* inputValues);
  /**
   * @brief Destroys the executor.
   */
  ~ComputeCAxisReferenceMisalignments() noexcept = default;

  ComputeCAxisReferenceMisalignments(const ComputeCAxisReferenceMisalignments&) = delete;
  ComputeCAxisReferenceMisalignments(ComputeCAxisReferenceMisalignments&&) noexcept = delete;
  ComputeCAxisReferenceMisalignments& operator=(const ComputeCAxisReferenceMisalignments&) = delete;
  ComputeCAxisReferenceMisalignments& operator=(ComputeCAxisReferenceMisalignments&&) noexcept = delete;

  /**
   * @brief Writes one angle per quaternion tuple using checked bulk I/O.
   *
   * @pre ReferenceDirection has three finite components and a length greater than 1.0e-12.
   * @pre QuatsArrayPath names a float32 array with component shape {4}; PhasesArrayPath an int32 array
   * with component shape {1}; CrystalStructuresArrayPath a uint32 array with component shape {1}; and,
   * when UseMask is set, MaskArrayPath a bool or uint8 array with component shape {1}.
   * @pre OutputArrayPath names a float32 array with component shape {1}. Phases, the enabled mask and
   * the output have one tuple per quaternion tuple.
   * @return Errors for phase indices outside Crystal Structures, or for I/O.
   * A warning counts only unmasked tuples with known non-hexagonal structures.
   * UnknownCrystalStructure does not contribute to the count.
   * Cancellation leaves only completed chunks written.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeCAxisReferenceMisalignmentsInputValues* m_InputValues = nullptr;
};
} // namespace nx::core
