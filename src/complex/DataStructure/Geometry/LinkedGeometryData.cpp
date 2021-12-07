
#include "LinkedGeometryData.hpp"

using namespace complex;

LinkedGeometryData::LinkedGeometryData() = default;

LinkedGeometryData::~LinkedGeometryData() = default;

void LinkedGeometryData::addVertexData(const DataPath& dataPath)
{
  m_VertexData.insert(dataPath);
}
void LinkedGeometryData::removeVertexData(const DataPath& dataPath)
{
  m_VertexData.erase(dataPath);
}
void LinkedGeometryData::clearVertexData()
{
  m_VertexData.clear();
}

std::set<DataPath> LinkedGeometryData::getVertexDataPaths()
{
  return m_VertexData;
}

void LinkedGeometryData::addEdgeData(const DataPath& dataPath)
{
  m_EdgeData.insert(dataPath);
}
void LinkedGeometryData::removeEdgeData(const DataPath& dataPath)
{
  m_EdgeData.erase(dataPath);
}
void LinkedGeometryData::clearEdgeData()
{
  m_EdgeData.clear();
}

std::set<DataPath> LinkedGeometryData::getEdgeDataPaths()
{
  return m_EdgeData;
}

void LinkedGeometryData::addFaceData(const DataPath& dataPath)
{
  m_FaceData.insert(dataPath);
}
void LinkedGeometryData::removeFaceData(const DataPath& dataPath)
{
  m_FaceData.erase(dataPath);
}

void LinkedGeometryData::clearFaceData()
{
  m_FaceData.clear();
}

std::set<DataPath> LinkedGeometryData::getFaceDataPaths()
{
  return m_FaceData;
}

void LinkedGeometryData::addCellData(const DataPath& dataPath)
{
  m_CellData.insert(dataPath);
}
void LinkedGeometryData::removeCellData(const DataPath& dataPath)
{
  m_CellData.erase(dataPath);
}

void LinkedGeometryData::clearCellData()
{
  m_CellData.clear();
}

std::set<DataPath> LinkedGeometryData::getCellDataPaths()
{
  return m_CellData;
}
