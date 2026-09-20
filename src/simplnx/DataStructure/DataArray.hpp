#pragma once

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Utilities/StoreCopyUtilities.hpp"

#include <fmt/format.h>

#include <iterator>
#include <vector>

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
template <typename T>
class NeighborList;

/**
 * @namespace DataArrayConstants
 * @brief Contains DataArray constants.
 */
namespace DataArrayConstants
{
/**
 * @brief Names the generic DataArray type.
 */
constexpr StringLiteral k_TypeName = "DataArray<T>";
} // namespace DataArrayConstants

/**
 * @class DataArray
 * @brief Stores typed array values through a storage-neutral data store.
 * @tparam T Stored value type.
 */
template <class T>
class DataArray : public IDataArray
{
  friend class NeighborList<T>;

public:
  /**
   * @brief Names the storage-neutral data-store type.
   */
  using store_type = AbstractDataStore<T>;

  /**
   * @brief Names the stored value type.
   */
  using value_type = typename store_type::value_type;

  /**
   * @brief Names the mutable value-proxy type.
   */
  using reference = typename store_type::reference;

  /**
   * @brief Names a non-owning data-store observer type.
   */
  using weak_store = std::weak_ptr<store_type>;

  /**
   * @brief Names the mutable data-store iterator type.
   */
  using Iterator = typename store_type::Iterator;

  /**
   * @brief Names the read-only data-store iterator type.
   */
  using ConstIterator = typename store_type::ConstIterator;

  /**
   * @brief Creates and inserts a typed array.
   *
   * DataStructure owns the array. The array shares ownership of store.
   * @param dataStructure Data structure that owns the array.
   * @param name Array name.
   * @param store Shared data store.
   * @param parentId Optional parent object identifier.
   * @return Array owned by dataStructure, or nullptr if store is null or insertion fails.
   */
  static DataArray* Create(DataStructure& dataStructure, std::string name, std::shared_ptr<store_type> store, const std::optional<IdType>& parentId = {})
  {
    if(store == nullptr)
    {
      return nullptr;
    }
    auto data = std::shared_ptr<DataArray>(new DataArray(dataStructure, std::move(name), std::move(store)));
    if(!AttemptToAddObject(dataStructure, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Creates and inserts a metadata array after storage planning succeeds.
   * @param dataStructure Data structure that owns the array.
   * @param name Array name.
   * @param tupleShape Planned tuple dimensions.
   * @param componentShape Planned component dimensions.
   * @param requestedFormat Explicit format, or empty to use destination policy.
   * @param parentId Optional parent object identifier.
   * @return Inserted array or a planning or insertion error.
   * @throws std::bad_alloc If metadata allocation fails.
   */
  static Result<DataArray*> CreatePlanned(DataStructure& dataStructure, const std::string& name, const ShapeType& tupleShape, const ShapeType& componentShape, const std::string& requestedFormat,
                                          const std::optional<IdType>& parentId = {})
  {
    auto pathResult = ResolveProspectivePath(dataStructure, name, parentId);
    if(pathResult.invalid())
    {
      return ConvertInvalidResult<DataArray*>(std::move(pathResult));
    }

    auto storeResult = PreparePlannedStore(dataStructure, pathResult.value(), tupleShape, componentShape, requestedFormat);
    if(storeResult.invalid())
    {
      return ConvertInvalidResult<DataArray*>(std::move(storeResult));
    }

    auto warnings = std::move(storeResult.warnings());
    auto* data = Create(dataStructure, name, std::move(storeResult.value()), parentId);
    if(data == nullptr)
    {
      auto result = MakeErrorResult<DataArray*>(k_PlannedInsertionError, fmt::format("Cannot insert planned numeric array at path '{}'.", pathResult.value().toString()));
      result.warnings() = std::move(warnings);
      return result;
    }

    Result<DataArray*> result{data};
    result.warnings() = std::move(warnings);
    return result;
  }

  /**
   * @brief Creates and inserts an array with a populated store type.
   * @tparam DataStoreType Concrete AbstractDataStore implementation for T.
   * @param dataStructure Data structure that owns the array.
   * @param name Array name.
   * @param tupleShape Tuple dimensions in slowest-to-fastest order.
   * @param componentShape Component dimensions in slowest-to-fastest order.
   * @param parentId Optional parent object identifier.
   * @return Array owned by dataStructure, or nullptr if insertion fails.
   *
   * EmptyDataStore is prohibited because placeholder creation requires destination policy and a prospective path. Use CreatePlanned or ImportPlanned.
   */
  template <typename DataStoreType>
  static DataArray* CreateWithStore(DataStructure& dataStructure, const std::string& name, const ShapeType& tupleShape, const ShapeType& componentShape, const std::optional<IdType>& parentId = {})
  {
    static_assert(std::is_base_of_v<AbstractDataStore<T>, DataStoreType>);

    if constexpr(std::is_same_v<DataStoreType, EmptyDataStore<T>>)
    {
      static_assert(dependent_false<DataStoreType>, "CreateWithStore<EmptyDataStore<T>> is not supported. Use CreatePlanned or ImportPlanned with a destination policy.");
      return nullptr;
    }
    else
    {
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
  }

  /**
   * @brief Imports and inserts a typed array with a fixed object identifier.
   *
   * DataStructure owns the array. The array shares ownership of store.
   * @param dataStructure Data structure that owns the array.
   * @param name Array name.
   * @param importId Imported object identifier.
   * @param store Shared data store.
   * @param parentId Optional parent object identifier.
   * @return Array owned by dataStructure, or nullptr if store is null or insertion fails.
   */
  static DataArray* Import(DataStructure& dataStructure, std::string name, IdType importId, std::shared_ptr<store_type> store, const std::optional<IdType>& parentId = {})
  {
    if(store == nullptr)
    {
      return nullptr;
    }
    auto data = std::shared_ptr<DataArray>(new DataArray(dataStructure, std::move(name), importId, std::move(store)));
    if(!AttemptToAddObject(dataStructure, data, parentId))
    {
      return nullptr;
    }
    return data.get();
  }

  /**
   * @brief Imports and inserts a metadata array after storage planning succeeds.
   * @param dataStructure Data structure that owns the array.
   * @param name Array name.
   * @param importId Imported object identifier.
   * @param tupleShape Planned tuple dimensions.
   * @param componentShape Planned component dimensions.
   * @param requestedFormat Explicit format, or empty to use destination policy.
   * @param parentId Optional parent object identifier.
   * @return Inserted array or a planning or insertion error.
   * @throws std::bad_alloc If metadata allocation fails.
   */
  static Result<DataArray*> ImportPlanned(DataStructure& dataStructure, const std::string& name, IdType importId, const ShapeType& tupleShape, const ShapeType& componentShape,
                                          const std::string& requestedFormat, const std::optional<IdType>& parentId = {})
  {
    auto pathResult = ResolveProspectivePath(dataStructure, name, parentId);
    if(pathResult.invalid())
    {
      return ConvertInvalidResult<DataArray*>(std::move(pathResult));
    }

    auto storeResult = PreparePlannedStore(dataStructure, pathResult.value(), tupleShape, componentShape, requestedFormat);
    if(storeResult.invalid())
    {
      return ConvertInvalidResult<DataArray*>(std::move(storeResult));
    }

    auto warnings = std::move(storeResult.warnings());
    auto* data = Import(dataStructure, name, importId, std::move(storeResult.value()), parentId);
    if(data == nullptr)
    {
      auto result = MakeErrorResult<DataArray*>(k_PlannedInsertionError, fmt::format("Cannot insert planned imported numeric array at path '{}'.", pathResult.value().toString()));
      result.warnings() = std::move(warnings);
      return result;
    }

    Result<DataArray*> result{data};
    result.warnings() = std::move(warnings);
    return result;
  }

  /**
   * @brief Copies an array and shares its data store.
   * @param other Source array.
   */
  DataArray(const DataArray<T>& other)
  : IDataArray(other)
  , m_DataStore(other.m_DataStore)
  {
  }

  /**
   * @brief Moves an array and its data-store ownership.
   * @param other Source array.
   */
  DataArray(DataArray<T>&& other)
  : IDataArray(std::move(other))
  , m_DataStore(std::move(other.m_DataStore))
  {
  }

  /**
   * @brief Destroys the array.
   */
  ~DataArray() override = default;

  DataObject::Type getDataObjectType() const override
  {
    return Type::DataArray;
  }

  ArrayType getArrayType() const override
  {
    return ArrayType::DataArray;
  }

  /**
   * @brief Creates an array that shares this data store.
   * @return Caller-owned shallow array copy.
   */
  DataObject* shallowCopy() override
  {
    return new DataArray(*this);
  }

  /**
   * @brief Creates and inserts an independent array copy under destination storage policy.
   * @param copyPath Destination path for policy resolution and insertion.
   * @return Inserted copy, or nullptr if the destination exists, ordinary copying fails, or insertion fails.
   * @throws std::bad_alloc If allocation fails.
   *
   * The current DataStructure resolver selects the store format from checked logical bytes and copyPath.
   * Unknown formats use memory in-core and fail in OOC builds. Empty stores remain metadata placeholders.
   * The numeric transfer buffer uses at most 1 MiB. Backend adapters can also allocate one complete tuple.
   * @warning An in-memory destination requires the complete copied array in RAM.
   * Direct store callers must resolve a destination format. This array operation resolves the policy automatically.
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override
  {
    DataStructure& dataStruct = getDataStructureRef();
    if(dataStruct.containsData(copyPath))
    {
      return nullptr;
    }
    std::shared_ptr<store_type> dataStore;
    try
    {
      const auto bytes = CalculateStoreCopyBytes(getTupleShape(), getComponentShape(), sizeof(T));
      auto formatResult = ResolveNumericStorageFormat(dataStruct, copyPath, GetDataType<T>(), bytes, "");
      if(formatResult.invalid())
      {
        throw std::runtime_error(formatResult.errors().front().message);
      }
      const std::shared_ptr<IDataStore> sharedStore = getDataStore()->deepCopy(formatResult.value());
      dataStore = std::dynamic_pointer_cast<store_type>(sharedStore);
    } catch(const std::bad_alloc&)
    {
      throw;
    } catch(const std::exception&)
    {
      return nullptr;
    }
    if(dataStore == nullptr)
    {
      return nullptr;
    }
    // Preserve resident growth initialization when destination policy also selects memory.
    const auto* sourceStorePtr = dynamic_cast<const DataStore<T>*>(getDataStore());
    auto* destinationStorePtr = dynamic_cast<DataStore<T>*>(dataStore.get());
    if(sourceStorePtr != nullptr && destinationStorePtr != nullptr && sourceStorePtr->getStoreType() == IDataStore::StoreType::InMemory &&
       destinationStorePtr->getStoreType() == IDataStore::StoreType::InMemory)
    {
      destinationStorePtr->setInitValue(sourceStorePtr->getInitValue());
    }
    // Insertion assigns the destination identifier.
    std::shared_ptr<DataArray<T>> copy = std::shared_ptr<DataArray<T>>(new DataArray<T>(dataStruct, copyPath.getTargetName(), dataStore));
    if(dataStruct.insert(copy, copyPath.getParent()))
    {
      return copy;
    }
    return nullptr;
  }

  usize getSize() const override
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  usize size() const override
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  bool empty() const override
  {
    return m_DataStore->getNumberOfTuples() == 0;
  }

  size_t getNumberOfTuples() const override
  {
    return m_DataStore->getNumberOfTuples();
  }

  size_t getNumberOfComponents() const override
  {
    return m_DataStore->getNumberOfComponents();
  }

  /**
   * @brief Returns a mutable proxy at a flat index.
   * @param index Flat value index.
   * @return Mutable value proxy.
   * @throws std::runtime_error If the data store is null.
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
   * @brief Sets every component in a tuple.
   * @param tupleIndex Tuple index.
   * @param value Value to store in each component.
   */
  void initializeTuple(usize tupleIndex, T value)
  {
    m_DataStore->fillTuple(tupleIndex, value);
  }

  /**
   * @brief Sets every array value.
   * @param value Value to store.
   */
  void fill(T value)
  {
    m_DataStore->fill(value);
  }

  /**
   * @brief Copies one tuple to another tuple.
   * @param from Source tuple index.
   * @param to Destination tuple index.
   *
   * The method does not check either index.
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
   * @brief Swaps two tuples.
   * @param index0 First tuple index.
   * @param index1 Second tuple index.
   *
   * The method does not check either index.
   */
  void swapTuples(usize index0, usize index1) override
  {
    if(index0 == index1)
    {
      return;
    }
    const auto numComponents = getNumberOfComponents();
    for(usize i = 0; i < numComponents; i++)
    {
      usize fromCompIndex = index0 * numComponents + i;
      usize toCompIndex = index1 * numComponents + i;
      auto valueIdx0 = m_DataStore->getValue(fromCompIndex);
      auto valueIdx1 = m_DataStore->getValue(toCompIndex);
      m_DataStore->setValue(toCompIndex, valueIdx0);
      m_DataStore->setValue(fromCompIndex, valueIdx1);
    }
  }

  /**
   * @brief Swaps the byte order of every value.
   */
  void byteSwapElements()
  {
    for(auto valueRef : *this)
    {
      valueRef.byteSwap();
    }
  }

  /**
   * @brief Returns a value at a flat index.
   * @param index Flat value index.
   * @return Stored value.
   * @throws std::runtime_error If the data store is null.
   */
  value_type operator[](usize index) const
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("DataArray::operator[] requires a valid DataStore");
    }

    return m_DataStore->getValue(index);
  }

  /**
   * @brief Returns a value at a flat index.
   * @param index Flat value index.
   * @return Stored value.
   * @throws std::runtime_error If the data store is null.
   */
  value_type getValue(usize index) const
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("DataArray::operator[] requires a valid DataStore");
    }

    return m_DataStore->getValue(index);
  }

  /**
   * @brief Returns a bounds-checked value at a flat index.
   * @param index Flat value index.
   * @return Stored value.
   * @throws std::runtime_error If the data store is null.
   * @throws std::out_of_range If index is not valid.
   */
  value_type at(usize index) const
  {
    if(m_DataStore == nullptr)
    {
      throw std::runtime_error("DataArray::operator[] requires a valid DataStore");
    }

    return m_DataStore->at(index);
  }

  /**
   * @brief Formats a tuple component as text.
   * @param tupleIndex Tuple index.
   * @param compIndex Component index.
   * @param format fmt format used for floating-point values.
   * @return Formatted component value.
   *
   * Integer values use the default format. Avoid this conversion in tight loops.
   */
  std::string toString(usize tupleIndex, usize compIndex, const std::string& format = "{}") const override;

  /**
   * @brief Parses and stores a tuple component value.
   * @param tupleIndex Tuple index.
   * @param compIndex Component index.
   * @param value Text to parse.
   * @return True if parsing and storage succeed.
   *
   * The array remains unchanged when parsing fails.
   */
  bool setValueFromString(usize tupleIndex, usize compIndex, const std::string& value) override;

  /**
   * @brief Returns a component value from a tuple.
   * @param tupleIndex Tuple index.
   * @param componentIndex Component index.
   * @return Stored component value.
   *
   * The method does not check either index.
   */
  value_type getComponent(usize tupleIndex, usize componentIndex)
  {
    const usize index = tupleIndex * getNumberOfComponents() + componentIndex;
    return m_DataStore->getValue(index);
  }

  /**
   * @brief Stores a component value in a tuple.
   * @param tupleIndex Tuple index.
   * @param componentIndex Component index.
   * @param value Value to store.
   *
   * The method does not check either index.
   */
  void setComponent(usize tupleIndex, usize componentIndex, value_type value)
  {
    const usize index = tupleIndex * getNumberOfComponents() + componentIndex;
    m_DataStore->setValue(index, value);
  }

  /**
   * @brief Stores a value at a flat index.
   * @param index Flat value index.
   * @param value Value to store.
   */
  void setValue(usize index, value_type value)
  {
    m_DataStore->setValue(index, value);
  }

  /**
   * @brief Returns the read-only data store pointer.
   * @return Pointer valid until this array replaces or destroys its store, or null without a store.
   */
  const store_type* getDataStore() const
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns the mutable data store pointer.
   * @return Pointer valid until this array replaces or destroys its store, or null without a store.
   */
  store_type* getDataStore()
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns the mutable storage-neutral data store pointer.
   * @return Pointer valid until this array replaces or destroys its store, or null without a store.
   */
  IDataStore* getIDataStore() override
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns the read-only storage-neutral data store pointer.
   * @return Pointer valid until this array replaces or destroys its store, or null without a store.
   */
  const IDataStore* getIDataStore() const override
  {
    return m_DataStore.get();
  }

  /**
   * @brief Returns the mutable data store.
   * @return Reference valid while this array retains the store.
   * @throws std::runtime_error If the data store is null.
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
   * @brief Returns the read-only data store.
   * @return Reference valid while this array retains the store.
   * @throws std::runtime_error If the data store is null.
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
   * @brief Returns a non-owning data-store observer.
   * @return Weak pointer that expires when no array or caller owns the store.
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
   * @brief Replaces the shared data store with a prepared non-null store.
   * @param store New shared data store.
   * @return Error if store is null. The array retains its current store on error.
   *
   * The array shares ownership of store. This method does not select or create storage.
   */
  [[nodiscard]] Result<> setDataStore(std::shared_ptr<store_type> store)
  {
    if(store == nullptr)
    {
      const auto paths = getDataPaths();
      const std::string location = paths.empty() ? getName() : paths.front().toString();
      return MakeErrorResult(k_NullStoreError, fmt::format("Cannot replace the data store for numeric array '{}': the prepared store is null.", location));
    }
    m_DataStore = std::move(store);
    return {};
  }

  std::string getDataFormat() const override
  {
    return m_DataStore->getDataFormat();
  }

  /**
   * @brief Returns the first value.
   * @return First stored value.
   * @throws std::out_of_range If the array is empty.
   * @throws std::runtime_error If the current store has no values.
   */
  value_type front() const
  {
    return at(0);
  }

  /**
   * @brief Returns the last value.
   * @return Last stored value.
   * @throws std::out_of_range If the array is empty.
   * @throws std::runtime_error If the current store has no values.
   */
  value_type back() const
  {
    return at(getSize() - 1);
  }

  Iterator begin()
  {
    return getDataStore()->begin();
  }

  Iterator end()
  {
    return getDataStore()->end();
  }

  ConstIterator begin() const
  {
    return getDataStore()->begin();
  }

  ConstIterator end() const
  {
    return getDataStore()->end();
  }

  ConstIterator cbegin() const
  {
    return getDataStore()->cbegin();
  }

  ConstIterator cend() const
  {
    return getDataStore()->cend();
  }

  /**
   * @brief Shares the source data store.
   * @param rhs Source array.
   * @return This array.
   */
  DataArray& operator=(const DataArray& rhs)
  {
    m_DataStore = rhs.m_DataStore;
    return *this;
  }

  /**
   * @brief Moves the source data store.
   * @param rhs Source array.
   * @return This array.
   */
  DataArray& operator=(DataArray&& rhs) noexcept
  {
    m_DataStore = std::move(rhs.m_DataStore);
    return *this;
  }

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

  std::string getTypeName() const override
  {
    return GetTypeName();
  }

  /**
   * @brief Flushes data-store changes to its backing target.
   *
   * In-memory stores do not have a backing target.
   */
  void flush() const override
  {
    m_DataStore->flush();
  }

  /**
   * @brief Returns data-store memory usage in bytes.
   * @return Approximate memory usage in bytes.
   */
  uint64 memoryUsage() const override
  {
    return m_DataStore->memoryUsage();
  }

protected:
  /**
   * @brief Creates an array with a shared data store.
   * @param dataStructure Parent data structure.
   * @param name Array name.
   * @param store Prepared shared data store.
   */
  DataArray(DataStructure& dataStructure, std::string name, std::shared_ptr<store_type> store)
  : IDataArray(dataStructure, std::move(name))
  , m_DataStore(std::move(store))
  {
  }

  /**
   * @brief Creates an imported array with a shared data store.
   * @param dataStructure Parent data structure.
   * @param name Array name.
   * @param importId Imported object identifier.
   * @param store Prepared shared data store.
   */
  DataArray(DataStructure& dataStructure, std::string name, IdType importId, std::shared_ptr<store_type> store)
  : IDataArray(dataStructure, std::move(name), importId)
  , m_DataStore(std::move(store))
  {
  }

private:
  static inline constexpr int32 k_PlannedPathError = -10610;
  static inline constexpr int32 k_PlannedCreationError = -10611;
  static inline constexpr int32 k_PlannedInsertionError = -10612;
  static inline constexpr int32 k_NullStoreError = -10613;

  static Result<DataPath> ResolveProspectivePath(DataStructure& dataStructure, const std::string& name, const std::optional<IdType>& parentId)
  {
    if(!DataObject::IsValidName(name))
    {
      return MakeErrorResult<DataPath>(k_PlannedPathError, fmt::format("Cannot plan numeric array '{}': the array name is empty or contains '/'.", name));
    }
    try
    {
      if(!parentId.has_value())
      {
        return {DataPath({name})};
      }

      auto* parent = dataStructure.getData(*parentId);
      if(parent == nullptr)
      {
        return MakeErrorResult<DataPath>(k_PlannedPathError, fmt::format("Cannot plan numeric array '{}': parent object ID {} does not exist.", name, *parentId));
      }
      const auto parentPaths = parent->getDataPaths();
      if(parentPaths.empty())
      {
        return MakeErrorResult<DataPath>(k_PlannedPathError, fmt::format("Cannot plan numeric array '{}': parent object ID {} has no path.", name, *parentId));
      }

      DataPath path = parentPaths.front().createChildPath(name);
      if(dataStructure.getData(path.getParent()) != parent)
      {
        return MakeErrorResult<DataPath>(k_PlannedPathError, fmt::format("Cannot plan numeric array at path '{}': the path does not identify parent object ID {}.", path.toString(), *parentId));
      }
      return {std::move(path)};
    } catch(const std::bad_alloc&)
    {
      throw;
    } catch(const std::exception& error)
    {
      return MakeErrorResult<DataPath>(k_PlannedPathError, fmt::format("Cannot plan numeric array '{}': {}", name, error.what()));
    }
  }

  static Result<std::shared_ptr<store_type>> PreparePlannedStore(const DataStructure& dataStructure, const DataPath& path, const ShapeType& tupleShape, const ShapeType& componentShape,
                                                                 const std::string& requestedFormat)
  {
    try
    {
      const uint64 logicalBytes = CalculateStoreCopyBytes(tupleShape, componentShape, sizeof(T));
      auto formatResult = ResolveNumericStorageFormat(dataStructure, path, GetDataType<T>(), logicalBytes, requestedFormat);
      if(formatResult.invalid())
      {
        return ConvertInvalidResult<std::shared_ptr<store_type>>(std::move(formatResult));
      }

      auto warnings = std::move(formatResult.warnings());
      auto placeholderResult = EmptyDataStore<T>::Create(tupleShape, componentShape, formatResult.value());
      if(placeholderResult.invalid())
      {
        auto result = ConvertInvalidResult<std::shared_ptr<store_type>>(std::move(placeholderResult));
        result.warnings().insert(result.warnings().begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
        for(auto& error : result.errors())
        {
          error.message = fmt::format("Cannot create planned numeric array at path '{}': {}", path.toString(), error.message);
        }
        return result;
      }

      std::shared_ptr<store_type> store(std::move(placeholderResult.value()));
      Result<std::shared_ptr<store_type>> result{std::move(store)};
      result.warnings() = std::move(warnings);
      for(auto&& warning : placeholderResult.warnings())
      {
        result.warnings().push_back(std::move(warning));
      }
      return result;
    } catch(const std::bad_alloc&)
    {
      throw;
    } catch(const std::exception& error)
    {
      return MakeErrorResult<std::shared_ptr<store_type>>(k_PlannedCreationError, fmt::format("Cannot create planned numeric array at path '{}': {}", path.toString(), error.what()));
    }
  }

  std::shared_ptr<store_type> m_DataStore = nullptr;
};

extern template std::string DataArray<int8>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<int16>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<int32>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<int64>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<uint8>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<uint16>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<uint32>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<uint64>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<float32>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<float64>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;
extern template std::string DataArray<bool>::toString(usize tupleIndex, usize compIndex, const std::string& format) const;

extern template bool DataArray<int8>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<uint8>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<int16>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<uint16>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<int32>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<uint32>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<int64>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<uint64>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<float32>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<float64>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);
extern template bool DataArray<bool>::setValueFromString(usize tupleIndex, usize compIndex, const std::string& value);

/**
 * @brief Names a uint8 data array.
 */
using UInt8Array = DataArray<uint8>;

/**
 * @brief Names a uint16 data array.
 */
using UInt16Array = DataArray<uint16>;

/**
 * @brief Names a uint32 data array.
 */
using UInt32Array = DataArray<uint32>;

/**
 * @brief Names a uint64 data array.
 */
using UInt64Array = DataArray<uint64>;

/**
 * @brief Names an int8 data array.
 */
using Int8Array = DataArray<int8>;

/**
 * @brief Names an int16 data array.
 */
using Int16Array = DataArray<int16>;

/**
 * @brief Names an int32 data array.
 */
using Int32Array = DataArray<int32>;

/**
 * @brief Names an int64 data array.
 */
using Int64Array = DataArray<int64>;

/**
 * @brief Names a float32 data array.
 */
using Float32Array = DataArray<float32>;

/**
 * @brief Names a float64 data array.
 */
using Float64Array = DataArray<float64>;

/**
 * @brief Names a bool data array.
 */
using BoolArray = DataArray<bool>;
} // namespace nx::core
