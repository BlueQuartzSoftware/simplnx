#pragma once

#include "complex/DataStructure/BaseGroup.hpp"

#include "complex/complex_export.hpp"

namespace complex
{

/**
 * @class DataContainer
 * @brief The DataGroup class is instantiable implementation of BaseGroup.
 * The DataGroup class does not impose restrictions on which types of
 * DataObject.
 */
class COMPLEX_EXPORT DataGroup : public BaseGroup
{
public:
  /**
   * @brief Creates the DataGroup for the target DataStructure and with the
   * specified name.
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
   * @brief Creates and returns a deep copy of the DataGroup. The caller is
   * responsible for deleting the returned pointer when it is no longer needed.
   * @return DataObject*
   */
  DataObject* deepCopy() override;

  /**
   * @brief Creates and returns a shallow copy of the DataGroup. The caller is
   * responsible for deleting the returned pointer when it is no longer needed.
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
   * @brief Checks if the provided DataObject can be added to the container.
   * Returns true if the DataObject can be added to the container. Otherwise,
   * returns false.
   * @param obj
   * @return bool
   */
  bool canInsert(const DataObject* obj) const override;

private:
};
} // namespace complex
