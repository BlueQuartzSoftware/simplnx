#include "ImportObjectAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"

using namespace complex;

namespace
{
template <class T>
IDataStore<T>* CreateDataStore(const typename IDataStore<T>::ShapeType& tupleShape, const typename IDataStore<T>::ShapeType& componentShape, IDataAction::Mode mode)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return new EmptyDataStore<T>(tupleShape, componentShape);
  }
  case IDataAction::Mode::Execute: {
    return new DataStore<T>(tupleShape, componentShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

template <class T>
Result<> CreateArray(DataStructure& dataStructure, const std::vector<usize>& dims, uint64 nComp, const DataPath& path, IDataAction::Mode mode)
{
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> id;

  if(parentPath.getLength() != 0)
  {
    auto parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return {nonstd::make_unexpected(std::vector<Error>{{-1, fmt::format("Parent object \"{}\" does not exist", parentPath.toString())}})};
    }

    id = parentObject->getId();
  }

  usize last = path.getLength() - 1;

  std::string name = path[last];

  auto* store = CreateDataStore<T>(dims, {nComp}, mode);
  auto dataArray = DataArray<T>::Create(dataStructure, name, store, id);
  if(dataArray == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("Unable to create DataArray at \"{}\"", path.toString())}})};
  }

  return {};
}
} // namespace

namespace complex
{
ImportObjectAction::ImportObjectAction(const std::shared_ptr<DataObject>& importObject, const DataPath& path)
: m_ImportData(importObject)
, m_Path(path)
{
}

ImportObjectAction::~ImportObjectAction() noexcept = default;

Result<> ImportObjectAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto importData = std::shared_ptr<DataObject>(m_ImportData->deepCopy());
  if(!dataStructure.insert(importData, m_Path))
  {
    return {nonstd::make_unexpected(std::vector<Error>{{-2, fmt::format("Unable to import DataObject at \"{}\"", m_Path.toString())}})};
  }

  return {};
}

std::shared_ptr<DataObject> ImportObjectAction::getImportObject() const
{
  return m_ImportData;
}

DataPath ImportObjectAction::path() const
{
  return m_Path;
}
} // namespace complex