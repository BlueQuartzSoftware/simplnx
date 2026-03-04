#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/IArrayThreshold.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <set>
#include <vector>

namespace nx::core
{

/**
 * @brief Abstract base class for array thresholds. Provides data storage for
 * inversion state and union operator, with concrete implementations of the
 * IArrayThreshold interface methods.
 */
class SIMPLNX_EXPORT AbstractArrayThreshold : public IArrayThreshold
{
public:
  AbstractArrayThreshold();
  AbstractArrayThreshold(const AbstractArrayThreshold& other);
  AbstractArrayThreshold(AbstractArrayThreshold&& other) noexcept;
  ~AbstractArrayThreshold() override;

  [[nodiscard]] bool isInverted() const override;
  void setInverted(bool inverted) override;

  [[nodiscard]] UnionOperator getUnionOperator() const override;
  void setUnionOperator(UnionOperator unionType) override;

  [[nodiscard]] nlohmann::json toJson() const override;

private:
  bool m_IsInverted{false};
  UnionOperator m_UnionType{UnionOperator::And};
};

/**
 * @brief
 */
class SIMPLNX_EXPORT ArrayThreshold : public AbstractArrayThreshold
{
public:
  using ComparisonValue = float64;
  enum class ComparisonType
  {
    GreaterThan,
    LessThan,
    Operator_Equal,
    Operator_NotEqual
  };

  ArrayThreshold();
  ArrayThreshold(const ArrayThreshold& other);
  ArrayThreshold(ArrayThreshold&& other) noexcept;
  ~ArrayThreshold() override;

  ArrayThreshold& operator=(const ArrayThreshold& other);
  ArrayThreshold& operator=(ArrayThreshold&& other) noexcept;

  [[nodiscard]] DataPath getArrayPath() const;
  void setArrayPath(const DataPath& path);

  [[nodiscard]] ComparisonValue getComparisonValue() const;
  void setComparisonValue(ComparisonValue value);

  [[nodiscard]] usize getComponentIndex() const;
  void setComponentIndex(usize index);

  [[nodiscard]] ComparisonType getComparisonType() const;
  void setComparisonType(ComparisonType comparison);

  [[nodiscard]] std::set<DataPath> getRequiredPaths() const override;

  [[nodiscard]] nlohmann::json toJson() const override;
  static std::shared_ptr<ArrayThreshold> FromJson(const nlohmann::json& json);

private:
  DataPath m_ArrayPath;
  ComparisonValue m_Value{0.0};
  usize m_ComponentIndex = 0;
  ComparisonType m_Comparison{ComparisonType::GreaterThan};
};

/**
 * @brief
 */
class SIMPLNX_EXPORT ArrayThresholdSet : public AbstractArrayThreshold
{
public:
  using CollectionType = std::vector<std::shared_ptr<IArrayThreshold>>;

  ArrayThresholdSet();
  ArrayThresholdSet(const ArrayThresholdSet& other);
  ArrayThresholdSet(ArrayThresholdSet&& other) noexcept;
  ~ArrayThresholdSet() override;

  ArrayThresholdSet& operator=(const ArrayThresholdSet& other);
  ArrayThresholdSet& operator=(ArrayThresholdSet&& other) noexcept;

  [[nodiscard]] CollectionType getArrayThresholds() const;
  void setArrayThresholds(const CollectionType& thresholds);

  [[nodiscard]] std::set<DataPath> getRequiredPaths() const override;

  [[nodiscard]] nlohmann::json toJson() const override;
  static std::shared_ptr<ArrayThresholdSet> FromJson(const nlohmann::json& json);

private:
  CollectionType m_Thresholds;
};
} // namespace nx::core
