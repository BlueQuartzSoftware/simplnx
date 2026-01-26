#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/DataStructure/DataObject.hpp"

namespace nx::core
{
class SIMPLNX_EXPORT IArray : public DataObject
{
public:
  static inline constexpr StringLiteral k_TypeName = "IArray";

  enum class ArrayType : EnumType
  {
    StringArray,
    DataArray,
    NeighborListArray,
    Any
  };

  ~IArray() override = default;

  IArray() = delete;
  IArray(const IArray&) = default;
  IArray(IArray&&) = default;

  IArray& operator=(const IArray&) = delete;
  IArray& operator=(IArray&&) noexcept = delete;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  virtual ArrayType getArrayType() const = 0;

  /**
   * @brief Returns the number of elements.
   * @return usize
   */
  virtual usize getSize() const = 0;

  /**
   * @brief Returns the number of elements.
   * @return usize
   */
  virtual usize size() const = 0;

  /**
   * @brief Returns if there are any elements in the array object
   * @return bool, true if the DataArray has a size() == 0
   */
  virtual bool empty() const = 0;

  /**
   * @brief Returns the tuple shape.
   * @return
   */
  virtual ShapeType getTupleShape() const = 0;

  /**
   * @brief Returns the component shape.
   * @return
   */
  virtual ShapeType getComponentShape() const = 0;

  /**
   * @brief Returns the number of tuples.
   * @return usize
   */
  virtual usize getNumberOfTuples() const = 0;

  /**
   * @brief Returns the number of components per tuple.
   * @return usize
   */
  virtual usize getNumberOfComponents() const = 0;

  /**
   * @brief Swaps the tuple values between the 2 indices
   * @param index0 The first index to swap
   * @param index1 The second index to swap
   */
  virtual void swapTuples(usize index0, usize index1) = 0;

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
  virtual void resizeTuples(const ShapeType& tupleShape) = 0;

  /**
   * @brief Returns the value at the tuple and component index as a std::string.
   *        NOTE: This function is slow and should be used sparingly and avoided inside of a tight loop!
   * @param tupleIndex
   * @param compIndex
   * @param format Optional fmt formatting, this will be ignored for integer types and a simple {} will always be used
   * @return std::string
   */
  virtual std::string toString(const usize tupleIndex, const usize compIndex, const std::string& format = "{}") const = 0;

  /**
   * @brief Sets the value at the tuple and component index to the value parsed as the appropriate type.
   *        If the value cannot be properly parsed as the appropriate type, nothing is changed.
   * @param tupleIndex
   * @param compIndex
   * @param value
   * @return bool
   */
  virtual bool setValueFromString(const usize tupleIndex, const usize compIndex, const std::string& value) = 0;

  static std::set<std::string> StringListFromArrayType(const std::set<ArrayType>& arrayTypes)
  {
    static const std::map<ArrayType, std::string> k_TypeToStringMap = {
        {ArrayType::StringArray, "StringArray"}, {ArrayType::DataArray, "DataArray"}, {ArrayType::NeighborListArray, "NeighborListArray"}, {ArrayType::Any, "Any"}};

    std::set<std::string> stringValues;
    for(auto arrayType : arrayTypes)
    {
      stringValues.insert(k_TypeToStringMap.at(arrayType));
    }
    return stringValues;
  }

protected:
  IArray(DataStructure& dataStructure, std::string name)
  : DataObject(dataStructure, std::move(name))
  {
  }

  IArray(DataStructure& dataStructure, std::string name, IdType importId)
  : DataObject(dataStructure, std::move(name), importId)
  {
  }
};
} // namespace nx::core
