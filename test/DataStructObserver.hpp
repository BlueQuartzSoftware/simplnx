#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Observers/AbstractDataStructureObserver.hpp"

#include <string>

namespace nx::core
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
class DataStructObserver : public nx::core::AbstractDataStructureObserver
{
public:
  DataStructObserver(nx::core::DataStructure& dataStruct);
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
  const nx::core::DataStructure& getDataStructure() const;

  usize getDataAddedCount() const;
  usize getDataRemovedCount() const;
  usize getDataRenamedCount() const;
  usize getDataReparentedCount() const;

private:
  nx::core::DataStructure& m_DataStructure;
  usize m_AddedCount = 0;
  usize m_RemovedCount = 0;
  usize m_RenamedCount = 0;
  usize m_ReparentedCount = 0;
};
} // namespace nx::core
