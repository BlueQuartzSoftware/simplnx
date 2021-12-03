
#include "LinkedGeometryData.hpp"

using namespace complex;

LinkedGeometryData::LinkedGeometryData() = default;

LinkedGeometryData::~LinkedGeometryData() = default;

void LinkedGeometryData::addVertexData(DataObject::IdType dataId, LinkedGeometryData::IDataArrayShrdPtrType& dataArray)
{
  m_VertexData[dataId] = dataArray;
}
void LinkedGeometryData::removeVertexData(DataObject::IdType dataId)
{
  m_VertexData.erase(dataId);
}
void LinkedGeometryData::clearVertexData()
{
  m_VertexData.clear();
}
LinkedGeometryData::IDataArrayShrdPtrType LinkedGeometryData::getVertexDataArray(DataObject::IdType dataArrayId)
{
  return (m_VertexData[dataArrayId].expired() ? nullptr : m_VertexData[dataArrayId].lock());
}
std::vector<LinkedGeometryData::IDataArrayShrdPtrType> LinkedGeometryData::getVertexData(bool onlyValid)
{
  std::vector<LinkedGeometryData::IDataArrayShrdPtrType> dataArrays;
  for(auto& keyValue : m_VertexData)
  {
    IDataArrayWeakPtrType dataArrayWeakPtr = keyValue.second;
    if(onlyValid && !dataArrayWeakPtr.expired())
    {
      dataArrays.push_back(dataArrayWeakPtr.lock());
    }
    else
    {
      dataArrays.push_back((dataArrayWeakPtr.expired() ? nullptr : dataArrayWeakPtr.lock()));
    }
  }
  return dataArrays;
}

void LinkedGeometryData::addEdgeData(DataObject::IdType dataId, LinkedGeometryData::IDataArrayShrdPtrType& dataArray)
{
  m_EdgeData[dataId] = dataArray;
}
void LinkedGeometryData::removeEdgeData(DataObject::IdType dataId)
{
  m_EdgeData.erase(dataId);
}
void LinkedGeometryData::clearEdgeData()
{
  m_EdgeData.clear();
}
LinkedGeometryData::IDataArrayShrdPtrType LinkedGeometryData::getEdgeDataArray(DataObject::IdType dataArrayId)
{
  return (m_EdgeData[dataArrayId].expired() ? nullptr : m_EdgeData[dataArrayId].lock());
}
std::vector<LinkedGeometryData::IDataArrayShrdPtrType> LinkedGeometryData::getEdgeData(bool onlyValid)
{
  std::vector<LinkedGeometryData::IDataArrayShrdPtrType> dataArrays;
  for(auto& keyValue : m_EdgeData)
  {
    IDataArrayWeakPtrType dataArrayWeakPtr = keyValue.second;
    if(onlyValid && !dataArrayWeakPtr.expired())
    {
      dataArrays.push_back(dataArrayWeakPtr.lock());
    }
    else
    {
      dataArrays.push_back((dataArrayWeakPtr.expired() ? nullptr : dataArrayWeakPtr.lock()));
    }
  }
  return dataArrays;
}

void LinkedGeometryData::addFaceData(DataObject::IdType dataId, LinkedGeometryData::IDataArrayShrdPtrType& dataArray)
{
  m_FaceData[dataId] = dataArray;
}
void LinkedGeometryData::removeFaceData(DataObject::IdType dataId)
{
  m_FaceData.erase(dataId);
}

void LinkedGeometryData::clearFaceData()
{
  m_FaceData.clear();
}
LinkedGeometryData::IDataArrayShrdPtrType LinkedGeometryData::getFaceDataArray(DataObject::IdType dataArrayId)
{
  return (m_FaceData[dataArrayId].expired() ? nullptr : m_FaceData[dataArrayId].lock());
}
std::vector<LinkedGeometryData::IDataArrayShrdPtrType> LinkedGeometryData::getFaceData(bool onlyValid)
{
  std::vector<LinkedGeometryData::IDataArrayShrdPtrType> dataArrays;
  for(auto& keyValue : m_FaceData)
  {
    IDataArrayWeakPtrType dataArrayWeakPtr = keyValue.second;
    if(onlyValid && !dataArrayWeakPtr.expired())
    {
      dataArrays.push_back(dataArrayWeakPtr.lock());
    }
    else
    {
      dataArrays.push_back((dataArrayWeakPtr.expired() ? nullptr : dataArrayWeakPtr.lock()));
    }
  }
  return dataArrays;
}

void LinkedGeometryData::addCellData(DataObject::IdType dataId, LinkedGeometryData::IDataArrayShrdPtrType& dataArray)
{
  m_CellData[dataId] = dataArray;
}
void LinkedGeometryData::removeCellData(DataObject::IdType dataId)
{
  m_CellData.erase(dataId);
}

void LinkedGeometryData::clearCellData()
{
  m_CellData.clear();
}
LinkedGeometryData::IDataArrayShrdPtrType LinkedGeometryData::getCellDataArray(DataObject::IdType dataArrayId)
{
  return (m_CellData[dataArrayId].expired() ? nullptr : m_CellData[dataArrayId].lock());
}
std::vector<LinkedGeometryData::IDataArrayShrdPtrType> LinkedGeometryData::getCellData(bool onlyValid)
{
  std::vector<LinkedGeometryData::IDataArrayShrdPtrType> dataArrays;
  for(auto& keyValue : m_CellData)
  {
    IDataArrayWeakPtrType dataArrayWeakPtr = keyValue.second;
    if(onlyValid && !dataArrayWeakPtr.expired())
    {
      dataArrays.push_back(dataArrayWeakPtr.lock());
    }
    else
    {
      dataArrays.push_back((dataArrayWeakPtr.expired() ? nullptr : dataArrayWeakPtr.lock()));
    }
  }
  return dataArrays;
}
