#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeCoordinateThresholdInputValues
 * @brief Stores filter values for coordinate-threshold execution.
 */
struct SIMPLNXCORE_EXPORT ComputeCoordinateThresholdInputValues
{
  ChoicesParameter::ValueType ShapeType;
  bool Invert;
  VectorFloat32Parameter::ValueType MinCoord;   ///< Stores rectangular minimum x, y, and z coordinates.
  VectorFloat32Parameter::ValueType MaxCoord;   ///< Stores rectangular maximum x, y, and z coordinates.
  VectorFloat32Parameter::ValueType SphereInfo; ///< Stores sphere center x, y, z and radius.
  DataPath GeometryPath;
  DataPath MaskArrayPath;
};

/**
 * @class ComputeCoordinateThreshold
 * @brief Creates a cell mask by testing geometry coordinates against rectangular or spherical bounds.
 *
 * Image geometry cells use contiguous direct writes in memory and bounded bulk DataStore I/O out of core.
 * This keeps disk access sequential without imposing scanline overhead on in-memory masks.
 *
 * A cell passes only when all of its vertices or ImageGeom corners are inside the selected bound.
 * Inversion reverses that result.
 *
 * Non-image geometry execution remains serial. Generic DataArray and DataStore access has no
 * concurrent-access guarantee.
 */
class SIMPLNXCORE_EXPORT ComputeCoordinateThreshold
{
public:
  /**
   * @brief Initializes the coordinate-threshold algorithm.
   * @param dataStructure Contains the input geometry and output mask.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation for ImageGeom execution.
   * @param inputValues Selects bounds and identifies required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeCoordinateThreshold(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeCoordinateThresholdInputValues* inputValues);
  /**
   * @brief Destroys the coordinate-threshold algorithm.
   */
  ~ComputeCoordinateThreshold() noexcept;

  ComputeCoordinateThreshold(const ComputeCoordinateThreshold&) = delete;
  ComputeCoordinateThreshold(ComputeCoordinateThreshold&&) noexcept = delete;
  ComputeCoordinateThreshold& operator=(const ComputeCoordinateThreshold&) = delete;
  ComputeCoordinateThreshold& operator=(ComputeCoordinateThreshold&&) noexcept = delete;

  /**
   * @enum BoundsType
   * @brief Identifies supported coordinate-bound shapes.
   */
  enum BoundsType : uint8
  {
    Rectangle = 0, ///< Uses minimum and maximum coordinates.
    Sphere = 1     ///< Uses center coordinates and radius.
  };

  /**
   * @brief Creates the coordinate-threshold mask.
   * @return Success or warning, or an input or output-storage error.
   *
   * ImageGeom paths return success when a cancellation checkpoint observes the signal. Data written
   * before that checkpoint remains in the mask. Later cells are not written. Non-image paths do not
   * inspect the cancellation flag.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeCoordinateThresholdInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
