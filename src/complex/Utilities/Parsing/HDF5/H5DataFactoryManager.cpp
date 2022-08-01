#include "H5DataFactoryManager.hpp"

#include "complex/DataStructure/Factory/DataArrayFactory.hpp"
#include "complex/DataStructure/Factory/DataGroupFactory.hpp"
#include "complex/DataStructure/Factory/EdgeGeomFactory.hpp"
#include "complex/DataStructure/Factory/GridMontageFactory.hpp"
#include "complex/DataStructure/Factory/HexahedralGeomFactory.hpp"
#include "complex/DataStructure/Factory/ImageGeomFactory.hpp"
#include "complex/DataStructure/Factory/NeighborListFactory.hpp"
#include "complex/DataStructure/Factory/QuadGeomFactory.hpp"
#include "complex/DataStructure/Factory/RectGridGeomFactory.hpp"
#include "complex/DataStructure/Factory/ScalarDataFactory.hpp"
#include "complex/DataStructure/Factory/StringArrayFactory.hpp"
#include "complex/DataStructure/Factory/TetrahedralGeomFactory.hpp"
#include "complex/DataStructure/Factory/TriangleGeomFactory.hpp"
#include "complex/DataStructure/Factory/VertexGeomFactory.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5IDataFactory.hpp"

using namespace complex;
using namespace complex::H5;

H5::DataFactoryManager::DataFactoryManager()
{
  addCoreFactories();
}

H5::DataFactoryManager::~DataFactoryManager() = default;

void H5::DataFactoryManager::addCoreFactories()
{
  addFactory(new UInt8ArrayFactory());
  addFactory(new UInt16ArrayFactory());
  addFactory(new UInt32ArrayFactory());
  addFactory(new UInt64ArrayFactory());
  addFactory(new Int8ArrayFactory());
  addFactory(new Int16ArrayFactory());
  addFactory(new Int32ArrayFactory());
  addFactory(new Int64ArrayFactory());
  addFactory(new USizeArrayFactory());
  addFactory(new Float32ArrayFactory());
  addFactory(new Float64ArrayFactory());
  addFactory(new BoolArrayFactory());
  addFactory(new StringArrayFactory());

  addFactory(new NeighborListFactory());

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

void H5::DataFactoryManager::addFactory(IDataFactory* factory)
{
  if(factory == nullptr)
  {
    return;
  }

  m_Factories[factory->getDataTypeName()] = std::shared_ptr<IDataFactory>(factory);
}

std::vector<H5::IDataFactory*> H5::DataFactoryManager::getFactories() const
{
  std::vector<H5::IDataFactory*> factories;
  for(const auto& iter : m_Factories)
  {
    factories.push_back(iter.second.get());
  }
  return factories;
}

H5::IDataFactory* H5::DataFactoryManager::getFactory(const std::string& typeName) const
{
  if(m_Factories.find(typeName) == m_Factories.end())
  {
    throw std::runtime_error("Could not find H5::IDataFactory for " + typeName);
  }
  return m_Factories.at(typeName).get();
}
