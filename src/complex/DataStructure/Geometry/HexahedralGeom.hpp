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
  HexahedralGeom(HexahedralGeom&& other) noexcept;

  ~HexahedralGeom() override;

  HexahedralGeom& operator=(const HexahedralGeom&) = delete;
  HexahedralGeom& operator=(HexahedralGeom&&) noexcept = delete;

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
   * @brief Returns the number of quads in the geometry. Returns 0 if there is
   * no quads array stored in the geometry.
   * @return usize
   */
  usize getNumberOfQuads() const;

  /**
   * @brief Resizes the quads array to contain the specified number of items.
   * @param numQuads
   */
  void resizeQuadList(usize numQuads);

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
   * @param numHexas
   */
  void resizeHexList(usize numHexas);

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
  void getVertCoordsAtHex(usize hexId, complex::Point3D<float32>& vert1, complex::Point3D<float32>& vert2, complex::Point3D<float32>& vert3, complex::Point3D<float32>& vert4,
                          complex::Point3D<float32>& vert5, complex::Point3D<float32>& vert6, complex::Point3D<float32>& vert7, complex::Point3D<float32>& vert8) const;

  /**
   * @brief
   * @return usize
   */
  usize getNumberOfHexas() const;

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
   * @return const Float32Array*
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
  const Float32Array* getElementCentroids() const override;

  /**
   * @brief
   */
  void deleteElementCentroids() override;

  /**
   * @brief
   * @return
   */
  complex::Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param  pCoords
   * @param  shape
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
   * @return uint32
   */
  uint32 getXdmfGridType() const override;

  /**
   * @brief Reads values from HDF5
   * @param dataStructureReader
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter) const override;

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

  /**
   * @brief
   * @param ds
   * @param name
   * @param numHexas
   * @param vertices
   * @param allocate
   */
  // HexahedralGeom(DataStructure& ds, std::string name, usize numHexas, const std::shared_ptr<SharedVertexList>& vertices, bool allocate = true);

  /**
   * @brief
   * @param ds
   * @param name
   * @param hexas
   * @param vertices
   */
  // HexahedralGeom(DataStructure& ds, std::string name, const std::shared_ptr<SharedHexList>& hexas, const std::shared_ptr<SharedVertexList>& vertices);

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

private:
  std::optional<IdType> m_HexListId;
  std::optional<IdType> m_HexasContainingVertId;
  std::optional<IdType> m_HexNeighborsId;
  std::optional<IdType> m_HexCentroidsId;
  std::optional<IdType> m_HexSizesId;
};
} // namespace complex
