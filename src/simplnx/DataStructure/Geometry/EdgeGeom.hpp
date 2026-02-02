#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry1D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class EdgeGeom
 * @brief Represents a 1D edge geometry consisting of vertices connected by edges.
 */
class SIMPLNX_EXPORT EdgeGeom : public INodeGeometry1D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumVerts = 2;
  static inline constexpr usize k_NumEdgeVerts = 2;
  static inline constexpr StringLiteral k_VoxelSizes = "Edge Lengths";
  static inline constexpr StringLiteral k_EltsContainingVert = "Edges Containing Vert";
  static inline constexpr StringLiteral k_EltNeighbors = "Edge Neighbors";
  static inline constexpr StringLiteral k_EltCentroids = "Edge Centroids";
  static inline constexpr StringLiteral k_TypeName = "EdgeGeom";

  /**
   * @brief Creates a new EdgeGeom in the specified DataStructure.
   * @param dataStructure The DataStructure to create the EdgeGeom in
   * @param name The name for the new EdgeGeom
   * @param parentId Optional parent object ID to insert the EdgeGeom under
   * @return EdgeGeom* Pointer to the created EdgeGeom, or nullptr if creation failed
   */
  static EdgeGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Imports an EdgeGeom into the specified DataStructure with a given import ID.
   * @param dataStructure The DataStructure to import the EdgeGeom into
   * @param name The name for the imported EdgeGeom
   * @param importId The ID to use for this imported object
   * @param parentId Optional parent object ID to insert the EdgeGeom under
   * @return EdgeGeom* Pointer to the imported EdgeGeom, or nullptr if import failed
   */
  static EdgeGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Copy constructor.
   * @param other The EdgeGeom to copy from
   */
  EdgeGeom(const EdgeGeom& other) = default;

  /**
   * @brief Move constructor.
   * @param other The EdgeGeom to move from
   */
  EdgeGeom(EdgeGeom&& other) = default;

  ~EdgeGeom() noexcept override = default;

  EdgeGeom& operator=(const EdgeGeom&) = delete;
  EdgeGeom& operator=(EdgeGeom&&) noexcept = delete;

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
   * @brief Creates a shallow copy of this EdgeGeom.
   * @return DataObject* Pointer to the shallow copy
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Creates a deep copy of this EdgeGeom at the specified path.
   * @param copyPath The path where the deep copy should be created
   * @return std::shared_ptr<DataObject> Shared pointer to the deep copy
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Returns the number of vertices per edge (always 2 for EdgeGeom).
   * @return usize The number of vertices per edge
   */
  usize getNumberOfVerticesPerEdge() const override;

  /**
   * @brief calculates the sizes of each edge in the geometry
   * and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when
   * an Element Sizes Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementSizes(bool recalculate) override;

  /**
   * @brief
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementsContainingVert(bool recalculate) override;

  /**
   * @brief finds the neighbors of each edge in the geometry
   * and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementNeighbors(bool recalculate) override;

  /**
   * @brief calculates the centroids of each edge in the geometry
   * and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an Element
   * Centroids Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementCentroids(bool recalculate) override;

  /**
   * @brief Returns the parametric center of an edge element (0.5 along the edge).
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
   * @brief Constructs an EdgeGeom with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  EdgeGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs an EdgeGeom with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  EdgeGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
