#include "Pipeline.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Filter/FilterList.hpp"
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
  resetCollectionParent();
}

Pipeline::Pipeline(Pipeline&& other) noexcept
: AbstractPipelineNode()
, m_Name(std::move(other.m_Name))
, m_Collection(std::move(other.m_Collection))
, m_FilterList(std::move(other.m_FilterList))
{
  resetCollectionParent();
}

Pipeline::~Pipeline()
{
  for(auto& node : m_Collection)
  {
    node->setParentPipeline(nullptr);
  }
}

Pipeline& Pipeline::operator=(const Pipeline& rhs) noexcept
{
  m_Name = rhs.m_Name;
  m_Collection = rhs.m_Collection;
  m_FilterList = rhs.m_FilterList;
  resetCollectionParent();
  return *this;
}

Pipeline& Pipeline::operator=(Pipeline&& rhs) noexcept
{
  m_Name = std::move(rhs.m_Name);
  m_Collection = std::move(rhs.m_Collection);
  m_FilterList = std::move(rhs.m_FilterList);
  resetCollectionParent();
  return *this;
}

void Pipeline::resetCollectionParent()
{
  for(auto& node : m_Collection)
  {
    node->setParentPipeline(this);
  }
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

std::string Pipeline::getName() const
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

bool Pipeline::isEmpty() const
{
  return size() == 0;
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

bool Pipeline::insertAt(index_type index, std::shared_ptr<AbstractPipelineNode> node)
{
  if(index > size())
  {
    return false;
  }
  return insertAt(begin() + index, std::move(node));
}

bool Pipeline::insertAt(index_type index, IFilter::UniquePointer&& filter, const Arguments& args)
{
  if(filter == nullptr)
  {
    return false;
  }

  return insertAt(index, std::make_shared<PipelineFilter>(std::move(filter), args));
}

bool Pipeline::insertAt(index_type index, const FilterHandle& handle, const Arguments& args)
{
  auto filterList = getActiveFilterList();
  IFilter::UniquePointer filter = filterList->createFilter(handle);
  return insertAt(index, std::move(filter), args);
}

bool Pipeline::insertAt(iterator pos, std::shared_ptr<AbstractPipelineNode> node)
{
  if(node == nullptr)
  {
    return false;
  }

  index_type index = pos - begin();
  if(pos == end())
  {
    index = size();
  }
  // Get raw pointer before move
  AbstractPipelineNode* ptr = node.get();
  m_Collection.insert(pos, std::move(node));
  ptr->setParentPipeline(this);
  notify(std::make_shared<NodeAddedMessage>(this, ptr, index));
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

bool Pipeline::push_front(std::shared_ptr<AbstractPipelineNode> node)
{
  return insertAt(begin(), std::move(node));
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

  auto filterNode = std::make_shared<PipelineFilter>(std::move(filter));
  filterNode->setArguments(args);
  return push_front(filterNode);
}

bool Pipeline::push_back(std::shared_ptr<AbstractPipelineNode> node)
{
  return insertAt(end(), std::move(node));
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

  auto filterNode = std::make_shared<PipelineFilter>(std::move(filter));
  filterNode->setArguments(args);
  return push_back(filterNode);
}

Pipeline::iterator Pipeline::find(AbstractPipelineNode* targetNode)
{
  if(size() == 0)
  {
    return end();
  }

  return std::find_if(begin(), end(), [=](const std::shared_ptr<AbstractPipelineNode>& node) { return node.get() == targetNode; });
}

Pipeline::const_iterator Pipeline::find(const AbstractPipelineNode* targetNode) const
{
  if(size() == 0)
  {
    return end();
  }

  return std::find_if(begin(), end(), [=](const std::shared_ptr<AbstractPipelineNode>& node) { return node.get() == targetNode; });
}

std::unique_ptr<AbstractPipelineNode> Pipeline::deepCopy() const
{
  auto pipelineCopy = std::make_unique<Pipeline>(getName(), m_FilterList);
  for(auto childNode : *this)
  {
    pipelineCopy->push_back(childNode->deepCopy());
  }
  return std::move(pipelineCopy);
}

std::unique_ptr<Pipeline> Pipeline::copySegment(const iterator& startIter, const iterator& endIter)
{
  auto pipelineCopy = std::make_unique<Pipeline>(getName(), m_FilterList);
  for(auto iter = startIter; iter != endIter; iter++)
  {
    pipelineCopy->push_back(iter->get()->deepCopy());
  }
  return std::move(pipelineCopy);
}

std::unique_ptr<Pipeline> Pipeline::copySegment(const const_iterator& startIter, const const_iterator& endIter) const
{
  auto pipelineCopy = std::make_unique<Pipeline>(getName(), m_FilterList);
  for(auto iter = startIter; iter != endIter; iter++)
  {
    pipelineCopy->push_back(iter->get()->deepCopy());
  }
  // Include the end iterator if it is a valid value
  if(endIter != end())
  {
    pipelineCopy->push_back(endIter->get()->deepCopy());
  }
  return std::move(pipelineCopy);
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

Result<Pipeline> Pipeline::FromJson(const nlohmann::json& json)
{
  return FromJson(json, Application::Instance()->getFilterList());
}

Result<Pipeline> Pipeline::FromJson(const nlohmann::json& json, FilterList* filterList)
{
  if(!json.contains(k_PipelineNameKey.view()))
  {
    return MakeErrorResult<Pipeline>(-1, fmt::format("Pipeline JSON did not contain key \"{}\"", k_PipelineNameKey.view()));
  }

  if(!json.contains(k_PipelineItemsKey.view()))
  {
    return MakeErrorResult<Pipeline>(-2, fmt::format("Pipeline JSON did not contain key \"{}\"", k_PipelineItemsKey.view()));
  }

  auto name = json[k_PipelineNameKey].get<std::string>();

  Pipeline pipeline(name, filterList);

  std::vector<Warning> warnings;

  for(const auto& item : json[k_PipelineItemsKey])
  {
    Result<std::unique_ptr<PipelineFilter>> filterResult = PipelineFilter::FromJson(item, *filterList);
    for(auto& warning : filterResult.warnings())
    {
      warnings.push_back(std::move(warning));
    }
    if(filterResult.invalid())
    {
      Result<Pipeline> result{nonstd::make_unexpected(std::move(filterResult.errors()))};
      result.warnings() = std::move(warnings);
      return result;
    }

    pipeline.push_back(std::move(filterResult.value()));
  }

  Result<Pipeline> result{std::move(pipeline)};
  result.warnings() = std::move(warnings);

  return result;
}
