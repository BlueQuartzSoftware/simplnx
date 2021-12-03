#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/complex_export.hpp"

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

  LinkedGeometryData(const LinkedGeometryData&) = delete;
  LinkedGeometryData(LinkedGeometryData&&) noexcept = delete;

  LinkedGeometryData& operator=(const LinkedGeometryData&) = delete;
  LinkedGeometryData& operator=(LinkedGeometryData&&) noexcept = delete;

  using IDataArrayShrdPtrType = std::shared_ptr<IDataArray>;
  using IDataArrayWeakPtrType = std::weak_ptr<IDataArray>;

  void addVertexData(DataObject::IdType dataId, IDataArrayShrdPtrType& dataArray);
  void removeVertexData(DataObject::IdType dataId);
  void clearVertexData();
  IDataArrayShrdPtrType getVertexDataArray(DataObject::IdType dataArrayId);
  std::vector<IDataArrayShrdPtrType> getVertexData(bool onlyValid = false);

  void addEdgeData(DataObject::IdType dataId, IDataArrayShrdPtrType& dataArray);
  void removeEdgeData(DataObject::IdType dataId);
  void clearEdgeData();
  IDataArrayShrdPtrType getEdgeDataArray(DataObject::IdType dataArrayId);
  std::vector<IDataArrayShrdPtrType> getEdgeData(bool onlyValid = false);

  void addFaceData(DataObject::IdType dataId, IDataArrayShrdPtrType& dataArray);
  void removeFaceData(DataObject::IdType dataId);
  void clearFaceData();
  IDataArrayShrdPtrType getFaceDataArray(DataObject::IdType dataArrayId);
  std::vector<IDataArrayShrdPtrType> getFaceData(bool onlyValid = false);

  void addCellData(DataObject::IdType dataId, IDataArrayShrdPtrType& dataArray);
  void removeCellData(DataObject::IdType dataId);
  void clearCellData();
  IDataArrayShrdPtrType getCellDataArray(DataObject::IdType dataArrayId);
  std::vector<IDataArrayShrdPtrType> getCellData(bool onlyValid = false);

protected:
private:
  std::map<DataObject::IdType, IDataArrayWeakPtrType> m_VertexData;
  std::map<DataObject::IdType, IDataArrayWeakPtrType> m_EdgeData;
  std::map<DataObject::IdType, IDataArrayWeakPtrType> m_FaceData;
  std::map<DataObject::IdType, IDataArrayWeakPtrType> m_CellData;
};

} // namespace complex
