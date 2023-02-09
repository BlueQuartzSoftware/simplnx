#pragma once

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry0D : public IGeometry
{
public:
  static inline constexpr StringLiteral k_VertexDataName = "Vertex Data";
  static inline constexpr StringLiteral k_TypeName = "INodeGeometry0D";

  INodeGeometry0D() = delete;
  INodeGeometry0D(const INodeGeometry0D&) = default;
  INodeGeometry0D(INodeGeometry0D&&) = default;

  INodeGeometry0D& operator=(const INodeGeometry0D&) = delete;
  INodeGeometry0D& operator=(INodeGeometry0D&&) noexcept = delete;

  ~INodeGeometry0D() noexcept override = default;

  using BoundingBox = std::pair<FloatVec3, FloatVec3>;

  /**
   * @brief Returns a shared pointer to the vertex coordinates array
   * @return
   */
  SharedVertexList* getVertices();

  /**
   * @brief Returns a shared pointer to the vertex coordinates array
   * @return
   */
  const SharedVertexList* getVertices() const;

  /**
   * @brief Returns a reference to the vertex coordinates array
   * @return
   */
  SharedVertexList& getVerticesRef();

  /**
   * @brief Returns a reference to the vertex coordinates array
   * @return
   */
  const SharedVertexList& getVerticesRef() const;

  /**
   * @brief Sets the internal reference to vertex coordinates to vertices
   * @param vertices The coordinate array that will now be used for the vertex coordinates
   */
  void setVertices(const SharedVertexList& vertices);

  /**
   * @brief Resizes the vertex list to the target size.
   * @param size
   */
  void resizeVertexList(usize size);

  /**
   * @brief Returns the number of vertices in the geometry.
   * @return usize
   */
  usize getNumberOfVertices() const;

  /**
   * @brief Returns the number of vertices for this geometry
   * @return
   */
  usize getNumberOfCells() const override;

  /**
   * @brief Calculates and returns the bounding box for this geometry
   * @return
   */
  std::optional<BoundingBox> getBoundingBox() const;

  /**
   * @brief Returns whether or not this geometry is in a YZ plane.
   * Returns empty if the vertex list does not exist.
   * @return
   */
  std::optional<bool> isYZPlane() const;

  /**
   * @brief Returns whether or not this geometry is in a XY plane
   * Returns empty if the vertex list does not exist.
   * @return
   */
  std::optional<bool> isXYPlane() const;

  /**
   * @brief Returns whether or not this geometry is in a XZ plane
   * Returns empty if the vertex list does not exist.
   * @return
   */
  std::optional<bool> isXZPlane() const;

  /**
   * @brief Gets the coordinates at the target vertex ID.
   * @param vertId
   * @return
   */
  Point3D<float32> getVertexCoordinate(usize vertId) const;

  /**
   * @brief Sets the coordinates for the specified vertex ID.
   * @param vertId
   * @param coords
   */
  void setVertexCoordinate(usize vertId, const Point3D<float32>& coords);

  /****************************************************************************
   * These functions get values related to where the Vertex Coordinates are
   * stored in the DataStructure
   */

  /**
   * @brief Returns the DataStructure unique ID of the vertex coordinate array
   * @return
   */
  const std::optional<IdType>& getSharedVertexDataArrayId() const;

  /**
   * @brief Returns the DataStructure unique ID of the Attribute Matrix that holds data assigned to each vertex coordinate
   * @return
   */
  const std::optional<IdType>& getVertexAttributeMatrixId() const;

  /**
   * @brief Returns pointer to the Attribute Matrix that holds data assigned to each vertex coordinate
   * @return
   */
  AttributeMatrix* getVertexAttributeMatrix();

  /**
   * @brief Returns pointer to the Attribute Matrix that holds data assigned to each vertex coordinate
   * @return
   */
  const AttributeMatrix* getVertexAttributeMatrix() const;

  /**
   * @brief Returns reference to the Attribute Matrix that holds data assigned to each vertex coordinate
   * @return
   */
  AttributeMatrix& getVertexAttributeMatrixRef();

  /**
   * @brief Returns reference to the Attribute Matrix that holds data assigned to each vertex coordinate
   * @return
   */
  const AttributeMatrix& getVertexAttributeMatrixRef() const;

  /**
   * @brief Returns the DataPath to the AttributeMatrix for the vertex data
   * @return
   */
  DataPath getVertexAttributeMatrixDataPath() const;

  /**
   * @brief Sets the Attribute Matrix for the data assigned to the vertex coordinates
   * @param attributeMatrix
   */
  void setVertexAttributeMatrix(const AttributeMatrix& attributeMatrix);

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
  INodeGeometry0D(DataStructure& dataStructure, std::string name);

  INodeGeometry0D(DataStructure& dataStructure, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the complex::DataStructure object.
   */
  std::optional<IdType> m_VertexDataArrayId;
  std::optional<IdType> m_VertexAttributeMatrixId;

private:
  /**
   * @brief Helper function that returns whether or not this geometry is in a plane
   * Returns empty if the vertex list or data store does not exist.
   * @param dimensionIndex The dimensional index to check for equality.
   * YZ plane --> dimensionIndex = 0
   * XZ plane --> dimensionIndex = 1
   * XY plane --> dimensionIndex = 2
   * @return
   */
  std::optional<bool> isPlane(usize dimensionIndex) const;
};
} // namespace complex
