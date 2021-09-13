#pragma once

#include "complex/complex_export.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

namespace complex
{

class COMPLEX_EXPORT StackFileListInfo
{
public:
  StackFileListInfo();
  ~StackFileListInfo();
  StackFileListInfo(const StackFileListInfo&) = default;            // Copy Constructor Default
  StackFileListInfo(StackFileListInfo&&) = default;                 // Move Constructor Default
  StackFileListInfo& operator=(const StackFileListInfo&) = default; // Copy Assignment Default
  StackFileListInfo& operator=(StackFileListInfo&&) = default;      // Move Assignment Default

  /**
   * @brief StackFileListInfo
   * @param paddingDigits
   * @param ordering
   * @param startIndex
   * @param endIndex
   * @param incrementIndex
   * @param inputPath
   * @param filePrefix
   * @param fileSuffix
   * @param fileExtension
   */
  StackFileListInfo(int32_t paddingDigits, uint32_t ordering, int32_t startIndex, int32_t endIndex, int32_t incrementIndex, const std::string& inputPath, const std::string& filePrefix,
                    const std::string& fileSuffix, const std::string& fileExtension);

  /**
   * @brief Sets the starting index to generate the file list
   * @param value
   */
  void setStartIndex(const int32_t& value);
  [[nodiscard]] int32_t getStartIndex() const;

  /**
   * @brief sets the ending index to generate the file list
   * @param value
   */
  void setEndIndex(const int32_t& value);
  [[nodiscard]] int32_t getEndIndex() const;

  /**
   * @brief Sets the increment to use when generating the file list
   * @param value
   */
  void setIncrementIndex(const int32_t& value);
  [[nodiscard]] int32_t getIncrementIndex() const;

  /**
   * @brief Sets the total number of digits to use when generating the index part of the filename.
   * @param value
   */
  void setPaddingDigits(const int32_t& value);
  [[nodiscard]] int32_t getPaddingDigits() const;

  /**
   * @brief Sets the order to use when generating the file list: Low to high or High to Low
   * @param value
   */
  void setOrdering(const uint32_t& value);
  [[nodiscard]] uint32_t getOrdering() const;

  /**
   * @brief Sets the input directory path
   * @param value
   */
  void setInputPath(const std::string& value);
  [[nodiscard]] std::string getInputPath() const;

  /**
   * @brief Sets the file prefix to use
   * @param value
   */
  void setFilePrefix(const std::string& value);
  [[nodiscard]] std::string getFilePrefix() const;

  /**
   * @brief Sets the file suffix to use
   * @param value
   */
  void setFileSuffix(const std::string& value);
  [[nodiscard]] std::string getFileSuffix() const;

  /**
   * @brief Sets the file extension to use
   * @param value
   */
  void setFileExtension(const std::string& value);
  [[nodiscard]] std::string getFileExtension() const;

  /**
   * @brief Encodes the class to a json object
   * @param json
   */
  nlohmann::json toJson() const;

  /**
   * @brief Decodes the class from a json object
   * @param json
   * @return
   */
  bool fromJson(const nlohmann::json& json);

private:
  int32_t m_StartIndex = 0;
  int32_t m_EndIndex = 0;
  int32_t m_IncrementIndex = 1;
  int32_t m_PaddingDigits = 3;
  uint32_t m_Ordering = 0;
  std::string m_InputPath;
  std::string m_FilePrefix;
  std::string m_FileSuffix;
  std::string m_FileExtension;
};

} // namespace complex
