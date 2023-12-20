#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include <memory>
#include <string>
#include <vector>

namespace nx::core
{
class DataStructure;
class DataPath;

/**
 * @brief The CalculatorItem class
 */
class SIMPLNXCORE_EXPORT CalculatorItem
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
    Success = 0,
    InvalidEquation = -4009,
    InvalidComponent = -4010,
    EmptyEquation = -4011,
    EmptyCalArray = -4012,
    EmptySelMatrix = -4013,
    LostAttrMatrix = -4014,
    IncorrectTupleCount = -4015,
    InconsistentTuples = -4016,
    UnrecognizedItem = -4017,
    MismatchedParentheses = -4018,
    UnexpectedOutput = -4019,
    ComponentOutOfRange = -4020,
    InvalidArrayName = -4022,
    InconsistentIndexing = -4023,
    InconsistentCompDims = -4024,
    AttrArrayZeroTuplesWarning = -4025,
    OrphanedComponent = -4026,
    OperatorNoLeftValue = -4027,
    OperatorNoRightValue = -4028,
    OperatorNoOpeningParen = -4029,
    OperatorNoClosingParen = -4030,
    NoNumericArguments = -4031,
    MissingArguments = -4032,
    NotEnoughArguments = -4033,
    TooManyArguments = -4034,
    InvalidSymbol = -4035,
    NoPrecedingUnaryOperator = -4036,
    InvalidOutputArrayType = -4037,
    AttributeMatrixInsertionError = -4038
  };

  enum class WarningCode : EnumType
  {
    None = 0,
    NumericValueWarning = -5010,
    AmbiguousNameWarning = -5011
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
  std::string m_InfixToken = {};

public:
  CalculatorItem(const CalculatorItem&) = delete;            // Copy Constructor Not Implemented
  CalculatorItem(CalculatorItem&&) = delete;                 // Move Constructor Not Implemented
  CalculatorItem& operator=(const CalculatorItem&) = delete; // Copy Assignment Not Implemented
  CalculatorItem& operator=(CalculatorItem&&) = delete;      // Move Assignment Not Implemented
};

} // namespace nx::core
