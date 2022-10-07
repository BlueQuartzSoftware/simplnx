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

  static inline constexpr usize k_NumEdgeVerts = 2;
  static inline constexpr usize k_NumFaceVerts = 4;
  static inline constexpr usize k_NumVerts = 8;

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
  HexahedralGeom(const HexahedralGeom& other) = default;

  /**
   * @brief
   * @param other
   */
  HexahedralGeom(HexahedralGeom&& other) = default;

  ~HexahedralGeom() noexcept override = default;

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
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  static std::string GetTypeName();

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
   * @return
   */
  usize getNumberOfVerticesPerFace() const override;

  /**
   * @brief
   * @return
   */
  usize getNumberOfVerticesPerCell() const override;

  /**
   * @brief Returns the number of hexahedrons in this geometry
   * @return usize
   */
  usize getNumberOfCells() const override;

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
