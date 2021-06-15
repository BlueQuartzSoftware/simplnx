#pragma once

#include "Complex/DataStructure/BaseGroup.h"

namespace Complex
{

/**
 * class DataContainer
 *
 */

class DataGroup : public BaseGroup
{
public:
  /**
   * @brief Creates the DataGroup for the target DataStructure and with the specified name.
   * @param ds
   * @param name
   */
  DataGroup(DataStructure* ds, const std::string& name);

  /**
   * @brief Copy constructor
   * @param other
   */
  DataGroup(const DataGroup& other);

  /**
   * @brief Move constructor
   * @param other
   */
  DataGroup(DataGroup&& other) noexcept;

  virtual ~DataGroup();

  /**
   * @brief Creates and returns a deep copy of the DataGroup.
   * @return DataObject*
   */
  DataObject* deepCopy() override;

  /**
   * @brief Creates and returns a shallow copy of the DataGroup.
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief Writes information for the XDMF file to the output stream.
   * @param out
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType generateXdmfText(std::ostream& out, const std::string& hdfFileName) const override;

  /**
   * @brief Reads information from the XDMF file through the input stream.
   * @param in
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType readFromXdmfText(std::istream& in, const std::string& hdfFileName) override;

protected:
  /**
   * @brief Checks if the provided DataObject can be added to the container. Returns true if
   * the DataObject can be added to the container. Otherwise, returns false.
   * @param obj
   * @return bool
   */
  bool canInsert(const DataObject* obj) const override;

private:
};
} // namespace Complex
