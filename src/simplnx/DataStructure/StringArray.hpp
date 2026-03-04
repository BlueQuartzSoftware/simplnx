#pragma once

#include "simplnx/DataStructure/AbstractArray.hpp"
#include "simplnx/DataStructure/IStringStore.hpp"

#include <mutex>

namespace nx::core
{
class SIMPLNX_EXPORT StringArray : public AbstractArray
{
public:
  using value_type = std::string;
  using collection_type = std::vector<value_type>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using store_type = IStringStore;
  using iterator = typename store_type::iterator;
  using const_iterator = typename store_type::const_iterator;

  static constexpr StringLiteral k_TypeName = "StringArray";

  static StringArray* Create(DataStructure& dataStructure, const std::string_view& name, const std::optional<IdType>& parentId = {});
  static StringArray* CreateWithValues(DataStructure& dataStructure, const std::string_view& name, const ShapeType& tupleShape, collection_type strings, const std::optional<IdType>& parentId = {});

  static StringArray* Import(DataStructure& dataStructure, const std::string_view& name, const ShapeType& tupleShape, IdType importId, collection_type strings,
                             const std::optional<IdType>& parentId = {});

  StringArray(const StringArray& other);
  StringArray(StringArray&& other) noexcept;

  ~StringArray() noexcept override;

  AbstractDataObject::Type getDataObjectType() const override;
  std::string getTypeName() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  ArrayType getArrayType() const override;

  AbstractDataObject* shallowCopy() override;
  std::shared_ptr<AbstractDataObject> deepCopy(const DataPath& copyPath) override;

  size_t size() const override;
  collection_type values() const;

  reference operator[](usize index);
  const_reference operator[](usize index) const;
  const_reference at(usize index) const;
  void setValue(usize index, const std::string& value);

  /**
   * @brief Returns the value at the tuple and component index as a std::string.
   *        NOTE: This function is slow and should be used sparingly and avoided inside of a tight loop!
   * @param tupleIndex
   * @param compIndex The component index is ignored here
   * @param format The fmt formatting string is ignored here
   * @return std::string
   */
  std::string toString(usize tupleIndex, usize compIndex, const std::string& format = "{}") const override;

  /**
   * @brief Sets the value at the tuple and component index to the passed in string value
   * @param tupleIndex
   * @param compIndex The component index is ignored here
   * @param value
   * @return bool
   */
  bool setValueFromString(usize tupleIndex, usize compIndex, const std::string& value) override;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  StringArray& operator=(const StringArray& rhs);
  StringArray& operator=(StringArray&& rhs) noexcept;

  /**
   * @brief Swaps the tuple values between the 2 indices
   * @param index0 The first index to swap
   * @param index1 The second index to swap
   */
  void swapTuples(usize index0, usize index1) override;

  /**
   * @brief Returns the number of elements.
   * @return usize
   */
  usize getSize() const override;

  /**
   * @brief Returns if there are any elements in the array object
   * @return bool, true if the DataArray has a size() == 0
   */
  bool empty() const override;

  /**
   * @brief Returns the tuple shape.
   * @return
   */
  ShapeType getTupleShape() const override;

  /**
   * @brief Returns the component shape.
   * @return
   */
  ShapeType getComponentShape() const override;

  /**
   * @brief Returns the number of tuples.
   * @return usize
   */
  usize getNumberOfTuples() const override;

  /**
   * @brief Returns the number of components per tuple.
   * @return usize
   */
  usize getNumberOfComponents() const override;

  /**
   * @brief This method sets the shape of the dimensions to `tupleShape`.
   * @param tupleShape The new shape of the data where the dimensions are "C" ordered
   * from *slowest* to *fastest*.
   */
  void resizeTuples(const ShapeType& tupleShape) override;

  void setStore(const std::shared_ptr<IStringStore>& newStore);

protected:
  StringArray(DataStructure& dataStructure, std::string name);
  StringArray(DataStructure& dataStructure, std::string name, const ShapeType& tupleShape, collection_type strings);
  StringArray(DataStructure& dataStructure, std::string name, std::shared_ptr<store_type>& store);
  StringArray(DataStructure& dataStructure, std::string name, const ShapeType& tupleShape, IdType importId, collection_type strings);

private:
  std::shared_ptr<IStringStore> m_Strings = nullptr;
};
} // namespace nx::core
