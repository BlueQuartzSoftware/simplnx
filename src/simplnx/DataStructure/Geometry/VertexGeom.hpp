#pragma once

#include "simplnx/DataStructure/Geometry/AbstractNodeGeometry0D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class VertexGeom
 * @brief
 */
class SIMPLNX_EXPORT VertexGeom : public AbstractNodeGeometry0D
{
public:
  friend class DataStructure;

  static constexpr usize k_NumVerts = 1;

  static constexpr StringLiteral k_TypeName = "VertexGeom";

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param parentId = {}
   * @return VertexGeom*
   */
  static VertexGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   * @param parentId = {}
   * @return VertexGeom*
   */
  static VertexGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param other
   */
  VertexGeom(const VertexGeom& other);

  /**
   * @brief
   * @param other
   */
  VertexGeom(VertexGeom&& other);

  ~VertexGeom() noexcept override;

  VertexGeom& operator=(const VertexGeom&) = delete;
  VertexGeom& operator=(VertexGeom&&) noexcept = delete;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  AbstractGeometry::Type getGeomType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  AbstractDataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return
   */
  GroupType getGroupType() const override;

  /**
   * @brief Returns typename of the AbstractDataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief
   * @return AbstractDataObject*
   */
  AbstractDataObject* shallowCopy() override;

  /**
   * @brief
   * @return AbstractDataObject*
   */
  std::shared_ptr<AbstractDataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Points have no space, so this function stores `0`s in a new
   * or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Element Sizes Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementSizes(bool recalculate) override;

  /**
   * @brief
   * @return nx::core::Point3D<float64>
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief
   * @param pCoords
   * @param shape
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override;

protected:
  /**
   * @brief
   * @param dataStructure
   * @param name
   */
  VertexGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   */
  VertexGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
