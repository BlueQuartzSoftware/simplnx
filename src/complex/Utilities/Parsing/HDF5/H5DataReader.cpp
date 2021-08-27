#include "H5DataReader.hpp"

#include "complex/DataStructure/Factory/DataArrayFactory.hpp"
#include "complex/DataStructure/Factory/DataGroupFactory.hpp"
#include "complex/DataStructure/Factory/EdgeGeomFactory.hpp"
#include "complex/DataStructure/Factory/GridMontageFactory.hpp"
#include "complex/DataStructure/Factory/HexahedralGeomFactory.hpp"
#include "complex/DataStructure/Factory/ImageGeomFactory.hpp"
#include "complex/DataStructure/Factory/QuadGeomFactory.hpp"
#include "complex/DataStructure/Factory/RectGridGeomFactory.hpp"
#include "complex/DataStructure/Factory/ScalarDataFactory.hpp"
#include "complex/DataStructure/Factory/TetrahedralGeomFactory.hpp"
#include "complex/DataStructure/Factory/TriangleGeomFactory.hpp"
#include "complex/DataStructure/Factory/VertexGeomFactory.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Reader.hpp"
#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

using namespace complex;

H5DataReader::H5DataReader()
{
  addCoreFactories();
}

H5DataReader::~H5DataReader() = default;

void H5DataReader::addCoreFactories()
{
  addFactory(new DataArrayFactory());
  addFactory(new DataGroupFactory());
  addFactory(new EdgeGeomFactory());
  addFactory(new GridMontageFactory());
  addFactory(new HexahedralGeomFactory());
  addFactory(new ImageGeomFactory());
  addFactory(new QuadGeomFactory());
  addFactory(new RectGridGeomFactory());
  addFactory(new ScalarDataFactory());
  addFactory(new TetrahedralGeomFactory());
  addFactory(new TriangleGeomFactory());
  addFactory(new VertexGeomFactory());
}

void H5DataReader::addFactory(IH5DataFactory* factory)
{
  if(factory == nullptr)
  {
    return;
  }

  m_Factories[factory->getDataTypeName()] = std::shared_ptr<IH5DataFactory>(factory);
}

std::vector<IH5DataFactory*> H5DataReader::getFactories() const
{
  std::vector<IH5DataFactory*> factories;
  for(auto iter : m_Factories)
  {
    factories.push_back(iter.second.get());
  }
  return factories;
}

IH5DataFactory* H5DataReader::getFactory(const std::string& typeName) const
{
  if(m_Factories.find(typeName) == m_Factories.end())
  {
    throw std::runtime_error("Could not find IH5DataFactory for " + typeName);
  }
  return m_Factories.at(typeName).get();
}

DataStructure H5DataReader::createDataStructure(H5::IdType topLevelGroup) const
{
  DataStructure ds;
  return ds;
}
