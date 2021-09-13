
#include "StackFileListInfo.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
StackFileListInfo::StackFileListInfo() = default;

// -----------------------------------------------------------------------------
StackFileListInfo::StackFileListInfo(int32_t paddingDigits, uint32_t ordering, int32_t startIndex, int32_t endIndex, int32_t incrementIndex, const std::string& inputPath,
                                     const std::string& filePrefix, const std::string& fileSuffix, const std::string& fileExtension)
: m_PaddingDigits(paddingDigits)
, m_Ordering(ordering)
, m_IncrementIndex(incrementIndex)
, m_InputPath(inputPath)
, m_FilePrefix(filePrefix)
, m_FileSuffix(fileSuffix)
, m_FileExtension(fileExtension)
, m_StartIndex(startIndex)
, m_EndIndex(endIndex)
{
}

// -----------------------------------------------------------------------------
StackFileListInfo::~StackFileListInfo() = default;

// -----------------------------------------------------------------------------
nlohmann::json StackFileListInfo::toJson() const
{
  nlohmann::json json;
  json["StartIndex"] = static_cast<int32_t>(m_StartIndex);
  json["EndIndex"] = static_cast<int32_t>(m_EndIndex);
  json["PaddingDigits"] = static_cast<int32_t>(m_PaddingDigits);
  json["Ordering"] = static_cast<int32_t>(m_Ordering);
  json["IncrementIndex"] = static_cast<int32_t>(m_IncrementIndex);
  json["InputPath"] = m_InputPath;
  json["FilePrefix"] = m_FilePrefix;
  json["FileSuffix"] = m_FileSuffix;
  json["FileExtension"] = m_FileExtension;
  return json;
}

// -----------------------------------------------------------------------------
bool StackFileListInfo::fromJson(const nlohmann::json& json)
{
  if(json["StartIndex"].is_number_float()
     && json["EndIndex"].is_number_float()
     && json["PaddingDigits"].is_number_float()
     && json["Ordering"].is_number_float()
     && json["IncrementIndex"].is_number_float()
     && json["InputPath"].is_string()
     && json["FilePrefix"].is_string()
     && json["FileSuffix"].is_string()
     && json["FileExtension"].is_string())
  {
    m_PaddingDigits = static_cast<int32_t>(json["PaddingDigits"].get<int32_t>());
    m_Ordering = static_cast<uint32_t>(json["Ordering"].get<int32_t>());
    m_IncrementIndex = static_cast<int32_t>(json["IncrementIndex"].get<int32_t>());
    m_InputPath = json["InputPath"].get<std::string>();
    m_FilePrefix = json["FilePrefix"].get<std::string>();
    m_FileSuffix = json["FileSuffix"].get<std::string>();
    m_FileExtension = json["FileExtension"].get<std::string>();
     m_StartIndex = static_cast<int32_t>(json["StartIndex"].get<int32_t>());
    m_EndIndex = static_cast<int32_t>(json["EndIndex"].get<int32_t>());
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
void StackFileListInfo::setStartIndex(const int32_t& value)
{
  m_StartIndex = value;
}
int32_t StackFileListInfo::getStartIndex() const
{
  return m_StartIndex;
}

// -----------------------------------------------------------------------------
void StackFileListInfo::setEndIndex(const int32_t& value)
{
  m_EndIndex = value;
}
int32_t StackFileListInfo::getEndIndex() const
{
  return m_EndIndex;
}

// -----------------------------------------------------------------------------
void StackFileListInfo::setIncrementIndex(const int32_t& value)
{
  m_IncrementIndex = value;
}
int32_t StackFileListInfo::getIncrementIndex() const
{
  return m_IncrementIndex;
}

// -----------------------------------------------------------------------------
void StackFileListInfo::setPaddingDigits(const int32_t& value)
{
  m_PaddingDigits = value;
}
int32_t StackFileListInfo::getPaddingDigits() const
{
  return m_PaddingDigits;
}

// -----------------------------------------------------------------------------
void StackFileListInfo::setOrdering(const uint32_t& value)
{
  m_Ordering = value;
}
uint32_t StackFileListInfo::getOrdering() const
{
  return m_Ordering;
}

// -----------------------------------------------------------------------------
void StackFileListInfo::setInputPath(const std::string& value)
{
  m_InputPath = value;
}
std::string StackFileListInfo::getInputPath() const
{
  return m_InputPath;
}

// -----------------------------------------------------------------------------
void StackFileListInfo::setFilePrefix(const std::string& value)
{
  m_FilePrefix = value;
}
std::string StackFileListInfo::getFilePrefix() const
{
  return m_FilePrefix;
}

// -----------------------------------------------------------------------------
void StackFileListInfo::setFileSuffix(const std::string& value)
{
  m_FileSuffix = value;
}
std::string StackFileListInfo::getFileSuffix() const
{
  return m_FileSuffix;
}

// -----------------------------------------------------------------------------
void StackFileListInfo::setFileExtension(const std::string& value)
{
  m_FileExtension = value;
}
std::string StackFileListInfo::getFileExtension() const
{
  return m_FileExtension;
}
