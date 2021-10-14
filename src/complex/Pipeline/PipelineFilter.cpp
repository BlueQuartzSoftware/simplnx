#include "PipelineFilter.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Filter/FilterList.hpp"
#include "complex/Pipeline/Messaging/FilterPreflightMessage.hpp"

#include <nlohmann/json.hpp>

using namespace complex;

namespace
{
constexpr StringLiteral k_ArgsKey = "args";
constexpr StringLiteral k_FilterKey = "filter";
constexpr StringLiteral k_FilterNameKey = "name";
constexpr StringLiteral k_FilterUuidKey = "uuid";
} // namespace

PipelineFilter* PipelineFilter::Create(const FilterHandle& handle, const Arguments& args, FilterList* filterList)
{
  // Use current Application FilterList if one is not provided.
  if(filterList == nullptr)
  {
    filterList = Application::Instance()->getFilterList();
  }

  auto filter = filterList->createFilter(handle);
  if(filter == nullptr)
  {
    return nullptr;
  }
  return new PipelineFilter(std::move(filter), args);
}

PipelineFilter* PipelineFilter::Create(const std::string& humanOrClassName, const Arguments& args, FilterList* filterList)
{
  // Use current Application FilterList if one is not provided.
  if(filterList == nullptr)
  {
    filterList = Application::Instance()->getFilterList();
  }
  auto filter = filterList->createFilter(humanOrClassName);
  if(filter == nullptr)
  {
    return nullptr;
  }
  return new PipelineFilter(std::move(filter), args);
}

PipelineFilter::PipelineFilter(IFilter::UniquePointer&& filter, const Arguments& args)
: AbstractPipelineNode()
, m_Filter(std::move(filter))
, m_Arguments(args)
{
}

PipelineFilter::~PipelineFilter() = default;

AbstractPipelineNode::NodeType PipelineFilter::getType() const
{
  return NodeType::Filter;
}

std::string PipelineFilter::getName()
{
  return m_Filter->humanName();
}

IFilter* PipelineFilter::getFilter() const
{
  return m_Filter.get();
}

Parameters PipelineFilter::getParameters() const
{
  return m_Filter->parameters();
}

Arguments PipelineFilter::getArguments() const
{
  return m_Arguments;
}

void PipelineFilter::setArguments(const Arguments& args)
{
  m_Arguments = args;
  markDirty();
}

bool PipelineFilter::preflight(DataStructure& data)
{
  Result<OutputActions> result = m_Filter->preflight(data, getArguments());
  m_Warnings = std::move(result.warnings());
  if(result.invalid())
  {
    m_Errors = std::move(result.errors());

    clearPreflightStructure();
    notify(std::make_shared<FilterPreflightMessage>(this, m_Warnings, m_Errors));
    return false;
  }

  m_Errors.clear();

  for(const auto& action : result.value().actions)
  {
    Result<> actionResult = action->apply(data, IDataAction::Mode::Preflight);
    for(auto&& warning : actionResult.warnings())
    {
      m_Warnings.push_back(std::move(warning));
    }
    if(actionResult.invalid())
    {
      m_Errors = std::move(actionResult.errors());
      clearPreflightStructure();
      notify(std::make_shared<FilterPreflightMessage>(this, m_Warnings, m_Errors));
      return false;
    }
  }

  setPreflightStructure(data);

  notify(std::make_shared<FilterPreflightMessage>(this, m_Warnings, m_Errors));
  return true;
}

bool PipelineFilter::execute(DataStructure& data)
{
  auto results = m_Filter->execute(data, getArguments());
  if(!results.valid())
  {
    m_Warnings = results.warnings();
    m_Errors = results.errors();

    clearPreflightStructure();
    markDirty();
    return false;
  }
  else
  {
    m_Warnings = results.warnings();
    m_Errors.clear();

    setDataStructure(data);
    setStatus(Status::Completed);
    return true;
  }
}

std::vector<complex::Warning> PipelineFilter::getWarnings() const
{
  return m_Warnings;
}

std::vector<complex::Error> PipelineFilter::getErrors() const
{
  return m_Errors;
}

nlohmann::json PipelineFilter::toJson() const
{
  nlohmann::json json;

  auto filterObjectJson = nlohmann::json::object();

  filterObjectJson[k_FilterUuidKey] = m_Filter->uuid().str();
  filterObjectJson[k_FilterNameKey] = m_Filter->name();

  nlohmann::json argsJsonArray = m_Filter->toJson(m_Arguments);

  json[k_FilterKey] = std::move(filterObjectJson);
  json[k_ArgsKey] = std::move(argsJsonArray);

  return json;
}

std::unique_ptr<PipelineFilter> PipelineFilter::FromJson(const nlohmann::json& json)
{
  return FromJson(json, *Application::Instance()->getFilterList());
}

std::unique_ptr<PipelineFilter> PipelineFilter::FromJson(const nlohmann::json& json, const FilterList& filterList)
{
  const auto& filterObject = json[k_FilterKey];
  auto uuidString = filterObject[k_FilterUuidKey].get<std::string>();
  std::optional<Uuid> uuid = Uuid::FromString(uuidString);
  if(!uuid.has_value())
  {
    return nullptr;
  }
  IFilter::UniquePointer filter = filterList.createFilter(*uuid);
  if(filter == nullptr)
  {
    return nullptr;
  }

  const auto& argsJson = json[k_ArgsKey];

  auto argsResult = filter->fromJson(argsJson);

  if(argsResult.invalid())
  {
    return nullptr;
  }

  return std::make_unique<PipelineFilter>(std::move(filter), std::move(argsResult.value()));
}
