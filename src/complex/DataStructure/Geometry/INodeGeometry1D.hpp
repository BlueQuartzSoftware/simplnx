#pragma once

#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry1D : public INodeGeometry0D
{
public:
  static inline constexpr StringLiteral k_EdgeDataName = "Edge Data";

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
  usize getNumberOfEdges() const;

  /**
   * @brief Returns the vertex coordinates for a specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param vert1
   * @param vert2
   */
  virtual void getVertCoordsAtEdge(usize edgeId, Point3D<float32>& vert1, Point3D<float32>& vert2) const = 0;

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
   * @brief Sets the vertex IDs making up the specified edge. This method does
   * nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  virtual void setVertsAtEdge(usize edgeId, const usize verts[2]) = 0;

  /**
   * @brief Returns the vertices that make up the specified edge by reference.
   * This method does nothing if the edge list could not be found.
   * @param edgeId
   * @param verts
   */
  virtual void getVertsAtEdge(usize edgeId, usize verts[2]) const = 0;

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
  INodeGeometry1D() = delete;

  INodeGeometry1D(DataStructure& ds, std::string name);

  INodeGeometry1D(DataStructure& ds, std::string name, IdType importId);

  INodeGeometry1D(const INodeGeometry1D&) = default;
  INodeGeometry1D(INodeGeometry1D&&) noexcept = default;

  INodeGeometry1D& operator=(const INodeGeometry1D&) = delete;
  INodeGeometry1D& operator=(INodeGeometry1D&&) noexcept = delete;

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  std::optional<IdType> m_EdgeListId;
  std::optional<IdType> m_EdgeDataId;
  std::optional<IdType> m_ElementContainingVertId;
  std::optional<IdType> m_ElementNeighborsId;
  std::optional<IdType> m_ElementCentroidsId;
};
} // namespace complex
