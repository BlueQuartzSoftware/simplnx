#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry1D : public INodeGeometry0D
{
public:
  static inline constexpr StringLiteral k_EdgeDataName = "Edge Data";

  static inline constexpr usize k_NumEdgeVerts = 2;

  INodeGeometry1D() = delete;
  INodeGeometry1D(const INodeGeometry1D&) = default;
  INodeGeometry1D(INodeGeometry1D&&) = default;

  INodeGeometry1D& operator=(const INodeGeometry1D&) = delete;
  INodeGeometry1D& operator=(INodeGeometry1D&&) noexcept = delete;

  ~INodeGeometry1D() noexcept override = default;

  const std::optional<IdType>& getEdgeListId() const;

  SharedEdgeList* getEdges();

  const SharedEdgeList* getEdges() const;

  SharedEdgeList& getEdgesRef();

  const SharedEdgeList& getEdgesRef() const;

  void setEdges(const SharedEdgeList& edges);

  /**
   * @brief Resizes the edge list to the target size.
   * @param size
   */
  void resizeEdgeList(usize size);

  /**
   * @brief Returns the number of edges in the geometry.
   * @return usize
   */
  virtual usize getNumberOfCells() const override;

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

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getEdgeDataId() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getEdgeData();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getEdgeData() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getEdgeDataRef();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getEdgeDataRef() const;

  /**
   * @brief
   * @return
   */
  DataPath getEdgeDataPath() const;

  /**
   * @brief
   * @param attributeMatrix
   */
  void setEdgeData(const AttributeMatrix& attributeMatrix);

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
  std::optional<IdType> m_EdgeListId;
  std::optional<IdType> m_EdgeDataId;
  std::optional<IdType> m_CellContainingVertId;
  std::optional<IdType> m_CellNeighborsId;
  std::optional<IdType> m_CellCentroidsId;
};
} // namespace complex
