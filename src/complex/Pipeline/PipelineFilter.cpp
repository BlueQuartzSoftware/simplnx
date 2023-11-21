#include "PipelineFilter.hpp"

#include "complex/Core/Application.hpp"
#include "complex/Filter/FilterList.hpp"
#include "complex/Pipeline/Messaging/FilterPreflightMessage.hpp"
#include "complex/Pipeline/Messaging/OutputRenamedMessage.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>

using namespace complex;

namespace
{
constexpr StringLiteral k_ArgsKey = "args";
constexpr StringLiteral k_FilterKey = "filter";
constexpr StringLiteral k_FilterNameKey = "name";
constexpr StringLiteral k_FilterUuidKey = "uuid";
constexpr StringLiteral k_FilterCommentsKey = "comments";

constexpr StringLiteral k_SIMPLFilterUuidKey = "Filter_Uuid";

nlohmann::json CreateFilterJson(std::string_view uuid, std::string_view name, nlohmann::json argsArray, std::string_view comments)
{
  nlohmann::json json;

  auto filterObjectJson = nlohmann::json::object();

  filterObjectJson[k_FilterUuidKey] = uuid;
  filterObjectJson[k_FilterNameKey] = name;

  nlohmann::json argsJsonArray = std::move(argsArray);

  json[k_FilterKey] = std::move(filterObjectJson);
  json[k_ArgsKey] = std::move(argsJsonArray);
  json[k_FilterCommentsKey] = comments;

  return json;
}

std::optional<AbstractPlugin::SIMPLData> FindComplexConversionFromSIMPL(const Uuid& uuid, const FilterList& filterList)
{
  auto plugins = filterList.getLoadedPlugins();
  for(const auto* plugin : plugins)
  {
    auto filterMap = plugin->getSimplToComplexMap();
    if(filterMap.count(uuid) > 0)
    {
      return filterMap.at(uuid);
    }
  }
  return {};
}
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
  if(m_Filter == nullptr)
  {
    return "Filter not Found";
  }

  return m_Filter->humanName();
}

IFilter* PipelineFilter::getFilter() const
{
  return m_Filter.get();
}

Parameters PipelineFilter::getParameters() const
{
  if(m_Filter == nullptr)
  {
    return {};
  }

  return m_Filter->parameters();
}

Arguments PipelineFilter::getArguments() const
{
  if(m_Filter == nullptr)
  {
    return {};
  }

  return m_Arguments;
}

void PipelineFilter::setArguments(const Arguments& args)
{
  m_Arguments = args;
}

void PipelineFilter::setIndex(int32 index)
{
  m_Index = index;
}

const std::string& PipelineFilter::getComments() const
{
  return m_Comments;
}

void PipelineFilter::setComments(const std::string& comments)
{
  m_Comments = comments;
}

// -----------------------------------------------------------------------------
bool PipelineFilter::preflight(DataStructure& data, const std::atomic_bool& shouldCancel)
{
  RenamedPaths renamedPaths;
  return preflight(data, renamedPaths, shouldCancel, true);
}

bool PipelineFilter::preflight(DataStructure& data, RenamedPaths& renamedPaths, const std::atomic_bool& shouldCancel, bool allowRenaming)
{
  sendFilterRunStateMessage(m_Index, RunState::Preflighting);

  std::vector<DataPath> oldCreatedPaths = m_CreatedPaths;

  IFilter::MessageHandler messageHandler{[this](const IFilter::Message& message) { this->notifyFilterMessage(message); }};

  clearFaultState();
  IFilter::PreflightResult result = m_Filter->preflight(data, getArguments(), messageHandler, shouldCancel);
  m_Warnings = std::move(result.outputActions.warnings());
  setHasWarnings(!m_Warnings.empty());
  m_PreflightValues = std::move(result.outputValues);
  if(result.outputActions.invalid())
  {
    m_Errors = std::move(result.outputActions.errors());
    setHasErrors();
    setPreflightStructure(data, false);
    sendFilterFaultMessage(m_Index, getFaultState());
    sendFilterFaultDetailMessage(m_Index, m_Warnings, m_Errors);
    return false;
  }

  m_Errors.clear();

  Result<> actionsResult = result.outputActions.value().applyAll(data, IDataAction::Mode::Preflight);

  for(auto&& warning : actionsResult.warnings())
  {
    m_Warnings.push_back(std::move(warning));
  }
  setHasWarnings(!m_Warnings.empty());
  if(actionsResult.invalid())
  {
    m_Errors = std::move(actionsResult.errors());
    setPreflightStructure(data, false);
    setHasErrors();
    sendFilterFaultMessage(m_Index, getFaultState());
    sendFilterFaultDetailMessage(m_Index, m_Warnings, m_Errors);
    return false;
  }

  std::vector<DataPath> newCreatedPaths;
  std::vector<DataPath> newModifiedPaths;
  for(const auto& action : result.outputActions.value().actions)
  {
    if(const auto* creationActionPtr = dynamic_cast<const IDataCreationAction*>(action.get()); creationActionPtr != nullptr)
    {
      auto allCreatedPaths = creationActionPtr->getAllCreatedPaths();
      newCreatedPaths.insert(newCreatedPaths.end(), allCreatedPaths.begin(), allCreatedPaths.end());
    }
  }

  for(const auto& action : result.outputActions.value().deferredActions)
  {
    if(const auto* creationActionPtr = dynamic_cast<const IDataCreationAction*>(action.get()); creationActionPtr != nullptr)
    {
      auto allCreatedPaths = creationActionPtr->getAllCreatedPaths();
      newCreatedPaths.insert(newCreatedPaths.end(), allCreatedPaths.begin(), allCreatedPaths.end());
    }
  }

  // Do not clear the created paths unless the preflight succeeded
  m_CreatedPaths = newCreatedPaths;
  m_DataModifiedActions = result.outputActions.value().modifiedActions;

  setPreflightStructure(data);
  sendFilterFaultMessage(m_Index, getFaultState());
  if(!m_Warnings.empty() || !m_Errors.empty())
  {
    sendFilterFaultDetailMessage(m_Index, m_Warnings, m_Errors);
  }

  if(allowRenaming)
  {
    renamedPaths = checkForRenamedPaths(oldCreatedPaths);
    notifyRenamedPaths(renamedPaths);
  }

  sendFilterRunStateMessage(m_Index, RunState::Idle);
  return true;
}

// -----------------------------------------------------------------------------
bool PipelineFilter::execute(DataStructure& data, const std::atomic_bool& shouldCancel)
{
  this->sendFilterRunStateMessage(m_Index, complex::RunState::Executing);
  this->sendFilterUpdateMessage(m_Index, "Begin");

  m_Warnings.clear();
  m_Errors.clear();
  clearFaultState();

  IFilter::MessageHandler messageHandler{[this](const IFilter::Message& message) { this->notifyFilterMessage(message); }};

  IFilter::ExecuteResult result = m_Filter->execute(data, getArguments(), this, messageHandler, shouldCancel);
  m_PreflightValues = std::move(result.outputValues);

  m_Warnings = result.result.warnings();

  if(result.result.invalid())
  {
    m_Errors = result.result.errors();
  }

  setHasWarnings(!m_Warnings.empty());
  setHasErrors(!m_Errors.empty());
  endExecution(data);

  if(!m_Warnings.empty() || !m_Errors.empty())
  {
    sendFilterFaultDetailMessage(m_Index, m_Warnings, m_Errors);
  }
  sendFilterFaultMessage(m_Index, getFaultState());
  this->sendFilterRunStateMessage(m_Index, complex::RunState::Idle);
  this->sendFilterUpdateMessage(m_Index, "End");

  return result.result.valid();
}

std::vector<DataPath> PipelineFilter::getCreatedPaths() const
{
  return m_CreatedPaths;
}

std::vector<DataObjectModification> PipelineFilter::getDataObjectModificationNotifications() const
{
  return m_DataModifiedActions;
}

namespace
{
/**
 * @brief Checks if one path could be a rename of another path.
 * Returns false if the lengths do not match or more than one segment has a different name.
 * @param path1
 * @param path2
 * @return bool
 */
bool checkIfRename(const DataPath& path1, const DataPath& path2)
{
  if(path1.getLength() != path2.getLength())
  {
    return false;
  }

  usize count = path1.getLength();
  usize numDifferences = 0;
  for(size_t i = 0; i < count; i++)
  {
    if(path1[i] != path2[i])
    {
      numDifferences++;
    }
  }

  return numDifferences == 1;
}

/**
 * @brief Finds and returns the overlap between two collections of DataPaths.
 * @param group1
 * @param group2
 * @return std::vector<DataPath>
 */
std::vector<DataPath> getOverlap(const std::vector<DataPath>& group1, const std::vector<DataPath>& group2)
{
  std::vector<DataPath> overlap;
  std::set_intersection(group1.begin(), group1.end(), group2.begin(), group2.end(), std::inserter(overlap, overlap.begin()));
  return overlap;
}

/**
 * @brief Removes the overlap between two groups of DataPaths
 * @param group1
 * @param group2
 */
void removeOverlap(std::vector<DataPath>& group1, std::vector<DataPath>& group2)
{
  auto overlap = getOverlap(group1, group2);
  for(const auto& value : overlap)
  {
    group1.erase(std::remove(group1.begin(), group1.end(), value), group1.end());
    group2.erase(std::remove(group2.begin(), group2.end(), value), group2.end());
  }
}

/**
 * @brief Finds and returns all detected rename paths between two collections of created DataPaths.
 * @param oldCreatedPaths
 * @param newCreatedPaths
 * @return PipelineFilter::RenamedPaths
 */
PipelineFilter::RenamedPaths findAllRenamePaths(const std::vector<DataPath>& oldCreatedPaths, const std::vector<DataPath>& newCreatedPaths)
{
  PipelineFilter::RenamedPaths renamedPairs;
  std::vector<DataPath> remainingPaths = oldCreatedPaths;
  for(const auto& createdPath : newCreatedPaths)
  {
    for(const auto& oldPath : remainingPaths)
    {
      if(checkIfRename(createdPath, oldPath))
      {
        renamedPairs.push_back({createdPath, oldPath});
      }
    }
  }
  return renamedPairs;
}

/**
 * @brief Takes a collection of possible renamed paths and returns a collection
 * of pairs that can be determined with a reasonable degree of certainty.
 *
 * Rename pairs that could have been between multiple sets of DataPaths are rejected.
 * @param renamedPairs
 * @return PipelineFilter::RenamedPaths
 */
PipelineFilter::RenamedPaths getConfirmedRenamePaths(const PipelineFilter::RenamedPaths& renamedPairs)
{
  std::vector<DataPath> rejectedFirstPaths;
  std::vector<DataPath> rejectedSecondPaths;

  // Find rejected RenamedPaths
  auto size = renamedPairs.size();
  for(usize i = 0; i + 1 < size; i++)
  {
    const auto& pair1 = renamedPairs[i];

    for(usize j = i; j < size; j++)
    {
      const auto& pair2 = renamedPairs[j];

      // Check for repeated paths
      if(pair1.first == pair2.first)
      {
        rejectedFirstPaths.push_back(pair1.first);
      }
      if(pair1.second == pair2.second)
      {
        rejectedSecondPaths.push_back(pair1.second);
      }
    }
  }

  PipelineFilter::RenamedPaths confirmed;
  for(const auto& renamedPair : renamedPairs)
  {
    if(std::find(rejectedFirstPaths.begin(), rejectedFirstPaths.end(), renamedPair.first) != rejectedFirstPaths.end())
    {
      continue;
    }
    if(std::find(rejectedSecondPaths.begin(), rejectedSecondPaths.end(), renamedPair.second) != rejectedSecondPaths.end())
    {
      continue;
    }
    confirmed.push_back(renamedPair);
  }

  return confirmed;
}
} // namespace

PipelineFilter::RenamedPaths PipelineFilter::checkForRenamedPaths(std::vector<DataPath> oldCreatedPaths) const
{
  // If created path sizes do not match, the values cannot be compared
  if(oldCreatedPaths.size() != m_CreatedPaths.size())
  {
    return {};
  }

  // Create a copy of the created paths to edit in removeOverlap(...)
  std::vector<DataPath> newCreatedPaths = m_CreatedPaths;

  removeOverlap(oldCreatedPaths, newCreatedPaths);
  auto renamedPairs = findAllRenamePaths(oldCreatedPaths, newCreatedPaths);
  renamedPairs = getConfirmedRenamePaths(renamedPairs);

  return renamedPairs;
}

std::vector<complex::Warning> PipelineFilter::getWarnings() const
{
  return m_Warnings;
}

std::vector<complex::Error> PipelineFilter::getErrors() const
{
  return m_Errors;
}

const std::vector<IFilter::PreflightValue>& PipelineFilter::getPreflightValues() const
{
  return m_PreflightValues;
}

std::unique_ptr<AbstractPipelineNode> PipelineFilter::deepCopy() const
{
  return std::make_unique<PipelineFilter>(m_Filter->clone(), m_Arguments);
}

void PipelineFilter::notifyFilterMessage(const IFilter::Message& message)
{
  if(message.type == IFilter::Message::Type::Info || message.type == IFilter::Message::Type::Debug)
  {
    sendFilterUpdateMessage(m_Index, message.message);
  }
  else if(message.type == IFilter::Message::Type::Progress)
  {
    const IFilter::ProgressMessage& progMessage = static_cast<const IFilter::ProgressMessage&>(message);
    sendFilterProgressMessage(m_Index, progMessage.progress, message.message);
  }
  else if(message.type == IFilter::Message::Type::Error)
  {
    sendFilterFaultMessage(m_Index, complex::FaultState::Errors);
  }
  else if(message.type == IFilter::Message::Type::Warning)
  {
    sendFilterFaultMessage(m_Index, complex::FaultState::Warnings);
  }
}

void PipelineFilter::notifyRenamedPaths(const RenamedPaths& renamedPathPairs)
{
  for(const auto& renamedPath : renamedPathPairs)
  {
    notify(std::make_shared<OutputRenamedMessage>(this, renamedPath));
  }
}

nlohmann::json PipelineFilter::toJsonImpl() const
{
  return CreateFilterJson(m_Filter->uuid().str(), m_Filter->name(), m_Filter->toJson(m_Arguments), m_Comments);
}

Result<std::unique_ptr<PipelineFilter>> PipelineFilter::FromJson(const nlohmann::json& json)
{
  return FromJson(json, *Application::Instance()->getFilterList());
}

Result<std::unique_ptr<PipelineFilter>> PipelineFilter::FromJson(const nlohmann::json& json, const FilterList& filterList)
{
  if(!json.contains(k_FilterKey.view()))
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-1, fmt::format("JSON does not contain key '{}'", k_FilterKey.view()));
  }

  const auto& filterObject = json[k_FilterKey];

  if(!filterObject.contains(k_FilterUuidKey.view()))
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-2, fmt::format("Filter JSON does not contain key '{}'", k_FilterUuidKey.view()));
  }

  const auto& filterNameObject = filterObject[k_FilterNameKey];
  if(!filterNameObject.is_string())
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-7, "'name' json value is not a string");
  }

  const auto& uuidObject = filterObject[k_FilterUuidKey];
  if(!uuidObject.is_string())
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-3, "UUID value is not a string");
  }
  auto filterName = filterNameObject.get<std::string>();
  auto uuidString = uuidObject.get<std::string>();
  std::optional<Uuid> uuid = Uuid::FromString(uuidString);
  if(!uuid.has_value())
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-4, fmt::format("'{}' is not a valid UUID", uuidString));
  }
  IFilter::UniquePointer filter = filterList.createFilter(*uuid);
  if(filter == nullptr)
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-5, fmt::format("Failed to create filter '{}' from UUID '{}'", filterName, uuidString));
  }

  if(!json.contains(k_ArgsKey.view()))
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-6, fmt::format("JSON does not contain key '{}'", k_ArgsKey.view()));
  }

  const auto& argsJson = json[k_ArgsKey];
  const bool isDisabled = ReadDisabledState(json);

  auto argsResult = filter->fromJson(argsJson);

  if(argsResult.invalid())
  {
    Result<std::unique_ptr<PipelineFilter>> result{nonstd::make_unexpected(std::move(argsResult.errors()))};
    result.warnings() = std::move(argsResult.warnings());
    return result;
  }

  auto pipelineFilter = std::make_unique<PipelineFilter>(std::move(filter), std::move(argsResult.value()));
  pipelineFilter->setDisabled(isDisabled);

  if(json.contains(k_FilterCommentsKey.view()))
  {
    pipelineFilter->setComments(json[k_FilterCommentsKey]);
  }

  Result<std::unique_ptr<PipelineFilter>> result{std::move(pipelineFilter)};
  result.warnings() = std::move(argsResult.warnings());
  return result;
}

bool isDataPath(const std::any& value)
{
  return value.type() == typeid(DataPath);
}

bool attemptRenamePath(DataPath& path, const AbstractPipelineNode::RenamedPaths& renamedPaths)
{
  bool pathChanged = false;
  for(const auto& renamedPath : renamedPaths)
  {
    if(path.attemptRename(renamedPath.second, renamedPath.first))
    {
      pathChanged = true;
    }
  }

  return pathChanged;
}

void PipelineFilter::renamePathArgs(const RenamedPaths& renamedPaths)
{
  for(const auto& arg : m_Arguments)
  {
    const std::any& argValue = arg.second;
    if(isDataPath(argValue))
    {
      auto argPath = std::any_cast<DataPath>(argValue);
      if(attemptRenamePath(argPath, renamedPaths))
      {
        m_Arguments.insertOrAssign(arg.first, std::make_any<DataPath>(argPath));
      }
    }
  }
}

complex::WarningCollection convertErrors(const complex::ErrorCollection& errors, const std::string& filterName)
{
  if(errors.empty())
  {
    return {};
  }

  std::string prefix = fmt::format("Filter: '{}' ", filterName);
  WarningCollection warnings;
  for(const auto& error : errors)
  {
    warnings.emplace_back(Warning{error.code, prefix + error.message});
  }
  return std::move(warnings);
}

std::string createErrorComments(const complex::ErrorCollection& errors, const std::string& filterName)
{
  if(errors.empty())
  {
    return "";
  }

  std::string prefix = "Parameter conversion error: ";
  std::string output;
  for(const auto& error : errors)
  {
    output += prefix + error.message + "\n";
  }
  return output;
}

Result<std::unique_ptr<PipelineFilter>> PipelineFilter::FromSIMPLJson(const nlohmann::json& json, const FilterList& filterList)
{
  if(!json.contains(k_SIMPLFilterUuidKey))
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-1, fmt::format("SIMPL filter does not contain key '{}'", k_SIMPLFilterUuidKey.view()));
  }

  auto uuidString = json[k_SIMPLFilterUuidKey].get<std::string>();
  std::optional<Uuid> filterUuid = Uuid::FromString(uuidString);

  if(!filterUuid.has_value())
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-2, fmt::format("Unable to parse uuid '{}'", uuidString));
  }

  std::optional<AbstractPlugin::SIMPLData> simplData = FindComplexConversionFromSIMPL(*filterUuid, filterList);

  if(!simplData.has_value())
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-3, fmt::format("Unable to parse find conversion data for filter '{}'", uuidString));
  }

  IFilter::UniquePointer filter = filterList.createFilter(simplData->complexUuid);

  if(!simplData->convertJson)
  {
    return MakeErrorResult<std::unique_ptr<PipelineFilter>>(-4, fmt::format("Conversion function for filter '{}' is null", uuidString));
  }

  Result<Arguments> argumentsResult = simplData->convertJson(json);

  const auto filterName = filter->name();
  auto pipelineFilter = std::make_unique<PipelineFilter>(std::move(filter));
  if(argumentsResult.valid())
  {
    pipelineFilter->setArguments(std::move(argumentsResult.value()));
  }

  WarningCollection warnings;
  if(argumentsResult.invalid())
  {
    warnings = convertErrors(argumentsResult.errors(), filterName);
    pipelineFilter->setComments(createErrorComments(argumentsResult.errors(), filterName));
  }

  return {std::move(pipelineFilter), std::move(warnings)};
}
