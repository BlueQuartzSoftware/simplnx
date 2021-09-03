#include "Pipeline.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/Core/FilterList.hpp"
#include "complex/Pipeline/FilterNode.hpp"
#include "complex/Pipeline/Messaging/NodeAddedMessage.hpp"
#include "complex/Pipeline/Messaging/NodeRemovedMessage.hpp"

using namespace complex;

Pipeline::Pipeline(const std::string& name)
: IPipelineNode()
, m_Name(name)
{
}

Pipeline::~Pipeline() = default;

std::string Pipeline::getName()
{
  return m_Name;
}

void Pipeline::setName(const std::string& name)
{
  m_Name = name;
}

bool Pipeline::preflight()
{
  DataStructure ds;
  return preflight(ds);
}

bool Pipeline::execute()
{
  DataStructure ds;
  return execute(ds);
}

bool Pipeline::preflight(DataStructure& ds)
{
  return preflightFrom(0, ds);
}

bool Pipeline::execute(DataStructure& ds)
{
  return executeFrom(0, ds);
}

bool Pipeline::preflightFrom(const index_type& index, DataStructure& ds)
{
  if(index >= size() && index != 0)
  {
    return false;
  }
  for(auto iter = begin() + index; iter < end(); iter++)
  {
    if(!iter->get()->preflight(ds))
    {
      return false;
    }
  }
  return true;
}

bool Pipeline::executeFrom(const index_type& index, DataStructure& ds)
{
  if(index >= size() && index != 0)
  {
    return false;
  }
  for(auto iter = begin() + index; iter < end(); iter++)
  {
    if(!iter->get()->execute(ds))
    {
      clearDataStructure();
      return false;
    }
  }
  setDataStructure(ds);
  setStatus(Status::Completed);
  return true;
}

size_t Pipeline::size() const
{
  return m_Collection.size();
}

IPipelineNode* Pipeline::operator[](index_type index)
{
  return m_Collection[index].get();
}

IPipelineNode* Pipeline::at(index_type index)
{
  if(index >= size())
  {
    return nullptr;
  }

  return m_Collection[index].get();
}

const IPipelineNode* Pipeline::at(index_type index) const
{
  if(index >= size())
  {
    return nullptr;
  }

  return m_Collection[index].get();
}

bool Pipeline::insertAt(index_type index, IPipelineNode* node)
{
  if(index > size())
  {
    return false;
  }
  auto ptr = std::shared_ptr<IPipelineNode>(node);
  return insertAt(begin() + index, ptr);
}

bool Pipeline::insertAt(index_type index, IFilter::UniquePointer& filter)
{
  if(filter == nullptr)
  {
    return false;
  }

  return insertAt(index, new FilterNode(std::move(filter)));
}

bool Pipeline::insertAt(index_type index, const FilterHandle& handle)
{
  auto filterList = Application::Instance()->getFilterList();
  IFilter::UniquePointer filter = filterList->createFilter(handle);
  return insertAt(index, filter);
}

bool Pipeline::insertAt(iterator pos, const std::shared_ptr<IPipelineNode>& ptr)
{
  if(ptr == nullptr)
  {
    return false;
  }

  index_type index = pos - begin();
  if(pos == end())
  {
    index = size();
  }
  ptr->setParentNode(this);
  m_Collection.insert(pos, ptr);
  notify(std::make_shared<NodeAddedMessage>(this, ptr.get(), index));
  return true;
}

bool Pipeline::remove(IPipelineNode* node)
{
  auto iter = find(node);
  return remove(iter);
}

bool Pipeline::remove(iterator iter)
{
  if(iter == end())
  {
    return false;
  }

  index_type index = iter - begin();
  auto node = iter->get();
  node->setParentNode(nullptr);
  m_Collection.erase(iter);
  notify(std::make_shared<NodeRemovedMessage>(this, index));
  return true;
}

bool Pipeline::remove(const_iterator iter)
{
  if(iter == end())
  {
    return false;
  }

  index_type index = iter - begin();
  auto node = iter->get();
  node->setParentNode(nullptr);
  m_Collection.erase(iter);
  notify(std::make_shared<NodeRemovedMessage>(this, index));
  return true;
}

bool Pipeline::removeAt(index_type pos)
{
  if(pos >= size())
  {
    return false;
  }

  return remove(begin() + pos);
}

void Pipeline::clear()
{
  for(auto iter = begin(); iter != end(); iter++)
  {
    remove(iter);
  }
}

bool Pipeline::contains(IPipelineNode* node) const
{
  return find(node) != end();
}

bool Pipeline::contains(IFilter* filter) const
{
  for(auto node : *this)
  {
    auto filterNode = dynamic_cast<FilterNode*>(node.get());
    if(filterNode == nullptr)
    {
      continue;
    }
    if(filterNode->getFilter() == filter)
    {
      return true;
    }
  }
  return false;
}

bool Pipeline::push_front(IPipelineNode* node)
{
  if(node == nullptr)
  {
    return false;
  }
  return insertAt(begin(), std::shared_ptr<IPipelineNode>(node));
}

bool Pipeline::push_front(const FilterHandle& handle)
{
  return push_front(FilterNode::Create(handle));
}

bool Pipeline::push_back(IPipelineNode* node)
{
  if(node == nullptr)
  {
    return false;
  }
  return insertAt(end(), std::shared_ptr<IPipelineNode>(node));
}

bool Pipeline::push_back(const FilterHandle& handle)
{
  return push_back(FilterNode::Create(handle));
}

Pipeline::iterator Pipeline::find(const IdType& id)
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<IPipelineNode>& node) { return node->getId() == id; });
}

Pipeline::const_iterator Pipeline::find(const IdType& id) const
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<IPipelineNode>& node) { return node->getId() == id; });
}

Pipeline::iterator Pipeline::find(IPipelineNode* targetNode)
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<IPipelineNode>& node) { return node.get() == targetNode; });
}

Pipeline::const_iterator Pipeline::find(IPipelineNode* targetNode) const
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<IPipelineNode>& node) { return node.get() == targetNode; });
}

Pipeline::iterator Pipeline::begin()
{
  return m_Collection.begin();
}

Pipeline::iterator Pipeline::end()
{
  return m_Collection.end();
}

Pipeline::const_iterator Pipeline::begin() const
{
  return m_Collection.begin();
}

Pipeline::const_iterator Pipeline::end() const
{
  return m_Collection.end();
}
