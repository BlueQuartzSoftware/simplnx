#include "ReadOnScaleTableFile.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <locale.h>
#include <string>
#include <string_view>

using namespace nx::core;

namespace
{
constexpr int32 k_FileAccessError = -12051;
constexpr int32 k_HeaderCountError = -12052;
constexpr int32 k_BoundValueError = -12053;
constexpr int32 k_BoundValuesTooShortError = -12054;
constexpr int32 k_NameValuesTooShortError = -12055;
constexpr int32 k_MaterialValueError = -12056;
constexpr int32 k_MaterialValuesTooShortError = -12057;
constexpr int32 k_InvalidBoundCountError = -12058;
constexpr int32 k_MissingMaterialSectionError = -12059;
constexpr int32 k_FileChangedError = -12060;
constexpr int32 k_MaterialCountMismatchWarning = -12061;
constexpr int32 k_ExtraMaterialValuesWarning = -12062;

enum class Section : uint8
{
  None,
  XBounds,
  YBounds,
  ZBounds,
  Names,
  Materials
};

class CLocale
{
public:
  CLocale()
  {
#ifdef _WIN32
    m_Locale = _create_locale(LC_ALL, "C");
#else
    m_Locale = newlocale(LC_ALL_MASK, "C", nullptr);
#endif
  }

  ~CLocale()
  {
    if(m_Locale != nullptr)
    {
#ifdef _WIN32
      _free_locale(m_Locale);
#else
      freelocale(m_Locale);
#endif
    }
  }

  CLocale(const CLocale&) = delete;
  CLocale(CLocale&&) noexcept = delete;
  CLocale& operator=(const CLocale&) = delete;
  CLocale& operator=(CLocale&&) noexcept = delete;

#ifdef _WIN32
  using LocaleType = _locale_t;
#else
  using LocaleType = locale_t;
#endif

  LocaleType get() const
  {
    return m_Locale;
  }

private:
  LocaleType m_Locale = nullptr;
};

std::string_view NextToken(std::string_view line, usize& offset)
{
  const usize first = line.find_first_not_of(" \t\r\n", offset);
  if(first == std::string_view::npos)
  {
    offset = line.size();
    return {};
  }

  const usize last = line.find_first_of(" \t\r\n", first);
  if(last == std::string_view::npos)
  {
    offset = line.size();
    return line.substr(first);
  }

  offset = last;
  return line.substr(first, last - first);
}

Section GetSection(std::string_view token)
{
  if(token == "xcrd")
  {
    return Section::XBounds;
  }
  if(token == "ycrd")
  {
    return Section::YBounds;
  }
  if(token == "zcrd")
  {
    return Section::ZBounds;
  }
  if(token == "name")
  {
    return Section::Names;
  }
  if(token == "matr")
  {
    return Section::Materials;
  }
  return Section::None;
}

std::string_view SectionName(Section section)
{
  switch(section)
  {
  case Section::XBounds:
    return "xcrd";
  case Section::YBounds:
    return "ycrd";
  case Section::ZBounds:
    return "zcrd";
  case Section::Names:
    return "name";
  case Section::Materials:
    return "matr";
  case Section::None:
    return "unknown";
  }
  return "unknown";
}

Result<usize> ParseCount(std::string_view line, usize lineNumber, Section section, const std::filesystem::path& inputFile)
{
  usize offset = 0;
  static_cast<void>(NextToken(line, offset));
  const std::string_view countToken = NextToken(line, offset);
  if(countToken.empty())
  {
    return MakeErrorResult<usize>(k_HeaderCountError,
                                  fmt::format("The '{}' header at line {} in '{}' does not contain a count. Line text: '{}'", SectionName(section), lineNumber, inputFile.string(), line));
  }

  usize count = 0;
  const auto [end, error] = std::from_chars(countToken.data(), countToken.data() + countToken.size(), count);
  if(error != std::errc{} || end != countToken.data() + countToken.size())
  {
    return MakeErrorResult<usize>(k_HeaderCountError, fmt::format("The '{}' header count '{}' at line {} in '{}' is not an unsigned integer. Line text: '{}'", SectionName(section), countToken,
                                                                  lineNumber, inputFile.string(), line));
  }
  return {count};
}

Result<float32> ParseFloat(std::string_view token, usize lineNumber, Section section, const std::filesystem::path& inputFile)
{
#ifdef __cpp_lib_to_chars
  float32 value = 0.0F;
  const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value, std::chars_format::general);
  if(error == std::errc::result_out_of_range)
  {
    return MakeErrorResult<float32>(k_BoundValueError,
                                    fmt::format("The {} bound value '{}' at line {} in '{}' is out of range for float32.", SectionName(section).substr(0, 1), token, lineNumber, inputFile.string()));
  }
  if(error != std::errc{} || end != token.data() + token.size())
  {
    return MakeErrorResult<float32>(k_BoundValueError,
                                    fmt::format("The {} bound value '{}' at line {} in '{}' is not numeric.", SectionName(section).substr(0, 1), token, lineNumber, inputFile.string()));
  }
#else
  // Process-locale strtof parsing is unsafe because the Qt application can select a locale that uses decimal commas.
  static const CLocale cLocale;
  const std::string tokenText(token);
  char* end = nullptr;
  errno = 0;
#ifdef _WIN32
  const float32 value = _strtof_l(tokenText.c_str(), &end, cLocale.get());
#else
  const float32 value = strtof_l(tokenText.c_str(), &end, cLocale.get());
#endif
  if(errno == ERANGE)
  {
    return MakeErrorResult<float32>(k_BoundValueError,
                                    fmt::format("The {} bound value '{}' at line {} in '{}' is out of range for float32.", SectionName(section).substr(0, 1), token, lineNumber, inputFile.string()));
  }
  if(end != tokenText.c_str() + tokenText.size())
  {
    return MakeErrorResult<float32>(k_BoundValueError,
                                    fmt::format("The {} bound value '{}' at line {} in '{}' is not numeric.", SectionName(section).substr(0, 1), token, lineNumber, inputFile.string()));
  }
#endif
  return {value};
}

Result<int32> ParseMaterial(std::string_view token, usize lineNumber, const std::filesystem::path& inputFile)
{
  int32 value = 0;
  const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
  if(error != std::errc{} || end != token.data() + token.size())
  {
    return MakeErrorResult<int32>(k_MaterialValueError, fmt::format("The material value '{}' at line {} in '{}' is not a 32-bit integer.", token, lineNumber, inputFile.string()));
  }
  return {value};
}

template <class ParseToken>
Result<> ReadSectionTokens(std::ifstream& input, std::string& line, usize& lineNumber, const std::filesystem::path& inputFile, Section section, usize expectedCount, bool stopAtSectionHeader,
                           int32 tooShortError, ParseToken&& parseToken)
{
  usize parsedCount = 0;
  while(parsedCount < expectedCount && std::getline(input, line))
  {
    lineNumber++;
    usize offset = 0;
    const std::string_view firstToken = NextToken(line, offset);
    if(stopAtSectionHeader && GetSection(firstToken) != Section::None)
    {
      return MakeErrorResult(tooShortError, fmt::format("The '{}' section in '{}' declares {} values, but only {} values were found before the next section at line {}.", SectionName(section),
                                                        inputFile.string(), expectedCount, parsedCount, lineNumber));
    }

    std::string_view token = firstToken;
    while(!token.empty() && parsedCount < expectedCount)
    {
      Result<> tokenResult = parseToken(token, lineNumber, parsedCount);
      if(tokenResult.invalid())
      {
        return tokenResult;
      }
      parsedCount++;
      token = NextToken(line, offset);
    }
  }

  if(parsedCount != expectedCount)
  {
    return MakeErrorResult(tooShortError, fmt::format("The '{}' section in '{}' contains {} values, but its header declares {} values. The file ended at line {}.", SectionName(section),
                                                      inputFile.string(), parsedCount, expectedCount, lineNumber));
  }
  return {};
}

usize AxisIndex(Section section)
{
  return static_cast<usize>(section) - static_cast<usize>(Section::XBounds);
}
} // namespace

namespace nx::core
{
ReadOnScaleTableFile::ReadOnScaleTableFile(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                           ReadOnScaleTableFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

ReadOnScaleTableFile::~ReadOnScaleTableFile() noexcept = default;

Result<OnScaleTableFileHeader> ReadOnScaleTableFile::ReadHeader(const std::filesystem::path& inputFile)
{
  std::ifstream input(inputFile, std::ios::binary);
  if(!input.is_open())
  {
    return MakeErrorResult<OnScaleTableFileHeader>(k_FileAccessError, fmt::format("The OnScale table file '{}' could not be opened for reading. Check the file permissions.", inputFile.string()));
  }

  OnScaleTableFileHeader header;
  header.InputFile = inputFile;

  // The header scan validates bounds without retaining a second copy of the coordinate data.
  std::string line;
  usize lineNumber = 0;
  bool foundMaterials = false;
  while(std::getline(input, line))
  {
    lineNumber++;
    usize offset = 0;
    const Section section = GetSection(NextToken(line, offset));
    if(section == Section::None)
    {
      continue;
    }

    Result<usize> countResult = ParseCount(line, lineNumber, section, inputFile);
    if(countResult.invalid())
    {
      return ConvertInvalidResult<OnScaleTableFileHeader>(std::move(countResult));
    }
    const usize count = countResult.value();

    if(section >= Section::XBounds && section <= Section::ZBounds)
    {
      if(count < 2)
      {
        return MakeErrorResult<OnScaleTableFileHeader>(k_InvalidBoundCountError, fmt::format("The '{}' section at line {} in '{}' declares {} bounds. A present axis must contain at least 2 bounds.",
                                                                                             SectionName(section), lineNumber, inputFile.string(), count));
      }

      const usize axis = AxisIndex(section);
      header.BoundsCounts[axis] = count;
      header.BoundsPresent[axis] = true;
      auto valuesResult =
          ReadSectionTokens(input, line, lineNumber, inputFile, section, count, true, k_BoundValuesTooShortError, [&](std::string_view token, usize tokenLine, usize index) -> Result<> {
            auto valueResult = ParseFloat(token, tokenLine, section, inputFile);
            if(valueResult.invalid())
            {
              return ConvertResult(std::move(valueResult));
            }
            if(index == 0)
            {
              header.FirstBounds[axis] = valueResult.value();
            }
            if(index + 1 == count)
            {
              header.LastBounds[axis] = valueResult.value();
            }
            return {};
          });
      if(valuesResult.invalid())
      {
        return ConvertInvalidResult<OnScaleTableFileHeader>(std::move(valuesResult));
      }
    }
    else if(section == Section::Names)
    {
      header.NameCount = count;
      auto namesResult = ReadSectionTokens(input, line, lineNumber, inputFile, section, count, true, k_NameValuesTooShortError, [](std::string_view, usize, usize) -> Result<> { return {}; });
      if(namesResult.invalid())
      {
        return ConvertInvalidResult<OnScaleTableFileHeader>(std::move(namesResult));
      }
    }
    else if(section == Section::Materials)
    {
      header.MaterialCount = count;
      foundMaterials = true;
      break;
    }
  }

  if(!foundMaterials)
  {
    return MakeErrorResult<OnScaleTableFileHeader>(k_MissingMaterialSectionError, fmt::format("The OnScale table file '{}' does not contain a 'matr' section header.", inputFile.string()));
  }

  return {std::move(header)};
}

Result<> ReadOnScaleTableFile::operator()()
{
  std::ifstream input(m_InputValues->InputFile, std::ios::binary);
  if(!input.is_open())
  {
    return MakeErrorResult(k_FileAccessError, fmt::format("The OnScale table file '{}' could not be opened for reading. Check the file permissions.", m_InputValues->InputFile.string()));
  }

  auto& geometry = m_DataStructure.getDataRefAs<RectGridGeom>(m_InputValues->RectGridGeometryPath);
  geometry.setUnits(IGeometry::LengthUnit::Meter);
  std::array<Float32Array*, 3> boundsArrays = {
      &m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->RectGridGeometryPath.createChildPath("X Bounds")),
      &m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->RectGridGeometryPath.createChildPath("Y Bounds")),
      &m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->RectGridGeometryPath.createChildPath("Z Bounds")),
  };
  auto& names = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->MaterialNamesArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  Result<> result;

  for(usize axis = 0; axis < boundsArrays.size(); axis++)
  {
    if(!m_InputValues->Header.BoundsPresent[axis])
    {
      (*boundsArrays[axis])[0] = m_InputValues->FallbackOrigin[axis];
      (*boundsArrays[axis])[1] = m_InputValues->FallbackOrigin[axis] + m_InputValues->FallbackSpacing[axis];
    }
  }

  MessageHelper messageHelper(m_MessageHandler);

  // The parser writes directly into the outputs and retains only one input line.
  std::string line;
  usize lineNumber = 0;
  bool foundMaterials = false;
  std::array<bool, 3> boundsRead = {false, false, false};
  bool namesRead = false;
  while(std::getline(input, line))
  {
    lineNumber++;
    usize offset = 0;
    const Section section = GetSection(NextToken(line, offset));
    if(section == Section::None)
    {
      continue;
    }

    Result<usize> countResult = ParseCount(line, lineNumber, section, m_InputValues->InputFile);
    if(countResult.invalid())
    {
      return ConvertResult(std::move(countResult));
    }
    const usize count = countResult.value();

    if(section >= Section::XBounds && section <= Section::ZBounds)
    {
      const usize axis = AxisIndex(section);
      if(count != boundsArrays[axis]->getNumberOfTuples())
      {
        return MakeErrorResult(k_FileChangedError, fmt::format("The '{}' section in '{}' declares {} bounds, but preflight found {} bounds. The file changed after preflight.", SectionName(section),
                                                               m_InputValues->InputFile.string(), count, boundsArrays[axis]->getNumberOfTuples()));
      }

      messageHelper.sendMessage(fmt::format("Reading {} bounds", SectionName(section).substr(0, 1)));
      auto valuesResult =
          ReadSectionTokens(input, line, lineNumber, m_InputValues->InputFile, section, count, true, k_BoundValuesTooShortError, [&](std::string_view token, usize tokenLine, usize index) -> Result<> {
            auto valueResult = ParseFloat(token, tokenLine, section, m_InputValues->InputFile);
            if(valueResult.invalid())
            {
              return ConvertResult(std::move(valueResult));
            }
            (*boundsArrays[axis])[index] = valueResult.value();
            return {};
          });
      if(valuesResult.invalid())
      {
        return valuesResult;
      }
      boundsRead[axis] = true;
    }
    else if(section == Section::Names)
    {
      if(count != names.getNumberOfTuples())
      {
        return MakeErrorResult(k_FileChangedError, fmt::format("The 'name' section in '{}' declares {} names, but preflight found {} names. The file changed after preflight.",
                                                               m_InputValues->InputFile.string(), count, names.getNumberOfTuples()));
      }

      messageHelper.sendMessage("Reading names");
      auto namesResult =
          ReadSectionTokens(input, line, lineNumber, m_InputValues->InputFile, section, count, true, k_NameValuesTooShortError, [&](std::string_view token, usize, usize index) -> Result<> {
            names.setValue(index, std::string(token));
            return {};
          });
      if(namesResult.invalid())
      {
        return namesResult;
      }
      namesRead = true;
    }
    else if(section == Section::Materials)
    {
      foundMaterials = true;
      const usize numCells = featureIds.getNumberOfTuples();
      if(count != numCells)
      {
        const std::string message = fmt::format("The 'matr' header in '{}' declares {} values, but the created geometry has {} cells. The reader will use exactly {} values.",
                                                m_InputValues->InputFile.string(), count, numCells, numCells);
        result.warnings().push_back({k_MaterialCountMismatchWarning, message});
      }

      messageHelper.sendMessage("Reading material values 0%");
      auto progressHelper = messageHelper.createProgressMessageHelper();
      progressHelper.setMaxProgresss(numCells);
      progressHelper.setProgressMessageTemplate("Reading material values {:.0f}%");
      auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(100));
      const usize cancelIncrement = std::max<usize>(1, numCells / 100);
      usize materialCount = 0;
      usize extraCount = 0;
      while(std::getline(input, line))
      {
        lineNumber++;
        usize materialOffset = 0;
        std::string_view token = NextToken(line, materialOffset);
        while(!token.empty())
        {
          if(materialCount < numCells)
          {
            if(materialCount % cancelIncrement == 0)
            {
              if(m_ShouldCancel)
              {
                return {};
              }
            }

            auto valueResult = ParseMaterial(token, lineNumber, m_InputValues->InputFile);
            if(valueResult.invalid())
            {
              return ConvertResult(std::move(valueResult));
            }
            featureIds[materialCount] = valueResult.value();
            materialCount++;
            progressMessenger.sendProgressMessage(1);
          }
          else
          {
            extraCount++;
          }
          token = NextToken(line, materialOffset);
        }
      }

      if(materialCount != numCells)
      {
        return MakeErrorResult(k_MaterialValuesTooShortError,
                               fmt::format("The 'matr' section in '{}' contains {} values, but the created geometry requires {} values.", m_InputValues->InputFile.string(), materialCount, numCells));
      }
      if(extraCount > 0)
      {
        const std::string message = fmt::format("The 'matr' section in '{}' contains {} trailing values after the required {} values. The reader ignored the trailing values.",
                                                m_InputValues->InputFile.string(), extraCount, numCells);
        result.warnings().push_back({k_ExtraMaterialValuesWarning, message});
      }
      break;
    }
  }

  for(usize axis = 0; axis < boundsRead.size(); axis++)
  {
    if(boundsRead[axis] != m_InputValues->Header.BoundsPresent[axis])
    {
      constexpr std::array<std::string_view, 3> k_AxisNames = {"X", "Y", "Z"};
      return MakeErrorResult(k_FileChangedError,
                             fmt::format("The {} bounds section in '{}' was expected to be {} after preflight but was found {}. The file changed after preflight.", k_AxisNames[axis],
                                         m_InputValues->InputFile.string(), m_InputValues->Header.BoundsPresent[axis] ? "present" : "absent", boundsRead[axis] ? "present" : "absent"));
    }
  }
  if(m_InputValues->Header.NameCount > 0 && !namesRead)
  {
    return MakeErrorResult(k_FileChangedError, fmt::format("The 'name' section in '{}' was expected because preflight found {} names, but the section was not found. The file changed after preflight.",
                                                           m_InputValues->InputFile.string(), m_InputValues->Header.NameCount));
  }
  if(!foundMaterials)
  {
    return MakeErrorResult(k_MissingMaterialSectionError, fmt::format("The OnScale table file '{}' does not contain a 'matr' section header.", m_InputValues->InputFile.string()));
  }
  return result;
}
} // namespace nx::core
