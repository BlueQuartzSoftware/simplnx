#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

/**
 * @struct ExtractInternalSurfacesFromTriangleGeometryInputValues
 * @brief Collects node criteria, copy selections, and geometry paths.
 */
struct SIMPLNXCORE_EXPORT ExtractInternalSurfacesFromTriangleGeometryInputValues
{
  MultiArraySelectionParameter::ValueType CopyTriangleArrayPaths;
  MultiArraySelectionParameter::ValueType CopyVertexArrayPaths;
  GeometrySelectionParameter::ValueType InputTriangleGeometryPath;
  uint64 CriterionMode;
  VectorInt8Parameter::ValueType NodeTypeRange;
  ArraySelectionParameter::ValueType NodeTypesPath;
  DataPath FaceLabelsPath;
  DataGroupCreationParameter::ValueType OutputTriangleGeometryPath;
  DataObjectNameParameter::ValueType TriangleAttributeMatrixName;
  DataObjectNameParameter::ValueType VertexAttributeMatrixName;
};

/**
 * @class ExtractInternalSurfacesFromTriangleGeometry
 * @brief Extracts triangles whose three vertices pass a node-type range.
 *
 * The inclusive NodeTypeRange selects vertices. A triangle remains only when
 * all three vertex types pass. Output triangles preserve source order. Output
 * vertices use their first triangle-traversal encounter order.
 *
 * A dense vertex remap scales with input vertex count. Triangle selection uses
 * a bitmap and sparse prefix table. Fixed tuple buffers bound transfer scratch.
 * Vertex output writes remain one tuple at a time because source and output
 * vertex orders are not both monotonic.
 */
class SIMPLNXCORE_EXPORT ExtractInternalSurfacesFromTriangleGeometry
{
public:
  /**
   * @brief Initializes internal-surface extraction.
   * @param dataStructure Contains source and destination geometry.
   * @param mesgHandler Supplies the common interface. This algorithm emits no messages.
   * @param shouldCancel Signals cancellation between bounded passes.
   * @param inputValues Selects node criteria, copied arrays, and paths.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  ExtractInternalSurfacesFromTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                              ExtractInternalSurfacesFromTriangleGeometryInputValues* inputValues);
  ~ExtractInternalSurfacesFromTriangleGeometry() noexcept;

  ExtractInternalSurfacesFromTriangleGeometry(const ExtractInternalSurfacesFromTriangleGeometry&) = delete;
  ExtractInternalSurfacesFromTriangleGeometry(ExtractInternalSurfacesFromTriangleGeometry&&) noexcept = delete;
  ExtractInternalSurfacesFromTriangleGeometry& operator=(const ExtractInternalSurfacesFromTriangleGeometry&) = delete;
  ExtractInternalSurfacesFromTriangleGeometry& operator=(ExtractInternalSurfacesFromTriangleGeometry&&) noexcept = delete;

  /**
   * @brief Builds compact geometry and copies selected attached arrays.
   * @return Success.
   * @pre NodeTypeRange contains two ordered values.
   * @pre Node types match input vertices and triangle indices are valid.
   * @pre Selected attached arrays have the required tuple counts and output types.
   *
   * The implementation discards all bulk-transfer Result values. A storage
   * failure can produce invalid output while this function returns success.
   * Cancellation before output resize leaves preflight output. Later
   * cancellation returns success with resized and partially copied output.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ExtractInternalSurfacesFromTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
