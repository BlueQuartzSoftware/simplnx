#include "Pipeline.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Core/FilterHandle.hpp"
#include "complex/Core/FilterList.hpp"
#include "complex/Pipeline/Messaging/NodeAddedMessage.hpp"
#include "complex/Pipeline/Messaging/NodeRemovedMessage.hpp"
#include "complex/Pipeline/PipelineFilter.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

using namespace complex;

namespace
{
constexpr StringLiteral k_PipelineNameKey = "name";
constexpr StringLiteral k_PipelineItemsKey = "pipeline";
} // namespace

Pipeline::Pipeline(const std::string& name, FilterList* filterList)
: AbstractPipelineNode()
, m_Name(name)
, m_FilterList(filterList)
{
}

Pipeline::Pipeline(const Pipeline& other)
: AbstractPipelineNode()
, m_Name(other.m_Name)
, m_Collection(other.m_Collection)
, m_FilterList(other.m_FilterList)
{
}

Pipeline::Pipeline(Pipeline&& other) noexcept
: AbstractPipelineNode()
, m_Name(std::move(other.m_Name))
, m_Collection(std::move(other.m_Collection))
, m_FilterList(std::move(other.m_FilterList))
{
}

Pipeline::~Pipeline() = default;

Pipeline& Pipeline::operator=(const Pipeline& rhs) noexcept
{
  m_Name = rhs.m_Name;
  m_Collection = rhs.m_Collection;
  m_FilterList = rhs.m_FilterList;
  return *this;
}

Pipeline& Pipeline::operator=(Pipeline&& rhs) noexcept
{
  m_Name = std::move(rhs.m_Name);
  m_Collection = std::move(rhs.m_Collection);
  m_FilterList = std::move(rhs.m_FilterList);
  return *this;
}

bool Pipeline::hasFilterList() const
{
  return m_FilterList != nullptr;
}

complex::FilterList* Pipeline::getFilterList() const
{
  return m_FilterList;
}

void Pipeline::setFilterList(complex::FilterList* filterList)
{
  m_FilterList = filterList;
}

FilterList* Pipeline::getActiveFilterList() const
{
  if(hasFilterList())
  {
    return getFilterList();
  }
  else
  {
    return Application::Instance()->getFilterList();
  }
}

AbstractPipelineNode::NodeType Pipeline::getType() const
{
  return NodeType::Pipeline;
}

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

bool Pipeline::canPreflightFrom(const index_type& index) const
{
  if(index == 0)
  {
    return true;
  }
  if(index >= size())
  {
    return false;
  }
  return at(index - 1)->isPreflighted();
}

bool Pipeline::preflightFrom(const index_type& index, DataStructure& ds)
{
  if(index >= size() && index != 0)
  {
    return false;
  }
  for(auto iter = begin() + index; iter != end(); iter++)
  {
    if(!iter->get()->preflight(ds))
    {
      return false;
    }
  }
  return true;
}

bool Pipeline::preflightFrom(const index_type& index)
{
  if(index == 0)
  {
    return preflight();
  }
  else if(index >= size())
  {
    return false;
  }

  auto node = at(index - 1);
  DataStructure ds = node->getPreflightStructure();
  return preflightFrom(index, ds);
}

bool Pipeline::canExecuteFrom(const index_type& index) const
{
  if(index == 0)
  {
    return true;
  }
  if(index >= size())
  {
    return false;
  }
  return at(index - 1)->getStatus() == Status::Completed;
}

bool Pipeline::executeFrom(const index_type& index, DataStructure& ds)
{
  if(index >= size() && index != 0)
  {
    return false;
  }
  for(auto iter = begin() + index; iter != end(); iter++)
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

bool Pipeline::executeFrom(const index_type& index)
{
  if(index == 0)
  {
    return execute();
  }
  else if(!canExecuteFrom(index))
  {
    return false;
  }

  auto node = at(index - 1);
  DataStructure ds = node->getDataStructure();
  return executeFrom(index, ds);
}

usize Pipeline::size() const
{
  return m_Collection.size();
}

AbstractPipelineNode* Pipeline::operator[](index_type index)
{
  return m_Collection[index].get();
}

AbstractPipelineNode* Pipeline::at(index_type index)
{
  if(index >= size())
  {
    return nullptr;
  }

  return m_Collection[index].get();
}

const AbstractPipelineNode* Pipeline::at(index_type index) const
{
  if(index >= size())
  {
    return nullptr;
  }

  return m_Collection[index].get();
}

bool Pipeline::insertAt(index_type index, AbstractPipelineNode* node)
{
  if(index > size())
  {
    return false;
  }
  auto ptr = std::shared_ptr<AbstractPipelineNode>(node);
  return insertAt(begin() + index, ptr);
}

bool Pipeline::insertAt(index_type index, IFilter::UniquePointer&& filter, const Arguments& args)
{
  if(filter == nullptr)
  {
    return false;
  }

  return insertAt(index, new PipelineFilter(std::move(filter), args));
}

bool Pipeline::insertAt(index_type index, const FilterHandle& handle, const Arguments& args)
{
  auto filterList = getActiveFilterList();
  IFilter::UniquePointer filter = filterList->createFilter(handle);
  return insertAt(index, std::move(filter), args);
}

bool Pipeline::insertAt(iterator pos, const std::shared_ptr<AbstractPipelineNode>& ptr)
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
  ptr->setParentPipeline(this);
  m_Collection.insert(pos, ptr);
  notify(std::make_shared<NodeAddedMessage>(this, ptr.get(), index));
  return true;
}

bool Pipeline::remove(AbstractPipelineNode* node)
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
  node->setParentPipeline(nullptr);
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
  node->setParentPipeline(nullptr);
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

bool Pipeline::contains(AbstractPipelineNode* node) const
{
  return find(node) != end();
}

bool Pipeline::contains(IFilter* filter) const
{
  for(auto node : *this)
  {
    auto filterNode = dynamic_cast<PipelineFilter*>(node.get());
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

bool Pipeline::push_front(AbstractPipelineNode* node)
{
  if(node == nullptr)
  {
    return false;
  }
  return insertAt(begin(), std::shared_ptr<AbstractPipelineNode>(node));
}

bool Pipeline::push_front(const FilterHandle& handle, const Arguments& args)
{
  return push_front(PipelineFilter::Create(handle, args, getActiveFilterList()));
}

bool Pipeline::push_front(IFilter::UniquePointer&& filter, const Arguments& args)
{
  if(filter == nullptr)
  {
    return false;
  }

  auto filterNode = new PipelineFilter(std::move(filter));
  filterNode->setArguments(args);
  return push_front(filterNode);
}

bool Pipeline::push_back(AbstractPipelineNode* node)
{
  if(node == nullptr)
  {
    return false;
  }
  return insertAt(end(), std::shared_ptr<AbstractPipelineNode>(node));
}

bool Pipeline::push_back(const FilterHandle& handle, const Arguments& args)
{
  return push_back(PipelineFilter::Create(handle, args, getActiveFilterList()));
}

bool Pipeline::push_back(IFilter::UniquePointer&& filter, const Arguments& args)
{
  if(filter == nullptr)
  {
    return false;
  }

  auto filterNode = new PipelineFilter(std::move(filter));
  filterNode->setArguments(args);
  return push_back(filterNode);
}

Pipeline::iterator Pipeline::find(AbstractPipelineNode* targetNode)
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<AbstractPipelineNode>& node) { return node.get() == targetNode; });
}

Pipeline::const_iterator Pipeline::find(AbstractPipelineNode* targetNode) const
{
  return std::find_if(begin(), end(), [=](const std::shared_ptr<AbstractPipelineNode>& node) { return node.get() == targetNode; });
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

nlohmann::json Pipeline::toJson() const
{
  auto jsonObject = nlohmann::json::object();
  jsonObject[k_PipelineNameKey] = m_Name;

  auto jsonArray = nlohmann::json::array();
  for(const auto& item : m_Collection)
  {
    jsonArray.push_back(item->toJson());
  }

  jsonObject[k_PipelineItemsKey] = std::move(jsonArray);

  return jsonObject;
}

Pipeline Pipeline::FromJson(const nlohmann::json& json)
{
  return FromJson(json, Application::Instance()->getFilterList());
}

Pipeline Pipeline::FromJson(const nlohmann::json& json, FilterList* filterList)
{
  auto name = json[k_PipelineNameKey].get<std::string>();

  Pipeline pipeline(name, filterList);

  for(const auto& item : json[k_PipelineItemsKey])
  {
    std::unique_ptr<PipelineFilter> pipelineFilter = PipelineFilter::FromJson(item, *filterList);
    if(pipelineFilter == nullptr)
    {
      throw std::runtime_error("Invalid filter json");
    }

    pipeline.push_back(pipelineFilter.release());
  }

  return pipeline;
}
