#include "StringArray.hpp"

#include "fmt/format.h"

#include <stdexcept>

namespace complex
{
std::string StringArray::GetTypeName()
{
  return "StringArray";
}

StringArray* StringArray::Create(DataStructure& ds, const std::string_view& name, const std::optional<IdType>& parentId)
{
  return CreateWithValues(ds, name, {}, parentId);
}
StringArray* StringArray::CreateWithValues(DataStructure& ds, const std::string_view& name, collection_type strings, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<StringArray>(new StringArray(ds, name.data()));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  data->m_Strings = std::move(strings);
  return data.get();
}

StringArray* StringArray::Import(DataStructure& ds, const std::string_view& name, IdType importId, collection_type strings, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<StringArray>(new StringArray(ds, name.data(), importId, std::move(strings)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

StringArray::StringArray(DataStructure& dataStructure, std::string name)
: DataObject(dataStructure, std::move(name))
{
}
StringArray::StringArray(DataStructure& dataStructure, std::string name, IdType importId, collection_type strings)
: DataObject(dataStructure, std::move(name), importId)
, m_Strings(std::move(strings))
{
}
StringArray::StringArray(const StringArray& other)
: DataObject(other)
, m_Strings(other.m_Strings)
{
}
StringArray::StringArray(StringArray&& other) noexcept
: DataObject(other)
, m_Strings(std::move(other.m_Strings))
{
}

StringArray::~StringArray() noexcept = default;

DataObject::Type StringArray::getDataObjectType() const
{
  return DataObject::Type::StringArray;
}
std::string StringArray::getTypeName() const
{
  return GetTypeName();
}

DataObject* StringArray::shallowCopy()
{
  return new StringArray(*this);
}
DataObject* StringArray::deepCopy()
{
  return new StringArray(*getDataStructure(), getName(), getId(), m_Strings);
}

size_t StringArray::size() const
{
  return m_Strings.size();
}

const StringArray::collection_type& StringArray::values() const
{
  return m_Strings;
}

StringArray::reference StringArray::operator[](usize index)
{
  return m_Strings[index];
}
StringArray::const_reference StringArray::operator[](usize index) const
{
  return m_Strings[index];
}
StringArray::const_reference StringArray::at(usize index) const
{
  if(index >= size())
  {
    throw std::out_of_range(fmt::format("Attempting to access string at index {} out of {}", index, size()));
  }
  return m_Strings[index];
}

StringArray::iterator StringArray::begin()
{
  return m_Strings.begin();
}
StringArray::iterator StringArray::end()
{
  return m_Strings.end();
}
StringArray::const_iterator StringArray::begin() const
{
  return m_Strings.begin();
}
StringArray::const_iterator StringArray::end() const
{
  return m_Strings.end();
}
StringArray::const_iterator StringArray::cbegin() const
{
  return m_Strings.begin();
}
StringArray::const_iterator StringArray::cend() const
{
  return m_Strings.end();
}

StringArray& StringArray::operator=(const StringArray& rhs)
{
  DataObject::operator=(rhs);
  m_Strings = rhs.m_Strings;
  return *this;
}
StringArray& StringArray::operator=(StringArray&& rhs) noexcept
{
  DataObject::operator=(rhs);
  m_Strings = std::move(rhs.m_Strings);
  return *this;
}

H5::ErrorType StringArray::writeHdf5(H5::DataStructureWriter& dataStructureWriter, H5::GroupWriter& parentGroupWriter, bool importable) const
{
  auto datasetWriter = parentGroupWriter.createDatasetWriter(getName());

  // writeVectorOfStrings may resize the collection
  collection_type strings = m_Strings;
  const auto err = datasetWriter.writeVectorOfStrings(strings);
  if(err < 0)
  {
    return err;
  }
  return writeH5ObjectAttributes(dataStructureWriter, datasetWriter, importable);
}
} // namespace complex
