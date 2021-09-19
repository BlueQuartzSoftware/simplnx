#include "IPipelineNode.hpp"

#include <algorithm>
#include <cstdlib>

#include "complex/Pipeline/Messaging/NodeStatusMessage.hpp"
#include "complex/Pipeline/Messaging/PipelineNodeObserver.hpp"

using namespace complex;

char createRandomDigit()
{
  char val = std::rand() % 62 + 48;
  if(val > 57)
  {
    val += 7;
  }
  if(val > 90)
  {
    val += 6;
  }
  return val;
}

IPipelineNode::IdType IPipelineNode::CreateId()
{
  return Uuid{};
  // std::string str;
  // for(size_t i = 0; i < 33; i++)
  //{
  //  str.push_back(createRandomDigit());
  //}
  // return *Uuid::FromString(str);
}

IPipelineNode::IPipelineNode(Pipeline* parent)
: m_Id(CreateId())
, m_Parent(parent)
{
}

IPipelineNode::~IPipelineNode() = default;

IPipelineNode::IdType IPipelineNode::getId() const
{
  return m_Id;
}

Pipeline* IPipelineNode::getParentNode() const
{
  return m_Parent;
}

void IPipelineNode::setParentNode(Pipeline* parent)
{
  m_Parent = parent;
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
  notify(std::make_shared<NodeStatusMessage>(this, status));
}

const DataStructure& IPipelineNode::getDataStructure() const
{
  return m_DataStructure;
}

void IPipelineNode::setDataStructure(const DataStructure& ds)
{
  m_DataStructure = ds;
}

const DataStructure& IPipelineNode::getPreflightStructure() const
{
  return m_PreflightStructure;
}

void IPipelineNode::setPreflightStructure(const DataStructure& ds)
{
  m_PreflightStructure = ds;
  m_IsPreflighted = true;
}

void IPipelineNode::clearDataStructure()
{
  m_DataStructure = DataStructure();
  m_Status = Status::Dirty;
}

void IPipelineNode::clearPreflightStructure()
{
  m_DataStructure = DataStructure();
  m_PreflightStructure = DataStructure();
  m_Status = Status::Dirty;
  m_IsPreflighted = false;
}

bool IPipelineNode::isPreflighted() const
{
  return m_IsPreflighted;
}

void IPipelineNode::notify(const std::shared_ptr<IPipelineMessage>& msg)
{
  m_Signal(this, msg);
}

void IPipelineNode::addObserver(PipelineNodeObserver* obs)
{
  m_Observers.push_back(obs);
}

void IPipelineNode::removeObserver(PipelineNodeObserver* obs)
{
  if(std::find(m_Observers.begin(), m_Observers.end(), obs) == m_Observers.end())
  {
    return;
  }
  m_Observers.erase(std::remove(m_Observers.begin(), m_Observers.end(), obs));
}

IPipelineNode::signal_type& IPipelineNode::getSignal()
{
  return m_Signal;
}
