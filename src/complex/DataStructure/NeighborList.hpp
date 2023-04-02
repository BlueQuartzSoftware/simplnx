#pragma once

#include "complex/Common/TypeTraits.hpp"
#include "complex/Common/Types.hpp"
#include "complex/DataStructure/INeighborList.hpp"

namespace complex
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
   * @brief Resizes the internal array to accomondate numTuples
   * @param numTuples
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
   * @brief getValue
   * @param grainId
   * @param index
   * @param ok
   * @return T
   */
  T getValue(int32 grainId, int32 index, bool& ok) const;

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
   * @brief Returns a reference to the target grain ID's data.
   * @param grainId
   * @return VectorType&
   */
  VectorType& getListReference(int32 grainId) const;

  /**
   * @brief getList
   * @param grainId
   * @return SharedVectorType
   */
  SharedVectorType getList(int32 grainId) const;

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
   * @return VectorType&
   */
  VectorType& operator[](int32 grainId);

  /**
   * @brief operator []
   * @param grainId
   * @return VectorType&
   */
  VectorType& operator[](usize grainId);

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

  const std::vector<SharedVectorType>& getValues() const;

protected:
  /**
   * @brief NeighborList
   */
  NeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples);

  /**
   * @brief NeighborList
   */
  NeighborList(DataStructure& dataStructure, const std::string& name, const std::vector<SharedVectorType>& dataVector, IdType importId);

private:
  std::vector<SharedVectorType> m_Array;
  bool m_IsAllocated;
  value_type m_InitValue;
};

template <>
DataType COMPLEX_EXPORT NeighborList<int8>::getDataType() const;

template <>
DataType COMPLEX_EXPORT NeighborList<int16>::getDataType() const;

template <>
DataType COMPLEX_EXPORT NeighborList<int32>::getDataType() const;

template <>
DataType COMPLEX_EXPORT NeighborList<int64>::getDataType() const;

template <>
DataType COMPLEX_EXPORT NeighborList<uint8>::getDataType() const;

template <>
DataType COMPLEX_EXPORT NeighborList<uint16>::getDataType() const;

template <>
DataType COMPLEX_EXPORT NeighborList<uint32>::getDataType() const;

template <>
DataType COMPLEX_EXPORT NeighborList<uint64>::getDataType() const;

template <>
DataType COMPLEX_EXPORT NeighborList<float32>::getDataType() const;

template <>
DataType COMPLEX_EXPORT NeighborList<float64>::getDataType() const;

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
} // namespace complex
