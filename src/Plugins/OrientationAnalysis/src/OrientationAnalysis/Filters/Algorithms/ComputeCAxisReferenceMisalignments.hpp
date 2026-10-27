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
   * @brief The reference direction has a non-finite component or a near-zero length.
   */
  static constexpr int32 k_InvalidReferenceDirectionError = -98710;
  /**
   * @brief The Phases array or the enabled mask has a different tuple count than the quaternion array.
   */
  static constexpr int32 k_TupleCountMismatchError = -98711;
  /**
   * @brief An unmasked phase value is not an index into the Crystal Structures array.
   */
  static constexpr int32 k_PhaseOutOfRangeError = -98712;
  /**
   * @brief Warning: some unmasked tuples have a known non-hexagonal crystal structure.
   */
  static constexpr int32 k_NonHexagonalPhasesWarning = -98713;
  /**
   * @brief The reference direction does not have exactly three components.
   */
  static constexpr int32 k_ReferenceDirectionSizeError = -98714;
  /**
   * @brief The quaternion path does not resolve to a float32 array.
   */
  static constexpr int32 k_InvalidQuatsArrayError = -98715;
  /**
   * @brief The quaternion array does not have component shape {4}.
   */
  static constexpr int32 k_QuatsComponentShapeError = -98716;
  /**
   * @brief The Phases path does not resolve to an int32 array.
   */
  static constexpr int32 k_InvalidPhasesArrayError = -98717;
  /**
   * @brief The Phases array does not have component shape {1}.
   */
  static constexpr int32 k_PhasesComponentShapeError = -98718;
  /**
   * @brief The enabled mask path does not resolve to a bool or uint8 array.
   */
  static constexpr int32 k_InvalidMaskArrayError = -98719;
  /**
   * @brief The enabled mask array does not have component shape {1}.
   */
  static constexpr int32 k_MaskComponentShapeError = -98720;
  /**
   * @brief The Crystal Structures path does not resolve to a uint32 array.
   */
  static constexpr int32 k_InvalidCrystalStructuresArrayError = -98721;
  /**
   * @brief The Crystal Structures array does not have component shape {1}.
   */
  static constexpr int32 k_CrystalStructuresComponentShapeError = -98722;
  /**
   * @brief The output path does not resolve to a float32 array.
   */
  static constexpr int32 k_InvalidOutputArrayError = -98723;
  /**
   * @brief The output array does not have component shape {1}.
   */
  static constexpr int32 k_OutputComponentShapeError = -98724;
  /**
   * @brief The output array has a different tuple count than the quaternion array.
   */
  static constexpr int32 k_OutputTupleCountMismatchError = -98725;
  /**
   * @brief Preflight: the output path equals the path of an input array.
   */
  static constexpr int32 k_OutputPathCollisionError = -98726;

  /**
   * @brief Reference directions at or below this dimensionless norm are treated as zero.
   */
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
  /**
   * @brief Destroys the executor.
   */
  ~ComputeCAxisReferenceMisalignments() noexcept = default;

  ComputeCAxisReferenceMisalignments(const ComputeCAxisReferenceMisalignments&) = delete;
  ComputeCAxisReferenceMisalignments(ComputeCAxisReferenceMisalignments&&) noexcept = delete;
  ComputeCAxisReferenceMisalignments& operator=(const ComputeCAxisReferenceMisalignments&) = delete;
  ComputeCAxisReferenceMisalignments& operator=(ComputeCAxisReferenceMisalignments&&) noexcept = delete;

  /**
   * @brief Validates the reference direction. ValidateInputs() calls it.
   * @param referenceDirection Three finite sample-frame components.
   * @return k_ReferenceDirectionSizeError for a size other than three, or
   * k_InvalidReferenceDirectionError for a non-finite component or a near-zero norm.
   */
  static Result<> ValidateReferenceDirection(const std::vector<float32>& referenceDirection);

  /**
   * @brief Makes every input check that preflight data can answer. preflightImpl() and operator()() both call it.
   *
   * Checks the reference direction, and the types and component shapes of the quaternion, Phases,
   * and Crystal Structures arrays. Checks the mask only when UseMask is set. Phases and the enabled
   * mask must have one tuple per quaternion. The output is not checked because preflight has not
   * created it yet. Array lookups do not throw.
   * @param dataStructure Holds the selected arrays.
   * @param inputValues Identifies the arrays and the reference direction. OutputArrayPath is not read.
   * @return The error of the first failed check, or a valid result.
   */
  static Result<> ValidateInputs(const DataStructure& dataStructure, const ComputeCAxisReferenceMisalignmentsInputValues& inputValues);

  /**
   * @brief Writes one angle per quaternion tuple using checked bulk I/O.
   *
   * Validates the reference direction, the input arrays and the output array (float32,
   * component shape {1}, one tuple per quaternion tuple) before any read.
   * @return Errors for invalid references or arrays, phase indices, or I/O.
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
