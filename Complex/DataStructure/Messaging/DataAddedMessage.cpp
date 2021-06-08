#include "DataAddedMessage.h"

#include "SIMPL/DataStructure/DataStructure.h"

using namespace SIMPL;

DataAddedMessage::DataAddedMessage(const DataStructure* ds, DataObject::IdType addedId)
: AbstractDataStructureMessage(ds)
, m_Id(addedId)
{
}

DataAddedMessage::DataAddedMessage(const DataAddedMessage& other)
: AbstractDataStructureMessage(other)
, m_Id(other.m_Id)
{
}
DataAddedMessage::DataAddedMessage(DataAddedMessage&& other) noexcept
: AbstractDataStructureMessage(other)
, m_Id(std::move(other.m_Id))
{
}

DataAddedMessage::~DataAddedMessage() = default;

int32_t DataAddedMessage::getMsgType() const
{
  return DataAddedMessage::MsgType;
}

DataObject::IdType DataAddedMessage::getId() const
{
  return m_Id;
}

const DataObject* DataAddedMessage::getData() const
{
  return getDataStructure()->getData(m_Id);
}

std::vector<DataPath> DataAddedMessage::getDataPaths() const
{
  return getData()->getDataPaths();
}