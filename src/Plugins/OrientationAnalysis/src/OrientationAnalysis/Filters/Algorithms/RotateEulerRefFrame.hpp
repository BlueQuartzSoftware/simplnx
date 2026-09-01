#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct RotateEulerRefFrameInputValues
 * @brief Identifies the axis-angle rotation and in-place Euler array.
 */
struct ORIENTATIONANALYSIS_EXPORT RotateEulerRefFrameInputValues
{
  std::vector<float> rotationAxis; ///< Axis {x, y, z} followed by an angle in degrees.
  DataPath eulerAngleDataPath;     ///< Three-component Float32 Euler angles in radians.
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
 * The sequential implementation reads, rotates, and writes at most 65,536
 * tuples per page. This bounds staging memory and gives out-of-core stores
 * contiguous access. Concurrent access to the same Euler array is not supported.
 *
 * Cancellation is checked between pages. It returns success and preserves
 * pages written before cancellation.
 */
class ORIENTATIONANALYSIS_EXPORT RotateEulerRefFrame
{
public:
  /**
   * @brief Initializes an in-place Euler reference-frame rotation.
   * @param dataStructure Provides the Euler array.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between pages.
   * @param inputValues Identifies the rotation and Euler array.
   * @pre All arguments outlive this executor.
   */
  RotateEulerRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateEulerRefFrameInputValues* inputValues);
  ~RotateEulerRefFrame() noexcept;

  RotateEulerRefFrame(const RotateEulerRefFrame&) = delete;
  RotateEulerRefFrame(RotateEulerRefFrame&&) = delete;
  RotateEulerRefFrame& operator=(const RotateEulerRefFrame&) = delete;
  RotateEulerRefFrame& operator=(RotateEulerRefFrame&&) = delete;

  /**
   * @brief Rotates the Euler array in bounded read-modify-write pages.
   * @return Error for a zero axis or a failed bulk transfer.
   * @pre rotationAxis contains x, y, z, and an angle in degrees.
   * @pre The Euler array has three components per tuple.
   */
  Result<> operator()();

  bool shouldCancel() const;

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const RotateEulerRefFrameInputValues* m_InputValues = nullptr;
};

} // namespace nx::core
