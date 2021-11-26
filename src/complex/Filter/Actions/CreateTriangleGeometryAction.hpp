#pragma once

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @brief Action for creating an ImageGeometry in a DataStructure
 */
class COMPLEX_EXPORT CreateTriangleGeometryAction : public IDataAction
{
public:
  using DimensionType = std::vector<size_t>;
  using OriginType = std::vector<float>;
  using SpacingType = std::vector<float>;

  CreateTriangleGeometryAction() = delete;

  CreateTriangleGeometryAction(DataPath geometryPath, AbstractGeometry::MeshIndexType numFaces, AbstractGeometry::MeshIndexType numVertices);

  ~CreateTriangleGeometryAction() noexcept override;

  CreateTriangleGeometryAction(const CreateTriangleGeometryAction&) = delete;
  CreateTriangleGeometryAction(CreateTriangleGeometryAction&&) noexcept = delete;
  CreateTriangleGeometryAction& operator=(const CreateTriangleGeometryAction&) = delete;
  CreateTriangleGeometryAction& operator=(CreateTriangleGeometryAction&&) noexcept = delete;

  /**
   * @brief Applies this action's change to the given DataStructure in the given mode.
   * Returns any warnings/errors. On error, DataStructure is not guaranteed to be consistent.
   * @param dataStructure
   * @return
   */
  Result<> apply(DataStructure& dataStructure, Mode mode) const override;

  /**
   * @brief Returns the path of the ImageGeometry to be created.
   * @return
   */
  const DataPath& path() const;

private:
  DataPath m_GeometryPath;
  AbstractGeometry::MeshIndexType m_NumFaces;
  AbstractGeometry::MeshIndexType m_NumVertices;
};
} // namespace complex
