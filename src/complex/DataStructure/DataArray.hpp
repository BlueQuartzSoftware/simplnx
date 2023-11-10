#pragma once

#include "complex/Common/Bit.hpp"
#include "complex/Common/TypeTraits.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/IDataArray.hpp"

#include <vector>

namespace complex
{
template <typename T>
class NeighborList;

namespace DataArrayConstants
{
constexpr StringLiteral k_TypeName = "DataArray<T>";
}

/**
 * @class DataArray
 * @brief The DataArray class is a type of DataObject that exists to store and
 * retrieve array data within the DataStructure. The DataArray is designed to
 * allow expandability into multiple sources of data, including out-of-core data,
 * through the use of derived DataStore classes.
 */
template <class T>
class DataArray : public IDataArray
{
  friend class NeighborList<T>;

public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using store_type = AbstractDataStore<T>;
  using weak_store = typename std::weak_ptr<AbstractDataStore<T>>;
  using Iterator = typename AbstractDataStore<T>::Iterator;
  using ConstIterator = typename AbstractDataStore<T>::ConstIterator;

  /**
   * @brief Attempts to create a DataArray with the specified values and add
   * it to the DataStructure. If a parentId is provided, then the DataArray
   * is created with the target DataObject as its parent. Otherwise, the
   * created DataArray is nested directly within the DataStructure.
   *
   * In either case, the DataArray is then owned by the DataStructure and will
   * be cleaned up when the DataStructure is finished with it. As such, it is
   * not recommended that the returned pointer be stored outside of the scope
   * in which it is created. Use the object's ID or DataPath to retrieve the
   * DataArray if it is needed after the program has left the current scope.
   *
   * Returns a pointer to the created DataArray if the process succeeds.
   * Returns nullptr otherwise.
   *
   * +++ The created DataArray takes ownership of the provided DataStore. +++
   *
   * @param dataStructure The parent DataStructure that will own the DataArray
   * @param name The name of the DataArray
   * @param store The IDataStore instance to use. The DataArray instance WILL TAKE OWNERSHIP of the DataStore Pointer.
   * @param parentId = {}
   * @return DataArray<T>* Instance of the DataArray object that is owned and managed by the DataStructure
   */
  static DataArray* Create(DataStructure& dataStructure, std::string name, std::shared_ptr<store_type> store, const std::optional<IdType>& parentId = {})
  {
    auto data = std::shared_ptr<DataArray>(new DataArray(dataStructure, std::move(name), std::move(store)));
    if(!AttemptToAddObject(dataStructure, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Creates a DataArray instance backed by the IDataStore type from the template argument
   * @tparam DataStoreType The concrete implementation of an IDataStore class
   * @param dataStructure The parent DataStructure that will own the DataArray
   * @param name The name of the DataArray
   * @param tupleShape  The tuple dimensions of the data. If you want to mimic an image then your shape should be {height, width} slowest to fastest dimension
   * @param componentShape The component dimensions of the data. If you want to mimic an RGB image then your component would be {3},
   * if you want to store a 3Rx4C matrix then it would be {3, 4}.
   * @param parentId The DataObject that will own the DataArray instance.
   * @return DataArray<T>* Instance of the DataArray object that is owned and managed by the DataStructure
   */
  template <typename DataStoreType>
  static DataArray* CreateWithStore(DataStructure& dataStructure, const std::string& name, const std::vector<usize>& tupleShape, const std::vector<usize>& componentShape,
                                    const std::optional<IdType>& parentId = {})
  {
    static_assert(std::is_base_of_v<AbstractDataStore<T>, DataStoreType>);

    std::shared_ptr<DataStoreType> dataStore;

    if constexpr(std::is_same_v<DataStoreType, DataStore<T>>)
    {
      dataStore = std::make_shared<DataStoreType>(tupleShape, componentShape, static_cast<T>(0));
    }
    else
    {
      dataStore = std::make_shared<DataStoreType>(tupleShape, componentShape);
    }

    auto data = std::shared_ptr<DataArray>(new DataArray(dataStructure, name, std::move(dataStore)));
    if(!AttemptToAddObject(dataStructure, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Attempts to create a DataArray with the specified values and add
   * it to the DataStructure. If a parentId is provided, then the DataArray
   * is created with the target DataObject as its parent. Otherwise, the
   * created DataArray is nested directly within the DataStructure.
   *
   * In either case, the DataArray is then owned by the DataStructure and will
   * be cleaned up when the DataStructure is finished with it. As such, it is
   * not recommended that the returned pointer be stored outside of the scope
   * in which it is created. Use the object's ID or DataPath to retrieve the
   * DataArray if it is needed after the program has left the current scope.
   *
   * Unlike Create, Import allows the DataObject ID to be set for use in
   * importing data.
   *
   * Returns a pointer to the created DataArray if the process succeeds.
   * Returns nullptr otherwise.
   *
   * The created DataArray takes ownership of the provided DataStore.
   * @param dataStructure
   * @param name
   * @param store
   * @param parentId = {}
   * @return DataArray<T>*
   */
  static DataArray* Import(DataStructure& dataStructure, std::string name, IdType importId, std::shared_ptr<store_type> store, const std::optional<IdType>& parentId = {})
  {
    auto data = std::shared_ptr<DataArray>(new DataArray(dataStructure, std::move(name), importId, std::move(store)));
    if(!AttemptToAddObject(dataStructure, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Creates a copy of the specified tuple getSize, count, and smart
   * pointer to the target DataStore. This copy is not added to the
   * DataStructure.
   * @param other
   */
  DataArray(const DataArray<T>& other)
  : IDataArray(other)
  , m_DataStore(other.m_DataStore)
  {
  }

  /**
   * @brief Move constructor moves the data from the provided DataArray.
   * @param other
   */
  DataArray(DataArray<T>&& other)
  : IDataArray(std::move(other))
  , m_DataStore(std::move(other.m_DataStore))
  {
  }

  /**
   * @brief Destroys the DataArray and the contained DataStore.
   */
  ~DataArray() override = default;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override
  {
    return Type::DataArray;
  }

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  ArrayType getArrayType() const override
  {
    return ArrayType::DataArray;
  }

  /**
   * @brief Returns a shallow copy of the DataArray without copying data. THE CALLING CODE
   * MUST DISPOSE OF THE RETURNED OBJECT.
   * @return DataObject*
   */
  DataObject* shallowCopy() override
  {
    return new DataArray(*this);
  }

  /**
   * @brief Returns a deep copy of the DataArray including a deep copy of the
   * data store. The object will be owned by the DataStructure.
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override
  {
    DataStructure& dataStruct = getDataStructureRef();
    if(dataStruct.containsData(copyPath))
    {
      return nullptr;
    }
    const std::shared_ptr<IDataStore> sharedStore = getDataStore()->deepCopy();
    std::shared_ptr<store_type> dataStore = std::dynamic_pointer_cast<store_type>(sharedStore);
    // Don't construct with identifier since it will get created when inserting into data structure
    std::shared_ptr<DataArray<T>> copy = std::shared_ptr<DataArray<T>>(new DataArray<T>(dataStruct, copyPath.getTargetName(), dataStore));
    if(dataStruct.insert(copy, copyPath.getParent()))
    {
      return copy;
    }
    return nullptr;
  }

  /**
   * @brief Returns the number of elements in the DataArray.
   * @return usize
   */
  usize getSize() const override
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  /**
   * @brief Returns the number of elements in the DataArray.
   * @return usize
   */
  usize size() const override
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  /**
   * @brief Returns if there are any elements in the array object
   * @return bool, true if the DataArray has a size() == 0
   */
  bool empty() const override
  {
    return m_DataStore->getNumberOfTuples() == 0;
  }

  /**
   * @brief Returns the number of tuples in the DataArray.
   * @return usize
   */
  size_t getNumberOfTuples() const override
  {
    return m_DataStore->getNumberOfTuples();
  }

  /**
   * @brief Returns the total number of components
   * @return usize
   */
  size_t getNumberOfComponents() const override
  {
    return m_DataStore->getNumberOfComponents();
  }

  /**
   * @brief Returns a reference to the value found at the specified index of
   * the data array. This can be used to edit the value found at the specified
   * index.
   *
   * Throws an exception if the DataStore has not been allocated.
   * @param index
   * @return reference
   */
  reference operator[](usize index)
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("DataArray::operator[] requires a valid DataStore");
    }

    return (*m_DataStore.get())[index];
  }

  /**
   * @brief Sets every value of a Tuple to the value
   * @param tupleIndex Index of the Tuple
   * @param value Value to set
   */
  void initializeTuple(usize tupleIndex, T value)
  {
    m_DataStore->fillTuple(tupleIndex, value);
  }

  /**
   * @brief Sets ALL values in the DataArray to "value"
   * @param value The value to set ALL elements of the array to.
   */
  void fill(T value)
  {
    m_DataStore->fill(value);
  }

  /**
   * @brief Copies values from one tuple to another within the same DataArray.
   *
   * NOTE: THERE IS NO ATTEMPT to check that either index is within bounds for the array.
   * @param from Index to copy data FROM
   * @param to Index to copy data TO
   */
  void copyTuple(usize from, usize to) override
  {
    if(from == to)
    {
      return;
    }
    const auto numComponents = getNumberOfComponents();
    for(usize i = 0; i < numComponents; i++)
    {
      usize fromCompIndex = from * numComponents + i;
      auto value = m_DataStore->getValue(fromCompIndex);
      usize toCompIndex = to * numComponents + i;
      m_DataStore->setValue(toCompIndex, value);
    }
  }

  /**
   * @brief Byte swaps all elements in the data array
   */
  void byteSwapElements()
  {
    for(auto& value : *this)
    {
      value = complex::byteswap(value);
    }
  }

  /**
   * @brief Returns const reference to the value found at the specified index
   * of the data array. This cannot be used to edit the value found at the
   * specified index.
   *
   * Throws an exception if the DataStore has not been allocated.
   * @param index
   * @return const_reference
   */
  const_reference operator[](usize index) const
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("DataArray::operator[] requires a valid DataStore");
    }

    return (*m_DataStore.get())[index];
  }

  /**
   * @brief Returns a const reference to the value found at the specified index
   * of the data array. This cannot be used to edit the value found at the
   * specified index.
   *
   * Throws an exception if the DataStore has not been allocated.
   * @param index
   * @return const_reference
   */
  value_type at(usize index) const
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("DataArray::operator[] requires a valid DataStore");
    }

    return m_DataStore->getValue(index);
  }

  const_reference getComponent(usize tupleIndex, usize componentIndex)
  {
    const usize index = tupleIndex * getNumberOfComponents() + componentIndex;
    return m_DataStore->getValue(index);
  }

  void setComponent(usize tupleIndex, usize componentIndex, value_type value)
  {
    const usize index = tupleIndex * getNumberOfComponents() + componentIndex;
    m_DataStore->setValue(index, value);
  }

  void setValue(usize index, value_type value)
  {
    m_DataStore->setValue(index, value);
  }

  /**
   * @brief Returns a const pointer to the DataStore for read-only access.
   * Returns nullptr if no DataStore is available.
   * @return DataStore<T>*
   */
  const store_type* getDataStore() const
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns a raw pointer to the DataStore.
   * @return DataStore<T>*
   */
  store_type* getDataStore()
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns a pointer to the array's IDataStore.
   * @return const IDataStore*
   */
  IDataStore* getIDataStore() override
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns a pointer to the array's IDataStore.
   * @return const IDataStore*
   */
  const IDataStore* getIDataStore() const override
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns a reference to the DataStore.
   * @return DataStore<T>&
   */
  store_type& getDataStoreRef()
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("DataArray: Null DataStore");
    }
    return *m_DataStore;
  }

  /**
   * @brief Returns a reference to the DataStore.
   * @return const DataStore<T>&
   */
  const store_type& getDataStoreRef() const
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("DataArray: Null DataStore");
    }
    return *m_DataStore;
  }

  /**
   * @brief Returns a std::weak_ptr for the stored DataStore.
   * @return std::weak_ptr<DataStore<T>>
   */
  weak_store getDataStorePtr() const
  {
    return m_DataStore;
  }

  //  /**
  //   * @brief Returns true if the DataStore has already been allocated. Returns false otherwise.
  //   * @return bool
  //   */
  //  bool isAllocated() const
  //  {
  //    return m_DataStore != nullptr;
  //  }

  /**
   * @brief Sets a new DataStore for the DataArray to handle. The existing DataStore
   * is deleted if there are no other references. To save the existing DataStore
   * before replacing it, call releaseDataStore() before setting the new DataStore.
   * @param store
   */
  void setDataStore(std::shared_ptr<store_type> store)
  {
    m_DataStore = std::move(store);
    if(m_DataStore == nullptr)
    {
      m_DataStore = std::make_shared<EmptyDataStore<T>>();
    }
  }

  /**
   * @brief Returns the data format used for storing the array data.
   * @return data format as string
   */
  std::string getDataFormat() const override
  {
    return m_DataStore->getDataFormat();
  }

  /**
   * @brief Returns the first item in the array.
   *
   * This method requires the DataArray to have at least one value in the
   * DataStore. Otherwise, this throws a runtime exception.
   * @return value_type
   */
  value_type front() const
  {
    return at(0);
  }

  /**
   * @brief Returns the last item in the array.
   *
   * This method requires the DataArray to have at least one value in the
   * DataStore. Otherwise, this throws a runtime exception.
   * @return value_type
   */
  value_type back() const
  {
    return at(getSize() - 1);
  }

  /**
   * @brief Returns an iterator to the begining of the DataArray.
   *
   * This method will fail if no DataStore has been set.
   * @return Iterator
   */
  Iterator begin()
  {
    return getDataStore()->begin();
  }

  /**
   * @brief Returns an iterator to the end of the DataArray.
   *
   * This method will fail if no DataStore has been set.
   * @return Iterator
   */
  Iterator end()
  {
    return getDataStore()->end();
  }

  /**
   * @brief Returns a const iterator to the begining of the DataArray.
   *
   * This method will fail if no DataStore has been set.
   * @return ConstIterator
   */
  ConstIterator begin() const
  {
    return getDataStore()->begin();
  }

  /**
   * @brief Returns a const iterator to the end of the DataArray.
   *
   * This method will fail if no DataStore has been set.
   * @return ConstIterator
   */
  ConstIterator end() const
  {
    return getDataStore()->end();
  }

  /**
   * @brief Returns a const iterator to the begining of the DataArray.
   *
   * This method will fail if no DataStore has been set.
   * @return ConstIterator
   */
  ConstIterator cbegin() const
  {
    return getDataStore()->cbegin();
  }

  /**
   * @brief Returns a const iterator to the end of the DataArray.
   *
   * This method will fail if no DataStore has been set.
   * @return ConstIterator
   */
  ConstIterator cend() const
  {
    return getDataStore()->cend();
  }

  /**
   * @brief Copies the specified DataArray's std::shared_ptr<DataStore> into
   * the current DataArray.
   * @param rhs
   * @return DataArray&
   */
  DataArray& operator=(const DataArray& rhs)
  {
    m_DataStore = rhs.m_DataStore;
    return *this;
  }

  /**
   * @brief Moves the specified DataArray's DataStore into the current DataArray.
   * @param rhs
   * @return DataArray&
   */
  DataArray& operator=(DataArray&& rhs) noexcept
  {
    m_DataStore = std::move(rhs.m_DataStore);
    return *this;
  }

  /**
   * @brief Static function to get the typename
   * @return
   */
  static std::string GetTypeName()
  {
    if constexpr(std::is_same_v<T, int8>)
    {
      return "DataArray<int8>";
    }
    else if constexpr(std::is_same_v<T, uint8>)
    {
      return "DataArray<uint8>";
    }
    else if constexpr(std::is_same_v<T, int16>)
    {
      return "DataArray<int16>";
    }
    else if constexpr(std::is_same_v<T, uint16>)
    {
      return "DataArray<uint16>";
    }
    else if constexpr(std::is_same_v<T, int32>)
    {
      return "DataArray<int32>";
    }
    else if constexpr(std::is_same_v<T, uint32>)
    {
      return "DataArray<uint32>";
    }
    else if constexpr(std::is_same_v<T, int64>)
    {
      return "DataArray<int64>";
    }
    else if constexpr(std::is_same_v<T, uint64>)
    {
      return "DataArray<uint64>";
    }
    else if constexpr(std::is_same_v<T, float32>)
    {
      return "DataArray<float32>";
    }
    else if constexpr(std::is_same_v<T, float64>)
    {
      return "DataArray<float64>";
    }
    else if constexpr(std::is_same_v<T, bool>)
    {
      return "DataArray<bool>";
    }
    else
    {
      static_assert(dependent_false<T>, "Unsupported type T in DataArray");
    }
  }

  /**
   * @brief getTypeName
   * @return
   */
  std::string getTypeName() const override
  {
    return GetTypeName();
  }

  /**
   * @brief Flushes the DataObject to its respective target.
   * In-memory DataObjects are not affected.
   */
  void flush() const override
  {
    m_DataStore->flush();
  }

  uint64 memoryUsage() const override
  {
    return m_DataStore->memoryUsage();
  }

protected:
  /**
   * @brief Constructs a DataArray with the specified name and DataStore.
   *
   * The DataArray takes ownership of the DataStore. If none is provided,
   * an EmptyDataStore is used instead.
   * @param dataStructure
   * @param name
   * @param store
   */
  DataArray(DataStructure& dataStructure, std::string name, std::shared_ptr<store_type> store = nullptr)
  : IDataArray(dataStructure, std::move(name))
  {
    setDataStore(std::move(store));
  }

  /**
   * @brief Constructs a DataArray with the specified name and DataStore.
   *
   * The DataArray takes ownership of the DataStore. If none is provided,
   * an EmptyDataStore is used instead.
   * @param dataStructure
   * @param name
   * @param importId
   * @param store
   */
  DataArray(DataStructure& dataStructure, std::string name, IdType importId, std::shared_ptr<store_type> store = nullptr)
  : IDataArray(dataStructure, std::move(name), importId)
  {
    setDataStore(std::move(store));
  }

private:
  std::shared_ptr<store_type> m_DataStore = nullptr;
};

// Declare aliases
using UInt8Array = DataArray<uint8>;
using UInt16Array = DataArray<uint16>;
using UInt32Array = DataArray<uint32>;
using UInt64Array = DataArray<uint64>;

using Int8Array = DataArray<int8>;
using Int16Array = DataArray<int16>;
using Int32Array = DataArray<int32>;
using Int64Array = DataArray<int64>;

using Float32Array = DataArray<float32>;
using Float64Array = DataArray<float64>;

using BoolArray = DataArray<bool>;
} // namespace complex
