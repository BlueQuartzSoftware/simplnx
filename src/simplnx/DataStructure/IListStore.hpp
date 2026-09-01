#pragma once

#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Common/Types.hpp"

#include <vector>

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

namespace HDF5
{
class DatasetIO;
}

/**
 * @class IListStore
 * @brief Defines shared metadata and I/O operations for list stores.
 */
class IListStore
{
public:
  /**
   * @brief Destroys the list store.
   */
  virtual ~IListStore() = default;

  /**
   * @brief Reports whether list data is out-of-core.
   * @return True when list data uses nonresident storage.
   */
  virtual bool isOutOfCore() const noexcept
  {
    return false;
  }

  virtual usize getNumberOfTuples() const = 0;

  /**
   * @brief Returns the tuple shape.
   * @return Reference that remains valid until the store changes shape or is destroyed.
   */
  virtual const ShapeType& getTupleShape() const = 0;

  /**
   * @brief Changes the tuple shape.
   * @param tupleShape New tuple dimensions in slowest-to-fastest order.
   */
  virtual void resizeTuples(const ShapeType& tupleShape) = 0;

  /**
   * @brief Removes all list values.
   */
  virtual void clearAllLists() = 0;

  /**
   * @brief Returns the value count in one list.
   * @param grainId Tuple index of the list.
   * @return Value count in the selected list.
   */
  virtual usize getListSize(usize grainId) const = 0;

  virtual usize getNumberOfLists() const = 0;

  virtual usize size() const = 0;

  /**
   * @brief Removes all list values.
   */
  virtual void clear() = 0;

  /**
   * @brief Reads list data from an HDF5 dataset.
   * @param datasetReader HDF5 dataset to read.
   */
  virtual void readHdf5(const HDF5::DatasetIO& datasetReader) = 0;

  /**
   * @brief Writes list data to an HDF5 dataset.
   * @param datasetReader HDF5 dataset to write.
   */
  virtual void writeHdf5(HDF5::DatasetIO& datasetReader) = 0;

protected:
  /**
   * @brief Creates a list store.
   */
  IListStore() = default;

  /**
   * @brief Copies base list-store state.
   * @param rhs List store to copy.
   */
  IListStore(const IListStore& rhs) = default;

  /**
   * @brief Moves base list-store state.
   * @param rhs List store to move.
   */
  IListStore(IListStore&& rhs) = default;
};
} // namespace nx::core
