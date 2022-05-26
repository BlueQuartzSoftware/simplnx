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
  EdgeGeom(EdgeGeom&& other) noexcept;

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
   * @param vertId
   * @param coords
   */
  void setCoords(usize vertId, const Point3D<float32>& coords) override;

  /**
   * @brief
   * @param vertId
   * @return Point3D<float32>
   */
  Point3D<float32> getCoords(usize vertId) const override;

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  void setVertsAtEdge(usize edgeId, const usize verts[2]) override;

  /**
   * @brief
   * @param edgeId
   * @param verts
   */
  void getVertsAtEdge(usize edgeId, usize verts[2]) const override;

  /**
   * @brief
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  void getVertCoordsAtEdge(usize edgeId, Point3D<float32>& vert1, Point3D<float32>& vert2) const override;

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

  /**
   * @brief Reads values from HDF5
   * @param dataStructureReader
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight = false) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override;

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
