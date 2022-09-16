#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry2D.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry3D : public INodeGeometry2D
{
public:
  static inline constexpr StringLiteral k_PolyhedronDataName = "Polyhedron Data";

  INodeGeometry3D() = delete;
  INodeGeometry3D(const INodeGeometry3D&) = default;
  INodeGeometry3D(INodeGeometry3D&&) = default;

  INodeGeometry3D& operator=(const INodeGeometry3D&) = delete;
  INodeGeometry3D& operator=(INodeGeometry3D&&) noexcept = delete;

  ~INodeGeometry3D() noexcept override = default;

  const std::optional<IdType>& getPolyhedronListId() const;

  SharedFaceList* getPolyhedra();

  const SharedFaceList* getPolyhedra() const;

  SharedFaceList& getPolyhedraRef();

  const SharedFaceList& getPolyhedraRef() const;

  void setPolyhedra(const SharedFaceList& polyhedra);

  /**
   * @brief Resizes the polyhedra list to the target size.
   * @param size
   */
  void resizePolyhedronList(usize size);

  /**
   * @brief Returns the number of polyhedra in the geometry.
   * @return usize
   */
  usize getNumberOfPolyhedra() const;

  /**
   * @brief Creates and assigns the face list array for the current values.
   * @return StatusCode
   */
  virtual StatusCode findFaces() = 0;

  /**
   * @brief Deletes the current face list array.
   */
  void deleteFaces();

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getUnsharedFacesId() const;

  /**
   * @brief Creates and assigns the unshared face list array for the current values.
   */
  virtual StatusCode findUnsharedFaces() = 0;

  /**
   * @brief Returns a pointer to the unshared face list array.
   * @return
   */
  const SharedFaceList* getUnsharedFaces() const;

  /**
   * @brief Deletes the current unshared face list array.
   */
  void deleteUnsharedFaces();

  /**
   * @brief Returns the number of vertices in the cell.
   * @return
   */
  virtual usize getNumberOfVerticesPerCell() const = 0;

  /**
   * @brief
   * @param tetId
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void setCellPointIds(usize tetId, nonstd::span<usize> vertexIds);

  /**
   * @brief
   * @param tetId
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void getCellPointIds(usize tetId, nonstd::span<usize> vertexIds) const;

  /**
   * @brief
   * @param tetId
   * @param coords The coordinates of each vertex
   */
  void getCellCoordinates(usize tetId, nonstd::span<Point3Df> coords) const;

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getPolyhedraDataId() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getPolyhedronData();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getPolyhedronData() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getPolyhedronDataRef();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getPolyhedronDataRef() const;

  /**
   * @brief
   * @return
   */
  DataPath getPolyhedronDataPath() const;

  /**
   * @brief
   * @param attributeMatrix
   */
  void setPolyhedronData(const AttributeMatrix& attributeMatrix);

  /**
   * @brief Reads values from HDF5
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
  INodeGeometry3D(DataStructure& ds, std::string name);

  INodeGeometry3D(DataStructure& ds, std::string name, IdType importId);

  SharedQuadList* createSharedQuadList(usize numQuads);

  SharedTriList* createSharedTriList(usize numTris);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the complex::DataStructure object.
   */
  std::optional<IdType> m_PolyhedronListId;
  std::optional<IdType> m_PolyhedronDataId;
  std::optional<IdType> m_UnsharedFaceListId;
};
} // namespace complex
