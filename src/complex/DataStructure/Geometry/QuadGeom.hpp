#pragma once

#include "complex/Common/Point3D.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry2D.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class QuadGeom
 * @brief
 */
class COMPLEX_EXPORT QuadGeom : public AbstractGeometry2D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumVerts = 4;

  /**
   * @brief
   * @param ds
   * @param name
   * @param parentId = {}
   * @return QuadGeom*
   */
  static QuadGeom* Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   * @param parentId = {}
   * @return QuadGeom*
   */
  static QuadGeom* Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  QuadGeom(const QuadGeom& other);

  /**
   * @brief
   * @param other
   */
  QuadGeom(QuadGeom&& other) noexcept;

  ~QuadGeom() override;

  QuadGeom& operator=(const QuadGeom&) = delete;
  QuadGeom& operator=(QuadGeom&&) noexcept = delete;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  AbstractGeometry::Type getGeomType() const override;

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
   * @return std::string
   */
  std::string getGeometryTypeAsString() const override;

  /**
   * @brief
   * @param numQuads
   */
  void resizeFaceList(usize numQuads);

  /**
   * @brief
   * @param quads
   */
  void setFaces(const SharedQuadList* quads);

  /**
   * @brief
   * @return SharedQuadList*
   */
  SharedQuadList* getFaces();

  /**
   * @brief
   * @return const SharedQuadList*
   */
  const SharedQuadList* getFaces() const;

  /**
   * @brief Returns the DataObject ID for the face list array. Returns an empty optional if no face list array has been set.
   * @return std::optional<IdType>
   */
  std::optional<DataObject::IdType> getFacesId() const;

  /**
   * @brief
   * @param quadId
   * @param verts
   */
  void setVertexIdsForFace(usize quadId, usize verts[4]);

  /**
   * @brief
   * @param faceId
   * @param verts
   */
  void getVertexIdsForFace(usize faceId, usize verts[4]) const;

  /**
   * @brief
   * @param faceId
   * @param vert1
   * @param vert2
   * @param vert3
   * @param vert4
   */
  void getVertexCoordsForFace(usize faceId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2, complex::Point3D<float32>& vert3, complex::Point3D<float32>& vert4) const;

  /**
   * @brief Returns the number of quads in the geometry. If the quad list has
   * not been set, this method returns 0.
   * @return usize
   */
  usize getNumberOfQuads() const;

  /**
   * @brief
   */
  void initializeWithZeros() override;

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
   * @return std::shared_ptr<Float32Array>
   */
  const Float32Array* getElementSizes() const override;

  /**
   * @brief
   */
  void deleteElementSizes() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementsContainingVert() override;

  /**
   * @brief
   * @return std::shared_ptr<ElementDynamicList>
   */
  const ElementDynamicList* getElementsContainingVert() const override;

  /**
   * @brief
   */
  void deleteElementsContainingVert() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementNeighbors() override;

  /**
   * @brief
   * @return std::shared_ptr<ElementDynamicList>
   */
  const ElementDynamicList* getElementNeighbors() const override;

  /**
   * @brief
   */
  void deleteElementNeighbors() override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementCentroids() override;

  /**
   * @brief
   * @return std::shared_ptr<Float32Array>
   */
  const Float32Array* getElementCentroids() const override;

  /**
   * @brief
   */
  void deleteElementCentroids() override;

  /**
   * @brief
   * @return complex::Point3D<float64>
   */
  complex::Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const complex::Point3D<float64>& pCoords, double* shape) const override;

  /**
   * @brief
   * @param field
   * @param derivatives
   * @param observable
   */
  void findDerivatives(Float64Array* field, Float64Array* derivatives, Observable* observable = nullptr) const override;

  /**
   * @brief
   * @return complex::TooltipGenerator
   */
  complex::TooltipGenerator getTooltipGenerator() const override;

  /**
   * @brief
   * @param vertId
   * @param Point3D<float32>
   */
  void setCoords(usize vertId, const Point3D<float32>& coord) override;

  /**
   * @brief
   * @param vertId
   * @return Point3D<float32>
   */
  Point3D<float32> getCoords(usize vertId) const override;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfVertices() const override;

  /**
   * @brief
   * @param numEdges
   */
  void resizeEdgeList(usize numEdges) override;

  /**
   * @brief
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  void getVertCoordsAtEdge(usize edgeId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2) const override;

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

  /**
   * @brief
   * @return uint32
   */
  uint32 getXdmfGridType() const override;

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
  QuadGeom(DataStructure& ds, std::string name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param importId
   */
  QuadGeom(DataStructure& ds, std::string name, IdType importId);

  /**
   * @brief
   * @param ds
   * @param name
   * @param numQuads
   * @param vertices
   * @param allocate
   */
  // QuadGeom(DataStructure& ds, std::string name, usize numQuads, const std::shared_ptr<SharedVertexList>& vertices, bool allocate = true);

  /**
   * @brief
   * @param ds
   * @param name
   * @param quads
   * @param vertices
   */
  // QuadGeom(DataStructure& ds, std::string name, const std::shared_ptr<SharedQuadList>& quads, const std::shared_ptr<SharedVertexList>& vertices);

  /**
   * @brief
   * @param elementsContainingVert
   */
  void setElementsContainingVert(const ElementDynamicList* elementsContainingVert) override;

  /**
   * @brief
   * @param elementNeighbors
   */
  void setElementNeighbors(const ElementDynamicList* elementNeighbors) override;

  /**
   * @brief
   * @param elementCentroids
   */
  void setElementCentroids(const Float32Array* elementCentroids) override;

  /**
   * @brief
   * @param elementSizes
   */
  void setElementSizes(const Float32Array* elementSizes) override;

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

private:
  std::optional<IdType> m_QuadListId;
  std::optional<IdType> m_QuadsContainingVertId;
  std::optional<IdType> m_QuadNeighborsId;
  std::optional<IdType> m_QuadCentroidsId;
  std::optional<IdType> m_QuadSizesId;
};
} // namespace complex
