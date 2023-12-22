#include "ArrayThreshold.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

using namespace nx::core;

namespace
{
constexpr StringLiteral k_Type_Tag = "type";
constexpr StringLiteral k_Inverted_Tag = "inverted";
constexpr StringLiteral k_Union_Tag = "union";
constexpr StringLiteral k_Value_Tag = "value";
constexpr StringLiteral k_ArrayPath_Tag = "array_path";
constexpr StringLiteral k_Comparison_Tag = "comparison";
constexpr StringLiteral k_Thresholds_Tag = "thresholds";

// Types
constexpr StringLiteral k_ArrayType = "array";
constexpr StringLiteral k_CollectionType = "collection";
} // namespace

IArrayThreshold::IArrayThreshold()

{
}
IArrayThreshold::IArrayThreshold(const IArrayThreshold& other)
: m_IsInverted(other.m_IsInverted)
, m_UnionType(other.m_UnionType)
{
}
IArrayThreshold::IArrayThreshold(IArrayThreshold&& other) noexcept
: m_IsInverted(std::move(other.m_IsInverted))
, m_UnionType(std::move(other.m_UnionType))
{
}
IArrayThreshold::~IArrayThreshold() = default;

bool IArrayThreshold::isInverted() const
{
  return m_IsInverted;
}
void IArrayThreshold::setInverted(bool inverted)
{
  m_IsInverted = inverted;
}

IArrayThreshold::UnionOperator IArrayThreshold::getUnionOperator() const
{
  return m_UnionType;
}
void IArrayThreshold::setUnionOperator(UnionOperator unionType)
{
  m_UnionType = unionType;
}

nlohmann::json IArrayThreshold::toJson() const
{
  nlohmann::json json;
  json[k_Inverted_Tag] = isInverted();
  json[k_Union_Tag] = getUnionOperator();

  return json;
}

ArrayThreshold::ArrayThreshold()
: IArrayThreshold()
, m_ArrayPath()

{
}
ArrayThreshold::ArrayThreshold(const ArrayThreshold& other)
: IArrayThreshold(other)
, m_ArrayPath(other.m_ArrayPath)
, m_Value(other.m_Value)
, m_Comparison(other.m_Comparison)
{
}
ArrayThreshold::ArrayThreshold(ArrayThreshold&& other) noexcept
: IArrayThreshold(std::move(other))
, m_ArrayPath(std::move(other.m_ArrayPath))
, m_Value(std::move(other.m_Value))
, m_Comparison(std::move(other.m_Comparison))
{
}
ArrayThreshold::~ArrayThreshold() = default;

ArrayThreshold& ArrayThreshold::operator=(const ArrayThreshold& other)
{
  setInverted(other.isInverted());
  setUnionOperator(other.getUnionOperator());

  m_ArrayPath = other.m_ArrayPath;
  m_Value = other.m_Value;
  m_Comparison = other.m_Comparison;

  return *this;
}
ArrayThreshold& ArrayThreshold::operator=(ArrayThreshold&& other) noexcept
{
  setInverted(other.isInverted());
  setUnionOperator(other.getUnionOperator());

  m_ArrayPath = std::move(other.m_ArrayPath);
  m_Value = std::move(other.m_Value);
  m_Comparison = std::move(other.m_Comparison);

  return *this;
}

DataPath ArrayThreshold::getArrayPath() const
{
  return m_ArrayPath;
}
void ArrayThreshold::setArrayPath(const DataPath& path)
{
  m_ArrayPath = path;
}

ArrayThreshold::ComparisonValue ArrayThreshold::getComparisonValue() const
{
  return m_Value;
}
void ArrayThreshold::setComparisonValue(ComparisonValue value)
{
  m_Value = value;
}

ArrayThreshold::ComparisonType ArrayThreshold::getComparisonType() const
{
  return m_Comparison;
}
void ArrayThreshold::setComparisonType(ComparisonType comparison)
{
  m_Comparison = comparison;
}

std::set<DataPath> ArrayThreshold::getRequiredPaths() const
{
  return {getArrayPath()};
}

template <typename T>
bool checkArrayThreshold(const DataArray<T>* dataArray, ArrayThreshold::ComparisonValue value, ArrayThreshold::ComparisonType comparison, usize tupleId)
{
  const auto dataStore = dataArray->getDataStore();
  const auto numComponents = dataStore->getNumberOfComponents();

  auto tuplePos = tupleId * numComponents;
  T tupleValue = 0;
  for(usize i = tuplePos; i < tuplePos + numComponents; i++)
  {
    // Avoid overflow
    tupleValue += dataStore->getValue(i) / static_cast<double>(numComponents);
  }

  bool threshold = false;
  switch(comparison)
  {
  case ArrayThreshold::ComparisonType::GreaterThan:
    threshold = (tupleValue > value);
    break;
  case ArrayThreshold::ComparisonType::LessThan:
    threshold = (tupleValue < value);
    break;
  default:
    threshold = false;
    break;
  }

  return threshold;
}

nlohmann::json ArrayThreshold::toJson() const
{
  auto json = IArrayThreshold::toJson();
  json[k_Type_Tag] = k_ArrayType;
  json[k_ArrayPath_Tag] = getArrayPath().toString();
  json[k_Value_Tag] = getComparisonValue();
  json[k_Comparison_Tag] = getComparisonType();

  return json;
}

std::shared_ptr<ArrayThreshold> ArrayThreshold::FromJson(const nlohmann::json& json)
{
  if(json[k_Type_Tag] != k_ArrayType)
  {
    return {};
  }
  try
  {
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setInverted(json[k_Inverted_Tag].get<bool>());
    threshold->setUnionOperator(static_cast<UnionOperator>(json[k_Union_Tag].get<int32>()));

    auto arrayPath = DataPath::FromString(json[k_ArrayPath_Tag].get<std::string>());
    if(!arrayPath.has_value())
    {
      return nullptr;
    }
    threshold->setArrayPath(arrayPath.value());
    threshold->setComparisonType(static_cast<ComparisonType>(json[k_Comparison_Tag].get<int32>()));
    threshold->setComparisonValue(json[k_Value_Tag].get<ComparisonValue>());

    return threshold;
  } catch(std::exception& e)
  {
    return nullptr;
  }
}

ArrayThresholdSet::ArrayThresholdSet()
: IArrayThreshold()
{
}
ArrayThresholdSet::ArrayThresholdSet(const ArrayThresholdSet& other)
: IArrayThreshold(other)
, m_Thresholds(other.m_Thresholds)
{
}
ArrayThresholdSet::ArrayThresholdSet(ArrayThresholdSet&& other) noexcept
: IArrayThreshold(std::move(other))
, m_Thresholds(std::move(other.m_Thresholds))
{
}
ArrayThresholdSet::~ArrayThresholdSet() = default;

ArrayThresholdSet& ArrayThresholdSet::operator=(const ArrayThresholdSet& other)
{
  setInverted(other.isInverted());
  setUnionOperator(other.getUnionOperator());

  m_Thresholds = other.m_Thresholds;
  return *this;
}

ArrayThresholdSet& ArrayThresholdSet::operator=(ArrayThresholdSet&& other) noexcept
{
  setInverted(other.isInverted());
  setUnionOperator(other.getUnionOperator());

  m_Thresholds = std::move(other.m_Thresholds);
  return *this;
}

ArrayThresholdSet::CollectionType ArrayThresholdSet::getArrayThresholds() const
{
  return m_Thresholds;
}
void ArrayThresholdSet::setArrayThresholds(const CollectionType& thresholds)
{
  m_Thresholds = thresholds;
}

std::set<DataPath> ArrayThresholdSet::getRequiredPaths() const
{
  std::set<DataPath> requiredPaths;
  for(const auto& threshold : getArrayThresholds())
  {
    requiredPaths.merge(threshold->getRequiredPaths());
  }
  return requiredPaths;
}

nlohmann::json ArrayThresholdSet::toJson() const
{
  auto json = IArrayThreshold::toJson();
  json[k_Type_Tag] = k_CollectionType;

  nlohmann::json collection = nlohmann::json::array();
  for(const auto& threshold : getArrayThresholds())
  {
    collection.push_back(threshold->toJson());
  }
  json[k_Thresholds_Tag] = collection;

  return json;
}

std::shared_ptr<ArrayThresholdSet> ArrayThresholdSet::FromJson(const nlohmann::json& json)
{
  if(json[k_Type_Tag] != k_CollectionType)
  {
    return nullptr;
  }
  try
  {
    auto thresholdSet = std::make_shared<ArrayThresholdSet>();
    thresholdSet->setInverted(json[k_Inverted_Tag].get<bool>());
    thresholdSet->setUnionOperator(static_cast<UnionOperator>(json[k_Union_Tag].get<int32>()));

    CollectionType collection;
    const auto& jsonCollection = json[k_Thresholds_Tag];
    for(const auto& jsonThreshold : jsonCollection)
    {
      const auto& type = jsonThreshold[k_Type_Tag];
      if(type == k_CollectionType)
      {
        auto item = ArrayThresholdSet::FromJson(jsonThreshold);
        if(item == nullptr)
        {
          return nullptr;
        }
        collection.push_back(item);
      }
      else if(type == k_ArrayType)
      {
        auto item = ArrayThreshold::FromJson(jsonThreshold);
        if(item == nullptr)
        {
          return nullptr;
        }
        collection.push_back(item);
      }
    }
    thresholdSet->setArrayThresholds(collection);
    return thresholdSet;
  } catch(std::exception& e)
  {
    return nullptr;
  }
}
