#include "EnsembleInfoParameter.hpp"

#include "simplnx/Common/Any.hpp"
#include "simplnx/Common/StringLiteral.hpp"

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{
namespace
{
constexpr StringLiteral k_InputFilePath = "input_file_path";
constexpr StringLiteral k_StartSlice = "start_slice";
constexpr StringLiteral k_EndSlice = "end_slice";
constexpr StringLiteral k_UseRecommendedTransform = "use_recommended_transform";
constexpr StringLiteral k_EulerRepresentation = "euler_representation";
constexpr StringLiteral k_HDF5DataPaths = "hdf5_data_paths";

} // namespace

//-----------------------------------------------------------------------------
EnsembleInfoParameter::EnsembleInfoParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

//-----------------------------------------------------------------------------
Uuid EnsembleInfoParameter::uuid() const
{
  return ParameterTraits<EnsembleInfoParameter>::uuid;
}

//-----------------------------------------------------------------------------
IParameter::AcceptedTypes EnsembleInfoParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

//------------------------------------------------------------------------------
IParameter::VersionType EnsembleInfoParameter::getVersion() const
{
  return 1;
}

//-----------------------------------------------------------------------------
nlohmann::json EnsembleInfoParameter::toJsonImpl(const std::any& value) const
{
  const auto& table = GetAnyRef<ValueType>(value);
  nlohmann::json rows;
  for(const auto& vector : table)
  {
    nlohmann::json cols;
    for(auto cell : vector)
    {
      cols.push_back(cell);
    }
    rows.push_back(cols);
  }
  return rows;
}

//-----------------------------------------------------------------------------
Result<std::any> EnsembleInfoParameter::fromJsonImpl(const nlohmann::json& json, VersionType version) const
{
  static constexpr StringLiteral prefix = "FilterParameter 'EnsembleInfoParameter' Error: ";

  if(!json.is_array())
  {
    return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}The JSON data entry for key '{}' is not an array.", prefix.view(), name()));
  }

  usize numRows = json.size();

  ValueType table;
  table.reserve(numRows);
  for(usize i = 0; i < json.size(); i++)
  {
    const auto& rowJson = json[i];
    if(!rowJson.is_array())
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry, fmt::format("{}Row {} json is not an array", prefix.view(), i));
    }

    usize numCols = rowJson.size();
    if(numCols != 3)
    {
      return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Missing_Entry,
                                       fmt::format("{}Row {} json array is not the correct size. Found {} values but expecting 3.", prefix.view(), i, numCols));
    }
    RowType row;
    for(usize j = 0; j < numCols; j++)
    {
      const auto& value = rowJson[j];
      if(!value.is_string())
      {
        return MakeErrorResult<std::any>(FilterParameter::Constants::k_Json_Value_Not_String, fmt::format("{}Element ({}, {}) is not a string", prefix.view(), i, j));
      }

      row[j] = value.get<std::string>();
    }

    table.push_back(std::move(row));
  }

  return {{std::move(table)}};
}

//-----------------------------------------------------------------------------
IParameter::UniquePointer EnsembleInfoParameter::clone() const
{
  return std::make_unique<EnsembleInfoParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

//-----------------------------------------------------------------------------
std::any EnsembleInfoParameter::defaultValue() const
{
  return m_DefaultValue;
}

//-----------------------------------------------------------------------------
Result<> EnsembleInfoParameter::validate(const std::any& valueRef) const
{
  const std::string prefix = fmt::format("Parameter Name: '{}'\n    Parameter Key: '{}'\n    Validation Error: ", humanName(), name());
  std::vector<Error> errors;

  const auto& table = GetAnyRef<ValueType>(valueRef);
  usize numRows = table.size();
  if(numRows == 0)
  {
    return MakeErrorResult<>(FilterParameter::Constants::k_Validate_TupleShapeValue, "{}You must add at least one row (ensemble phase)");
  }
  usize lastNumCols = table.front().size();
  for(usize i = 1; i < numRows; i++)
  {
    const auto& row = table[i];
    usize numCols = row.size();
    if(numCols != lastNumCols)
    {
      return MakeErrorResult<>(FilterParameter::Constants::k_Validate_TupleShapeValue,
                               fmt::format("{}Row {} does not have the correct number of elements. Found {} values but expecting {}.", prefix, i, numCols, lastNumCols));
    }
  }
  return {};
}

namespace SIMPLConversion
{
namespace
{
constexpr StringLiteral k_CrystalStructureKey = "CrystalStructure";
constexpr StringLiteral k_PhaseNameKey = "PhaseName";
constexpr StringLiteral k_PhaseTypeKey = "PhaseType";
} // namespace

Result<EnsembleInfoFilterParameterConverter::ValueType> EnsembleInfoFilterParameterConverter::convert(const nlohmann::json& json)
{
  if(!json.is_array())
  {
    return MakeErrorResult<ValueType>(-1, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not an array", json.dump()));
  }

  ValueType value;

  for(const auto& iter : json)
  {
    if(!iter.is_object())
    {
      return MakeErrorResult<ValueType>(-2, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not an object", iter.dump()));
    }

    if(!iter[k_CrystalStructureKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-3, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not an integer '{}'", iter.dump(), iter[k_CrystalStructureKey].dump()));
    }
    if(!iter[k_PhaseNameKey].is_string())
    {
      return MakeErrorResult<ValueType>(-4, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not a string '{}'", iter.dump(), iter[k_PhaseNameKey].dump()));
    }
    if(!iter[k_PhaseTypeKey].is_number_integer())
    {
      return MakeErrorResult<ValueType>(-5, fmt::format("EnsembleInfoFilterParameterConverter json '{}' is not an integer '{}'", iter.dump(), iter[k_PhaseTypeKey].dump()));
    }

    ParameterType::RowType row;
    row[0] = ParameterType::k_CrystalStructures[iter[k_CrystalStructureKey].get<int32>()];
    row[1] = ParameterType::k_PhaseTypes[iter[k_PhaseTypeKey].get<int32>()];
    row[2] = iter[k_PhaseNameKey].get<std::string>();

    value.push_back(std::move(row));
  }

  return {std::move(value)};
}
} // namespace SIMPLConversion
} // namespace nx::core
