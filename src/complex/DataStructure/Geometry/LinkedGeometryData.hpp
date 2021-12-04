#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/complex_export.hpp"

#include <set>

namespace complex
{

/**
 *
 */
class COMPLEX_EXPORT LinkedGeometryData
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
  std::set<DataPath> getVertexDataPaths();

  void addEdgeData(const DataPath& dataPath);
  void removeEdgeData(const DataPath& dataPath);
  void clearEdgeData();
  std::set<DataPath> getEdgeDataPaths();

  void addFaceData(const DataPath& dataPath);
  void removeFaceData(const DataPath& dataPath);
  void clearFaceData();
  std::set<DataPath> getFaceDataPaths();

  void addCellData(const DataPath& dataPath);
  void removeCellData(const DataPath& dataPath);
  void clearCellData();
  std::set<DataPath> getCellDataPaths();

protected:
private:
  std::set<DataPath> m_VertexData;
  std::set<DataPath> m_EdgeData;
  std::set<DataPath> m_FaceData;
  std::set<DataPath> m_CellData;
};

} // namespace complex
