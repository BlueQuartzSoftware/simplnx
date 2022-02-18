#pragma once

#include "complex/DataStructure/IDataArray.hpp"

namespace complex
{
/**
 * @brief Non-templated base class for NeighborList class.
 */
class COMPLEX_EXPORT INeighborList : public IDataArray
{
public:
  virtual ~INeighborList();

  /**
   * @brief Returns typename of the DataObject as a std::string.
   * @return std::string
   */
  std::string getTypeName() const override;

  /**
   * @brief setNumNeighborsArrayName
   * @param name
   */
  void setNumNeighborsArrayName(const std::string& name);

  /**
   * @brief Returns the Num Neighbors array name for use in HDF5.
   * @return std::string
   */
  std::string getNumNeighborsArrayName() const;

  /**
   * @brief Returns the number of elements in the internal array.
   * @return usize
   */
  usize getNumberOfTuples() const override;

protected:
  /**
   * @brief Constructs a new INeighborList
   * @param dataStructure
   * @param name
   * @param numTuples
   */
  INeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples);

  /**
   * @brief Constructor for use when importing INeighborLists
   * @param dataStructure
   * @param name
   * @param numTuples
   * @param importId
   */
  INeighborList(DataStructure& dataStructure, const std::string& name, usize numTuples, IdType importId);

  /**
   * @brief Sets the number of tuples.
   * @param numTuples
   */
  void setNumberOfTuples(usize numTuples);

private:
  std::string m_NumNeighborsArrayName;
  usize m_NumTuples;
};
} // namespace complex
