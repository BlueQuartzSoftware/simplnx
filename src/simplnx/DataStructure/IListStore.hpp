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
