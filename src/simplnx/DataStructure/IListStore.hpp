#pragma once

#include "simplnx/Common/Types.hpp"

namespace nx::core
{
namespace HDF5
{
class DatasetIO;
}

class IListStore
{
public:
  virtual ~IListStore() = default;

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
  virtual void resizeTuples(usize tupleCount) = 0;

  /**
   * @brief Clear All Lists
   */
  virtual void clearAllLists() = 0;

  virtual usize getListSize(usize grainId) const = 0;

  /**
   * @brief getNumberOfLists
   * @return int32
   */
  virtual uint64 getNumberOfLists() const = 0;

  uint64 size() const
  {
    return getNumberOfLists();
  }

  /**
   * @brief Clears the array.
   */
  virtual void clear() = 0;

  virtual void readHdf5(const HDF5::DatasetIO& datasetReader) = 0;
  virtual void writeHdf5(HDF5::DatasetIO& datasetReader) = 0;

protected:
  IListStore() = default;
  IListStore(const IListStore& rhs) = default;
  IListStore(IListStore&& rhs) = default;
};
} // namespace nx::core
