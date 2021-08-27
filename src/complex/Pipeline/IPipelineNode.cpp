#include "IPipelineNode.hpp"

#include <cstdlib>

#include "complex/Pipeline/Messaging/PipelineNodeObserver.hpp"
#include "complex/Pipeline/Messaging/FilterStatusMessage.hpp"

using namespace complex;


char createRandomDigit()
{
  return std::rand() % 256;
}

IPipelineNode::IdType IPipelineNode::CreateId()
{
  std::string str;
  for(size_t i = 0; i < 32; i++)
  {
    str.push_back(createRandomDigit());
  }
  return *Uuid::FromString(str);
}

IPipelineNode::IPipelineNode()
: m_Id(CreateId())
{
}

IPipelineNode::~IPipelineNode() = default;

IPipelineNode::IdType IPipelineNode::getId() const
{
  return m_Id;
}

void IPipelineNode::markDirty()
{
  m_Status = Status::Dirty;
}

bool IPipelineNode::isDirty() const
{
  return m_Status == Status::Dirty;
}

IPipelineNode::Status IPipelineNode::getStatus() const
{
  return m_Status;
}

void IPipelineNode::setStatus(Status status)
{
  m_Status = status;
  notify(std::make_shared<FilterStatusMessage>(this, status));
}

const DataStructure& IPipelineNode::getDataStructure() const
{
  return m_DataStructure;
}

void IPipelineNode::setDataStructure(const DataStructure& ds)
{
  m_DataStructure = ds;
}

void IPipelineNode::clearDataStructure()
{
  m_DataStructure = DataStructure();
  m_Status = Status::Dirty;
}

void IPipelineNode::notify(const std::shared_ptr<IPipelineMessage>& msg)
{
  for(auto obs : m_Observers)
  {
    obs->onNotify(this, msg);
  }
}

void IPipelineNode::addObserver(PipelineNodeObserver* obs)
{
  m_Observers.push_back(obs);
}

void IPipelineNode::removeObserver(PipelineNodeObserver* obs)
{
  m_Observers.erase(std::remove(m_Observers.begin(), m_Observers.end(), obs));
}
