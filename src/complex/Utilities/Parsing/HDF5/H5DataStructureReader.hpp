#pragma once

#include <optional>

#include "complex/DataStructure/DataStructure.hpp"

namespace complex
{
class H5DataReader;
class IH5DataFactory;

namespace H5
{
class GroupReader;

class DataStructureReader
{
public:
  DataStructureReader(H5DataReader* h5DataReader = nullptr);
  virtual ~DataStructureReader();

  complex::DataStructure readH5Group(const H5::GroupReader& groupReader, H5::ErrorType& errorCode);

  H5::ErrorType readObjectFromGroup(const H5::GroupReader& parentGroup, const std::string& objectName, const std::optional<DataObject::IdType>& parentId = {});

  DataStructure& getDataStructure();
  void clearDataStructure();

protected:
  H5DataReader* getDataReader() const;
  IH5DataFactory* getDataFactory(const std::string& typeName) const;

private:
  H5DataReader* m_DataReader = nullptr;
  DataStructure m_CurrentStructure;
};
} // namespace H5
} // namespace complex
