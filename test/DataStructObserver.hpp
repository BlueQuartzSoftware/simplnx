#pragma once

#include "complex/Common/Types.hpp"
#include "complex/DataStructure/Observers/AbstractDataStructureObserver.hpp"

#include <string>

namespace complex
{
class DataObject;
class DataStructure;
class BaseGroup;
class DataAddedMessage;
class DataRemovedMessage;
class DataRenamedMessage;
class DataReparentedMessage;

/**
 * @brief
 */
class DataStructObserver : public complex::AbstractDataStructureObserver
{
public:
  DataStructObserver(complex::DataStructure& dataStruct);
  ~DataStructObserver() override;

  /**
   * @brief
   * @param target
   * @param msg
   */
  void onNotify(DataStructure* target, const std::shared_ptr<AbstractDataStructureMessage>& msg) override;

  /**
   * @brief
   * @return
   */
  const complex::DataStructure& getDataStructure() const;

  usize getDataAddedCount() const;
  usize getDataRemovedCount() const;
  usize getDataRenamedCount() const;
  usize getDataReparentedCount() const;

private:
  complex::DataStructure& m_DataStructure;
  usize m_AddedCount = 0;
  usize m_RemovedCount = 0;
  usize m_RenamedCount = 0;
  usize m_ReparentedCount = 0;
};
} // namespace complex
