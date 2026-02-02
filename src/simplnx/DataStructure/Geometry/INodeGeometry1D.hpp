#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT INodeGeometry1D : public INodeGeometry0D
{
public:
  static constexpr StringLiteral k_EdgeAttributeMatrixName = "Edge Data";
  static constexpr StringLiteral k_EdgeFeatureAttributeMatrix = "Edge Feature Data";
  static constexpr StringLiteral k_SharedEdgeListName = "Shared Edge List";
  static constexpr StringLiteral k_UnsharedEdgesListName = "Unshared Edge List";
  static constexpr StringLiteral k_UnsharedFacesListName = "Unshared Face List";

  static constexpr StringLiteral k_TypeName = "INodeGeometry1D";

  static constexpr usize k_NumEdgeVerts = 2;

  ~INodeGeometry1D() noexcept override = default;

  /**
   * @brief Returns a pointer to the edge list array.
   * @return SharedEdgeList* Pointer to the edge list, or nullptr if not available
   */
  SharedEdgeList* getEdges();

  /**
   * @brief Returns a const pointer to the edge list array.
   * @return const SharedEdgeList* Const pointer to the edge list, or nullptr if not available
   */
  const SharedEdgeList* getEdges() const;

  /**
   * @brief Returns a reference to the edge list array.
   * @return SharedEdgeList& Reference to the edge list
   */
  SharedEdgeList& getEdgesRef();

  /**
   * @brief Returns a const reference to the edge list array.
   * @return const SharedEdgeList& Const reference to the edge list
   */
  const SharedEdgeList& getEdgesRef() const;

  /**
   * @brief Sets the internal reference to the edge list.
   * @param edges The edge list array to use
   */
  void setEdgeList(const SharedEdgeList& edges);

  /**
   * @brief Returns the optional ID of the edge list array.
   * @return std::optional<IdType> The edge list ID if it exists
   */
  std::optional<IdType> getEdgeListId() const;

  /**
   * @brief Sets the ID of the edge list array.
   * @param edgeList The optional ID of the edge list array
   */
  void setEdgeListId(const std::optional<IdType>& edgeList);

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
   * @brief Returns the number of vertices that make up each edge.
   * @return usize The number of vertices per edge
   */
  virtual usize getNumberOfVerticesPerEdge() const = 0;

  /**
   * @brief Sets the vertex IDs making up the specified edge. This method does
   * nothing if the edge list could not be found.
   * @param edgeId The index of the edge to modify
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void setEdgePointIds(usize edgeId, nonstd::span<usize> vertexIds);

  /**
   * @brief Returns the vertices that make up the specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId The index of the edge to query
   * @param vertexIds The index into the shared vertex list of each vertex
   */
  void getEdgePointIds(usize edgeId, nonstd::span<usize> vertexIds) const;

  /**
   * @brief Returns the vertex coordinates for a specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId The index of the edge to query
   * @param coords Output span to store the vertex coordinates
   */
  void getEdgeCoordinates(usize edgeId, nonstd::span<Point3Df> coords) const;

  /**
   * @brief Pure-Virtual
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  virtual Result<> findElementsContainingVert(bool recalculate) = 0;

  /**
   * @brief Returns a pointer to the array of elements containing each vertex.
   * @return const ElementDynamicList* Pointer to the elements containing vertices array, or nullptr if not available
   */
  const ElementDynamicList* getElementsContainingVert() const;

  /**
   * @brief Deletes the elements containing vertices array from the geometry.
   */
  void deleteElementsContainingVert();

  /**
   * @brief Pure-Virtual intended to find the neighbors of each element
   * in the geometry and store it in a new or existing array in the datastructure
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  virtual Result<> findElementNeighbors(bool recalculate) = 0;

  /**
   * @brief Returns a pointer to the element neighbors array.
   * @return const ElementDynamicList* Pointer to the element neighbors array, or nullptr if not available
   */
  const ElementDynamicList* getElementNeighbors() const;

  /**
   * @brief Deletes the element neighbors array from the geometry.
   */
  void deleteElementNeighbors();

  /**
   * @brief Pure-Virtual intended to calculate the centroids of each element
   * in the geometry and store it in a new or existing array in the datastructure
   * @param recalculate This will allow for skipping execution when an Element Centroids
   * Array exists and recalculate is `false`
   * @return Result<>
   */
  virtual Result<> findElementCentroids(bool recalculate) = 0;

  /**
   * @brief Returns a pointer to the element centroids array.
   * @return const Float32Array* Pointer to the element centroids array, or nullptr if not available
   */
  const Float32Array* getElementCentroids() const;

  /**
   * @brief Deletes the element centroids array from the geometry.
   */
  void deleteElementCentroids();

  /****************************************************************************
   * These functions get values related to where the Vertex Coordinates are
   * stored in the DataStructure
   */

  /**
   * @brief Returns the DataStructure unique ID of the edge list array.
   * @return const std::optional<IdType>& The edge list array ID if it exists
   */
  const std::optional<IdType>& getEdgeListDataArrayId() const;

  /**
   * @brief Returns the DataStructure unique ID of the edge AttributeMatrix.
   * @return const std::optional<IdType>& The edge AttributeMatrix ID if it exists
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
   * @brief Sets the ID of the edge data AttributeMatrix.
   * @param edgeDataId The optional ID of the edge data AttributeMatrix
   */
  void setEdgeDataId(const std::optional<IdType>& edgeDataId);

  /**
   * @brief Returns the optional ID of the elements containing vertices array.
   * @return std::optional<IdType> The ID if it exists
   */
  std::optional<IdType> getElementContainingVertId() const;

  /**
   * @brief Returns the optional ID of the element neighbors array.
   * @return std::optional<IdType> The ID if it exists
   */
  std::optional<IdType> getElementNeighborsId() const;

  /**
   * @brief Returns the optional ID of the element centroids array.
   * @return std::optional<IdType> The ID if it exists
   */
  std::optional<IdType> getElementCentroidsId() const;

  /**
   * @brief Sets the ID of the elements containing vertices array.
   * @param elementsContainingVertId The optional ID of the elements containing vertices array
   */
  void setElementContainingVertId(const std::optional<IdType>& elementsContainingVertId);

  /**
   * @brief Sets the ID of the element neighbors array.
   * @param elementNeighborsId The optional ID of the element neighbors array
   */
  void setElementNeighborsId(const std::optional<IdType>& elementNeighborsId);

  /**
   * @brief Sets the ID of the element centroids array.
   * @param centroidsId The optional ID of the element centroids array
   */
  void setElementCentroidsId(const std::optional<IdType>& centroidsId);

  /**
   * @brief validates that linkages between shared node lists and their associated Attribute Matrix is correct.
   * @return A Result<> object possibly with error code and message.
   */
  Result<> validate() const override;

protected:
  /**
   * @brief Constructs an INodeGeometry1D with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  INodeGeometry1D(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs an INodeGeometry1D with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  INodeGeometry1D(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIdsMap
   */
  void checkUpdatedIdsImpl(const std::unordered_map<DataObject::IdType, DataObject::IdType>& updatedIdsMap) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the DataStructure object.
   */
  std::optional<IdType> m_EdgeDataArrayId;
  std::optional<IdType> m_EdgeAttributeMatrixId;
  std::optional<IdType> m_CellContainingVertDataArrayId;
  std::optional<IdType> m_CellNeighborsDataArrayId;
  std::optional<IdType> m_CellCentroidsDataArrayId;
};
} // namespace nx::core
