#include "PipelineFilter.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Filter/FilterList.hpp"
#include "complex/Pipeline/Messaging/FilterPreflightMessage.hpp"
#include "complex/Pipeline/Messaging/PipelineFilterMessage.hpp"

#include <nlohmann/json.hpp>

using namespace complex;

namespace
{
constexpr StringLiteral k_ArgsKey = "args";
constexpr StringLiteral k_FilterKey = "filter";
constexpr StringLiteral k_FilterNameKey = "name";
constexpr StringLiteral k_FilterUuidKey = "uuid";
} // namespace

std::unique_ptr<PipelineFilter> PipelineFilter::Create(const FilterHandle& handle, const Arguments& args, FilterList* filterList)
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
  return std::make_unique<PipelineFilter>(std::move(filter), args);
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

std::string PipelineFilter::getName() const
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
  IFilter::MessageHandler messageHandler{[this](const IFilter::Message& message) { this->notifyFilterMessage(message); }};

  Result<OutputActions> result = m_Filter->preflight(data, getArguments(), messageHandler);
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
  IFilter::MessageHandler messageHandler{[this](const IFilter::Message& message) { this->notifyFilterMessage(message); }};

  auto results = m_Filter->execute(data, getArguments(), this, messageHandler);
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

std::unique_ptr<AbstractPipelineNode> PipelineFilter::deepCopy() const
{
  return std::make_unique<PipelineFilter>(m_Filter->clone(), m_Arguments);
}

void PipelineFilter::notifyFilterMessage(const IFilter::Message& message)
{
  notify(std::make_shared<PipelineFilterMessage>(this, message));
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

Result<std::unique_ptr<PipelineFilter>> PipelineFilter::FromJson(const nlohmann::json& json)
{
  return FromJson(json, *Application::Instance()->getFilterList());
}

Result<std::unique_ptr<PipelineFilter>> PipelineFilter::FromJson(const nlohmann::json& json, const FilterList& filterList)
{
  if(!json.contains(k_FilterKey.view()))
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-1, fmt::format("JSON does not contain key \"{}\"", k_FilterKey.view()));
  }

  const auto& filterObject = json[k_FilterKey];

  if(!filterObject.contains(k_FilterUuidKey.view()))
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-2, fmt::format("Filter JSON does not contain key \"{}\"", k_FilterUuidKey.view()));
  }

  const auto& uuidObject = filterObject[k_FilterUuidKey];

  if(!uuidObject.is_string())
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-3, "UUID value is not a string");
  }

  auto uuidString = uuidObject.get<std::string>();
  std::optional<Uuid> uuid = Uuid::FromString(uuidString);
  if(!uuid.has_value())
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-4, fmt::format("\"{}\" is not a valid UUID", uuidString));
  }
  IFilter::UniquePointer filter = filterList.createFilter(*uuid);
  if(filter == nullptr)
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-5, fmt::format("Failed to create filter with UUID \"{}\"", uuidString));
  }

  if(!json.contains(k_ArgsKey.view()))
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-6, fmt::format("JSON does not contain key \"{}\"", k_ArgsKey.view()));
  }

  const auto& argsJson = json[k_ArgsKey];

  auto argsResult = filter->fromJson(argsJson);

  if(argsResult.invalid())
  {
    Result<std::unique_ptr<PipelineFilter>> result{nonstd::make_unexpected(std::move(argsResult.errors()))};
    result.warnings() = std::move(argsResult.warnings());
    return result;
  }

  Result<std::unique_ptr<PipelineFilter>> result{std::make_unique<PipelineFilter>(std::move(filter), std::move(argsResult.value()))};
  result.warnings() = std::move(argsResult.warnings());
  return result;
}
