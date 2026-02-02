#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"

#include <vector>

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
   * @brief Returns the number of tuples in the DataStore.
   * @return usize
   */
  virtual usize getNumberOfTuples() const = 0;
  /**
   * @brief Returns the dimensions of the Tuples
   * @return
   */
  virtual const ShapeType& getTupleShape() const = 0;

  /**
   * @brief This method sets the shape of the dimensions to `tupleShape`.
   * @param tupleShape The new shape of the data where the dimensions are "C" ordered
   * from *slowest* to *fastest*.
   */
  virtual void resizeTuples(const ShapeType& tupleShape) = 0;

  /**
   * @brief Clear All Lists
   */
  virtual void clearAllLists() = 0;

  /**
   * @brief Returns the number of elements in the list at the specified grain/tuple index.
   * @param grainId The grain/tuple index to query
   * @return usize The number of elements in the specified list
   */
  virtual usize getListSize(usize grainId) const = 0;

  /**
   * @brief Returns the total number of lists in the list store.
   * @return uint64 The number of lists
   */
  virtual uint64 getNumberOfLists() const = 0;

  /**
   * @brief Returns the total number of lists in the list store.
   * Alias for getNumberOfLists().
   * @return uint64 The number of lists
   */
  uint64 size() const
  {
    return getNumberOfLists();
  }

  /**
   * @brief Clears the array.
   */
  virtual void clear() = 0;

  /**
   * @brief Reads list data from an HDF5 dataset.
   * @param datasetReader The HDF5 DatasetIO to read from
   */
  virtual void readHdf5(const HDF5::DatasetIO& datasetReader) = 0;

  /**
   * @brief Writes list data to an HDF5 dataset.
   * @param datasetReader The HDF5 DatasetIO to write to
   */
  virtual void writeHdf5(HDF5::DatasetIO& datasetReader) = 0;

protected:
  /**
   * @brief Default constructor.
   */
  IListStore() = default;

  /**
   * @brief Copy constructor.
   * @param rhs The IListStore to copy from
   */
  IListStore(const IListStore& rhs) = default;

  /**
   * @brief Move constructor.
   * @param rhs The IListStore to move from
   */
  IListStore(IListStore&& rhs) = default;
};
} // namespace nx::core
