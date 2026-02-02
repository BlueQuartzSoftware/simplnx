#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class QuadGeom
 * @brief Represents a 2D quadrilateral geometry consisting of vertices connected by quadrilateral faces.
 */
class SIMPLNX_EXPORT QuadGeom : public INodeGeometry2D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumEdgeVerts = 2;
  static inline constexpr usize k_NumFaceVerts = 4;
  static inline constexpr usize k_NumVerts = 4;
  static inline constexpr StringLiteral k_VoxelSizes = "Quad Areas";
  static inline constexpr StringLiteral k_EltsContainingVert = "Quads Containing Vert";
  static inline constexpr StringLiteral k_EltNeighbors = "Quad Neighbors";
  static inline constexpr StringLiteral k_EltCentroids = "Quad Centroids";
  static inline constexpr StringLiteral k_TypeName = "QuadGeom";
  /**
   * @brief Creates a new QuadGeom in the specified DataStructure.
   * @param dataStructure The DataStructure to create the QuadGeom in
   * @param name The name for the new QuadGeom
   * @param parentId Optional parent object ID to insert the QuadGeom under
   * @return QuadGeom* Pointer to the created QuadGeom, or nullptr if creation failed
   */
  static QuadGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Imports a QuadGeom into the specified DataStructure with a given import ID.
   * @param dataStructure The DataStructure to import the QuadGeom into
   * @param name The name for the imported QuadGeom
   * @param importId The ID to use for this imported object
   * @param parentId Optional parent object ID to insert the QuadGeom under
   * @return QuadGeom* Pointer to the imported QuadGeom, or nullptr if import failed
   */
  static QuadGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Copy constructor.
   * @param other The QuadGeom to copy from
   */
  QuadGeom(const QuadGeom& other) = default;

  /**
   * @brief Move constructor.
   * @param other The QuadGeom to move from
   */
  QuadGeom(QuadGeom&& other) = default;

  ~QuadGeom() noexcept override = default;

  QuadGeom& operator=(const QuadGeom&) = delete;
  QuadGeom& operator=(QuadGeom&&) noexcept = delete;

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
   * @brief Creates a shallow copy of this QuadGeom.
   * @return DataObject* Pointer to the shallow copy
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Creates a deep copy of this QuadGeom at the specified path.
   * @param copyPath The path where the deep copy should be created
   * @return std::shared_ptr<DataObject> Shared pointer to the deep copy
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Returns the number of vertices per edge (always 2 for QuadGeom).
   * @return usize The number of vertices per edge
   */
  usize getNumberOfVerticesPerEdge() const override;

  /**
   * @brief Returns the number of quadrilaterals in this geometry.
   * @return usize The number of cells
   */
  usize getNumberOfCells() const override;

  /**
   * @brief Returns the number of vertices per face (always 4 for QuadGeom).
   * @return usize The number of vertices per face
   */
  usize getNumberOfVerticesPerFace() const override;

  /**
   * @brief calculates the sizes of each quad in the geometry
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
   * @brief finds the neighbors of each quad in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementNeighbors(bool recalculate) override;

  /**
   * @brief calculates the centroid of each quad in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Element Centroids Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementCentroids(bool recalculate) override;

  /**
   * @brief Returns the parametric center of a quadrilateral element.
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
   * @brief finds the shared edges (no duplicates) of each quad in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when a
   * Shared Edge Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findEdges(bool recalculate) override;

  /**
   * @brief finds the edges (including duplicates) of each quad in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Unshared Edges Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findUnsharedEdges(bool recalculate) override;

protected:
  /**
   * @brief Constructs a QuadGeom with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  QuadGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs a QuadGeom with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  QuadGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
