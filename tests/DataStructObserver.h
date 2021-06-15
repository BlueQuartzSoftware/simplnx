#pragma once

#include <string>

#include "Complex/DataStructure/Observers/AbstractDataStructureObserver.h"

namespace Complex
{
class DataObject;
class DataStructure;
class BaseGroup;
class DataAddedMessage;
class DataRemovedMessage;
class DataRenamedMessage;
class DataReparentedMessage;
} // namespace SIMPL

using namespace Complex;

/**
 * @brief
 */
class DataStructObserver : public Complex::AbstractDataStructureObserver
{
public:
  DataStructObserver(Complex::DataStructure& dataStruct);
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
  const Complex::DataStructure& getDataStructure() const;

  /**
   * @brief
   */
  void printDataStructure() const;

protected:
  void handleDataMsg(DataStructure* ds, DataAddedMessage* msg);
  void handleDataMsg(DataStructure* ds, DataRemovedMessage* msg);
  void handleDataMsg(DataStructure* ds, DataRenamedMessage* msg);
  void handleDataMsg(DataStructure* ds, DataReparentedMessage* msg);

  /**
   * @brief
   * @param data
   * @param prefix
   */
  void printData(DataObject* data, const std::string& prefix = " ") const;

  /**
   * @brief
   * @param data
   * @param prefix
   */
  void printDataObject(DataObject* data, const std::string& prefix = " ") const;

  /**
   * @brief
   * @param target
   * @param prefix
   */
  void printDataContainer(BaseGroup* target, const std::string& prefix = " ") const;

private:
  Complex::DataStructure& m_DataStructure;
};
