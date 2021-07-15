#include "DataGroup.hpp"

#include <exception>
#include <stdexcept>

using namespace complex;

DataGroup::DataGroup(DataStructure* ds, const std::string& name)
: BaseGroup(ds, name)
{
}

DataGroup::DataGroup(const DataGroup& other)
: BaseGroup(other)
{
}

DataGroup::DataGroup(DataGroup&& other) noexcept
: BaseGroup(std::move(other))
{
}

DataGroup::~DataGroup() = default;

DataObject* DataGroup::deepCopy()
{
  auto copy = new DataGroup(getDataStructure(), getName());
  for(auto& pair : getDataMap())
  {
    copy->insert(pair.second);
  }
  return copy;
}

DataObject* DataGroup::shallowCopy()
{
  return new DataGroup(*this);
}

bool DataGroup::canInsert(const DataObject* obj) const
{
  return BaseGroup::canInsert(obj);
}

H5::ErrorType DataGroup::generateXdmfText(std::ostream& out, const std::string& hdfFileName) const
{
  throw std::runtime_error("");
}

H5::ErrorType DataGroup::readFromXdmfText(std::istream& in, const std::string& hdfFileName)
{
  throw std::runtime_error("");
}
