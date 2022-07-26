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

  ~INodeGeometry0D() noexcept override = default;

  const std::optional<IdType>& getVertexListId() const;

  SharedVertexList* getVertices();

  const SharedVertexList* getVertices() const;

  SharedVertexList& getVerticesRef();

  const SharedVertexList& getVerticesRef() const;

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
   * @brief Gets the coordinates at the target vertex ID.
   * @param vertId
   * @return
   */
  virtual Point3D<float32> getCoords(usize vertId) const = 0;

  /**
   * @brief Sets the coordinates for the specified vertex ID.
   * @param vertId
   * @param coords
   */
  virtual void setCoords(usize vertId, const Point3D<float32>& coords) = 0;

  /**
   * @brief
   * @return
   */
  const std::optional<IdType>& getVertexDataId() const;

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
  INodeGeometry0D() = delete;

  INodeGeometry0D(DataStructure& ds, std::string name);

  INodeGeometry0D(DataStructure& ds, std::string name, IdType importId);

  INodeGeometry0D(const INodeGeometry0D&) = default;
  INodeGeometry0D(INodeGeometry0D&&) noexcept = default;

  INodeGeometry0D& operator=(const INodeGeometry0D&) = delete;
  INodeGeometry0D& operator=(INodeGeometry0D&&) noexcept = delete;

  /**
   * @brief Updates the array IDs. Should only be called by DataObject::checkUpdatedIds.
   * @param updatedIds
   */
  void checkUpdatedIdsImpl(const std::vector<std::pair<IdType, IdType>>& updatedIds) override;

  std::optional<IdType> m_VertexListId;
  std::optional<IdType> m_VertexDataId;
};
} // namespace complex
