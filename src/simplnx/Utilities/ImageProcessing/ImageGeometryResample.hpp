#pragma once

#include "simplnx/simplnx_export.hpp"

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
 * Spacing, CellDataGroupPath, and RemoveOriginalImageGeom are filter workflow values.
 * The executor uses the preflight-created destination geometry instead.
 */
struct SIMPLNX_EXPORT ResampleImageGeomInputValues
{
  ChoicesParameter::ValueType ResamplingMode = 0;
  std::vector<float32> Spacing;
  std::vector<float32> Scaling;
  std::vector<uint64> ExactDimensions;
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
 * @brief Copies Image Geometry cell data to a preflight-created regular grid.
 *
 * Each destination cell uses the source cell that contains its minimum corner. A corner outside the source bounds produces a zero tuple.
 * Axis lookup tables avoid repeated coordinate division. Each array uses one source-row buffer and one destination-row buffer.
 *
 * Cell arrays run as independent parallel tasks. Each task checks cancellation between destination Z slices.
 * Thus, cancellation can leave arrays at different completed slices. Source and destination bulk-I/O results are discarded.
 *
 * Optional feature renumbering starts after all cell tasks finish. The algorithm deep-copies feature arrays before compaction can resize them.
 */
class SIMPLNX_EXPORT ResampleImageGeom
{
public:
  /**
   * @brief Initializes Image Geometry resampling.
   * @param dataStructure Contains source and destination objects.
   * @param msgHandler Receives array and progress messages.
   * @param shouldCancel Signals cancellation between scheduling and Z slices.
   * @param inputValues Identifies source, destination, and feature data.
   * @pre The input values and all constructor arguments outlive this object.
   */
  ResampleImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ResampleImageGeomInputValues* inputValues);

  /**
   * @brief Destroys the Image Geometry resampling algorithm.
   */
  ~ResampleImageGeom() noexcept;

  ResampleImageGeom(const ResampleImageGeom&) = delete;
  ResampleImageGeom(ResampleImageGeom&&) noexcept = delete;
  ResampleImageGeom& operator=(const ResampleImageGeom&) = delete;
  ResampleImageGeom& operator=(ResampleImageGeom&&) noexcept = delete;

  /**
   * @brief Resamples cell arrays and optionally renumbers feature data.
   * @return Feature validation, deep-copy, or renumber result.
   *
   * The method cannot report cell-array bulk-I/O failures. Cancellation can leave partial destination arrays.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Sends one worker progress message through the shared throttle.
   * @param message Message to send.
   * @pre Call only from a worker while operator() is active.
   *
   * A mutex serializes access because the throttle is not thread-safe. The stored pointer refers to operator() stack state.
   */
  void sendThreadSafeProgressMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const ResampleImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // The mutex serializes worker access to the non-thread-safe throttle.
  mutable std::mutex m_ProgressMessage_Mutex;

  // This pointer borrows operator() stack state and is valid only while that method is active.
  ThrottledMessenger* m_ThrottledMessengerPtr = nullptr;
};

/**
 * @brief Plans Image Geometry resampling without a plugin dependency.
 * @param dataStructure Contains the source Image Geometry and child objects.
 * @param inputValues Contains the resampling settings and data paths.
 * @return Output actions, display values, or a validation error.
 */
SIMPLNX_EXPORT IFilter::PreflightResult PreflightImageGeometryResample(const DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues);

/**
 * @brief Resamples an Image Geometry without a plugin dependency.
 * @param dataStructure Contains the source and destination geometries.
 * @param inputValues Contains the resampling settings and data paths.
 * @param messageHandler Receives algorithm messages.
 * @param shouldCancel Stops resampling when the value is true.
 * @return An error if data copying or feature renumbering fails.
 */
SIMPLNX_EXPORT Result<> ResampleImageGeometry(DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues, const IFilter::MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel);

} // namespace nx::core
