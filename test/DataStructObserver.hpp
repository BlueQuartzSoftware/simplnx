#pragma once

#include <string>

#include "Complex/DataStructure/Observers/AbstractDataStructureObserver.hpp"

namespace complex
{
class DataObject;
class DataStructure;
class BaseGroup;
class DataAddedMessage;
class DataRemovedMessage;
class DataRenamedMessage;
class DataReparentedMessage;
} // namespace complex

using namespace complex;

/**
 * @brief
 */
class DataStructObserver : public complex::AbstractDataStructureObserver
{
public:
  DataStructObserver(complex::DataStructure& dataStruct);
  virtual ~DataStructObserver();

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

  size_t getDataAddedCount() const;
  size_t getDataRemovedCount() const;
  size_t getDataRenamedCount() const;
  size_t getDataReparentedCount() const;

private:
  complex::DataStructure& m_DataStructure;
  size_t m_AddedCount = 0;
  size_t m_RemovedCount = 0;
  size_t m_RenamedCount = 0;
  size_t m_ReparentedCount = 0;
};
