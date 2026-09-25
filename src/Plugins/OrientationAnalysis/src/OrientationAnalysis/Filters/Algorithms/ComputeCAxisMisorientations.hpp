#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

/**
 * @struct ComputeCAxisMisorientationsInputValues
 * @brief Identifies c-axis misorientation inputs.
 *
 * AvgCAxes must be a feature-level float32[3] array produced by
 * ComputeAvgCAxes. Non-hexagonal features are encoded as NaN by that
 * algorithm; this executor propagates those NaN values to the output.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeCAxisMisorientationsInputValues
{
  std::vector<float32> ReferenceDir;
  DataPath AvgCAxesArrayPath;
  DataPath MisorientationArrayPath;
};

/**
 * @class ComputeCAxisMisorientations
 * @brief Computes the angle between each feature's average c-axis and a
 *        user-supplied reference direction.
 *
 * The c-axis has antipodal symmetry in hexagonal systems, so the result is
 * arccos(|cAxis · refDir|) converted to degrees, always in [0°, 90°].
 *
 * The executor reads AvgCAxes in bounded chunks for OOC compatibility.
 * Non-hexagonal features (NaN in AvgCAxes) produce NaN in the output.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeCAxisMisorientations
{
public:
  /**
   * @brief Initializes c-axis misorientation computation.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and reference direction.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive
   *      this executor.
   */
  ComputeCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeCAxisMisorientationsInputValues* inputValues);

  /**
   * @brief Destroys the c-axis misorientation executor.
   */
  ~ComputeCAxisMisorientations() noexcept;

  ComputeCAxisMisorientations(const ComputeCAxisMisorientations&) = delete;
  ComputeCAxisMisorientations(ComputeCAxisMisorientations&&) noexcept = delete;
  ComputeCAxisMisorientations& operator=(const ComputeCAxisMisorientations&) = delete;
  ComputeCAxisMisorientations& operator=(ComputeCAxisMisorientations&&) noexcept = delete;

  /**
   * @brief Computes c-axis misorientations.
   * @return An error if the reference direction has zero magnitude or bulk I/O
   *         fails. Cancellation returns success with completed chunks preserved.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeCAxisMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
