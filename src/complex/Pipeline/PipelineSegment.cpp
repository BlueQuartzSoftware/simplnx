#include "PipelineSegment.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterList.hpp"
#include "complex/Pipeline/FilterNode.hpp"

using namespace complex;

PipelineSegment::PipelineSegment()
: IPipelineNode()
{
}

PipelineSegment::~PipelineSegment() = default;

bool PipelineSegment::preflight() const
{
  DataStructure ds;
  return preflight(ds);
}

bool PipelineSegment::execute()
{
  DataStructure ds;
  return execute(ds);
}

bool PipelineSegment::preflight(DataStructure& ds) const
{
  return preflightFrom(0, ds);
}

bool PipelineSegment::execute(DataStructure& ds)
{
  return executeFrom(0, ds);
}

bool PipelineSegment::preflightFrom(const index_type& index, DataStructure& ds) const
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

bool PipelineSegment::executeFrom(const index_type& index, DataStructure& ds)
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
  return true;
}

size_t PipelineSegment::size() const
{
  return m_Collection.size();
}

IPipelineNode* PipelineSegment::operator[](index_type index)
{
  return m_Collection[index].get();
}

IPipelineNode* PipelineSegment::at(index_type index)
{
  if(index >= size())
  {
    return nullptr;
  }

  return m_Collection[index].get();
}

const IPipelineNode* PipelineSegment::at(index_type index) const
{
  if(index >= size())
  {
    return nullptr;
  }

  return m_Collection[index].get();
}

bool PipelineSegment::insertAt(index_type index, IPipelineNode* node)
{
  if(index >= size())
  {
    return false;
  }
  collection_type::iterator pos = m_Collection.begin() + index;
  auto ptr = std::shared_ptr<IPipelineNode>(node);
  m_Collection.emplace(pos, ptr);
}

bool PipelineSegment::insertAt(index_type index, IFilter::UniquePointer& filter)
{
  if(filter == nullptr)
  {
    return false;
  }

  return insertAt(index, new FilterNode(std::move(filter)));
}

bool PipelineSegment::insertAt(index_type index, const FilterHandle& handle)
{
  auto filterList = Application::Instance()->getFilterList();
  IFilter::UniquePointer filter = filterList->createFilter(handle);
  return insertAt(index, filter);
}

bool PipelineSegment::remove(IPipelineNode* node)
{
  auto iter = find(node);
  return remove(iter);
}

bool PipelineSegment::remove(iterator iter)
{
  if(iter == end())
  {
    return false;
  }

  m_Collection.erase(iter);
  return true;
}

bool PipelineSegment::remove(const_iterator iter)
{
  if(iter == end())
  {
    return false;
  }

  m_Collection.erase(iter);
  return true;
}

bool PipelineSegment::contains(IPipelineNode* node) const
{
  return find(node) != end();
}

bool PipelineSegment::push_front(IPipelineNode* node)
{
  if(node == nullptr)
  {
    return false;
  }
  m_Collection.emplace(m_Collection.begin(), node);
  return true;
}

bool PipelineSegment::push_front(const FilterHandle& handle)
{
  return push_front(FilterNode::Create(handle));
}

bool PipelineSegment::push_back(IPipelineNode* node)
{
  if(node == nullptr)
  {
    return false;
  }
  m_Collection.emplace(m_Collection.end(), node);
  return true;
}

bool PipelineSegment::push_back(const FilterHandle& handle)
{
  return push_back(FilterNode::Create(handle));
}

PipelineSegment::iterator PipelineSegment::find(const IdType& id)
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<IPipelineNode>& node) { return node->getId() == id; });
}

PipelineSegment::const_iterator PipelineSegment::find(const IdType& id) const
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<IPipelineNode>& node) { return node->getId() == id; });
}

PipelineSegment::iterator PipelineSegment::find(IPipelineNode* targetNode)
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<IPipelineNode>& node) { return node.get() == targetNode; });
}

PipelineSegment::const_iterator PipelineSegment::find(IPipelineNode* targetNode) const
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<IPipelineNode>& node) { return node.get() == targetNode; });
}

PipelineSegment::iterator PipelineSegment::begin()
{
  return m_Collection.begin();
}

PipelineSegment::iterator PipelineSegment::end()
{
  return m_Collection.end();
}

PipelineSegment::const_iterator PipelineSegment::begin() const
{
  return m_Collection.begin();
}

PipelineSegment::const_iterator PipelineSegment::end() const
{
  return m_Collection.end();
}
