#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeFZQuaternionsInputValues
 * @brief Identifies fundamental-zone quaternion inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeFZQuaternionsInputValues
{
  ArraySelectionParameter::ValueType CellPhasesArrayPath;
  ArraySelectionParameter::ValueType CrystalStructuresArrayPath;
  ArraySelectionParameter::ValueType InputQuatsArrayPath;
  ArraySelectionParameter::ValueType MaskArrayPath;
  DataObjectNameParameter::ValueType OutputFzQuatsArrayName;
  BoolParameter::ValueType UseMask;
};

/**
 * @class ComputeFZQuaternions
 * @brief Computes a fundamental-zone quaternion for each input tuple.
 *
 * The direct path uses contiguous in-memory buffers when available. The
 * scanline path uses 65,536-tuple bulk buffers for OOC arrays. This avoids
 * per-cell OOC reads and bounds local memory.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFZQuaternions
{
public:
  /**
   * @brief Initializes fundamental-zone quaternion computation.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and mask use.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeFZQuaternions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFZQuaternionsInputValues* inputValues);

  /**
   * @brief Destroys the fundamental-zone quaternion executor.
   */
  ~ComputeFZQuaternions() noexcept;

  ComputeFZQuaternions(const ComputeFZQuaternions&) = delete;
  ComputeFZQuaternions(ComputeFZQuaternions&&) noexcept = delete;
  ComputeFZQuaternions& operator=(const ComputeFZQuaternions&) = delete;
  ComputeFZQuaternions& operator=(ComputeFZQuaternions&&) noexcept = delete;

  /**
   * @brief Executes the storage-appropriate implementation.
   * @pre Cell phase IDs are nonnegative.
   * @return Success, or an out-of-range phase or bulk-I/O error.
   *
   * Cancellation returns success with completed tuple ranges preserved.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFZQuaternionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
