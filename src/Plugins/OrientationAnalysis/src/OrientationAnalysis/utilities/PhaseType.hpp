#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Common/Types.hpp"

#include <map>
#include <string>
#include <vector>

namespace nx::core
{
/**
 * @class PhaseType PhaseType.h
 * @brief
 * @author Mike Jackson for BlueQuartz.net
 * @version 1.0
 */
class ORIENTATIONANALYSIS_EXPORT PhaseType
{
public:
  virtual ~PhaseType();

  using EnumType = uint32;

  // The lists here are IN ORDER of the enumerations from the EBSDLib. DO NOT CHANGE THE ORDER.
  enum class Type : EnumType
  {
    Primary = 0,        //!<
    Precipitate = 1,    //!<
    Transformation = 2, //!<
    Matrix = 3,         //!<
    Boundary = 4,       //!<
    Unknown = 999,      //!<
    Any = 4294967295U

  };

  using Types = std::vector<Type>;

  /**
   * @brief Converts to a std::vector<EnumType>. Useful for
   * writing to/from HDF5 or other streams that do not understand Type
   * @param types
   * @return
   */
  static std::vector<EnumType> ToVector(Types& types);

  /**
   * @brief Converts <b>From</b> a std::vector<EnumType> to std::vector<Type>. Useful for
   * writing to/from HDF5 or other streams that do not understand Type
   * @param types
   * @return
   */
  static Types FromVector(std::vector<EnumType>& types);

  /**
   * @brief PrimaryStr
   * @return
   */
  static std::string PrimaryStr();

  /**
   * @brief PrecipitateStr
   * @return
   */
  static std::string PrecipitateStr();

  /**
   * @brief TransformationStr
   * @return
   */
  static std::string TransformationStr();

  /**
   * @brief MatrixStr
   * @return
   */
  static std::string MatrixStr();

  /**
   * @brief BoundaryStr
   * @return
   */
  static std::string BoundaryStr();

  /**
   * @brief UnknownStr
   * @return
   */
  static std::string UnknownStr();

  /**
   * @brief getPhaseTypeString
   * @param phaseType
   * @return
   */
  static std::string getPhaseTypeString(Type phaseType);

  /**
   * @brief getPhaseType
   * @param str
   * @return
   */
  static Type getPhaseType(const char* str);

  /**
   * @brief getPhaseTypeStrings
   * @param strings
   */
  static void getPhaseTypeStrings(std::vector<std::string>& strings);

  /**
   * @brief getPhaseTypeEnums
   * @param types
   */
  static void getPhaseTypeEnums(std::vector<Type>& types);

  /**
   * @brief getPhaseTypeMap
   * @param map
   */
  static void getPhaseTypeMap(std::map<Type, std::string>& phaseMap);

protected:
  PhaseType();

public:
  PhaseType(const PhaseType&) = delete;            // Copy Constructor Not Implemented
  PhaseType(PhaseType&&) = delete;                 // Move Constructor Not Implemented
  PhaseType& operator=(const PhaseType&) = delete; // Copy Assignment Not Implemented
  PhaseType& operator=(PhaseType&&) = delete;      // Move Assignment Not Implemented
};

} // namespace nx::core
