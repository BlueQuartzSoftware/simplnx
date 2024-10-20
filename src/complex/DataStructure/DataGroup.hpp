#pragma once

#include "complex/DataStructure/BaseGroup.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * class DataContainer
 *
 */
class COMPLEX_EXPORT DataGroup : public BaseGroup
{
public:
  /**
   * @brief
   * @param ds
   * @param name
   */
  DataGroup(DataStructure* ds, const std::string& name);

  /**
   * @brief
   * @param other
   */
  DataGroup(const DataGroup& other);

  /**
   * @brief
   * @param other
   */
  DataGroup(DataGroup&& other) noexcept;

  /**
   * @brief
   */
  virtual ~DataGroup();

  /**
   * @brief
   * @return DataObject*
   */
  DataObject* deepCopy() override;

  /**
   * @brief
   * @return DataObject*
   */
  DataObject* shallowCopy() override;

  /**
   * @brief
   * @param out
   * @param hdfFileName
   * @return H5::ErrorType
   */
  H5::ErrorType generateXdmfText(std::ostream& out, const std::string& hdfFileName) const override;

  /**
   * @brief
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
} // namespace complex
