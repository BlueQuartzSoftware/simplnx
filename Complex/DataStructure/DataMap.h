#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace SIMPL
{
class DataStructure;
class DataObject;

class DataMap
{
public:
  using IdType = size_t;
  using MapType = std::map<IdType, std::shared_ptr<DataObject>>;
  using Iterator = MapType::iterator;
  using ConstIterator = MapType::const_iterator;

  DataMap();
  DataMap(const DataMap& other);
  DataMap(DataMap&& other) noexcept;
  ~DataMap();

  DataMap deepCopy() const;

  bool insert(const std::shared_ptr<DataObject>& obj);
  bool remove(DataObject* obj);
  bool remove(IdType id);
  bool erase(const Iterator& iter);

  std::vector<IdType> getKeys() const;
  std::vector<IdType> getAllKeys() const;
  std::map<IdType, std::weak_ptr<DataObject>> getAllItems() const;

  DataObject* operator[](IdType key);
  const DataObject* operator[](IdType key) const;

  DataObject* operator[](const std::string& name);
  const DataObject* operator[](const std::string& name) const;

  bool contains(IdType id) const;
  bool contains(const DataObject* obj) const;
  bool contains(const std::string& name) const;

  Iterator find(IdType id);
  ConstIterator find(IdType id) const;
  Iterator find(const std::string& name);
  ConstIterator find(const std::string& name) const;

  void setDataStructure(DataStructure* dataStr);

  Iterator begin();
  Iterator end();
  ConstIterator begin() const;
  ConstIterator end() const;

private:
  MapType m_Map;
};
} // namespace SIMPL
