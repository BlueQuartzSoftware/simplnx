#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <vector>

namespace nx::core
{

/**
 * @struct ResampleImageGeomInputValues
 * @brief Stores source, destination, and optional feature-renumber selections.
 *
 * Spacing, CellDataGroupPath, and RemoveOriginalImageGeom are filter workflow
 * values. This executor uses the preflight-created destination geometry instead.
 */
struct SIMPLNXCORE_EXPORT ResampleImageGeomInputValues
{
  std::vector<float32> Spacing;
  DataPath SelectedImageGeometryPath;
  DataPath CellDataGroupPath;
  bool RemoveOriginalImageGeom;
  DataPath CreatedImageGeometryPath;
  bool RenumberFeatures;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrix;
};

/**
 * @class ResampleImageGeom
 * @brief Copies ImageGeom cell data to a preflight-created regular grid.
 *
 * Each destination cell uses the source cell that contains its minimum corner.
 * A corner outside the source's half-open bounds produces a zero tuple. Axis
 * lookup tables avoid repeated coordinate division for every destination cell.
 * Each array then uses one source-row and one destination-row buffer.
 *
 * Cell arrays run as independent parallel tasks. Each task checks cancellation
 * between destination Z slices. Thus, cancellation can leave arrays at different
 * completed slices. Source and destination bulk-I/O results are discarded.
 *
 * Optional feature renumbering starts only after all cell tasks finish. It deep
 * copies feature arrays before compaction because renumbering can resize them.
 * A later copy, validation, or renumber error does not restore cell output or
 * feature arrays that were copied earlier.
 */
class SIMPLNXCORE_EXPORT ResampleImageGeom
{
public:
  /**
   * @brief Initializes the ImageGeom resampling algorithm.
   * @param dataStructure Contains source and destination objects.
   * @param msgHandler Receives array and progress messages.
   * @param shouldCancel Signals cancellation between scheduling and Z slices.
   * @param inputValues Identifies source, destination, and feature data.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ResampleImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ResampleImageGeomInputValues* inputValues);
  /**
   * @brief Destroys the ImageGeom resampling algorithm.
   */
  ~ResampleImageGeom() noexcept;

  ResampleImageGeom(const ResampleImageGeom&) = delete;
  ResampleImageGeom(ResampleImageGeom&&) noexcept = delete;
  ResampleImageGeom& operator=(const ResampleImageGeom&) = delete;
  ResampleImageGeom& operator=(ResampleImageGeom&&) noexcept = delete;

  /**
   * @brief Resamples cell arrays and optionally renumbers feature data.
   * @return Feature validation, deep-copy, or renumber result.
   * @pre Every source cell child is an IDataArray with a matching destination array.
   * @pre Source and destination dimensions and spacing are positive.
   *
   * The method cannot report cell-array bulk-I/O failures. Cancellation returns
   * success after active tasks stop and can leave partial destination arrays.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Sends one worker progress message through the shared throttle.
   * @param message Message to send.
   * @pre Call only from a worker while operator() is active.
   *
   * A mutex serializes access because the throttle is not thread-safe. The
   * stored throttle pointer refers to operator() stack state.
   */
  void sendThreadSafeProgressMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const ResampleImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Serializes worker access to the non-thread-safe throttle.
  mutable std::mutex m_ProgressMessage_Mutex;

  // Borrows operator() stack state and is valid only while that method is active.
  ThrottledMessenger* m_ThrottledMessengerPtr = nullptr;
};

} // namespace nx::core
