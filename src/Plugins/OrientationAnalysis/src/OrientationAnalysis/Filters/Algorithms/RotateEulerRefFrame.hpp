#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @brief Input values for the RotateEulerRefFrame algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT RotateEulerRefFrameInputValues
{
  std::vector<float> rotationAxis; ///< Rotation axis {x, y, z, angle_degrees}
  DataPath eulerAngleDataPath;     ///< Cell-level Float32 Euler angles (3 components, radians)
};

/**
 * @class RotateEulerRefFrame
 * @brief Performs a passive rotation of Euler angles about a user-defined axis-angle pair.
 *
 * The reference frame is rotated, so the Euler angles are updated to represent
 * the same physical orientation in the new frame. Each Euler angle triplet is
 * converted to an orientation matrix, multiplied by the rotation matrix, and
 * converted back to Euler angles.
 *
 * ## OOC Optimization (Major Rewrite)
 *
 * The original implementation used `ParallelDataAlgorithm` with a threaded
 * worker that accessed the Euler angle array via `operator[]`. This caused
 * severe performance degradation with OOC storage due to per-element virtual
 * dispatch and random chunk access from multiple threads.
 *
 * The optimized implementation uses sequential chunked bulk I/O:
 *   - Euler angles are read in chunks of 65536 tuples via `copyIntoBuffer()`.
 *   - The rotation is applied to each tuple in the local buffer.
 *   - The modified buffer is written back via `copyFromBuffer()`.
 *
 * This in-place read-modify-write pattern is inherently sequential but
 * provides excellent OOC throughput since each chunk is a single contiguous
 * I/O operation.
 */
class ORIENTATIONANALYSIS_EXPORT RotateEulerRefFrame
{
public:
  RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateEulerRefFrameInputValues* inputValues);
  ~RotateEulerRefFrame() noexcept;

  RotateEulerRefFrame(const RotateEulerRefFrame&) = delete;            // Copy Constructor Not Implemented
  RotateEulerRefFrame(RotateEulerRefFrame&&) = delete;                 // Move Constructor Not Implemented
  RotateEulerRefFrame& operator=(const RotateEulerRefFrame&) = delete; // Copy Assignment Not Implemented
  RotateEulerRefFrame& operator=(RotateEulerRefFrame&&) = delete;      // Move Assignment Not Implemented

  /**
   * @brief Executes the Euler angle rotation using chunked read-modify-write I/O.
   * @return Result<> with any errors encountered during execution.
   */
  Result<> operator()();

  /** @brief Returns whether the algorithm should cancel. */
  bool shouldCancel() const;

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const RotateEulerRefFrameInputValues* m_InputValues = nullptr;
};

} // namespace nx::core
