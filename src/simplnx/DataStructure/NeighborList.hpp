#pragma once

#include "simplnx/Common/TypeTraits.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractListStore.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"

#include <mutex>

namespace nx::core
{
namespace NeighborListConstants
{
inline constexpr StringLiteral k_TypeName = "NeighborList<T>";
}

/**
 * @class NeighborList
 * @brief
 * @tparam T
 */
template <class T>
class NeighborList : public INeighborList
{
public:
  using value_type = T;
  using VectorType = std::vector<T>;
  using SharedVectorType = std::shared_ptr<VectorType>;
  using store_type = AbstractListStore<T>;
  using iterator = typename store_type::iterator;
  using const_iterator = typename store_type::const_iterator;

  NeighborList() = default;

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param numTuples
   * @param parentId = {}
   * @tparam T
   * @return NeighborList<T>*
   */
  static NeighborList* Create(DataStructure& dataStructure, const std::string& name, usize numTuples, const std::optional<IdType>& parentId = {});

  /**
   * @brief
   * @param dataStructure
   * @param name
   * @param importId
   * @param data
   * @param parentId
   * @return NeighborList<T>*
   */
  static NeighborList* Import(DataStructure& dataStructure, const std::string& name, IdType importId, const std::vector<SharedVectorType>& data, const std::optional<IdType>& parentId = {});

  ~NeighborList() override = default;

  /**
   * @brief Returns a shallow copy of the NeighborList without copying data.
   * THE CALLING CODE MUST DISPOSE OF THE RETURNED OBJECT.
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns a deep copy of the NeighborList including a deep copy of the
   * data.
   * @return DataObject*
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Gives this array a human readable name
   * @param name The name of this array
   */
  virtual void setInitValue(value_type initValue);

  /**
   * @brief Removes Tuples from the Array. If the size of the vector is Zero nothing is done. If the size of the
   * vector is greater than or Equal to the number of Tuples then the Array is Resized to Zero. If there are
   * indices that are larger than the size of the original (before erasing operations) then an error code (-100) is
   * returned from the program.
   * @param idxs The indices to remove
   * @return int32 Error code
   */
  int32 eraseTuples(const std::vector<usize>& idxs);

  /**
   * @brief copyTuple
   * @param currentPos
   * @param newPos
   */
  void copyTuple(usize currentPos, usize newPos) override;

  /**
   * @brief getSize Returns the total number of data items that are being stored. This is the sum of all the sizes
   * of the internal storage arrays for this class.
   * @return usize
   */
  usize getSize() const override;

  /**
   * @brief getSize Returns the total number of data items that are being stored. This is the sum of all the sizes
   * of the internal storage arrays for this class.
   * @return usize
   */
  usize size() const override;

  /**
   * @brief Returns if there are any elements in the array object
   * @return bool, true if the DataArray has a size() == 0
   */
  bool empty() const override;

  /**
   * @brief Set number of components.
   * @param nc
   */
  void setNumberOfComponents(int32 nc);

  /**
   * @brief getTypeSize
   * @return usize
   */
  usize getTypeSize() const;

  /**
   * @brief This will remove all neighbor lists and set the isAllocated flag to false.
   *
   * This is the same as calling ```clearAllLists()```
   */
  void initializeWithZeros();

  /**
   * @brief resizeTotalElements
   * @param size
   * @return int32
   */
  int32 resizeTotalElements(usize size);

  /**
   * @brief This method sets the shape of the dimensions to `tupleShape`.
   *
   * There are 3 possibilities when using this function:
   * [1] The number of tuples of the new shape is *LESS* than the original. In this
   * case a memory allocation will take place and the first 'N' elements of data
   * will be copied into the new array. The remaining data is *LOST*
   *
   * [2] The number of tuples of the new shape is *EQUAL* to the original. In this
   * case the shape is set and the function returns.
   *
   * [3] The number of tuples of the new shape is *GREATER* than the original. In
   * this case a new array is allocated and all the data from the original array
   * is copied into the new array and the remaining elements are initialized to
   * the default initialization value.
   *
   * @param tupleShape The new shape of the data where the dimensions are "C" ordered
   * from *slowest* to *fastest*.
   */
  void resizeTuples(usize numTuples);

  /**
   * @brief addEntry
   * @param grainId
   * @param value
   */
  void addEntry(int32 grainId, value_type value);

  /**
   * @brief Clear All Lists
   */
  void clearAllLists();

  /**
   * @brief setList
   * @param grainId
   * @param neighborList
   */
  void setList(int32 grainId, const SharedVectorType& neighborList);

  /**
   * @brief Updates the tuple count, internal list size, and sets the list values.
   * @param neighborLists
   */
  void setLists(const std::vector<std::vector<T>>& neighborLists);

  /**
   * @brief Specifies the length of the longest list.
   * @param size
   */
  void reserveListSize(uint64 size);

  /**
   * @brief getValue
   * @param grainId
   * @param index
   * @param ok
   * @return T
   */
  T getValue(int32 grainId, int32 index, bool& ok) const;

  /**
   * @brief Sets the value at the given index using mutex locks.
   * @param index
   * @param value
   */
  void setValue(usize index, const VectorType& value);

  /**
   * @brief getNumberOfLists
   * @return int32
   */
  int32 getNumberOfLists() const;

  /**
   * @brief getListSize
   * @param grainId
   * @return int32
   */
  int32 getListSize(int32 grainId) const;

  /**
   * @brief getList
   * @param grainId
   * @return VectorType
   */
  VectorType getList(int32 grainId) const;

  /**
   * @brief Static function to get the typename
   * @return
   */
  static std::string GetTypeName()
  {
    if constexpr(std::is_same_v<T, int8>)
    {
      return "NeighborList<int8>";
    }
    else if constexpr(std::is_same_v<T, uint8>)
    {
      return "NeighborList<uint8>";
    }
    else if constexpr(std::is_same_v<T, int16>)
    {
      return "NeighborList<int16>";
    }
    else if constexpr(std::is_same_v<T, uint16>)
    {
      return "NeighborList<uint16>";
    }
    else if constexpr(std::is_same_v<T, int32>)
    {
      return "NeighborList<int32>";
    }
    else if constexpr(std::is_same_v<T, uint32>)
    {
      return "NeighborList<uint32>";
    }
    else if constexpr(std::is_same_v<T, int64>)
    {
      return "NeighborList<int64>";
    }
    else if constexpr(std::is_same_v<T, uint64>)
    {
      return "NeighborList<uint64>";
    }
    else if constexpr(std::is_same_v<T, float32>)
    {
      return "NeighborList<float32>";
    }
    else if constexpr(std::is_same_v<T, float64>)
    {
      return "NeighborList<float64>";
    }
    else
    {
      static_assert(dependent_false<T>, "Unsupported type T in NeighborList");
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
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  ArrayType getArrayType() const override
  {
    return ArrayType::NeighborListArray;
  }

  /**
   * @brief copyOfList
   * @param grainId
   * @return VectorType
   */
  VectorType copyOfList(int32 grainId) const;

  /**
   * @brief operator []
   * @param grainId
   * @return VectorType
   */
  VectorType operator[](int32 grainId);

  /**
   * @brief operator []
   * @param grainId
   * @return VectorType
   */
  VectorType operator[](usize grainId);

  /**
   * @brief Returns a const reference to the VectorType value found at the specified index. This cannot be used to edit the VectorType value found at the specified index.
   * @param grainId
   * @return VectorType
   */
  VectorType at(int32 grainId) const;

  /**
   * @brief Returns a const reference to the VectorType value found at the specified index. This cannot be used to edit the VectorType value found at the specified index.
   * @param grainId
   * @return VectorType
   */
  VectorType at(usize grainId) const;

  /**
   * @brief Returns the DataArray's value type as an enum
   * @return DataType
   */
  DataType getDataType() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Resizes the internal array to accomondate
   */
  void resizeTuples(const std::vector<usize>& tupleShape) override;

  /**
   * @brief Returns a shared_ptr to the underlying list store.
   * @return std::shared_ptr<store_type>
   */
  std::shared_ptr<store_type> getStore() const;

  /**
   * @brief Returns a vector of vectors containing the current values.
   */
  std::vector<VectorType> getVectors() const;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  NeighborList& operator=(const NeighborList& rhs);
  NeighborList& operator=(NeighborList&& rhs);

protected:
  /**
   * @brief NeighborList
   */
  NeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples);

  /**
   * @brief NeighborList
   */
  NeighborList(DataStructure& dataStructure, const std::string& name, const std::vector<SharedVectorType>& dataVector, IdType importId);

  NeighborList(const NeighborList& other);

private:
  std::shared_ptr<store_type> m_Store;
  mutable std::mutex m_Mutex;
  bool m_IsAllocated;
  value_type m_InitValue;
};

template <>
DataType SIMPLNX_EXPORT NeighborList<int8>::getDataType() const;

template <>
DataType SIMPLNX_EXPORT NeighborList<int16>::getDataType() const;

template <>
DataType SIMPLNX_EXPORT NeighborList<int32>::getDataType() const;

template <>
DataType SIMPLNX_EXPORT NeighborList<int64>::getDataType() const;

template <>
DataType SIMPLNX_EXPORT NeighborList<uint8>::getDataType() const;

template <>
DataType SIMPLNX_EXPORT NeighborList<uint16>::getDataType() const;

template <>
DataType SIMPLNX_EXPORT NeighborList<uint32>::getDataType() const;

template <>
DataType SIMPLNX_EXPORT NeighborList<uint64>::getDataType() const;

template <>
DataType SIMPLNX_EXPORT NeighborList<float32>::getDataType() const;

template <>
DataType SIMPLNX_EXPORT NeighborList<float64>::getDataType() const;

/**
 * Boolean NeighborLists are uncompilable
 */

// -----------------------------------------------------------------------------
// Declare our extern templates
extern template class NeighborList<int8>;
extern template class NeighborList<uint8>;
extern template class NeighborList<int16>;
extern template class NeighborList<uint16>;
extern template class NeighborList<int32>;
extern template class NeighborList<uint32>;
extern template class NeighborList<int64>;
extern template class NeighborList<uint64>;

extern template class NeighborList<float32>;
extern template class NeighborList<float64>;

extern template class NeighborList<usize>;

// Declare Aliases
using UInt8NeighborList = NeighborList<uint8>;
using UInt16NeighborList = NeighborList<uint16>;
using UInt32NeighborList = NeighborList<uint32>;
using UInt64NeighborList = NeighborList<uint64>;

using Int8NeighborList = NeighborList<int8>;
using Int16NeighborList = NeighborList<int16>;
using Int32NeighborList = NeighborList<int32>;
using Int64NeighborList = NeighborList<int64>;

using Float32NeighborList = NeighborList<float32>;
using Float64NeighborList = NeighborList<float64>;

using VectorOfFloat32NeighborList = std::vector<std::shared_ptr<Float32NeighborList>>;
} // namespace nx::core
