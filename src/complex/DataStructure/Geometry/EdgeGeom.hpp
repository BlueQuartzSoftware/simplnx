#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry1D.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class EdgeGeom
 * @brief
 */
class COMPLEX_EXPORT EdgeGeom : public INodeGeometry1D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumVerts = 2;
  static inline constexpr usize k_NumEdgeVerts = 2;

  /**
   * @brief
   * @param ds
   * @param name
   * @param parentId = {}
   * @return EdgeGeom*
   */
  static EdgeGeom* Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   * @param parentId = {}
   * @return EdgeGeom*
   */
  static EdgeGeom* Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  EdgeGeom(const EdgeGeom& other);

  /**
   * @brief
   * @param other
   */
  EdgeGeom(EdgeGeom&& other) ;

  ~EdgeGeom() noexcept override;

  EdgeGeom& operator=(const EdgeGeom&) = delete;
  EdgeGeom& operator=(EdgeGeom&&) noexcept = delete;

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
  DataObject* deepCopy() override;

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
   * @return Point3D<float64>
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  EdgeGeom(DataStructure& ds, std::string name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   */
  EdgeGeom(DataStructure& ds, std::string name, IdType importId);
};
} // namespace complex
