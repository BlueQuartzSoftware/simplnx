#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"

namespace complex
{
/**
 * @class DataArray
 * @brief The DataArray class is a type of DataObject that exists to store and
 * retrieve array data within the DataStructure. The DataArray is designed to
 * allow expandability into multiple sources of data, including out-of-core data,
 * through the use of derived DataStore classes.
 */
template <class T>
class DataArray : public DataObject
{
public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using store_type = IDataStore<T>;
  using weak_store = typename std::weak_ptr<IDataStore<T>>;
  using Iterator = typename IDataStore<T>::Iterator;
  using ConstIterator = typename IDataStore<T>::ConstIterator;

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
   * The created DataArray takes ownership of the provided DataStore.
   * @param ds
   * @param name
   * @param store
   * @param parentId = {}
   * @return DataArray<T>*
   */
  static DataArray* Create(DataStructure& ds, const std::string& name, store_type* store, const std::optional<IdType>& parentId = {})
  {
    auto data = std::shared_ptr<DataArray>(new DataArray(ds, name, store));
    if(!AttemptToAddObject(ds, data, parentId))
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
  : DataObject(other)
  , m_DataStore(other.m_DataStore)
  {
  }

  /**
   * @brief Move constructor moves the data from the provided DataArray.
   * @param other
   */
  DataArray(DataArray<T>&& other) noexcept
  : DataObject(std::move(other))
  , m_DataStore(std::move(other.m_DataStore))
  {
  }

  /**
   * @brief Destroys the DataArray and the contained DataStore.
   */
  ~DataArray() override = default;

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
   * data store.
   * @return DataObject*
   */
  DataObject* deepCopy() override
  {
    return new DataArray(*getDataStructure(), getName(), getDataStore()->deepCopy());
  }

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
  usize getSize() const
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  /**
   * @brief Returns the number of tuples in the DataArray.
   * @return usize
   */
  size_t getNumberOfTuples() const
  {
    return m_DataStore->getNumberOfTuples();
  }

  /**
   * @brief Returns the tuple getSize.
   * @return usize
   */
  size_t getNumberOfComponents() const
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
      throw std::runtime_error("");
    }

    return (*m_DataStore.get())[index];
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
      throw std::runtime_error("");
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
  const_reference at(usize index) const
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("");
    }

    return (*m_DataStore.get())[index];
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
  void setDataStore(store_type* store)
  {
    m_DataStore = std::shared_ptr<store_type>(store);
    if(m_DataStore == nullptr)
    {
      m_DataStore = std::shared_ptr<store_type>(new EmptyDataStore<T>());
    }
  }

  /**
   * @brief Sets a new DataStore for the DataArray to handle. The existing
   * DataStore is deleted if there are no other std::shared_ptr references.
   * To save the existing DataStore before replacing it, call
   * releaseDataStore() before setting the new DataStore.
   * @param store
   */
  void setDataStore(const weak_store& store)
  {
    m_DataStore = store.lock();
    if(m_DataStore == nullptr)
    {
      m_DataStore = std::shared_ptr<store_type>(new EmptyDataStore<T>());
    }
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

protected:
  /**
   * @brief Constructs a DataArray with the specified name and DataStore.
   *
   * The DataArray takes ownership of the DataStore. If none is provided,
   * an EmptyDataStore is used instead.
   * @param ds
   * @param name
   * @param store
   */
  DataArray(DataStructure& ds, const std::string& name, store_type* store = nullptr)
  : DataObject(ds, name)
  {
    setDataStore(store);
  }

  /**
   * @brief Constructs a DataArray with the specified tuple getSize, count, and
   * DataStore.
   *
   * The DataArray takes ownership of the DataStore.
   * @param ds
   * @param name
   * @param dataStore
   */
  DataArray(DataStructure& ds, const std::string& name, const weak_store& store)
  : DataObject(ds, name)
  {
    setDataStore(store);
  }

  /**
   * @brief Writes the DataArray to HDF5 using the provided group ID.
   *
   * This method will fail if no DataStore has been set.
   * @param parentId
   * @param dataId
   * @return H5::ErrorType
   */
  H5::ErrorType writeHdf5_impl(H5::IdType parentId, H5::IdType dataId) const override
  {
    return m_DataStore->writeHdf5(parentId, getName(), getId());
  }

private:
  std::shared_ptr<IDataStore<T>> m_DataStore = nullptr;
};

// Declare extern templates
// extern template class DataArray<uint8>;
// extern template class DataArray<uint16>;
// extern template class DataArray<uint32>;
// extern template class DataArray<uint64>;

// extern template class DataArray<int8>;
// extern template class DataArray<int16>;
// extern template class DataArray<int32>;
// extern template class DataArray<int64>;
// extern template class DataArray<usize>;

// extern template class DataArray<float32>;
// extern template class DataArray<float64>;

// Declare aliases
using UInt8Array = DataArray<uint8>;
using UInt16Array = DataArray<uint16>;
using UInt32Array = DataArray<uint32>;
using UInt64Array = DataArray<uint64>;

using Int8Array = DataArray<int8>;
using Int16Array = DataArray<int16>;
using Int32Array = DataArray<int32>;
using Int64Array = DataArray<int64>;

using USizeArray = DataArray<usize>;

using Float32Array = DataArray<float32>;
using Float64Array = DataArray<float64>;

using VectorOfFloat32Array = std::vector<std::shared_ptr<Float32Array>>;
} // namespace complex
