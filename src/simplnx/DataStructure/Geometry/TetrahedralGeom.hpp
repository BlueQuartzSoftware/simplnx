#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class TetrahedralGeom
 * @brief Represents a 3D tetrahedral geometry consisting of vertices connected by tetrahedral elements.
 */
class SIMPLNX_EXPORT TetrahedralGeom : public INodeGeometry3D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumEdgeVerts = 2;
  static inline constexpr usize k_NumFaceVerts = 3;
  static inline constexpr usize k_NumVerts = 4;
  static inline constexpr StringLiteral k_VoxelSizes = "Tet Volumes";
  static inline constexpr StringLiteral k_EltsContainingVert = "Elements Containing Vert";
  static inline constexpr StringLiteral k_EltNeighbors = "Tet Neighbors";
  static inline constexpr StringLiteral k_EltCentroids = "Tet Centroids";
  static inline constexpr StringLiteral k_TypeName = "TetrahedralGeom";

  /**
   * @brief Creates a new TetrahedralGeom in the specified DataStructure.
   * @param dataStructure The DataStructure to create the TetrahedralGeom in
   * @param name The name for the new TetrahedralGeom
   * @param parentId Optional parent object ID to insert the TetrahedralGeom under
   * @return TetrahedralGeom* Pointer to the created TetrahedralGeom, or nullptr if creation failed
   */
  static TetrahedralGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Imports a TetrahedralGeom into the specified DataStructure with a given import ID.
   * @param dataStructure The DataStructure to import the TetrahedralGeom into
   * @param name The name for the imported TetrahedralGeom
   * @param importId The ID to use for this imported object
   * @param parentId Optional parent object ID to insert the TetrahedralGeom under
   * @return TetrahedralGeom* Pointer to the imported TetrahedralGeom, or nullptr if import failed
   */
  static TetrahedralGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Copy constructor.
   * @param other The TetrahedralGeom to copy from
   */
  TetrahedralGeom(const TetrahedralGeom& other) = default;

  /**
   * @brief Move constructor.
   * @param other The TetrahedralGeom to move from
   */
  TetrahedralGeom(TetrahedralGeom&& other) = default;

  ~TetrahedralGeom() noexcept override = default;

  TetrahedralGeom& operator=(const TetrahedralGeom&) = delete;
  TetrahedralGeom& operator=(TetrahedralGeom&&) noexcept = delete;

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
   * @brief Creates a shallow copy of this TetrahedralGeom.
   * @return DataObject* Pointer to the shallow copy
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Creates a deep copy of this TetrahedralGeom at the specified path.
   * @param copyPath The path where the deep copy should be created
   * @return std::shared_ptr<DataObject> Shared pointer to the deep copy
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Returns the number of vertices per edge (always 2 for TetrahedralGeom).
   * @return usize The number of vertices per edge
   */
  usize getNumberOfVerticesPerEdge() const override;

  /**
   * @brief Returns the number of vertices per face (always 3 for TetrahedralGeom).
   * @return usize The number of vertices per face
   */
  usize getNumberOfVerticesPerFace() const override;

  /**
   * @brief Returns the number of vertices per cell (always 4 for TetrahedralGeom).
   * @return usize The number of vertices per cell
   */
  usize getNumberOfVerticesPerCell() const override;

  /**
   * @brief Returns the number of tetrahedrons in this geometry
   * @return usize
   */
  usize getNumberOfCells() const override;

  /**
   * @brief calculates the sizes of each tetrahedron in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Element Sizes Array exists and recalculate is `false`
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
   * @brief finds the neighbors of each tetrahedron in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementNeighbors(bool recalculate) override;

  /**
   * @brief calculates the centroid of each tetrahedron in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Element Centroids Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementCentroids(bool recalculate) override;

  /**
   * @brief Returns the parametric center of a tetrahedral element.
   * @return Point3D<float64> The parametric center coordinates
   */
  Point3D<float64> getParametricCenter() const override;

  /**
   * @brief Calculates shape functions at the given parametric coordinates.
   * @param pCoords The parametric coordinates
   * @param shape Output array to store the calculated shape function values
   */
  void getShapeFunctions(const Point3D<float64>& pCoords, float64* shape) const override;

  /**
   * @brief finds the shared edges (no duplicates) of each tetrahedron in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when a
   * Shared Edge Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findEdges(bool recalculate) override;

  /**
   * @brief finds the shared faces (no duplicates) of each tetrahedron in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when a
   * Shared Faces Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findFaces(bool recalculate) override;

  /**
   * @brief finds the edges (including duplicates) of each tetrahedron in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Unshared Edges Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findUnsharedEdges(bool recalculate) override;

  /**
   * @brief finds the faces (including duplicates) of each tetrahedron in the
   * geometry and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Unshared Faces Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findUnsharedFaces(bool recalculate) override;

protected:
  /**
   * @brief Constructs a TetrahedralGeom with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  TetrahedralGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs a TetrahedralGeom with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  TetrahedralGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
