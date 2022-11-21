#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Core/Core_export.hpp"

namespace complex
{
class DataStructure;
class DataPath;

/**
 * @brief The CalculatorItem class
 */
class CORE_EXPORT CalculatorItem
{
public:
  using Self = CalculatorItem;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;
  static Pointer NullPointer();

  virtual ~CalculatorItem();

  using EnumType = int;

  enum class ErrorCode : EnumType
  {
    SUCCESS = 0,
    INVALID_EQUATION = -4009,
    INVALID_COMPONENT = -4010,
    EMPTY_EQUATION = -4011,
    EMPTY_CAL_ARRAY = -4012,
    EMPTY_SEL_MATRIX = -4013,
    LOST_ATTR_MATRIX = -4014,
    INCORRECT_TUPLE_COUNT = -4015,
    INCONSISTENT_TUPLES = -4016,
    UNRECOGNIZED_ITEM = -4017,
    MISMATCHED_PARENTHESES = -4018,
    UNEXPECTED_OUTPUT = -4019,
    COMPONENT_OUT_OF_RANGE = -4020,
    INVALID_ARRAY_NAME = -4022,
    INCONSISTENT_INDEXING = -4023,
    INCONSISTENT_COMP_DIMS = -4024,
    ATTRARRAY_ZEROTUPLES_WARNING = -4025,
    ORPHANED_COMPONENT = -4026,
    OPERATOR_NO_LEFT_VALUE = -4027,
    OPERATOR_NO_RIGHT_VALUE = -4028,
    OPERATOR_NO_OPENING_PAREN = -4029,
    OPERATOR_NO_CLOSING_PAREN = -4030,
    NO_NUMERIC_ARGUMENTS = -4031,
    MISSING_ARGUMENTS = -4032,
    NOT_ENOUGH_ARGUMENTS = -4033,
    TOO_MANY_ARGUMENTS = -4034,
    INVALID_SYMBOL = -4035,
    NO_PRECEDING_UNARY_OPERATOR = -4036,
    InvalidOutputArrayType = -4037,
    AttributeMatrixInsertionError = -4038
  };

  enum class WarningCode : EnumType
  {
    NONE = 0,
    NUMERIC_VALUE_WARNING = -5010,
    AMBIGUOUS_NAME_WARNING = -5011
  };

  std::string getInfixToken();

  virtual CalculatorItem::ErrorCode checkValidity(std::vector<CalculatorItem::Pointer> infixVector, int currentIndex, std::string& msg) = 0;

  bool isICalculatorArray();

  bool isArray();

  bool isNumber();

  static DataPath GetUniquePathName(const DataStructure& dataStructure, DataPath path);

protected:
  CalculatorItem();

  void setInfixToken(const std::string& token);

private:
  std::string m_InfixToken = "";

public:
  CalculatorItem(const CalculatorItem&) = delete;            // Copy Constructor Not Implemented
  CalculatorItem(CalculatorItem&&) = delete;                 // Move Constructor Not Implemented
  CalculatorItem& operator=(const CalculatorItem&) = delete; // Copy Assignment Not Implemented
  CalculatorItem& operator=(CalculatorItem&&) = delete;      // Move Assignment Not Implemented
};

} // namespace complex