#include "ImportDeformKeyFileV12.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/Geometry/IGeometry.hpp"
#include "complex/DataStructure/Geometry/INodeGeometry0D.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <fstream>

using namespace complex;

namespace
{
const std::string k_CellTitle = "ELMCON";
const std::string k_VertexTitle = "RZ";
const std::string k_Star = "*";
const std::vector<std::string> k_FileTitle = {"DEFORM-2D", "KEYWORD", "FILE", "(Qt)"};
const std::vector<std::string> k_ProcessDefinition = {"Process", "Definition"};
const std::vector<std::string> k_StoppingAndStepControls = {"Stopping", "&", "Step", "Controls"};
const std::vector<std::string> k_IterationControls = {"Iteration", "Controls"};
const std::vector<std::string> k_ProcessingConditions = {"Processing", "Conditions"};
const std::vector<std::string> k_UserDefinedVariables = {"User", "Defined", "Variables"};
const std::vector<std::string> k_PropertyDataOfMaterial = {"Property", "Data", "of", "Material"};
const std::vector<std::string> k_InterMaterialData = {"Inter-Material", "Data"};
const std::vector<std::string> k_InterObjectData = {"Inter-Object", "Data"};
const std::vector<std::string> k_DataForObject = {"Data", "for", "Object", "#"};

[[nodiscard]] Result<> setUSRNODTuple(usize tuple, const std::vector<std::string>& tokens, usize lineCount)
{
  for(usize i = 0; i < m_UserDefinedVariables.size(); i++)
  {
    float value = 0.0f;
    try
    {
      value = std::stof(tokens[i]);
    } catch(const std::exception& e)
    {
      std::string msg = fmt::format("Error at line {}: Unable to convert data array {}'s string value \"{}\" to float.  Threw standard exception with text: \"{}\"", lineCount, m_UserDefinedVariables[i], tokens[i + 1], e.what());
      return MakeErrorResult(-2009, msg);
    }
    m_UserDefinedArrays[i]->setTuple(tuple, &value);
  }
  return {};
}

[[nodiscard]] Result<> setTuple(usize tuple, Float32Array& data, const std::vector<std::string>& tokens, usize lineCount)
{
  usize numComp = data.getNumberOfComponents();
  for(usize comp = 0; comp < numComp; comp++)
  {
    float value = 0.0f;
    try
    {
      value = std::stof(tokens[comp]);
    } catch(const std::exception& e)
    {
      std::string msg = fmt::format("Error at line {}: Unable to convert data array {}'s string value \"{}\" to float.  Threw standard exception with text: \"{}\"", lineCount, data.getName(),
                                    tokens[comp + 1], e.what());
      return MakeErrorResult(-2008, msg);
    }
    data[tuple * numComp + comp] = value;
  }
  return {};
}

inline Result<> parse_ull(const std::string& token, usize lineCount, usize& value)
{
  try
  {
    value = std::stoull(token);
  } catch(const std::exception& e)
  {
    std::string msg =
        fmt::format("Error at line {}: Unable to convert string value \"{}\" to unsigned long long.  Threw standard exception with text: \"{}\"", StringUtilities::number(lineCount), token, e.what());
    return MakeErrorResult(-2000, msg);
  }

  return {};
}

std::vector<std::string> getNextLineTokens(std::ifstream& inStream, usize& lineCount)
{
  std::vector<std::string> tokens; /* vector to store the split data */
  std::string buf;
  std::getline(inStream, buf);
  buf = StringUtilities::trimmed(buf);
  buf = StringUtilities::simplified(buf);
  tokens = StringUtilities::split(buf, ' ');
  lineCount++;

  return tokens;
}

void findNextSection(std::ifstream& inStream, usize& lineCount)
{
  std::vector<std::string> tokens;
  while(inStream.peek() != EOF)
  {
    tokens = getNextLineTokens(inStream, lineCount);
    if(!tokens.empty() && tokens.at(0) == k_Star)
    {
      return;
    }
  }
}

void readProcessDefinition(std::ifstream& inStream, usize& lineCount)
{
  // We currently don't care about the "Process Definition" section...
  findNextSection(inStream, lineCount);
}

void readStoppingAndStepControls(std::ifstream& inStream, usize& lineCount)
{
  // We currently don't care about the "Stopping & Step Controls" section...
  findNextSection(inStream, lineCount);
}

void readIterationControls(std::ifstream& inStream, usize& lineCount)
{
  // We currently don't care about the "Iteration Controls" section...
  findNextSection(inStream, lineCount);
}

void readProcessingConditions(std::ifstream& inStream, usize& lineCount)
{
  // We currently don't care about the "Processing Conditions" section...
  findNextSection(inStream, lineCount);
}

void readPropertyDataOfMaterial(std::ifstream& inStream, usize& lineCount)
{
  // We currently don't care about the "Property Data of Material" section...
  findNextSection(inStream, lineCount);
}

void readInterMaterialData(std::ifstream& inStream, usize& lineCount)
{
  // We currently don't care about the "Inter-Material Data" section...
  findNextSection(inStream, lineCount);
}

void readInterObjectData(std::ifstream& inStream, usize& lineCount)
{
  // We currently don't care about the "Inter-Object Data" section...
  findNextSection(inStream, lineCount);
}

[[nodiscard]] Result<> readUserDefinedVariables(std::ifstream& inStream, usize& lineCount)
{
  std::string buf;
  std::getline(inStream, buf);
  buf = StringUtilities::trimmed(buf);
  buf = StringUtilities::simplified(buf);
  auto tokens = StringUtilities::split(buf, ' ');
  lineCount++;

  usize numVars;
  Result<> result = parse_ull(tokens.at(2), lineCount, numVars);
  if(result.invalid())
  {
    return result;
  }

  m_UserDefinedVariables.resize(12);
  for(usize i = 0; i < numVars; i++)
  {
    std::getline(inStream, buf);
    buf = StringUtilities::trimmed(buf);
    lineCount++;
    std::string cleanedString = StringUtilities::replace(buf, "/", "|");
    m_UserDefinedVariables[i] = cleanedString;
  }
}

[[nodiscard]] Result<> readDataArray(std::ifstream& inStream, usize& lineCount, AttributeMatrix& attrMat, const std::string& dataArrayName, usize arrayTupleSize, bool allocate, const std::atomic_bool& shouldCancel)
{
  if(arrayTupleSize == 0)
  {
    return {};
  }
  usize componentCount = 0;
  auto data = dataStructure.getDataRefAs<Float32Array>();
  int32_t tupleLineCount = 1;
  // We are here because the calling function *should* have determined correctly (hopefully) that we are now reading a
  // section of data. We are going to make the assumption this is correct and go from here. What is *not* figured out
  // at this point is how many components are in each tuple. That is what this next scoped section is going to figure out.
  // We are going to read the very fist line, strip off the index value that appears (at least it seems that way from the
  // file) in the first 8 chars of the line. (The file is written by FORTRAN so I'm hoping that they used fixed width
  // fields when writing the data otherwise this whole section is based on a very bad assumption). We then tokenize the
  // values and read the next line. IFF there are NON SPACE characters in the first 8 bytes of this second line that was
  // read then we now know that all of the components are on a single line. We count the tokens and use that as the number
  // of components, otherwise we add these tokens to the last set of tokens and read another line. This continues until
  // we find a line that DOES have a NON SPACE character in the first 8 bytes of the line.
  {
    std::vector<std::string> tokens;
    std::string line;

    std::getline(inStream, line);
    lineCount++;
    // First Scan the first 8 characters to see if there are any non-space characters
    for(usize i = 0; i < 8; i++)
    {
      if(line[i] != ' ')
      {
        line[i] = ' ';
      }
    }
    // We have replaced all the first 8 chars with spaces, so now tokenize and save the tokens
    {
      line = StringUtilities::trimmed(line);
      line = StringUtilities::simplified(line);
      std::vector<std::string> tempTokens = StringUtilities::split(line, ' ');
      tokens.insert(tokens.end(), tempTokens.begin(), tempTokens.end());
    }

    // Now look to subsequent lines to see if the data continues
    bool keepGoing = true;
    while(keepGoing)
    {
      auto currentFilePos = inStream.tellg();
      // Read the next line
      std::getline(inStream, line);
      lineCount++;
      // Figure out if there is anything in the first 8 chars
      for(usize i = 0; i < 8; i++)
      {
        if(line[i] != ' ')
        {
          inStream.seekg(currentFilePos); // Roll back the inStream to just before we read this line.
          keepGoing = false;
          break;
        }
      }
      // If there is NOTHING in the first 8 chars, then tokenize
      if(keepGoing)
      {
        tupleLineCount++;
        // We have replaced all the first 8 chars with spaces, so now tokenize and save the tokens
        line = StringUtilities::trimmed(line);
        line = StringUtilities::simplified(line);
        std::vector<std::string> tempTokens = StringUtilities::split(line, ' ');
        tokens.insert(tokens.end(), tempTokens.begin(), tempTokens.end());
      }
    }
    componentCount = tokens.size();

    // Create the data array using the first line of data to determine the component count
    if(dataArrayName == "USRNOD")
    {
      m_UserDefinedArrays.clear();
      for(const auto& userDefinedVariable : m_UserDefinedVariables)
      {
        data = Float32Array::CreateArray(arrayTupleSize, {1}, userDefinedVariable, allocate);
        attrMat->insertOrAssign(data);
        m_UserDefinedArrays.push_back(data);
      }
    }
    else
    {
      data = Float32Array::CreateArray(arrayTupleSize, {componentCount}, dataArrayName, allocate);
      attrMat->insertOrAssign(data);
    }

    if(allocate)
    {
      Result<> setResult;
      if(dataArrayName == "USRNOD")
      {
        setResult = setUSRNODTuple(0, tokens, lineCount);
      }
      else
      {
        setResult = setTuple(0, data, tokens, lineCount);
      }
      if(setResult.invalid())
      {
        return setResult;
      }
    }
  }

  // Just read and skip since this is probably a preflight, or we are just reading through the file to find the interesting data
  if(!allocate)
  {
    std::string line;
    usize totalLinesToRead = (arrayTupleSize - 1) * tupleLineCount;
    for(usize i = 0; i < totalLinesToRead; i++)
    {
      std::getline(inStream, line);
      lineCount++;
    }
  }
  else
  {
    // Read each tuple's data starting at the second tuple
    for(usize tupleIndex = 1; tupleIndex < arrayTupleSize; tupleIndex++)
    {
      if(shouldCancel)
      {
        return {};
      }
      std::vector<std::string> tokens;

      // Read the data in groups of each tuple to build up the tokens
      for(int32_t compLine = 0; compLine < tupleLineCount; compLine++)
      {
        // Now read the line
        std::string compLineData;
        std::getline(inStream, compLineData);
        lineCount++;
        usize offset = (compLine == 0 ? 1 : 0);
        compLineData = StringUtilities::trimmed(compLineData);
        compLineData = StringUtilities::simplified(compLineData);
        auto subTokens = StringUtilities::split(compLineData, ' ');
        tokens.insert(tokens.end(), subTokens.begin() + offset, subTokens.end());
      }

      Result<> setResult;
      if(dataArrayName == "USRNOD")
      {
       setResult = setUSRNODTuple(tupleIndex, tokens, lineCount);
      }
      else
      {
        setResult = setTuple(tupleIndex, data, tokens, lineCount);
      }
      if(setResult.invalid())
      {
        return setResult;
      }
    }
  }
  return {};
}

Result<> readVertexCoordinates(ImportDeformKeyFileV12* filter, std::ifstream& inStream, usize& lineCount, IGeometry::SharedVertexList& vertex, usize numVerts)
{
  std::string buf;
  std::vector<std::string> tokens; /* vector to store the split data */

  // Read or Skip past all the vertex data
  for(usize i = 0; i < numVerts; i++)
  {
    if(filter->getCancel())
    {
      return {};
    }

    tokens = getNextLineTokens(inStream, lineCount);

    try
    {
      vertex[3 * i] = std::stof(tokens[1]);
    } catch(const std::exception& e)
    {
      std::string msg = fmt::format("Error at line {}: Unable to convert vertex coordinate {}'s 1st string value \"{}\" to float.  Threw standard exception with text: \"{}\"",
                                    StringUtilities::number(lineCount), StringUtilities::number(i + 1), tokens[1], e.what());
      return MakeErrorResult(-2001, std::move(msg));
    }

    try
    {
      vertex[3 * i + 1] = std::stof(tokens[2]);
    } catch(const std::exception& e)
    {
      std::string msg = fmt::format("Error at line {}: Unable to convert vertex coordinate {}'s 2nd string value \"{}\" to float.  Threw standard exception with text: \"{}\"",
                                    StringUtilities::number(lineCount), StringUtilities::number(i + 1), tokens[2], e.what());
      return MakeErrorResult(-2002, std::move(msg));
    }

    vertex[3 * i + 2] = 0.0f;
  }
}

Result<> readQuadGeometry(std::ifstream& inStream, usize& lineCount, IGeometry::MeshIndexArrayType& quads, usize numCells, const std::atomic_bool& shouldCancel)
{
  std::string buf;
  std::vector<std::string> tokens; /* vector to store the split data */

  for(usize i = 0; i < numCells; i++)
  {
    if(shouldCancel)
    {
      return {};
    }

    tokens = getNextLineTokens(inStream, lineCount);

    // Subtract one from the node number because DEFORM starts at node 1 and we start at node 0
    try
    {
      quads[4 * i] = std::stoi(tokens[1]) - 1;
    } catch(const std::exception& e)
    {
      std::string msg =
          fmt::format("Error at line {}: Unable to convert quad {}'s 1st string value \"{}\" to integer.  Threw standard exception with text: \"{}\"", lineCount, (i + 1), tokens[1], e.what());
      return MakeErrorResult(-2004, msg);
    }

    try
    {
      quads[4 * i + 1] = std::stoi(tokens[2]) - 1;
    } catch(const std::exception& e)
    {
      std::string msg =
          fmt::format("Error at line {}: Unable to convert quad {}'s 2nd string value \"{}\" to integer.  Threw standard exception with text: \"{}\"", lineCount, (i + 1), tokens[2], e.what());
      return MakeErrorResult(-2005, msg);
    }

    try
    {
      quads[4 * i + 2] = std::stoi(tokens[3]) - 1;
    } catch(const std::exception& e)
    {
      std::string msg =
          fmt::format("Error at line {}: Unable to convert quad {}'s 3rd string value \"{}\" to integer.  Threw standard exception with text: \"{}\"", lineCount, (i + 1), tokens[3], e.what());
      return MakeErrorResult(-2006, msg);
    }

    try
    {
      quads[4 * i + 3] = std::stoi(tokens[4]) - 1;
    } catch(const std::exception& e)
    {
      std::string msg =
          fmt::format("Error at line {}: Unable to convert quad {}'s 4th string value \"{}\" to integer.  Threw standard exception with text: \"{}\"", lineCount, (i + 1), tokens[4], e.what());
      return MakeErrorResult(-2007, msg);
    }
  }
}

[[nodiscard]] Result<> readDataForObject(ImportDeformKeyFileV12* filter, std::ifstream& inStream, usize& lineCount, AttributeMatrix& vertexAttributeMatrix, AttributeMatrix& cellAttributeMatrix, bool allocate)
{
  IGeometry::SharedVertexList& vertex;
  QuadGeom& quadGeom;

  while(inStream.peek() != EOF)
  {
    if(filter->getCancel())
    {
      return {};
    }
    bool isWord = false;
    std::vector<std::string> tokens;
    // Read the line. This line _Should_ be the start of "section" of data.
    {
      std::string buf;
      std::getline(inStream, buf);
      isWord = (buf[0] > 64 /*@ character */ && buf[0] < 91);
      buf = StringUtilities::trimmed(buf);
      buf = StringUtilities::simplified(buf);
      tokens = StringUtilities::split(buf, ' ');
      lineCount++;
    }

    if(tokens.empty())
    {
      // This is an empty line
      continue;
    }
    if(!isWord)
    {
      continue;
    }

    std::string word = tokens.at(0);

    if(tokens.size() == 1 && word == k_Star)
    {
      // We are at the next header, so we are done with this section
      return {};
    }
    if(word == k_VertexTitle)
    {
      usize numVerts;
      Result<> result = parse_ull(tokens.at(2), lineCount, numVerts);
      if(result.invalid())
      {
        filter->updateProgress(result.errors().data()->message);
        continue;
      }

      // Set the number of vertices and then create vertices array and resize vertex attr mat.
      filter->updateProgress(fmt::format("DEFORM Data File: Number of Vertex Points = {}", StringUtilities::number(numVerts)));
      vertexAttributeMatrix.resizeTuples({numVerts});
      IGeometry::SharedVertexList vertex = CreateSharedVertexList(static_cast<int64_t>(numVerts), true);

      auto vertexResult = readVertexCoordinates(filter, inStream, lineCount, vertex, numVerts);
      if(vertexResult.invalid())
      {
        return vertexResult;
      }
    }
    else if(word == k_CellTitle)
    {
      usize numCells;
      Result<> result = parse_ull(tokens.at(2), lineCount, numCells);
      if(result.invalid())
      {
        filter->updateProgress(result.errors().data()->message);
        continue;
      }

      // Set the number of cells and then create cells array and resize cell attr mat.
      filter->updateProgress(fmt::format("DEFORM Data File: Number of Quad Cells = {}", StringUtilities::number(numCells)));
      cellAttributeMatrix.resizeTuples({numCells});
      QuadGeom quadGeom = QuadGeom::CreateGeometry(static_cast<int64>(numCells), vertex, IGeometry::QuadGeometry, true);
      quadGeom.setSpatialDimensionality(2);
      IGeometry::MeshIndexArrayType& quads = quadGeom.getFacesRef();

      auto quadResult = readQuadGeometry(inStream, lineCount, quads, numCells, filter->getCancel());

      if(quadResult.invalid())
      {
        return quadResult;
      }
    }
    else if(tokens.size() >= 3 && (vertex != SharedVertexList::NullPointer() || quadGeom != QuadGeom::NullPointer()))
    {
      // This is most likely the beginning of a data array
      std::string dataArrayName = tokens.at(0);
      usize tupleCount;
      Result<> tupResult = parse_ull(tokens.at(2), lineCount, tupleCount);
      if(tupResult.invalid())
      {
        filter->updateProgress(tupResult.errors().data()->message);
        continue;
      }

      Result<> readInResult;
      if(tupleCount == vertex.getNumberOfTuples())
      {
        filter->updateProgress(fmt::format("Reading Vertex Data: {}", dataArrayName));

        readInResult = readDataArray(inStream, lineCount, vertexAttributeMatrix, dataArrayName, tupleCount, allocate, filter->getCancel());
      }
      else if(tupleCount == quadGeom.getNumberOfCells())
      {
        filter->updateProgress(fmt::format("Reading Cell Data: {}", dataArrayName));

        readInResult =readDataArray(inStream, lineCount, cellAttributeMatrix, dataArrayName, tupleCount, allocate, filter->getCancel());
      }
      else if(m_InputValues->verboseOutput)
      {
        // If verbose, dump the word, number of tuples, and some warning saying that it doesn't have the right number of tuples
        // for either vertex or cell arrays.

        // This data is not able to be read.  Display a status message that explains why, based on what information we have available.
        if(vertex == SharedVertexList::NullPointer() && quadGeom != QuadGeom::NullPointer())
        {
          std::string msg = fmt::format(
              "Unable to read data: {}.  Its tuple size ({}) doesn't match the correct number of tuples to be a cell array ({]), and the vertex tuple count has not been read yet.  Skipping...",
              dataArrayName, tupleCount, quadGeom.getNumberOfCells());
          filter->updateProgress(msg);
        }
        else if(vertex != SharedVertexList::NullPointer() && quadGeom == QuadGeom::NullPointer())
        {
          std::string msg = fmt::format(
              "Unable to read data: {}.  Its tuple size ({}) doesn't match the correct number of tuples to be a vertex array ({}), and the cell tuple count has not been read yet.  Skipping...",
              dataArrayName, tupleCount, vertex.getNumberOfTuples());
          filter->updateProgress(msg);
        }
        else
        {
          std::string msg = fmt::format("Unable to read data: {}.  Its tuple size ({}) doesn't match the correct number of tuples to be either a vertex array ({}) or cell array ({}).  Skipping...",
                                        dataArrayName, tupleCount, vertex.getNumberOfTuples(), quadGeom.getNumberOfCells());
          filter->updateProgress(msg);
        }
      }
      if(readInResult.invalid())
      {
        return readInResult;
      }
    }
    else
    {
      continue;
    }
  }

  return {};
}

[[nodiscard]] Result<> readDEFORMFile(ImportDeformKeyFileV12* filter, std::ifstream& inStream, DataStructure& dataStructure, AttributeMatrix& vertexAttributeMatrix, AttributeMatrix& cellAttributeMatrix, bool allocate)
{
  usize lineCount = 0;
  std::string word;
  std::string buf;
  std::vector<std::string> tokens; /* vector to store the split data */

  IGeometry::SharedVertexList& vertex;
  QuadGeom& quadGeom;

  while(inStream.peek() != EOF)
  {
    if(filter->getCancel())
    {
      return {};
    }

    tokens = getNextLineTokens(inStream, lineCount);

    if(tokens.size() <= 1)
    {
      // This is either an empty line or a singular star comment
      continue;
    }

    // Erase the star
    tokens.erase(tokens.begin());

    if(tokens == k_ProcessDefinition)
    {
      // Skip the comment line that ends the header
      getNextLineTokens(inStream, lineCount);

      // Read "Process Definition" section
      readProcessDefinition(inStream, lineCount);
      continue;
    }
    else if(tokens == k_StoppingAndStepControls)
    {
      // Skip the comment line that ends the header
      getNextLineTokens(inStream, lineCount);

      // Read "Stopping & Step Controls" section
      readStoppingAndStepControls(inStream, lineCount);
      continue;
    }
    else if(tokens == k_IterationControls)
    {
      // Skip the comment line that ends the header
      getNextLineTokens(inStream, lineCount);

      // Read "Iteration Controls" section
      readIterationControls(inStream, lineCount);
      continue;
    }
    else if(tokens == k_ProcessingConditions)
    {
      // Skip the comment line that ends the header
      getNextLineTokens(inStream, lineCount);

      // Read "Processing Conditions" section
      readProcessingConditions(inStream, lineCount);
      continue;
    }
    else if(tokens == k_UserDefinedVariables)
    {
      // Skip the comment line that ends the header
      getNextLineTokens(inStream, lineCount);

      // Read "User Defined Variables" section
      auto result = readUserDefinedVariables(inStream, lineCount);
      if(result.invalid())
      {
        filter->updateProgress(result.errors().data()->message);
      }
      continue;
    }
    else if(tokens == k_InterMaterialData)
    {
      // Skip the comment line that ends the header
      getNextLineTokens(inStream, lineCount);

      // Read "Inter-Material Data" section
      readInterMaterialData(inStream, lineCount);
      continue;
    }
    else if(tokens == k_InterObjectData)
    {
      // Skip the comment line that ends the header
      getNextLineTokens(inStream, lineCount);

      // Read "Inter-Object Data" section
      readInterObjectData(inStream, lineCount);
      continue;
    }
    else if(tokens.size() == 5)
    {
      std::vector<std::string> tmp = tokens;
      tmp.erase(tmp.begin() + 1);
      if(tmp == k_FileTitle)
      {
        // File title.  Continue...
        continue;
      }

      tmp = tokens;
      tmp.pop_back();

      if(tmp == k_PropertyDataOfMaterial)
      {
        // Skip the comment line that ends the header
        getNextLineTokens(inStream, lineCount);

        // Read "Property Data of Material" section
        readPropertyDataOfMaterial(inStream, lineCount);
        continue;
      }
      else if(tmp == k_DataForObject)
      {
        // Skip the comment line that ends the header
        getNextLineTokens(inStream, lineCount);

        // Read "Data For Object" section
        Result<> result = readDataForObject(filter, inStream, lineCount,vertexAttributeMatrix, cellAttributeMatrix, allocate);
        if(result.invalid())
        {
          return result;
        }

        continue;
      }
    }

    // Unrecognized section
    filter->updateProgress(fmt::format("Warning at line {}: Unrecognized section \"{}\"", StringUtilities::number(lineCount), StringUtilities::join(tokens.data(), ' ')));
  }
}
} // namespace

// -----------------------------------------------------------------------------
ImportDeformKeyFileV12::ImportDeformKeyFileV12(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ImportDeformKeyFileV12InputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportDeformKeyFileV12::~ImportDeformKeyFileV12() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportDeformKeyFileV12::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void ImportDeformKeyFileV12::updateProgress(const std::string& progressMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progressMessage});
}

// -----------------------------------------------------------------------------
Result<> ImportDeformKeyFileV12::operator()()
{
  std::ifstream inStream(m_InputValues->deformInputFile);
  ///!!!!!! VALIDATE THE STREAM !!!!!!!///

  // Read from the file
  readDEFORMFile(this, inStream, m_DataStructure, vertexAttrMat.get(), cellAttrMat.get(), !getInPreflight());
  auto userDefinedValues = algorithm.getUserDefinedVariables();

  // Cache the results
  std::vector<DataArrayMetadata> dataArrays;

  d_ptr->m_Cache.vertexAttrMatTupleCount = vertexAttrMat.getNumberOfTuples();
  d_ptr->m_Cache.cellAttrMatTupleCount = cellAttrMat.getNumberOfTuples();

  for(const auto& vertexArray : *vertexAttrMat)
  {
    if(vertexArray->getName() == "USRNOD")
    {
      for(const auto& userDefinedValue : userDefinedValues)
      {
        dataArrays.push_back({userDefinedValue, vertexArray->getNumberOfTuples(), static_cast<usize>(vertexArray->getNumberOfComponents()), DataArrayType::VERTEX});
      }
    }
    else
    {
      dataArrays.push_back({vertexArray->getName().toStdString(), vertexArray->getNumberOfTuples(), static_cast<usize>(vertexArray->getNumberOfComponents()), DataArrayType::VERTEX});
    }
  }
  for(const auto& cellArray : *cellAttrMat)
  {
    dataArrays.push_back({cellArray->getName().toStdString(), cellArray->getNumberOfTuples(), static_cast<usize>(cellArray->getNumberOfComponents()), DataArrayType::CELL});
  }

  d_ptr->m_Cache.inputFile = getDEFORMInputFile();
  d_ptr->m_Cache.dataArrays = dataArrays;
  d_ptr->m_Cache.timeStamp = fs::last_write_time(getDEFORMInputFile());

  return {};
}
