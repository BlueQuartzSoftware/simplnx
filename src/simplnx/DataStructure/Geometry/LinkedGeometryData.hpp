#pragma once

#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/simplnx_export.hpp"

#include <set>

namespace nx::core
{

/**
 * @brief This class stores the DataPaths DataArrays that are associated with specific
 * types of a geometry, i.e, the Vertex(Point) data, the Edge Data, the Cell(Face) data
 * or the Volume Data.
 *
 * Vertex Data is data that goes on 3D Points or Vertices
 * Edge Data is data that goes on a 3D Edge where an Edge is defined as the line
 * that connects 2 specific 3DPoints in space.
 * Face Data is data that goes on a "Face" such as a Triangle or a Quad. Note that VTK
 * will refer to this as "Cell" data.
 * Cell Data is data that is associated with a 3D Volume of space such as a voxel from
 * an ImageGeom/RectilinearGridGeom or the space of a Hexahedral voxel or Tetrahedral voxel
 *
 * There are functions to add, remove, clear and get a specific set. The DataPath(s) are
 * stored in a std::set so there can only be a single instance in the list of the
 * same exact path.
 */
class SIMPLNX_EXPORT LinkedGeometryData
{
public:
  LinkedGeometryData();
  ~LinkedGeometryData();

  LinkedGeometryData(const LinkedGeometryData&) = default;
  LinkedGeometryData(LinkedGeometryData&&) noexcept = default;

  LinkedGeometryData& operator=(const LinkedGeometryData&) = default;
  LinkedGeometryData& operator=(LinkedGeometryData&&) noexcept = default;

  using IDataArrayShrdPtrType = std::shared_ptr<IDataArray>;
  using IDataArrayWeakPtrType = std::weak_ptr<IDataArray>;

  void addVertexData(const DataPath& dataPath);
  void removeVertexData(const DataPath& dataPath);
  void clearVertexData();
  const std::set<DataPath>& getVertexDataPaths() const;

  void addEdgeData(const DataPath& dataPath);
  void removeEdgeData(const DataPath& dataPath);
  void clearEdgeData();
  const std::set<DataPath>& getEdgeDataPaths() const;

  void addFaceData(const DataPath& dataPath);
  void removeFaceData(const DataPath& dataPath);
  void clearFaceData();
  const std::set<DataPath>& getFaceDataPaths() const;

  void addCellData(const DataPath& dataPath);
  void removeCellData(const DataPath& dataPath);
  void clearCellData();
  const std::set<DataPath>& getCellDataPaths() const;

protected:
private:
  std::set<DataPath> m_VertexData;
  std::set<DataPath> m_EdgeData;
  std::set<DataPath> m_FaceData;
  std::set<DataPath> m_CellData;
};

} // namespace nx::core
