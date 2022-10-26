#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry1D : public INodeGeometry0D
{
public:
  static inline constexpr StringLiteral k_EdgeDataName = "Edge Data";
  static inline constexpr StringLiteral k_FaceFeatureAttributeMatrix = "Face Feature Data";

  static inline constexpr usize k_NumEdgeVerts = 2;

  INodeGeometry1D() = delete;
  INodeGeometry1D(const INodeGeometry1D&) = default;
  INodeGeometry1D(INodeGeometry1D&&) = default;

  INodeGeometry1D& operator=(const INodeGeometry1D&) = delete;
  INodeGeometry1D& operator=(INodeGeometry1D&&) noexcept = delete;

  ~INodeGeometry1D() noexcept override = default;

  /**
   * @brief
   * @return
   */
  SharedEdgeList* getEdges();

  /**
   * @brief
   * @return
   */
  const SharedEdgeList* getEdges() const;

  /**
   * @brief
   * @return
   */
  SharedEdgeList& getEdgesRef();

  /**
   * @brief
   * @return
   */
  const SharedEdgeList& getEdgesRef() const;

  /**
   * @brief
   * @return
   */
  void setEdgeList(const SharedEdgeList& edges);

  /**
   * @brief Resizes the edge list to the target size.
   * @param size
   */
  void resizeEdgeList(usize size);

  /**
   * @brief Returns the number of edges in the geometry.
   * @return usize
   */
  usize getNumberOfCells() const override;

  /**
   * @brief returns the number of edges in the geometry
   * @return
   */
  usize getNumberOfEdges() const;

  /**
   * @brief Sets the vertex IDs making up the specified edge. This method does
   * nothing if the edge list could not be found.
   * @param edgeId
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void setEdgePointIds(usize edgeId, nonstd::span<usize> vertexIds);

  /**
   * @brief Returns the vertices that make up the specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void getEdgePointIds(usize edgeId, nonstd::span<usize> vertexIds) const;

  /**
   * @brief Returns the vertex coordinates for a specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  void getEdgeCoordinates(usize edgeId, nonstd::span<Point3Df> coords) const;

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementsContainingVert() = 0;

  /**
   * @brief
   * @return const ElementDynamicList*
   */
  const ElementDynamicList* getElementsContainingVert() const;

  /**
   * @brief
   */
  void deleteElementsContainingVert();

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementNeighbors() = 0;

  /**
   * @brief
   * @return const ElementDynamicList*
   */
  const ElementDynamicList* getElementNeighbors() const;

  /**
   * @brief
   */
  void deleteElementNeighbors();

  /**
   * @brief
   * @return StatusCode
   */
  virtual StatusCode findElementCentroids() = 0;

  /**
   * @brief
   * @return const Float32Array*
   */
  const Float32Array* getElementCentroids() const;

  /**
   * @brief
   */
  void deleteElementCentroids();

  /****************************************************************************
   * These functions get values related to where the Vertex Coordinates are
   * stored in the DataStructure
   */

  const std::optional<IdType>& getEdgeListDataArrayId() const;

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getEdgeAttributeMatrixId() const;

  /**
   * @brief Returns pointer to the Attribute Matrix that holds data assigned to each edge
   * @return
   */
  AttributeMatrix* getEdgeAttributeMatrix();

  /**
   * @brief Returns pointer to the Attribute Matrix that holds data assigned to each edge
   * @return
   */
  const AttributeMatrix* getEdgeAttributeMatrix() const;

  /**
   * @brief Returns reference to the Attribute Matrix that holds data assigned to each edge
   * @return
   */
  AttributeMatrix& getEdgeAttributeMatrixRef();

  /**
   * @brief Returns reference to the Attribute Matrix that holds data assigned to each edge
   * @return
   */
  const AttributeMatrix& getEdgeAttributeMatrixRef() const;

  /**
   * @brief Returns the DataPath to the AttributeMatrix for the edge data
   * @return
   */
  DataPath getEdgeAttributeMatrixDataPath() const;

  /**
   * @brief Sets the Attribute Matrix for the data assigned to the edges
   * @param attributeMatrix
   */
  void setEdgeAttributeMatrix(const AttributeMatrix& attributeMatrix);

  /**
   * @brief Reads values from HDF5
   * @param groupReader
   * @return H5::ErrorType
   */
  H5::ErrorType readHdf5(H5::DataStructureReader& dataStructureReader, const H5::GroupReader& groupReader, bool preflight) override;

  /**
   * @brief Writes the geometry to HDF5 using the provided parent group ID.
   * @param dataStructureWriter
   * @param parentGroupWriter
   * @param importable
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const override;

protected:
  INodeGeometry1D(DataStructure& ds, std::string name);

  INodeGeometry1D(DataStructure& ds, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the complex::DataStructure object.
   */
  std::optional<IdType> m_EdgeDataArrayId;
  std::optional<IdType> m_EdgeAttributeMatrixId;
  std::optional<IdType> m_CellContainingVertDataArrayId;
  std::optional<IdType> m_CellNeighborsDataArrayId;
  std::optional<IdType> m_CellCentroidsDataArrayId;
};
} // namespace complex
