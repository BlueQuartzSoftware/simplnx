#pragma once

#include <optional>

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DataFactoryManager.hpp"

namespace complex
{
class DataFactoryManager;
class IH5DataFactory;

namespace H5
{
class GroupReader;

class DataStructureReader
{
public:
  DataStructureReader(H5::DataFactoryManager* h5FactoryManager = nullptr);
  virtual ~DataStructureReader();

  complex::DataStructure readH5Group(const H5::GroupReader& groupReader, H5::ErrorType& errorCode);

  H5::ErrorType readObjectFromGroup(const H5::GroupReader& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId = {});

  DataStructure& getDataStructure();
  void clearDataStructure();

protected:
  H5::DataFactoryManager* getDataReader() const;
  IH5DataFactory* getDataFactory(const std::string& typeName) const;

private:
  H5::DataFactoryManager* m_FactoryManager = nullptr;
  DataStructure m_CurrentStructure;
};
} // namespace H5
} // namespace complex
