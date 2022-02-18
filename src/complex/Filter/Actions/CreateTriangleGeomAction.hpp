#pragma once

#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for creating a TriangleGeom in a DataStructure
 */
class COMPLEX_EXPORT CreateTriangleGeomAction : public IDataAction
{
public:
  using ShapeType = std::vector<usize>;

  enum class AdditionalData
  {
    None = 0,
    Vertices = 1,
    Triangles = 2,
    VerticesTriangles = 3
  };

  static constexpr StringLiteral k_DefaultVerticesName = "Vertex Array";
  static constexpr StringLiteral k_DefaultFacesName = "Triangle Array";

  CreateTriangleGeomAction() = delete;

  CreateTriangleGeomAction(const DataPath& path, AdditionalData additional = AdditionalData::None);
  CreateTriangleGeomAction(const DataPath& path, const ShapeType& tupleSize, AdditionalData additional);

  ~CreateTriangleGeomAction() noexcept override;

  CreateTriangleGeomAction(const CreateTriangleGeomAction&) = delete;
  CreateTriangleGeomAction(CreateTriangleGeomAction&&) noexcept = delete;
  CreateTriangleGeomAction& operator=(const CreateTriangleGeomAction&) = delete;
  CreateTriangleGeomAction& operator=(CreateTriangleGeomAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return Result<>
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the path of the DataArray to be created.
   * @return DataPath
   */
  const DataPath& path() const;

  /**
   * @brief Returns the target tuple size.
   * @return ShapeType
   */
  ShapeType tupleSize() const;

  /**
   * @brief Returns true if the corresponding vertex array should be created.
   * Returns false otherwise.
   * @return bool
   */
  bool shouldCreateVertexArray() const;

  /**
   * @brief Returns true if the corresponding triangle array should be created.
   * Returns false otherwise.
   * @return bool
   */
  bool shouldCreateTriangleArray() const;

private:
  /**
   * @brief Attempts to create the target geometry and returns the result.
   * @param dataStructure
   * @param mode
   * @return Result<>
   */
  Result<> createGeometry(DataStructure& dataStructure, Mode mode) const;

  /**
   * @brief Attempts to create the vertex array and returns the result.
   * @param dataStructure
   * @param mode
   * @return Result<>
   */
  Result<> createVertexArray(DataStructure& dataStructure, Mode mode) const;

  /**
   * @brief Attempts to create the triangle array and returns the result.
   * @param dataStructure
   * @param mode
   * @return Result<>
   */
  Result<> createTriangleArray(DataStructure& dataStructure, Mode mode) const;

  DataPath m_Path;
  ShapeType m_TupleSize = {1};
  AdditionalData m_AdditionalData = AdditionalData::None;
};
} // namespace complex
