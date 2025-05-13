#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AbstractDataStore.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"

namespace nx::core::MaskCompareUtilities
{
/**
 * @brief These structs and functions are meant to make using a "mask array" or "Good Voxels Array" easier
 * for the developer. There is virtual function call overhead with using these structs and functions.
 *
 * An example use of these functions would be the following:
 * @code
 *  std::unique_ptr<MaskCompare> maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->goodVoxelsArrayPath);
 *  if(!maskCompare->bothTrue(arrayIndex, anotherArrayIndex))
 *  {
 *    // Do something based on the if statement...
 *  }
 * @endcode
 */
struct MaskCompare
{
  virtual ~MaskCompare() noexcept = default;

  /**
   * @brief Both of the values pointed to by the index *must* be `true` or non-zero. If either of the values or
   * *both* of the values are false, this will return false.
   * @param indexA First index
   * @param indexB Second index
   * @return
   */
  virtual bool bothTrue(usize indexA, usize indexB) const = 0;

  /**
   * @brief Both of the values pointed to by the index *must* be `false` or non-zero. If either of the values or
   * *both* of the values are `true`, this will return `false`.
   * @param indexA
   * @param indexB
   * @return
   */
  virtual bool bothFalse(usize indexA, usize indexB) const = 0;

  /**
   * @brief Returns `true` or `false` based on the value at the index
   * @param index index to check
   * @return
   */
  virtual bool isTrue(usize index) const = 0;

  virtual void setValue(usize index, bool val) = 0;

  virtual usize getNumberOfTuples() const = 0;

  virtual usize getNumberOfComponents() const = 0;

  virtual usize countTrueValues() const = 0;
};

struct BoolMaskCompare : public MaskCompare
{
  BoolMaskCompare(AbstractDataStore<bool>& dataStore)
  : m_DataStore(dataStore)
  {
  }
  ~BoolMaskCompare() noexcept override = default;

  AbstractDataStore<bool>& m_DataStore;
  bool bothTrue(usize indexA, usize indexB) const override
  {
    return m_DataStore.at(indexA) && m_DataStore.at(indexB);
  }
  bool bothFalse(usize indexA, usize indexB) const override
  {
    return !m_DataStore.at(indexA) && !m_DataStore.at(indexB);
  }
  bool isTrue(usize index) const override
  {
    return m_DataStore.at(index);
  }
  void setValue(usize index, bool val) override
  {
    m_DataStore[index] = val;
  }
  usize getNumberOfTuples() const override
  {
    return m_DataStore.getNumberOfTuples();
  }
  usize getNumberOfComponents() const override
  {
    return m_DataStore.getNumberOfComponents();
  }

  usize countTrueValues() const override
  {
    return std::count(m_DataStore.begin(), m_DataStore.end(), true);
  }
};

struct UInt8MaskCompare : public MaskCompare
{
  UInt8MaskCompare(AbstractDataStore<uint8>& dataStore)
  : m_DataStore(dataStore)
  {
  }
  ~UInt8MaskCompare() noexcept override = default;

  AbstractDataStore<uint8>& m_DataStore;
  bool bothTrue(usize indexA, usize indexB) const override
  {
    return m_DataStore.at(indexA) != 0 && m_DataStore.at(indexB) != 0;
  }
  bool bothFalse(usize indexA, usize indexB) const override
  {
    return m_DataStore.at(indexA) == 0 && m_DataStore.at(indexB) == 0;
  }
  bool isTrue(usize index) const override
  {
    return m_DataStore.at(index) != 0;
  }
  void setValue(usize index, bool val) override
  {
    m_DataStore[index] = static_cast<uint8>(val);
  }
  usize getNumberOfTuples() const override
  {
    return m_DataStore.getNumberOfTuples();
  }
  usize getNumberOfComponents() const override
  {
    return m_DataStore.getNumberOfComponents();
  }

  usize countTrueValues() const override
  {
    const usize falseCount = std::count(m_DataStore.begin(), m_DataStore.end(), 0);
    return getNumberOfTuples() - falseCount;
  }
};

/**
 * @brief Convenience method to create an instance of the MaskCompare subclass.
 *
 * An example use of these functions would be the following:
 * @code
 *  std::unique_ptr<MaskCompare> maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->goodVoxelsArrayPath);
 *  if(!maskCompare->bothTrue(arrayIndex, anotherArrayIndex))
 *  {
 *    // Do something based on the if statement...
 *  }
 * @endcode
 *
 * @param dataStructure The DataStructure object to pull the DataArray from
 * @param maskArrayPath The DataPath of the mask array.
 * @return
 */
SIMPLNX_EXPORT std::unique_ptr<MaskCompare> InstantiateMaskCompare(DataStructure& dataStructure, const DataPath& maskArrayPath);

/**
 * @brief Convenience method to create an instance of the MaskCompare subclass
 * @param maskArrayPtr A Pointer to the mask array which can be of either `bool` or `uint8` type.
 * @return
 */
SIMPLNX_EXPORT std::unique_ptr<MaskCompare> InstantiateMaskCompare(IDataArray& maskArrayPtr);
} // namespace nx::core::MaskCompareUtilities