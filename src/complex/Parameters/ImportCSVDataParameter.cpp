/* ============================================================================
 * Copyright (c) 2022-2022 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ImportCSVDataParameter.hpp"

namespace complex
{
// -----------------------------------------------------------------------------
ImportCSVDataParameter::ImportCSVDataParameter(const std::string& name, const std::string& humanName, const std::string& helpText, const ValueType& defaultValue)
: ValueParameter(name, humanName, helpText)
, m_DefaultValue(defaultValue)
{
}

// -----------------------------------------------------------------------------
Uuid ImportCSVDataParameter::uuid() const
{
  return ParameterTraits<ImportCSVDataParameter>::uuid;
}

// -----------------------------------------------------------------------------
IParameter::AcceptedTypes ImportCSVDataParameter::acceptedTypes() const
{
  return {typeid(ValueType)};
}

// -----------------------------------------------------------------------------
nlohmann::json ImportCSVDataParameter::toJson(const std::any& value) const
{
  const auto& CSVWizardData = GetAnyRef<ValueType>(value);
  nlohmann::json json = CSVWizardData.writeJson();
  return json;
}

// -----------------------------------------------------------------------------
Result<std::any> ImportCSVDataParameter::fromJson(const nlohmann::json& json) const
{
  return {ConvertResultTo<std::any>(CSVWizardData::ReadJson(json))};
}

// -----------------------------------------------------------------------------
IParameter::UniquePointer ImportCSVDataParameter::clone() const
{
  return std::make_unique<ImportCSVDataParameter>(name(), humanName(), helpText(), m_DefaultValue);
}

// -----------------------------------------------------------------------------
std::any ImportCSVDataParameter::defaultValue() const
{
  return m_DefaultValue;
}

// -----------------------------------------------------------------------------
Result<> ImportCSVDataParameter::validate(const std::any& value) const
{
  [[maybe_unused]] auto data = std::any_cast<CSVWizardData>(value);
  return {};
}
} // namespace complex
