#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/IDataStore.hpp"

namespace complex
{
/**
 * @class IDataArray
 * @brief The IDataArray class is a typeless interface for the templated DataArray class.
 */
class COMPLEX_EXPORT IDataArray : public DataObject
{
public:
  virtual ~IDataArray() = default;

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override
  {
    return "DataArray";
  }

  /**
   * @brief Returns the number of elements in the DataArray.
   * @return usize
   */
  virtual usize getSize() const = 0;

  /**
   * @brief Returns the number of tuples in the DataArray.
   * @return usize
   */
  virtual usize getNumberOfTuples() const = 0;

  /**
   * @brief Returns the tuple getSize.
   * @return usize
   */
  virtual usize getNumberOfComponents() const = 0;

  /**
   * @brief Copies values from one tuple to another.
   * @param from
   * @param to
   */
  virtual void copyTuple(usize from, usize to) = 0;

  /**
   * @brief Returns a pointer to the array's IDataStore.
   * @return IDataStore*
   */
  virtual IDataStore* getIDataStore() = 0;

  /**
   * @brief Returns a pointer to the array's IDataStore.
   * @return const IDataStore*
   */
  virtual const IDataStore* getIDataStore() const = 0;

  /**
   * @brief Returns a reference to the array's IDataStore.
   * @return IDataStore&
   */
  IDataStore& getIDataStoreRef()
  {
    IDataStore* store = getIDataStore();
    if(store == nullptr)
    {
      throw std::runtime_error("IDataArray: Null IDataStore");
    }
    return *store;
  }

  /**
   * @brief Returns a reference to the array's IDataStore.
   * @return const IDataStore&
   */
  const IDataStore& getIDataStoreRef() const
  {
    const IDataStore* store = getIDataStore();
    if(store == nullptr)
    {
      throw std::runtime_error("IDataArray: Null IDataStore");
    }
    return *store;
  }

  /**
   * @brief Returns a pointer to the DataStore cast as type StoreT.
   * @return const StoreT*
   */
  template <class StoreT>
  const StoreT* getIDataStoreAs() const
  {
    static_assert(std::is_base_of_v<IDataStore, StoreT>);
    return dynamic_cast<const StoreT*>(getIDataStore());
  }

  /**
   * @brief Returns a pointer to the DataStore cast as type StoreT.
   * @return StoreT*
   */
  template <class StoreT>
  StoreT* getIDataStoreAs()
  {
    static_assert(std::is_base_of_v<IDataStore, StoreT>);
    return dynamic_cast<StoreT*>(getIDataStore());
  }

  /**
   * @brief Returns a reference to the DataStore cast as type StoreT.
   * @return const StoreT&
   */
  template <class StoreT>
  const StoreT& getIDataStoreRefAs() const
  {
    static_assert(std::is_base_of_v<IDataStore, StoreT>);
    return dynamic_cast<const StoreT&>(getIDataStoreRef());
  }

  /**
   * @brief Returns a reference to the DataStore cast as type StoreT.
   * @return StoreT&
   */
  template <class StoreT>
  StoreT& getIDataStoreRefAs()
  {
    static_assert(std::is_base_of_v<IDataStore, StoreT>);
    return dynamic_cast<StoreT&>(getIDataStoreRef());
  }

  /**
   * @brief Returns the DataArray's value type as an enum
   * @return DataType
   */
  virtual DataType getDataType() const
  {
    if(auto store = getIDataStore())
    {
      return store->getDataType();
    }
    return DataType::error;
  }

protected:
  IDataArray(DataStructure& dataStructure, std::string name)
  : DataObject(dataStructure, std::move(name))
  {
  }
  IDataArray(DataStructure& dataStructure, std::string name, IdType importId)
  : DataObject(dataStructure, std::move(name), importId)
  {
  }
};
} // namespace complex
