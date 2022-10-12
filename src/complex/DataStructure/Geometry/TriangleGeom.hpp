#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry2D.hpp"

#include "complex/complex_export.hpp"

#include <nonstd/span.hpp>

namespace complex
{
/**
 * @class TriangleGeom
 * @brief
 */
class COMPLEX_EXPORT TriangleGeom : public INodeGeometry2D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumEdgeVerts = 2;
  static inline constexpr usize k_NumFaceVerts = 3;
  static inline constexpr usize k_NumVerts = 3;

  /**
   * @brief
   * @param ds
   * @param name
   * @param parentId = {}
   * @return TriangleGeom*
   */
  static TriangleGeom* Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   * @param parentId = {}
   * @return TriangleGeom*
   */
  static TriangleGeom* Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  TriangleGeom(const TriangleGeom& other) = default;

  /**
   * @brief
   * @param other
   */
  TriangleGeom(TriangleGeom&& other) = default;

  ~TriangleGeom() noexcept override = default;

  TriangleGeom& operator=(const TriangleGeom&) = delete;
  TriangleGeom& operator=(TriangleGeom&&) noexcept = delete;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  IGeometry::Type getGeomType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfCells() const override;

  /**
   * @brief
   * @return
   */
  usize getNumberOfVerticesPerFace() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementSizes() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementsContainingVert() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementNeighbors() override;
  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementCentroids() override;

  /**
   * @brief
   * @param pCoords
   * @return Point3D<float64>
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findEdges() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedEdges() override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  TriangleGeom(DataStructure& ds, std::string name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   */
  TriangleGeom(DataStructure& ds, std::string name, IdType importId);
};
} // namespace complex
