#include "ArrayThreshold.hpp"

#include "simplnx/DataStructure/DataArray.hpp"

using namespace nx::core;

namespace
{
constexpr StringLiteralType k_TypeTag = "type";
constexpr StringLiteralType k_InvertedTag = "inverted";
constexpr StringLiteralType k_UnionTag = "union";
constexpr StringLiteralType k_ValueTag = "value";
constexpr StringLiteralType k_ArrayPathTag = "array_path";
constexpr StringLiteralType k_ComponentIndexTag = "component_index";
constexpr StringLiteralType k_ComparisonTag = "comparison";
constexpr StringLiteralType k_ThresholdsTag = "thresholds";

// Types
constexpr StringLiteralType k_ArrayType = "array";
constexpr StringLiteralType k_CollectionType = "collection";
} // namespace

IArrayThreshold::IArrayThreshold() = default;
IArrayThreshold::IArrayThreshold(const IArrayThreshold& other) = default;

IArrayThreshold::IArrayThreshold(IArrayThreshold&& other) noexcept
: m_IsInverted(other.m_IsInverted)
, m_UnionType(other.m_UnionType)
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
  json[k_InvertedTag] = isInverted();
  json[k_UnionTag] = getUnionOperator();

  return json;
}

ArrayThreshold::ArrayThreshold()

{
}
ArrayThreshold::ArrayThreshold(const ArrayThreshold& other) = default;

ArrayThreshold::ArrayThreshold(ArrayThreshold&& other) noexcept
: IArrayThreshold(std::move(other))
, m_ArrayPath(std::move(other.m_ArrayPath))
, m_Value(other.m_Value)
, m_Comparison(other.m_Comparison)
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
  m_Value = other.m_Value;
  m_Comparison = other.m_Comparison;

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

usize ArrayThreshold::getComponentIndex() const
{
  return m_ComponentIndex;
}

void ArrayThreshold::setComponentIndex(usize index)
{
  m_ComponentIndex = index;
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

nlohmann::json ArrayThreshold::toJson() const
{
  auto json = IArrayThreshold::toJson();
  json[k_TypeTag] = k_ArrayType;
  json[k_ArrayPathTag] = getArrayPath().toString();
  json[k_ComponentIndexTag] = getComponentIndex();
  json[k_ValueTag] = getComparisonValue();
  json[k_ComparisonTag] = getComparisonType();

  return json;
}

std::shared_ptr<ArrayThreshold> ArrayThreshold::FromJson(const nlohmann::json& json)
{
  if(json[k_TypeTag] != k_ArrayType)
  {
    return {};
  }
  try
  {
    auto threshold = std::make_shared<ArrayThreshold>();
    threshold->setInverted(json[k_InvertedTag].get<bool>());
    threshold->setUnionOperator(static_cast<UnionOperator>(json[k_UnionTag].get<int32>()));

    auto arrayPath = DataPath::FromString(json[k_ArrayPathTag].get<std::string>());
    if(!arrayPath.has_value())
    {
      return nullptr;
    }
    threshold->setArrayPath(arrayPath.value());
    usize componentIndex = json.contains(k_ComponentIndexTag) ? json[k_ComponentIndexTag].get<usize>() : 0;
    threshold->setComponentIndex(componentIndex);
    threshold->setComparisonType(static_cast<ComparisonType>(json[k_ComparisonTag].get<int32>()));
    threshold->setComparisonValue(json[k_ValueTag].get<ComparisonValue>());

    return threshold;
  } catch(std::exception& e)
  {
    return nullptr;
  }
}

ArrayThresholdSet::ArrayThresholdSet()

{
}

ArrayThresholdSet::ArrayThresholdSet(const ArrayThresholdSet& other) = default;

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
  json[k_TypeTag] = k_CollectionType;

  nlohmann::json collection = nlohmann::json::array();
  for(const auto& threshold : getArrayThresholds())
  {
    collection.push_back(threshold->toJson());
  }
  json[k_ThresholdsTag] = collection;

  return json;
}

std::shared_ptr<ArrayThresholdSet> ArrayThresholdSet::FromJson(const nlohmann::json& json)
{
  if(json[k_TypeTag] != k_CollectionType)
  {
    return nullptr;
  }
  try
  {
    auto thresholdSet = std::make_shared<ArrayThresholdSet>();
    thresholdSet->setInverted(json[k_InvertedTag].get<bool>());
    thresholdSet->setUnionOperator(static_cast<UnionOperator>(json[k_UnionTag].get<int32>()));

    CollectionType collection;
    const auto& jsonCollection = json[k_ThresholdsTag];
    for(const auto& jsonThreshold : jsonCollection)
    {
      const auto& type = jsonThreshold[k_TypeTag];
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
