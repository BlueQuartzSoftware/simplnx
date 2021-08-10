#include "H5DataReader.hpp"

#include "complex/Utilities/Parsing/HDF5/IH5DataFactory.hpp"

using namespace complex;

H5DataReader::H5DataReader()
{
}
H5DataReader::~H5DataReader() = default;

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
  return m_Factories.at(typeName).get();
}

DataStructure H5DataReader::createDataStructure(H5::IdType topLevelGroup) const
{
  DataStructure ds;

  return ds;
}
