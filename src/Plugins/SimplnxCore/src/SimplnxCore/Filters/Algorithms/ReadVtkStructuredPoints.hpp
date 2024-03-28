#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadVtkStructuredPointsInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  bool ReadPointData;
  bool ReadCellData;
  DataPath PointGeomPath;
  DataPath ImageGeomPath;
  std::string PointAttributeMatrixName;
  std::string CellAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT ReadVtkStructuredPoints
{
public:
  ReadVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadVtkStructuredPointsInputValues* inputValues);
  ~ReadVtkStructuredPoints() noexcept;

  ReadVtkStructuredPoints(const ReadVtkStructuredPoints&) = delete;
  ReadVtkStructuredPoints(ReadVtkStructuredPoints&&) noexcept = delete;
  ReadVtkStructuredPoints& operator=(const ReadVtkStructuredPoints&) = delete;
  ReadVtkStructuredPoints& operator=(ReadVtkStructuredPoints&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

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
  static nx::core::Result<OutputActions> PreflightFile(ReadVtkStructuredPointsInputValues& inputValues);

  enum class CurrentSectionType : uint32
  {
    NotSet = 0,
    Point = 1,
    Cell = 2,
  };

protected:
  nx::core::Result<OutputActions> getOutputActions() const
  {
    return m_OutputActions;
  }

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

  int32_t preflightSkipVolume(nx::core::DataType nxDType, std::istream& in, bool binary, size_t numElements);

private:
  DataStructure& m_DataStructure;
  const ReadVtkStructuredPointsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  std::string m_Comment = {""};
  std::string m_DatasetType = {""};

  bool m_FileIsBinary = {true};

  bool m_Preflight = false;
  int m_ErrorCode = 0;
  std::string m_ErrorMessage;

  CurrentSectionType m_CurrentSectionType = CurrentSectionType::NotSet;
  CreateImageGeometryAction::DimensionType m_CurrentGeomDims;

  nx::core::Result<OutputActions> m_OutputActions;
};

} // namespace nx::core
