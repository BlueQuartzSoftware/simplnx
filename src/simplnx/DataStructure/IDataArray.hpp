#pragma once

#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"

namespace nx::core
{
/**
 * @class IDataArray
 * @brief The IDataArray class is a typeless interface for the templated DataArray class.
 */
class SIMPLNX_EXPORT IDataArray : public IArray
{
public:
  static inline constexpr StringLiteral k_TypeName = "IDataArray";

  ~IDataArray() override = default;

  /**
   * @brief Returns the tuple shape.
   * @return ShapeType The shape of the tuples in the array
   */
  ShapeType getTupleShape() const override
  {
    return getIDataStoreRef().getTupleShape();
  }

  /**
   * @brief Returns the component shape.
   * @return ShapeType The shape of the components in the array
   */
  ShapeType getComponentShape() const override
  {
    return getIDataStoreRef().getComponentShape();
  }

  /**
   * @brief Copies values from one tuple to another.
   * @param from The index of the source tuple to copy from
   * @param to The index of the destination tuple to copy to
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
   * @tparam StoreT The target IDataStore-derived type to cast to
   * @return const StoreT* Pointer to the DataStore cast to the specified type, or nullptr if the cast fails
   */
  template <class StoreT>
  const StoreT* getIDataStoreAs() const
  {
    static_assert(std::is_base_of_v<IDataStore, StoreT>);
    return dynamic_cast<const StoreT*>(getIDataStore());
  }

  /**
   * @brief Returns a pointer to the DataStore cast as type StoreT.
   * @tparam StoreT The target IDataStore-derived type to cast to
   * @return StoreT* Pointer to the DataStore cast to the specified type, or nullptr if the cast fails
   */
  template <class StoreT>
  StoreT* getIDataStoreAs()
  {
    static_assert(std::is_base_of_v<IDataStore, StoreT>);
    return dynamic_cast<StoreT*>(getIDataStore());
  }

  /**
   * @brief Returns a reference to the DataStore cast as type StoreT.
   * @tparam StoreT The target IDataStore-derived type to cast to
   * @return const StoreT& Reference to the DataStore cast to the specified type
   * @throws std::bad_cast if the cast fails
   */
  template <class StoreT>
  const StoreT& getIDataStoreRefAs() const
  {
    static_assert(std::is_base_of_v<IDataStore, StoreT>);
    return dynamic_cast<const StoreT&>(getIDataStoreRef());
  }

  /**
   * @brief Returns a reference to the DataStore cast as type StoreT.
   * @tparam StoreT The target IDataStore-derived type to cast to
   * @return StoreT& Reference to the DataStore cast to the specified type
   * @throws std::bad_cast if the cast fails
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
  DataType getDataType() const
  {
    return getIDataStoreRef().getDataType();
  }

  /**
   * @brief Resizes the internal array to accommodate the specified tuple shape.
   * @param tupleShape The new shape for the tuples
   */
  void resizeTuples(const ShapeType& tupleShape) override
  {
    getIDataStoreRef().resizeTuples(tupleShape);
  }

  /**
   * @brief Returns the StoreType of the underlying IDataStore specifying if
   * the IDataStore is in memory, out of core, or empty.
   * @return IDataStore::StoreType
   */
  IDataStore::StoreType getStoreType() const
  {
    return getIDataStoreRef().getStoreType();
  }

  /**
   * @brief Returns the data format used for storing the array data.
   * @return data format as string
   */
  virtual std::string getDataFormat() const = 0;

protected:
  /**
   * @brief Constructs an IDataArray with the given name.
   * @param dataStructure The DataStructure that owns this array
   * @param name The name of the array
   */
  IDataArray(DataStructure& dataStructure, std::string name)
  : IArray(dataStructure, std::move(name))
  {
  }

  /**
   * @brief Constructs an IDataArray with the given name and import ID.
   * @param dataStructure The DataStructure that owns this array
   * @param name The name of the array
   * @param importId The import ID for tracking imported objects
   */
  IDataArray(DataStructure& dataStructure, std::string name, IdType importId)
  : IArray(dataStructure, std::move(name), importId)
  {
  }
};
} // namespace nx::core
