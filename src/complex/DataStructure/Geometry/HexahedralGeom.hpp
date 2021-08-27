#pragma once

#include "complex/DataStructure/Geometry/AbstractGeometry3D.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class HexahedralGeom
 * @brief
 */
class COMPLEX_EXPORT HexahedralGeom : public AbstractGeometry3D
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
  static HexahedralGeom* Create(DataStructure& ds, const std::string& name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  HexahedralGeom(const HexahedralGeom& other);

  /**
   * @brief
   * @param other
   */
  HexahedralGeom(HexahedralGeom&& other) noexcept;

  virtual ~HexahedralGeom();

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
   * @brief Returns the number of quads in the geometry. Returns 0 if there is
   * no quads array stored in the geometry.
   * @return size_t
   */
  size_t getNumberOfQuads() const;

  /**
   * @brief Resizes the quads array to contain the specified number of items.
   * @param numQuads
   */
  void resizeQuadList(size_t numQuads);

  /**
   * @brief
   * @param quads
   */
  void setQuads(const SharedQuadList* quads);

  /**
   * @brief
   * @return SharedQuadList*
   */
  SharedQuadList* getQuads();

  /**
   * @brief
   * @return const SharedQuadList*
   */
  const SharedQuadList* getQuads() const;

  /**
   * @brief
   * @param quadId
   * @param verts
   */
  void setVertsAtQuad(size_t quadId, size_t verts[4]);

  /**
   * @brief
   * @param quadId
   * @param verts
   */
  void getVertsAtQuad(size_t quadId, size_t verts[4]) const;

  /**
   * @brief
   * @param quadId
   * @param vert1
   * @param vert2
   * @param vert3
   * @param vert4
   */
  void getVertCoordsAtQuad(size_t quadId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2, complex::Point3D<float>& vert3, complex::Point3D<float>& vert4) const;

  /**
   * @brief
   * @param numHexas
   */
  void resizeHexList(size_t numHexas);

  /**
   * @brief
   * @param hexas
   */
  void setHexahedra(const SharedHexList* hexas);

  /**
   * @brief
   * @return SharedHexList*
   */
  SharedHexList* getHexahedrals();

  /**
   * @brief
   * @return const SharedHexList*
   */
  const SharedHexList* getHexahedrals() const;

  /**
   * @brief
   * @param hexId
   * @param verts
   */
  void setVertsAtHex(size_t hexId, size_t verts[8]);

  /**
   * @brief
   * @param hexId
   * @param verts
   */
  void getVertsAtHex(size_t hexId, size_t verts[8]) const;

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
  void getVertCoordsAtHex(size_t hexId, complex::Point3D<float>& vert1, complex::Point3D<float>& vert2, complex::Point3D<float>& vert3, complex::Point3D<float>& vert4, complex::Point3D<float>& vert5,
                          complex::Point3D<float>& vert6, complex::Point3D<float>& vert7, complex::Point3D<float>& vert8) const;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfHexas() const;

  /**
   * @brief
   */
  void initializeWithZeros() override;

  /**
   * @brief
   * @return size_t
   */
  size_t getNumberOfElements() const override;

  /**
   * @brief
   * @return StatusCode
   */
  StatusCode findElementSizes() override;

  /**
   * @brief
   * @return const FloatArray*
   */
  const FloatArray* getElementSizes() const override;

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
   * @return const ElementDynamicList*
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
   * @return const ElementDynamicList*
   */
  const ElementDynamicList* getElementNeighbors() const override;

  /**
   * @brief
   */
  void deleteElementNeighbors() override;

  /**
   * @return StatusCode
   */
  StatusCode findElementCentroids() override;

  /**
   * @brief
   * @return
   */
  const FloatArray* getElementCentroids() const override;

  /**
   * @brief
   */
  void deleteElementCentroids() override;

  /**
   * @brief
   * @return
   */
  complex::Point3D<double> getParametricCenter() const override;

  /**
   * @brief
   * @param  pCoords
   * @param  shape
   */
  void getShapeFunctions(const complex::Point3D<double>& pCoords, double* shape) const override;

  /**
   * @brief
   * @param field
   * @param derivatives
   * @param observable
   */
  void findDerivatives(DoubleArray* field, DoubleArray* derivatives, Observable* observable = nullptr) const override;

  /**
   * @brief
   * @return complex::TooltipGenerator
   */
  complex::TooltipGenerator getTooltipGenerator() const override;

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

  /**
   * @brief
   * @return uint32_t
   */
  uint32_t getXdmfGridType() const override;

  /**
   * @brief Reads values from HDF5
   * @param targetId
   * @param groupId
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::IdType targetId, H5::IdType groupId) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param parentId
   * @param groupId
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5_impl(H5::IdType parentId, H5::IdType groupId) const override;

protected:
  /**
   * @brief
   * @param ds
   * @param name
   */
  HexahedralGeom(DataStructure& ds, const std::string& name);

  /**
   * @brief
   * @param ds
   * @param name
   * @param numHexas
   * @param vertices
   * @param allocate
   */
  HexahedralGeom(DataStructure& ds, const std::string& name, size_t numHexas, const std::shared_ptr<SharedVertexList>& vertices, bool allocate = true);

  /**
   * @brief
   * @param ds
   * @param name
   * @param hexas
   * @param vertices
   */
  HexahedralGeom(DataStructure& ds, const std::string& name, const std::shared_ptr<SharedHexList>& hexas, const std::shared_ptr<SharedVertexList>& vertices);

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
  void setElementCentroids(const FloatArray* elementCentroids) override;

  /**
   * @brief
   * @param elementSizes
   */
  void setElementSizes(const FloatArray* elementSizes) override;

private:
  std::optional<DataObject::IdType> m_HexListId;
  std::optional<DataObject::IdType> m_HexasContainingVertId;
  std::optional<DataObject::IdType> m_HexNeighborsId;
  std::optional<DataObject::IdType> m_HexCentroidsId;
  std::optional<DataObject::IdType> m_HexSizesId;
};
} // namespace complex
