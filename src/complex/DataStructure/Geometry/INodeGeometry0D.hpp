#pragma once

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"

namespace complex
{
class COMPLEX_EXPORT INodeGeometry0D : public IGeometry
{
public:
  static inline constexpr StringLiteral k_VertexDataName = "Vertex Data";

  INodeGeometry0D() = delete;
  INodeGeometry0D(const INodeGeometry0D&) = default;
  INodeGeometry0D(INodeGeometry0D&&) = default;

  INodeGeometry0D& operator=(const INodeGeometry0D&) = delete;
  INodeGeometry0D& operator=(INodeGeometry0D&&) noexcept = delete;

  ~INodeGeometry0D() noexcept override = default;

  const std::optional<IdType>& getVertexListId() const;

  SharedVertexList* getVertices();

  const SharedVertexList* getVertices() const;

  SharedVertexList& getVerticesRef();

  const SharedVertexList& getVerticesRef() const;

  void setVertices(const SharedVertexList& vertices);

  void setVertexListId(const std::optional<IdType>& vertices);

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

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getVertexDataId() const;

  void setVertexDataId(const OptionalId& vertexDataId);

  /**
   * @brief
   * @return
   */
  AttributeMatrix* getVertexData();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix* getVertexData() const;

  /**
   * @brief
   * @return
   */
  AttributeMatrix& getVertexDataRef();

  /**
   * @brief
   * @return
   */
  const AttributeMatrix& getVertexDataRef() const;

  /**
   * @brief
   * @return
   */
  DataPath getVertexDataPath() const;

  /**
   * @brief
   * @param attributeMatrix
   */
  void setVertexData(const AttributeMatrix& attributeMatrix);

protected:
  INodeGeometry0D(DataStructure& ds, std::string name);

  INodeGeometry0D(DataStructure& ds, std::string name, IdType importId);

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  /* ***************************************************************************
   * These variables are the Ids of the arrays from the complex::DataStructure object.
   */
  std::optional<IdType> m_VertexListId;
  std::optional<IdType> m_VertexDataId;
};
} // namespace complex
