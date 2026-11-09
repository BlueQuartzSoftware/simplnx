#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct FlyingEdges3DInputValues
 * @brief Collects isovalue and geometry paths.
 */
struct SIMPLNXCORE_EXPORT FlyingEdges3DInputValues
{
  DataPath imageGeomPath;
  DataPath triangleGeomPath;
  DataPath contouringArrayPath;
  DataPath normalsArrayPath;
  float64 isoVal;
};

/**
 * @class FlyingEdges3D
 * @brief Creates an isosurface TriangleGeom from an ImageGeom scalar array.
 *
 * Flying Edges keeps four input Z slices resident. Grid-edge and triangle-count
 * state scales with the YZ cross-section. Output points, faces, and normals are
 * allocated after classification and prefix passes.
 *
 * The executor records direct or OOC telemetry from the input store. Both paths
 * use the same slice-buffered implementation.
 */
class SIMPLNXCORE_EXPORT FlyingEdges3D
{
public:
  /**
   * @brief Initializes isosurface extraction.
   * @param dataStructure Contains input and output geometry.
   * @param mesgHandler Supplies the common interface. This algorithm emits no messages.
   * @param shouldCancel Supplies the common cancellation interface.
   * @param inputValues Selects the isovalue and array paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  FlyingEdges3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FlyingEdges3DInputValues* inputValues);
  ~FlyingEdges3D() noexcept;

  FlyingEdges3D(const FlyingEdges3D&) = delete;
  FlyingEdges3D(FlyingEdges3D&&) noexcept = delete;
  FlyingEdges3D& operator=(const FlyingEdges3D&) = delete;
  FlyingEdges3D& operator=(FlyingEdges3D&&) noexcept = delete;

  /**
   * @brief Runs the four Flying Edges passes.
   * @return An array-kind, component-shape, tuple-count, or source bulk-read error, or success.
   * @pre Image dimensions are nonzero.
   * @pre The contour array is scalar and matches the ImageGeom dimensions.
   *
   * The isovalue converts to the input primitive type before classification.
   * Integral input therefore truncates a fractional isovalue.
   *
   * The algorithm does not inspect the cancellation flag. A read error during
   * output generation can leave resized and partially written geometry. The
   * face AttributeMatrix resizes only after successful output generation.
   */
  Result<> operator()();

  /**
   * @brief Checks numeric input kind, scalar shape, and image cell count without reading values.
   * @param dataStructure Contains the input array and image geometry.
   * @param inputValues Identifies the input array and geometry.
   * @return An array-kind, component-shape, or tuple-count error, or success.
   */
  static Result<> ValidateInputs(const DataStructure& dataStructure, const FlyingEdges3DInputValues& inputValues);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FlyingEdges3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
