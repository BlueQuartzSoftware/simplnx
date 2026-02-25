#pragma once

#include "simplnx/DataStructure/Geometry/INodeGeometry3D.hpp"
#include "simplnx/simplnx_export.hpp"

namespace nx::core
{
/**
 * @class HexahedralGeom
 * @brief Represents a 3D hexahedral geometry consisting of vertices connected by hexahedral elements.
 */
class SIMPLNX_EXPORT HexahedralGeom : public INodeGeometry3D
{
public:
  friend class DataStructure;

  static inline constexpr usize k_NumEdgeVerts = 2;
  static inline constexpr usize k_NumFaceVerts = 4;
  static inline constexpr usize k_NumVerts = 8;
  static inline constexpr StringLiteral k_VoxelSizes = "Hex Volumes";
  static inline constexpr StringLiteral k_EltsContainingVert = "Hex Containing Vertices";
  static inline constexpr StringLiteral k_EltNeighbors = "Hex Neighbors";
  static inline constexpr StringLiteral k_EltCentroids = "Hex Centroids";
  static inline constexpr StringLiteral k_TypeName = "HexahedralGeom";

  /**
   * @brief Creates a new HexahedralGeom in the specified DataStructure.
   * @param dataStructure The DataStructure to create the HexahedralGeom in
   * @param name The name for the new HexahedralGeom
   * @param parentId Optional parent object ID to insert the HexahedralGeom under
   * @return HexahedralGeom* Pointer to the created HexahedralGeom, or nullptr if creation failed
   */
  static HexahedralGeom* Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Imports a HexahedralGeom into the specified DataStructure with a given import ID.
   * @param dataStructure The DataStructure to import the HexahedralGeom into
   * @param name The name for the imported HexahedralGeom
   * @param importId The ID to use for this imported object
   * @param parentId Optional parent object ID to insert the HexahedralGeom under
   * @return HexahedralGeom* Pointer to the imported HexahedralGeom, or nullptr if import failed
   */
  static HexahedralGeom* Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId = {});

  /**
   * @brief Copy constructor.
   * @param other The HexahedralGeom to copy from
   */
  HexahedralGeom(const HexahedralGeom& other) = default;

  /**
   * @brief Move constructor.
   * @param other The HexahedralGeom to move from
   */
  HexahedralGeom(HexahedralGeom&& other) = default;

  ~HexahedralGeom() noexcept override = default;

  HexahedralGeom& operator=(const HexahedralGeom&) = delete;
  HexahedralGeom& operator=(HexahedralGeom&&) noexcept = delete;

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
   * @brief Creates a shallow copy of this HexahedralGeom.
   * @return DataObject* Pointer to the shallow copy
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Creates a deep copy of this HexahedralGeom at the specified path.
   * @param copyPath The path where the deep copy should be created
   * @return std::shared_ptr<DataObject> Shared pointer to the deep copy
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Returns the number of vertices per edge (always 2 for HexahedralGeom).
   * @return usize The number of vertices per edge
   */
  usize getNumberOfVerticesPerEdge() const override;

  /**
   * @brief Returns the number of vertices per face (always 4 for HexahedralGeom).
   * @return usize The number of vertices per face
   */
  usize getNumberOfVerticesPerFace() const override;

  /**
   * @brief Returns the number of vertices per cell (always 8 for HexahedralGeom).
   * @return usize The number of vertices per cell
   */
  usize getNumberOfVerticesPerCell() const override;

  /**
   * @brief Returns the number of hexahedrons in this geometry
   * @return usize
   */
  usize getNumberOfCells() const override;

  /**
   * @brief calculates the sizes of each hexahedron in the geometry
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
   * @brief finds the neighbors of each hexahedron in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * `ElementDynamicList` exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementNeighbors(bool recalculate) override;

  /**
   * @brief calculates the centroid of each hexahedron in the geometry
   * and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Element Centroids Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findElementCentroids(bool recalculate) override;

  /**
   * @brief Returns the parametric center of a hexahedral element.
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
   * @brief finds the shared edges (no duplicates) of each hexahedron in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when a
   * Shared Edge Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findEdges(bool recalculate) override;

  /**
   * @brief finds the shared faces (no duplicates) of each hexahedron in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when a
   * Shared Faces Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findFaces(bool recalculate) override;

  /**
   * @brief finds the edges (including duplicates) of each hexahedron in the
   * geometry and stores it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Unshared Edges Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findUnsharedEdges(bool recalculate) override;

  /**
   * @brief finds the faces (including duplicates) of each hexahedron in the
   * geometry and store it in a new or existing array in the DataStructure
   * @param recalculate This will allow for skipping execution when an
   * Unshared Faces Array exists and recalculate is `false`
   * @return Result<>
   */
  Result<> findUnsharedFaces(bool recalculate) override;

protected:
  /**
   * @brief Constructs a HexahedralGeom with the specified name.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   */
  HexahedralGeom(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs a HexahedralGeom with the specified name and import ID.
   * @param dataStructure The DataStructure this geometry belongs to
   * @param name The name for this geometry
   * @param importId The ID to use for this imported object
   */
  HexahedralGeom(DataStructure& dataStructure, std::string name, IdType importId);
};
} // namespace nx::core
