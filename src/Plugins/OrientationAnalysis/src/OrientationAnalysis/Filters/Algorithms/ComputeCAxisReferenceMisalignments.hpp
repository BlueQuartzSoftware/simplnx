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
  /** @brief Reference directions at or below this dimensionless norm are treated as zero. */
  static constexpr float64 k_MinReferenceDirectionNorm = 1.0e-12;

  /**
   * @brief Initializes the executor. All arguments must outlive this instance.
   * @param dataStructure Provides the selected arrays and pre-created output.
   * @param messageHandler Receives progress updates.
   * @param shouldCancel Signals cancellation at a chunk boundary.
   * @param inputValues Identifies arrays and the reference direction.
   */
  ComputeCAxisReferenceMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                     const ComputeCAxisReferenceMisalignmentsInputValues* inputValues);
  /** @brief Destroys the executor. */
  ~ComputeCAxisReferenceMisalignments() noexcept = default;

  ComputeCAxisReferenceMisalignments(const ComputeCAxisReferenceMisalignments&) = delete;
  ComputeCAxisReferenceMisalignments(ComputeCAxisReferenceMisalignments&&) noexcept = delete;
  ComputeCAxisReferenceMisalignments& operator=(const ComputeCAxisReferenceMisalignments&) = delete;
  ComputeCAxisReferenceMisalignments& operator=(ComputeCAxisReferenceMisalignments&&) noexcept = delete;

  /**
   * @brief Validates the reference for both preflight and direct algorithm calls.
   * @param referenceDirection Three finite sample-frame components.
   * @return An error if the vector has an invalid size or an invalid or near-zero norm.
   */
  static Result<> ValidateReferenceDirection(const std::vector<float32>& referenceDirection);

  /**
   * @brief Writes one angle per quaternion tuple using checked bulk I/O.
   * @return Errors for invalid references, phase indices, or I/O.
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
