#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry3D.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class HexahedralGeom
 * @brief
 */
class COMPLEX_EXPORT HexahedralGeom : public INodeGeometry3D
{
public:
  friend class DataStructure;

  /**
   * @brief
   * @param ds
   * @param name
   * @param parentId = {}
   * @return HexahedralGeom*
   */
  static HexahedralGeom* Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   * @param parentId = {}
   * @return HexahedralGeom*
   */
  static HexahedralGeom* Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  HexahedralGeom(const HexahedralGeom& other);

  /**
   * @brief
   * @param other
   */
  HexahedralGeom(HexahedralGeom&& other);

  ~HexahedralGeom() noexcept override;

  HexahedralGeom& operator=(const HexahedralGeom&) = delete;
  HexahedralGeom& operator=(HexahedralGeom&&) noexcept = delete;

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
   * @brief Returns the number of quads in the geometry. Returns 0 if there is
   * no quads array stored in the geometry.
   * @return usize
   */
  usize getNumberOfQuads() const;

  /**
   * @brief
   * @param quadId
   * @param verts
   */
  void setVertsAtQuad(usize quadId, usize verts[4]);

  /**
   * @brief
   * @param quadId
   * @param verts
   */
  void getVertsAtQuad(usize quadId, usize verts[4]) const;

  /**
   * @brief
   * @param quadId
   * @param vert1
   * @param vert2
   * @param vert3
   * @param vert4
   */
  void getVertCoordsAtQuad(usize quadId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2, complex::Point3D<float32>& vert3, complex::Point3D<float32>& vert4) const;

  /**
   * @brief
   * @param hexId
   * @param verts
   */
  void setVertsAtHex(usize hexId, usize verts[8]);

  /**
   * @brief
   * @param hexId
   * @param verts
   */
  void getVertsAtHex(usize hexId, usize verts[8]) const;

  /**
   * @brief
   * @param hexId
   * @param vert1
   * @param vert2
   * @param vert3
   * @param vert4
   * @param vert5
   * @param vert6
   * @param vert7
   * @param vert8
   */
  void getVertCoordsAtHex(usize hexId, Point3D<float32>& vert1, Point3D<float32>& vert2, Point3D<float32>& vert3, Point3D<float32>& vert4, Point3D<float32>& vert5, Point3D<float32>& vert6,
                          Point3D<float32>& vert7, Point3D<float32>& vert8) const;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfHexas() const;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfElements() const override;

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
   * @return StatusCode
   */
  StatusCode findElementCentroids() override;

  /**
   * @brief
   * @return
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
  StatusCode findFaces() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedEdges() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findUnsharedFaces() override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  HexahedralGeom(DataStructure& ds, std::string name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   */
  HexahedralGeom(DataStructure& ds, std::string name, IdType importId);
};
} // namespace complex
