#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class VertexGeom
 * @brief Represents a 0D vertex (point cloud) geometry consisting of individual vertices.
 */
class SIMPLNX_EXPORT VertexGeom : public INodeGeometry0D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumVerts = 1;

  static inline constexpr StringLiteral k_TypeName = "VertexGeom";

  /**
   * @brief Creates a new VertexGeom in the specified DataStructure.
   * @param dataStructure The DataStructure to create the VertexGeom in
   * @param name The name for the new VertexGeom
   * @param parentId Optional parent object ID to insert the VertexGeom under
   * @return VertexGeom* Pointer to the created VertexGeom, or nullptr if creation failed
   */
  static VertexGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Imports a VertexGeom into the specified DataStructure with a given import ID.
   * @param dataStructure The DataStructure to import the VertexGeom into
   * @param name The name for the imported VertexGeom
   * @param importId The ID to use for this imported object
   * @param parentId Optional parent object ID to insert the VertexGeom under
   * @return VertexGeom* Pointer to the imported VertexGeom, or nullptr if import failed
   */
  static VertexGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Copy constructor.
   * @param other The VertexGeom to copy from
   */
  VertexGeom(const VertexGeom& other);

  /**
   * @brief Move constructor.
   * @param other The VertexGeom to move from
   */
  VertexGeom(VertexGeom&& other);

  ~VertexGeom() noexcept override;

  VertexGeom& operator=(const VertexGeom&) = delete;
  VertexGeom& operator=(VertexGeom&&) noexcept = delete;

  /**
   * @brief Returns the type of geometry.
   * @return
   */
  IGeometry::Type getGeomType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass GroupType. Used for quick comparison or type deduction
   * @return
   */
  GroupType getGroupType() const override;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief Creates a shallow copy of this VertexGeom.
   * @return DataObject* Pointer to the shallow copy
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Creates a deep copy of this VertexGeom at the specified path.
   * @param copyPath The path where the deep copy should be created
   * @return std::shared_ptr<DataObject> Shared pointer to the deep copy
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Points have no space, so this function stores `0`s in a new
   * or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Element Sizes Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementSizes(bool recalculate) override;

  /**
   * @brief Returns the parametric center of a vertex element (always at the vertex position).
   * @return Point3D<float64> The parametric center coordinates
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief Calculates shape functions at the given parametric coordinates.
   * @param pCoords The parametric coordinates
   * @param shape Output array to store the calculated shape function values
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override;

protected:
  /**
   * @brief Constructs a VertexGeom with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  VertexGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs a VertexGeom with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  VertexGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
