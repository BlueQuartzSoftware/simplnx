#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

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
 * @struct ResampledImageLayout
 * @brief Stores representable resampling dimensions and spacing.
 * @note Defaults provide unit dimensions and spacing for provisional preflight layouts.
 */
struct ResampledImageLayout
{
  SizeVec3 Dimensions = {1, 1, 1};
  FloatVec3 Spacing = {1, 1, 1};
};

/**
 * @brief Checks active Spacing or Scaling Factor parameters during filter preflight.
 * @param inputValues Selects the mode and its active parameter vector.
 * @return An error for an invalid active value, otherwise success.
 * @pre Active vectors have three components. Inactive vectors are not read.
 */
SIMPLNX_EXPORT Result<> ValidateResamplingParameters(const ResampleImageGeomInputValues& inputValues);

/**
 * @brief Computes a layout from source metadata without allocating cell arrays.
 * @param source Provides geometry dimensions and spacing, which can be provisional during preflight.
 * @param inputValues Selects the active resampling formula and parameters.
 * @pre Active parameter vectors have three components and valid domains.
 * @return The layout or an error for an unrepresentable derived result.
 */
SIMPLNX_EXPORT Result<ResampledImageLayout> ComputeResampledImageLayout(const ImageGeom& source, const ResampleImageGeomInputValues& inputValues);

/**
 * @class ResampleImageGeom
 * @brief Copies Image Geometry cell data to a preflight-created regular grid.
 *
 * Each destination cell uses the source cell that contains its minimum corner. A corner outside the source bounds produces a zero tuple.
 * Axis lookup tables avoid repeated coordinate division. Each array uses one source-row buffer and one destination-row buffer.
 *
 * Cell arrays run as independent parallel tasks. Each task checks cancellation between destination Z slices.
 * Thus, cancellation can leave arrays at different completed slices. Source and destination bulk-I/O failures stop the affected task.
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
   * @return Final geometry, cell-copy, feature validation, deep-copy or renumber result.
   *
   * @pre Source and destination paths identify prepared geometries with matching cell arrays.
   * Cancellation can leave partial destination arrays.
   */
  Result<> operator()();

  /**
   * @brief Checks final coordinate metadata and consumed array sizes before copy.
   * @param dataStructure Contains the prepared source and destination geometries.
   * @param inputValues Identifies the source and destination paths.
   * @pre The paths identify geometries with cell Attribute Matrices and matching arrays.
   * @return An error for unrepresentable metadata or sizes, otherwise success.
   */
  static Result<> ValidatePreparedGeometry(const DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues);

  /**
   * @brief Maps final coordinates to source indices, retaining an unmapped sentinel outside coverage.
   * @param destDim Gives the destination axis length.
   * @param destOrigin Gives the destination axis origin in geometry length units.
   * @param destSpacing Gives the destination cell spacing in geometry length units.
   * @param srcDim Gives the source axis length.
   * @param srcOrigin Gives the source axis origin in geometry length units.
   * @param srcSpacing Gives the source cell spacing in geometry length units.
   * @pre The destination length fits its index-vector container. Origins are finite and spacings are finite and positive.
   * @return Source indices or an error before an invalid integer conversion.
   */
  static Result<std::vector<usize>> ComputeAxisSourceIndices(usize destDim, float64 destOrigin, float64 destSpacing, usize srcDim, float64 srcOrigin, float64 srcSpacing);

  const std::atomic_bool& getCancel();

  /**
   * @brief Reports completed rows through the shared throttle.
   * @param completedRows Number of rows completed since the previous call.
   * @pre Call only from a worker while operator() is active.
   *
   * A mutex serializes access to the owned throttle because it is not thread-safe.
   */
  void sendThreadSafeProgressMessage(usize completedRows);

private:
  DataStructure& m_DataStructure;
  const ResampleImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // The mutex serializes worker access to the non-thread-safe throttle.
  mutable std::mutex m_ProgressMessage_Mutex;

  // Throttles progress updates from array workers.
  ThrottledMessageHandler m_Throttle;
};

/**
 * @brief Plans Image Geometry resampling without a plugin dependency.
 * @param dataStructure Contains the source Image Geometry and child objects.
 * @param inputValues Contains the resampling settings and data paths.
 * @pre Active resampling parameters have valid domains and three components.
 * @return Output actions and warnings with safe stand-ins when metadata cannot yet determine a layout.
 *
 * The created Image Geometry retains the source length unit.
 */
SIMPLNX_EXPORT IFilter::PreflightResult PreflightImageGeometryResample(const DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues);

/**
 * @brief Resamples an Image Geometry without a plugin dependency.
 * @param dataStructure Contains the source and destination geometries.
 * @param inputValues Contains the resampling settings and data paths.
 * @param messageHandler Receives algorithm messages.
 * @param shouldCancel Stops resampling when the value is true.
 * @pre Active resampling parameters have valid domains and three components.
 * @return An error if final layout computation, data copying or feature renumbering fails.
 */
SIMPLNX_EXPORT Result<> ResampleImageGeometry(DataStructure& dataStructure, const ResampleImageGeomInputValues& inputValues, const IFilter::MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel);

} // namespace nx::core
