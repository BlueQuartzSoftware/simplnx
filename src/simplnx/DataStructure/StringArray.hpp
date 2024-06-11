#pragma once

#include "simplnx/DataStructure/IArray.hpp"
#include "simplnx/DataStructure/AbstractStringStore.hpp"

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

  static StringArray* Create(DataStructure& dataStructure, const std::string_view& name, const std::optional<IdType>& parentId = {});
  static StringArray* CreateWithValues(DataStructure& dataStructure, const std::string_view& name, collection_type strings, const std::optional<IdType>& parentId = {});

  static StringArray* Import(DataStructure& dataStructure, const std::string_view& name, IdType importId, collection_type strings, const std::optional<IdType>& parentId = {});

  StringArray(const StringArray& other);
  StringArray(StringArray&& other) noexcept;

  ~StringArray() noexcept override;

  DataObject::Type getDataObjectType() const override;
  std::string getTypeName() const override;

  /**
   * @brief Returns an enumeration of the class or subclass. Used for quick comparison or type deduction
   * @return
   */
  ArrayType getArrayType() const override;

  DataObject* shallowCopy() override;
  std::shared_ptr<DataObject> deepCopy(const DataPath& copyPath) override;

  size_t size() const override;
  const collection_type values() const;

  reference operator[](usize index);
  const_reference operator[](usize index) const;
  const_reference at(usize index) const;
  void setValue(usize index, const std::string& value);

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  StringArray& operator=(const StringArray& rhs);
  StringArray& operator=(StringArray&& rhs) noexcept;

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
  void resizeTuples(const std::vector<usize>& tupleShape) override;

protected:
  StringArray(DataStructure& dataStructure, std::string name);
  StringArray(DataStructure& dataStructure, std::string name, collection_type strings);
  StringArray(DataStructure& dataStructure, std::string name, std::shared_ptr<store_type>& store);
  StringArray(DataStructure& dataStructure, std::string name, IdType importId, collection_type strings);

private:
  std::shared_ptr<AbstractStringStore> m_Strings = nullptr;
  mutable std::mutex m_Mutex;
};
} // namespace nx::core
