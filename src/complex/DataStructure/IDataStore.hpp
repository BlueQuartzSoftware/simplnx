#pragma once

#include "complex/DataStructure/DataObject.hpp"
#include "complex/Utilities/Parsing/HDF5/H5.hpp"
#include "complex/Utilities/Parsing/HDF5/H5DatasetReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Support.hpp"

#include <algorithm>
#include <iterator>

#include "fmt/format.h"

namespace complex
{
namespace H5
{
class DatasetWriter;
} // namespace H5

/**
 * @class IDataStore
 * @brief The IDataStore class serves as an interface class for the
 * various types of data stores used in DataArrays. The basic API and iterators
 * are defined but the specifics relating to how data is stored are implemented
 * in subclasses.
 */
class COMPLEX_EXPORT IDataStore
{
public:
  using ShapeType = typename std::vector<usize>;

  enum class StoreType : int32
  {
    Unknown = -1,
    InMemory = 0,
    Empty,
  };

  virtual ~IDataStore() = default;

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
   * @brief Returns the number of components.
   * @return usize
   */
  virtual usize getNumberOfComponents() const = 0;

  /**
   * @brief Returns the dimensions of the Components
   * @return
   */
  virtual const ShapeType& getComponentShape() const = 0;

  /**
   * @brief Returns the number of values stored within the DataStore.
   * @return usize
   */
  usize getSize() const
  {
    return getNumberOfTuples() * getNumberOfComponents();
  }

  /**
   * @brief Resizes the DataStore to handle the specified number of tuples.
   * @param numTuples
   */
  virtual void reshapeTuples(const ShapeType& tupleShape) = 0;

  /**
   * @brief Returns the DataStore's DataType as an enum
   * @return DataType
   */
  virtual DataType getDataType() const = 0;

  /**
   * @brief Returns the store type e.g. in memory, out of core, etc.
   * @return StoreType
   */
  virtual StoreType getStoreType() const = 0;

  /**
   * @brief Returns the size of the stored type of the data store.
   * @return usize
   */
  virtual usize getTypeSize() const = 0;

  /**
   * @brief Returns a deep copy of the data store and all its data.
   * @return std::unique_ptr<IDataStore>
   */
  virtual std::unique_ptr<IDataStore> deepCopy() const = 0;

  /**
   * @brief Returns a data store of the same type as this but with default initialized data.
   * @return std::unique_ptr<IDataStore>
   */
  virtual std::unique_ptr<IDataStore> createNewInstance() const = 0;

  /**
   * @brief Writes the data store to HDF5. Returns the HDF5 error code should
   * one be encountered. Otherwise, returns 0.
   * @param datasetWriter
   * @return H5::ErrorType
   */
  virtual H5::ErrorType writeHdf5(H5::DatasetWriter& datasetWriter) const = 0;

protected:
  /**
   * @brief Default constructor
   */
  IDataStore() = default;

  static ShapeType ReadTupleShape(const H5::DatasetReader& datasetReader)
  {
    H5::AttributeReader tupleShapeAttribute = datasetReader.getAttribute(complex::H5::k_TupleShapeTag);
    if(!tupleShapeAttribute.isValid())
    {
      throw std::runtime_error(fmt::format("Error reading Tuple Shape from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
    }
    return tupleShapeAttribute.readAsVector<usize>();
  }

  static ShapeType ReadComponentShape(const H5::DatasetReader& datasetReader)
  {
    H5::AttributeReader componentShapeAttribute = datasetReader.getAttribute(complex::H5::k_ComponentShapeTag);
    if(!componentShapeAttribute.isValid())
    {
      throw std::runtime_error(fmt::format("Error reading Component Shape from HDF5 at {}/{}", H5::Support::GetObjectPath(datasetReader.getParentId()), datasetReader.getName()));
    }
    return componentShapeAttribute.readAsVector<usize>();
  }
};
} // namespace complex
