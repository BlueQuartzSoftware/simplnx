#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

/**
 * @struct RotateSampleRefFrameInputValues
 * @brief Stores rotation, slice, origin, and geometry selections.
 */
struct SIMPLNXCORE_EXPORT RotateSampleRefFrameInputValues
{
  bool SliceBySlice;
  ChoicesParameter::ValueType RotationRepresentationIndex;
  DataPath SourceGeometryPath;
  DataPath DestGeometryPath;
  VectorFloat32Parameter::ValueType RotationAxisAngle;
  DynamicTableParameter::ValueType RotationMatrixTable;
  bool KeepInputGeometryOrigin;
};

/**
 * @class RotateSampleRefFrame
 * @brief Rotates ImageGeom cell arrays with nearest-neighbor sampling.
 *
 * In-core arrays use parallel per-array tasks and a source Z-slab. The slab can
 * approach the complete source array for a rotation with a large Z span.
 * If any cell array is out of core, per-array tasks run serially. Each worker
 * then uses an eight-page bounded source cache and one destination Z slice.
 * This avoids competing disk caches and bounds source memory.
 * Storage test overrides can force either path.
 *
 * Workers check cancellation between output Z slices. They report storage
 * errors through one shared callback. Other array tasks can continue after one
 * task reports an error, so arrays can contain different completed ranges.
 *
 * Slice mode forces destination slice k to read source slice k. The rotation
 * must preserve the Z axis. KeepInputGeometryOrigin changes destination metadata
 * only after sampling; it does not change the coordinates used for resampling.
 */
class SIMPLNXCORE_EXPORT RotateSampleRefFrame
{
public:
  /**
   * @brief Initializes the sample-reference rotation algorithm.
   * @param dataStructure Contains source and destination ImageGeom objects.
   * @param mesgHandler Receives array and progress messages.
   * @param shouldCancel Signals cancellation between scheduling and output slices.
   * @param inputValues Selects rotation and output behavior.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  RotateSampleRefFrame(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RotateSampleRefFrameInputValues* inputValues);
  /**
   * @brief Destroys the sample-reference rotation algorithm.
   */
  ~RotateSampleRefFrame() noexcept;

  RotateSampleRefFrame(const RotateSampleRefFrame&) = delete;
  RotateSampleRefFrame(RotateSampleRefFrame&&) noexcept = delete;
  RotateSampleRefFrame& operator=(const RotateSampleRefFrame&) = delete;
  RotateSampleRefFrame& operator=(RotateSampleRefFrame&&) noexcept = delete;

  /**
   * @enum RotationRepresentation
   * @brief Selects the form of the input rotation.
   */
  enum class RotationRepresentation : uint64
  {
    AxisAngle = 0,     ///< Uses an axis and an angle in degrees.
    RotationMatrix = 1 ///< Uses a 4 by 4 transformation table.
  };

  /**
   * @brief Rotates all cell arrays and applies the selected output origin.
   * @return Merged worker storage errors and warnings.
   * @pre Every source cell child is an IDataArray with a matching destination array.
   * @pre The selected rotation is invertible.
   * @pre SliceBySlice is false or the rotation preserves the Z axis.
   *
   * Cancellation returns success after active workers stop. Completed slices
   * remain. Cancellation during scheduling returns before the origin reset. If
   * all tasks were scheduled, the origin reset occurs after canceled workers join.
   */
  Result<> operator()();

  /**
   * @brief Sends an information message for compatible worker interfaces.
   * @param message Message to send.
   *
   * The current rotation operator uses FilterProgressCallback instead.
   */
  void updateProgress(const std::string& message);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RotateSampleRefFrameInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
