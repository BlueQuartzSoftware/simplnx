#pragma once

#include "simplnx/DataStructure/AbstractStringStore.hpp"
#include "simplnx/DataStructure/IArray.hpp"

#include <mutex>

namespace nx::core
{
class SIMPLNX_EXPORT StringArray : public IArray
{
public:
  using value_type = std::string;
  using collection_type = std::vector<value_type>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using store_type = AbstractStringStore;
  using iterator = typename store_type::iterator;
  using const_iterator = typename store_type::const_iterator;

  static inline constexpr StringLiteral k_TypeName = "StringArray";

  /**
   * @brief Creates an empty StringArray in the specified DataStructure.
   * @param dataStructure The DataStructure to create the StringArray in
   * @param name The name for the new StringArray
   * @param parentId Optional parent object ID to insert the StringArray under
   * @return StringArray* Pointer to the created StringArray, or nullptr if creation failed
   */
  static StringArray* Create(DataStructure& dataStructure, const std::string_view& name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Creates a StringArray in the specified DataStructure with the given tuple shape and initial values.
   * @param dataStructure The DataStructure to create the StringArray in
   * @param name The name for the new StringArray
   * @param tupleShape The shape of the tuple dimensions
   * @param strings Vector of initial string values
   * @param parentId Optional parent object ID to insert the StringArray under
   * @return StringArray* Pointer to the created StringArray, or nullptr if creation failed
   */
  static StringArray* CreateWithValues(DataStructure& dataStructure, const std::string_view& name, const ShapeType& tupleShape, collection_type strings, const std::optional<IdType>& parentId = {});

  /**
   * @brief Imports a StringArray into the specified DataStructure with the given values and import ID.
   * @param dataStructure The DataStructure to import the StringArray into
   * @param name The name for the imported StringArray
   * @param tupleShape The shape of the tuple dimensions
   * @param importId The ID to use for the imported object
   * @param strings Vector of string values to import
   * @param parentId Optional parent object ID to insert the StringArray under
   * @return StringArray* Pointer to the imported StringArray, or nullptr if import failed
   */
  static StringArray* Import(DataStructure& dataStructure, const std::string_view& name, const ShapeType& tupleShape, IdType importId, collection_type strings,
                             const std::optional<IdType>& parentId = {});

  /**
   * @brief Copy constructor.
   * @param other The StringArray to copy from
   */
  StringArray(const StringArray& other);

  /**
   * @brief Move constructor.
   * @param other The StringArray to move from
   */
  StringArray(StringArray&& other) noexcept;

  /**
   * @brief Destructor.
   */
  ~StringArray() noexcept override;

  /**
   * @brief Returns the type of this DataObject.
   * @return DataObject::Type The DataObject type
   */
  DataObject::Type getDataObjectType() const override;

  /**
   * @brief Returns the typename of the instantiated object.
   * @return std::string The typename
   */
  std::string getTypeName() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  ArrayType getArrayType() const override;

  /**
   * @brief Returns a shallow copy of the StringArray without copying data.
   * THE CALLING CODE MUST DISPOSE OF THE RETURNED OBJECT.
   * @return DataObject* Pointer to the shallow copy
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Returns a deep copy of the StringArray including a deep copy of the data.
   * @param copyPath The DataPath for the copied object
   * @return std::shared_ptr<DataObject> Shared pointer to the deep copy
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  /**
   * @brief Returns the total number of strings in the array.
   * @return size_t The number of strings
   */
  size_t size() const override;

  /**
   * @brief Returns a copy of all string values as a vector.
   * @return collection_type Vector containing all string values
   */
  collection_type values() const;

  /**
   * @brief Array subscript operator to access the string at the specified index.
   * @param index The index to access
   * @return reference Reference to the string at the specified index
   */
  reference operator[](usize index);

  /**
   * @brief Const array subscript operator to access the string at the specified index.
   * @param index The index to access
   * @return const_reference Const reference to the string at the specified index
   */
  const_reference operator[](usize index) const;

  /**
   * @brief Returns a const reference to the string at the specified index with bounds checking.
   * @param index The index to access
   * @return const_reference Const reference to the string at the specified index
   */
  const_reference at(usize index) const;

  /**
   * @brief Sets the string value at the specified index.
   * @param index The index to set
   * @param value The string value to set
   */
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

  /**
   * @brief Returns an iterator to the beginning of the strings.
   * @return iterator Iterator to the first string
   */
  iterator begin();

  /**
   * @brief Returns an iterator to the end of the strings.
   * @return iterator Iterator past the last string
   */
  iterator end();

  /**
   * @brief Returns a const iterator to the beginning of the strings.
   * @return const_iterator Const iterator to the first string
   */
  const_iterator begin() const;

  /**
   * @brief Returns a const iterator to the end of the strings.
   * @return const_iterator Const iterator past the last string
   */
  const_iterator end() const;

  /**
   * @brief Returns a const iterator to the beginning of the strings.
   * @return const_iterator Const iterator to the first string
   */
  const_iterator cbegin() const;

  /**
   * @brief Returns a const iterator to the end of the strings.
   * @return const_iterator Const iterator past the last string
   */
  const_iterator cend() const;

  /**
   * @brief Copy assignment operator.
   * @param rhs The StringArray to copy from
   * @return StringArray& Reference to this StringArray
   */
  StringArray& operator=(const StringArray& rhs);

  /**
   * @brief Move assignment operator.
   * @param rhs The StringArray to move from
   * @return StringArray& Reference to this StringArray
   */
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

  /**
   * @brief Replaces the AbstractStringStore used to store values.
   * @param newStore Shared pointer to the new AbstractStringStore
   */
  void setStore(const std::shared_ptr<AbstractStringStore>& newStore);

protected:
  /**
   * @brief Constructs an empty StringArray.
   * @param dataStructure The DataStructure this StringArray belongs to
   * @param name The name for this StringArray
   */
  StringArray(DataStructure& dataStructure, std::string name);

  /**
   * @brief Constructs a StringArray with the specified tuple shape and initial values.
   * @param dataStructure The DataStructure this StringArray belongs to
   * @param name The name for this StringArray
   * @param tupleShape The shape of the tuple dimensions
   * @param strings Vector of initial string values
   */
  StringArray(DataStructure& dataStructure, std::string name, const ShapeType& tupleShape, collection_type strings);

  /**
   * @brief Constructs a StringArray with an existing string store.
   * @param dataStructure The DataStructure this StringArray belongs to
   * @param name The name for this StringArray
   * @param store Shared pointer to the AbstractStringStore to use for storage
   */
  StringArray(DataStructure& dataStructure, std::string name, std::shared_ptr<store_type>& store);

  /**
   * @brief Constructs a StringArray with the specified tuple shape, values, and import ID.
   * @param dataStructure The DataStructure this StringArray belongs to
   * @param name The name for this StringArray
   * @param tupleShape The shape of the tuple dimensions
   * @param importId The ID to use for this imported object
   * @param strings Vector of string values
   */
  StringArray(DataStructure& dataStructure, std::string name, const ShapeType& tupleShape, IdType importId, collection_type strings);

private:
  std::shared_ptr<AbstractStringStore> m_Strings = nullptr;
};
} // namespace nx::core
