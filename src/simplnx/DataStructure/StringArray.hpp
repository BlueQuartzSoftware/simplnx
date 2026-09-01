#pragma once

#include "simplnx/DataStructure/AbstractStringStore.hpp"
#include "simplnx/DataStructure/IArray.hpp"

#include <mutex>

namespace nx::core
{

/**
 * @class StringArray
 * @brief Stores one string for each tuple.
 *
 * StringArray delegates values and tuple metadata to a shared string store.
 * A placeholder store preserves import metadata without materializing strings.
 * Copy construction and copy assignment share the store. Move assignment copies
 * DataObject metadata and moves only the store.
 */
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
   * @brief Creates an empty array.
   * @param dataStructure Owns the new array.
   * @param name Names the array.
   * @param parentId Identifies the optional parent object.
   * @return DataStructure-owned array, or nullptr when insertion fails.
   */
  static StringArray* Create(DataStructure& dataStructure, const std::string_view& name, const std::optional<IdType>& parentId = {});

  /**
   * @brief Creates an array with initial values.
   * @param dataStructure Owns the new array.
   * @param name Names the array.
   * @param tupleShape Specifies tuple dimensions.
   * @param strings Supplies initial values.
   * @param parentId Identifies the optional parent object.
   * @return DataStructure-owned array, or nullptr when insertion fails.
   *
   * When tupleShape represents nonzero tuples, an empty initial value set must
   * replace its store before value access. A zero-tuple array is valid.
   */
  static StringArray* CreateWithValues(DataStructure& dataStructure, const std::string_view& name, const ShapeType& tupleShape, collection_type strings, const std::optional<IdType>& parentId = {});

  /**
   * @brief Imports an array with an object identifier.
   * @param dataStructure Owns the new array.
   * @param name Names the array.
   * @param tupleShape Specifies tuple dimensions.
   * @param importId Identifies the imported object.
   * @param strings Supplies imported values.
   * @param parentId Identifies the optional parent object.
   * @return DataStructure-owned array, or nullptr when insertion fails.
   *
   * When tupleShape represents nonzero tuples, an empty imported value set must
   * replace its initial store before value access. A zero-tuple array is valid.
   */
  static StringArray* Import(DataStructure& dataStructure, const std::string_view& name, const ShapeType& tupleShape, IdType importId, collection_type strings,
                             const std::optional<IdType>& parentId = {});

  /**
   * @brief Copies the array.
   * @param other Source array.
   * @post Shares the source string store.
   */
  StringArray(const StringArray& other);

  /**
   * @brief Moves the array.
   * @param other Source array.
   * @post Moves the base object and string store.
   */
  StringArray(StringArray&& other) noexcept;

  ~StringArray() noexcept override;

  DataObject::Type getDataObjectType() const override;
  std::string getTypeName() const override;
  ArrayType getArrayType() const override;

  /**
   * @brief Creates a shallow copy.
   * @return Caller-owned array that shares this string store.
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Copies the array into the data structure.
   * @param copyPath Identifies the destination array.
   * @return Inserted object, or nullptr when copyPath exists or insertion fails.
   * @post The inserted object shares this string store.
   */
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  size_t size() const override;

  /**
   * @brief Returns all string values.
   * @return Copy of stored values in flat tuple order.
   * @throws std::runtime_error if the store is a placeholder.
   * @pre The array has a non-null store.
   */
  collection_type values() const;

  /**
   * @brief Returns a mutable string.
   * @param index Identifies the flat string.
   * @return Mutable stored string.
   * @pre index is less than size().
   * @pre The store is materialized.
   */
  reference operator[](usize index);

  /**
   * @brief Returns a constant string.
   * @param index Identifies the flat string.
   * @return Constant stored string.
   * @pre index is less than size().
   * @pre The store is materialized.
   */
  const_reference operator[](usize index) const;

  /**
   * @brief Returns a bounds-checked string.
   * @param index Identifies the flat string.
   * @return Constant stored string.
   * @throws std::out_of_range if index is invalid for a materialized store.
   * @throws std::runtime_error if the store is a placeholder.
   * @pre The array has a non-null store.
   */
  const_reference at(usize index) const;

  /**
   * @brief Stores a string.
   * @param index Identifies the flat string.
   * @param value Supplies the string.
   * @throws std::out_of_range if index is invalid for a materialized store.
   * @throws std::runtime_error if the store is a placeholder.
   * @pre The array has a non-null store.
   */
  void setValue(usize index, const std::string& value);

  /**
   * @brief Returns a tuple string.
   * @param tupleIndex Identifies the tuple.
   * @param compIndex Must be zero because StringArray has one component.
   * @param format Is ignored.
   * @return Selected string.
   * @pre The array has a non-null store.
   */
  std::string toString(usize tupleIndex, usize compIndex, const std::string& format = "{}") const override;

  /**
   * @brief Stores a tuple string.
   * @param tupleIndex Identifies the tuple.
   * @param compIndex Is ignored.
   * @param value Supplies the string.
   * @return True after the store accepts value.
   */
  bool setValueFromString(usize tupleIndex, usize compIndex, const std::string& value) override;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  /**
   * @brief Copies array metadata and the shared store.
   * @param rhs Source array.
   * @return This array.
   */
  StringArray& operator=(const StringArray& rhs);

  /**
   * @brief Copies base metadata and moves the source store.
   * @param rhs Source array.
   * @return This array.
   *
   * The implementation calls DataObject::operator=(rhs) and moves only the
   * string store.
   */
  StringArray& operator=(StringArray&& rhs) noexcept;

  void swapTuples(usize index0, usize index1) override;

  usize getSize() const override;
  bool empty() const override;
  ShapeType getTupleShape() const override;
  ShapeType getComponentShape() const override;
  usize getNumberOfTuples() const override;
  usize getNumberOfComponents() const override;

  /**
   * @brief Changes tuple dimensions.
   * @param tupleShape Specifies tuple dimensions.
   * @pre The array has a non-null store.
   */
  void resizeTuples(const ShapeType& tupleShape) override;

  /**
   * @brief Replaces the shared string store.
   * @param newStore Shared store for values and tuple metadata.
   *
   * A null store is treated as a placeholder. Other value operations require a
   * materialized non-null store.
   */
  void setStore(const std::shared_ptr<AbstractStringStore>& newStore);

  /**
   * @brief Reports whether values are unavailable.
   * @return True when the store is null or is a placeholder.
   *
   * Import uses placeholders to preserve tuple shape without allocating strings.
   */
  bool isPlaceholder() const;

protected:
  StringArray(DataStructure& dataStructure, std::string name);
  StringArray(DataStructure& dataStructure, std::string name, const ShapeType& tupleShape, collection_type strings);
  StringArray(DataStructure& dataStructure, std::string name, std::shared_ptr<store_type>& store);
  StringArray(DataStructure& dataStructure, std::string name, const ShapeType& tupleShape, IdType importId, collection_type strings);

private:
  std::shared_ptr<AbstractStringStore> m_Strings = nullptr;
};
} // namespace nx::core
