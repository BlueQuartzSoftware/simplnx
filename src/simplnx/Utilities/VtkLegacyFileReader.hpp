#pragma once

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Output.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace nx::core
{

class VtkLegacyFileReader
{
public:
  explicit VtkLegacyFileReader(const std::string& filePath);
  ~VtkLegacyFileReader();

  VtkLegacyFileReader(const VtkLegacyFileReader&) = delete;            // Copy Constructor Not Implemented
  VtkLegacyFileReader(VtkLegacyFileReader&&) = delete;                 // Move Constructor Not Implemented
  VtkLegacyFileReader& operator=(const VtkLegacyFileReader&) = delete; // Copy Assignment Not Implemented
  VtkLegacyFileReader& operator=(VtkLegacyFileReader&&) = delete;      // Move Assignment Not Implemented

  void setPreflight(bool value);

  /**
   *
   * @param readPointData
   * @param readCellData
   * @param pointGeometryPath
   * @param imageGeometryPath
   * @param vertAMName
   * @param cellAMName
   * @return
   */
  nx::core::Result<OutputActions> preflightFile(bool readPointData, bool readCellData, DataPath pointGeometryPath, DataPath imageGeometryPath, const std::string& vertAMName,
                                                const std::string& cellAMName);

  /**
   *
   * @param dataStructure
   * @param readPointData
   * @param readCellData
   * @param pointGeometryPath
   * @param imageGeometryPath
   * @param vertAMName
   * @param cellAMName
   * @return
   */
  int32_t readFile(DataStructure& dataStructure, bool readPointData, bool readCellData, const DataPath& pointGeometryPath, const DataPath& imageGeometryPath, const std::string& vertAMName,
                   const std::string& cellAMName);

  enum class CurrentSectionType : uint32
  {
    NotSet = 0,
    Point = 1,
    Cell = 2,
  };

protected:
  /**
   * @brief readFile Handles the main reading of the .vtk file
   * @return Integer error value
   */
  int32_t readFile();

  /**
   * @brief readData Reads a section of data from the .vtk file
   * @param instream Incoming file stream
   */
  virtual void readData(std::istream& instream);

  /**
   * @brief parseCoordinateLine Parses a line representing coordinates
   * @param input Incoming line to parse
   * @param value Coordinate value
   * @return Integer error value
   */
  int32_t parseCoordinateLine(const char* input, size_t& value);

  /**
   * @brief parseByteSize Parses the byte size from a data set declaration line
   * @param text Line to parse
   * @return Integer error value
   */
  size_t parseByteSize(const std::string& text);

  /**
   * @brief readLine Reads a line from the .vtk file
   * @param in Incoming file stream
   * @param result Char pointer to store line
   * @param length Length of line
   * @return Integer error value
   */
  int32_t readLine(std::istream& in, char* result, size_t length);

  /**
   * @brief readString Reas a string from the .vtk file
   * @param in Incoming file stream
   * @param result Char pointer to store string
   * @param length Length of string
   * @return Integer error value
   */
  int32_t readString(std::istream& in, char* result, size_t length);

  /**
   * @brief lowerCase Converts a string to lower case
   * @param str Incoming string to convert
   * @param len Length of string
   * @return Integer error value
   */
  char* lowerCase(char* str, size_t len);

  /**
   * @brief readDataTypeSection Determines the type of data to be read from the .vtk file
   * @param in Incoming file stream
   * @param numPts Number of points to read
   * @param nextKeyWord Keyword for data type
   * @return Integer error value
   */
  int32_t readDataTypeSection(std::istream& in, int numPts, const std::string& nextKeyWord);

  /**
   * @brief readScalarData Reads scalar data attribute types
   * @param in Incoming file stream
   * @return Integer error value
   */
  int32_t readScalarData(std::istream& in, int numPts);

  /**
   * @brief readVectorData Reads vector data attribute types
   * @param in Incoming file stream
   * @param numPts Number of points
   * @return Integer error value
   */
  int32_t readVectorData(std::istream& in, int numPts);

  /**
   * @brief DecodeString Decodes a binary string from the .vtk file
   * @param resname Resulting decoded string
   * @param name Binary string to decode
   * @return Integer error value
   */
  int32_t DecodeString(char* resname, const char* name);

  void setErrorCondition(int error, const std::string& message);

  void setComment(const std::string& comment);

  void setFileIsBinary(bool value);

  int32 getErrorCode() const;

  void setDatasetType(const std::string& dataSetType);

  int32_t preflightSkipVolume(nx::core::DataType nxDType, std::istream& in, bool binary, size_t totalSize);

private:
  bool m_ReadPointData = {true};
  DataPath m_PointGeomPath;
  std::string m_PointAMName;

  bool m_ReadCellData = {true};
  DataPath m_ImageGeomPath;
  std::string m_CellAMName;

  std::string m_InputFile = {""};
  std::string m_Comment = {""};
  std::string m_DatasetType = {""};
  bool m_FileIsBinary = {true};

  bool m_Preflight = true;
  int m_ErrorCode = 0;
  std::string m_ErrorMessage;

  CurrentSectionType m_CurrentSectionType = CurrentSectionType::NotSet;
  CreateImageGeometryAction::DimensionType m_CurrentGeomDims;

  nx::core::Result<OutputActions> m_OutputActions;

  DataStructure* m_DataStructurePtr;
};

} // namespace nx::core
