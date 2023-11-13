#include "ReadDeformKeyFileV12.hpp"

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

class IOHandler
{
public:
  IOHandler(ReadDeformKeyFileV12* filter, DataStructure& dataStructure, std::ifstream& inStream, const DataPath& quadGeomPath, const DataPath& vertexAMPath, const DataPath& cellAMPath,
            const bool allocate)
  : m_Filter(filter)
  , m_DataStructure(dataStructure)
  , m_InStream(inStream)
  , m_QuadGeomPath(quadGeomPath)
  , m_VertexAMPath(vertexAMPath)
  , m_CellAMPath(cellAMPath)
  , m_Allocate(allocate)
  , m_Cache(m_Filter->getCache())
  {
  }
  ~IOHandler() = default;

  IOHandler(const IOHandler&) = delete;
  IOHandler(IOHandler&&) noexcept = delete;
  IOHandler& operator=(const IOHandler&) = delete;
  IOHandler& operator=(IOHandler&&) noexcept = delete;

  [[nodiscard]] Result<> readDEFORMFile()
  {
    std::vector<std::string> tokens; /* vector to store the split data */

    while(m_InStream.peek() != EOF)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }

      tokens = getNextLineTokens();

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
        getNextLineTokens();

        // Read "Process Definition" section
        readProcessDefinition();
        continue;
      }
      else if(tokens == k_StoppingAndStepControls)
      {
        // Skip the comment line that ends the header
        getNextLineTokens();

        // Read "Stopping & Step Controls" section
        readStoppingAndStepControls();
        continue;
      }
      else if(tokens == k_IterationControls)
      {
        // Skip the comment line that ends the header
        getNextLineTokens();

        // Read "Iteration Controls" section
        readIterationControls();
        continue;
      }
      else if(tokens == k_ProcessingConditions)
      {
        // Skip the comment line that ends the header
        getNextLineTokens();

        // Read "Processing Conditions" section
        readProcessingConditions();
        continue;
      }
      else if(tokens == k_UserDefinedVariables)
      {
        // Skip the comment line that ends the header
        getNextLineTokens();

        // Read "User Defined Variables" section
        auto result = readUserDefinedVariables();
        if(result.invalid())
        {
          m_Filter->updateProgress(result.errors().data()->message);
        }
        continue;
      }
      else if(tokens == k_InterMaterialData)
      {
        // Skip the comment line that ends the header
        getNextLineTokens();

        // Read "Inter-Material Data" section
        readInterMaterialData();
        continue;
      }
      else if(tokens == k_InterObjectData)
      {
        // Skip the comment line that ends the header
        getNextLineTokens();

        // Read "Inter-Object Data" section
        readInterObjectData();
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
          getNextLineTokens();

          // Read "Property Data of Material" section
          readPropertyDataOfMaterial();
          continue;
        }
        else if(tmp == k_DataForObject)
        {
          // Skip the comment line that ends the header
          getNextLineTokens();

          // Read "Data For Object" section
          Result<> result = readDataForObject();
          if(result.invalid())
          {
            return result;
          }

          continue;
        }
      }

      // Unrecognized section
      m_Filter->updateProgress(fmt::format("Warning at line {}: Unrecognized section", StringUtilities::number(m_LineCount)));
    }

    return {};
  }

private:
  // member variables
  ReadDeformKeyFileV12* m_Filter;
  DataStructure& m_DataStructure;
  std::ifstream& m_InStream;
  const DataPath& m_QuadGeomPath;
  const DataPath& m_VertexAMPath;
  const DataPath& m_CellAMPath;
  const bool m_Allocate;
  FileCache& m_Cache;
  usize m_LineCount = 0;
  std::vector<std::string> m_UserDefinedVariables = {};
  std::vector<DataPath> m_UserDefinedArrays = {};

  // helper functions
  [[nodiscard]] Result<> setTuple(usize tuple, Float32Array& data, const std::vector<std::string>& tokens)
  {
    usize numComp = data.getNumberOfComponents();
    if(numComp > tokens.size())
    {
      std::string msg = fmt::format("Error at line {}: Unable to process line because the read in token count was {} and the expected amount was {}", m_LineCount, tokens.size(), numComp);
      return MakeErrorResult(-2023, msg);
    }
    for(usize comp = 0; comp < numComp; comp++)
    {
      float32 value = 0.0f;
      try
      {
        value = std::stof(tokens[comp]);
      } catch(const std::exception& e)
      {
        std::string msg = fmt::format("Error at line {}: Unable to convert data array {}'s string value \"{}\" to float.  Threw standard exception with text: \"{}\"", m_LineCount, data.getName(),
                                      tokens[comp], e.what());
        return MakeErrorResult(-2008, msg);
      }
      data[tuple * numComp + comp] = value;
    }
    return {};
  }

  inline Result<> parse_ull(const std::string& token, usize& value) const
  {
    try
    {
      value = std::stoull(token);
    } catch(const std::exception& e)
    {
      std::string msg = fmt::format("Error at line {}: Unable to convert string value \"{}\" to unsigned long long.  Threw standard exception with text: \"{}\"", StringUtilities::number(m_LineCount),
                                    token, e.what());
      return MakeErrorResult(-2000, msg);
    }

    return {};
  }

  std::vector<std::string> getNextLineTokens()
  {
    std::vector<std::string> tokens; /* vector to store the split data */
    std::string buf;
    std::getline(m_InStream, buf);
    buf = StringUtilities::trimmed(buf);
    buf = StringUtilities::simplified(buf);
    tokens = StringUtilities::split(buf, ' ');
    m_LineCount++;

    return tokens;
  }

  void findNextSection()
  {
    std::vector<std::string> tokens;
    while(m_InStream.peek() != EOF)
    {
      tokens = getNextLineTokens();
      if(!tokens.empty() && tokens.at(0) == k_Star)
      {
        return;
      }
    }
  }

  void readProcessDefinition()
  {
    // We currently don't care about the "Process Definition" section...
    findNextSection();
  }

  void readStoppingAndStepControls()
  {
    // We currently don't care about the "Stopping & Step Controls" section...
    findNextSection();
  }

  void readIterationControls()
  {
    // We currently don't care about the "Iteration Controls" section...
    findNextSection();
  }

  void readProcessingConditions()
  {
    // We currently don't care about the "Processing Conditions" section...
    findNextSection();
  }

  void readPropertyDataOfMaterial()
  {
    // We currently don't care about the "Property Data of Material" section...
    findNextSection();
  }

  void readInterMaterialData()
  {
    // We currently don't care about the "Inter-Material Data" section...
    findNextSection();
  }

  void readInterObjectData()
  {
    // We currently don't care about the "Inter-Object Data" section...
    findNextSection();
  }

  [[nodiscard]] Result<> readUserDefinedVariables()
  {
    std::string buf;
    std::getline(m_InStream, buf);
    buf = StringUtilities::trimmed(buf);
    buf = StringUtilities::simplified(buf);
    auto tokens = StringUtilities::split(buf, ' ');
    m_LineCount++;

    if(tokens.size() != 3)
    {
      findNextSection();
      return {};
    }

    usize numVars;
    Result<> result = parse_ull(tokens.at(2), numVars);
    if(result.invalid())
    {
      return result;
    }

    m_UserDefinedVariables.resize(12);
    for(usize i = 0; i < numVars; i++)
    {
      std::getline(m_InStream, buf);
      buf = StringUtilities::trimmed(buf);
      m_LineCount++;
      std::string cleanedString = StringUtilities::replace(buf, "/", "|");
      m_UserDefinedVariables[i] = cleanedString;
    }

    return {};
  }

  [[nodiscard]] Result<> readDataArray(const DataPath& parentPath, const std::string& dataArrayName, usize arrayTupleSize)
  {
    if(arrayTupleSize == 0)
    {
      return {};
    }

    usize componentCount = 0;
    int32 tupleLineCount = 1;
    // We are here because the calling function *should* have determined correctly (hopefully) that we are now reading a
    // section of data. We are going to make the assumption this is correct and go from here. What is *not* figured out
    // at this point is how many components are in each tuple. That is what this next scoped section is going to figure out.
    // We are going to read the very fist line, strip off the index value that appears (at least it seems that way from the
    // file) in the first 8 chars of the line. (The file is written by FORTRAN, so I'm hoping that they used fixed width
    // fields when writing the data otherwise this whole section is based on a very bad assumption). We then tokenize the
    // values and read the next line. IFF there are NON-SPACE characters in the first 8 bytes of this second line that was
    // read then we now know that all the components are on a single line. We count the tokens and use that as the number
    // of components, otherwise we add these tokens to the last set of tokens and read another line. This continues until
    // we find a line that DOES have a NON-SPACE character in the first 8 bytes of the line.
    {
      std::vector<std::string> tokens;
      std::string line;

      std::getline(m_InStream, line);
      m_LineCount++;
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
        auto currentFilePos = m_InStream.tellg();
        // Read the next line
        std::getline(m_InStream, line);
        m_LineCount++;
        // Figure out if there is anything in the first 8 chars
        for(usize i = 0; i < 8; i++)
        {
          if(line[i] != ' ')
          {
            m_InStream.seekg(currentFilePos); // Roll back the m_InStream to just before we read this line.
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
        m_UserDefinedArrays.reserve(m_UserDefinedVariables.size());
        for(const auto& userDefinedVariable : m_UserDefinedVariables)
        {
          DataArrayMetadata metadata = {};
          metadata.path = parentPath.createChildPath(userDefinedVariable);
          metadata.tupleCount = arrayTupleSize;
          metadata.componentCount = 1;

          m_Cache.dataArrays.emplace_back(std::move(metadata));
          m_UserDefinedArrays.emplace_back(parentPath.createChildPath(userDefinedVariable));
        }
      }
      else
      {
        DataArrayMetadata metadata = {};
        metadata.path = parentPath.createChildPath(dataArrayName);
        metadata.tupleCount = arrayTupleSize;
        metadata.componentCount = componentCount;

        m_Cache.dataArrays.emplace_back(std::move(metadata));
      }

      if(m_Allocate)
      {
        Result<> setResult;
        if(dataArrayName == "USRNOD")
        {
          for(usize i = 0; i < m_UserDefinedVariables.size(); i++)
          {
            auto data = m_DataStructure.getDataRefAs<Float32Array>(m_UserDefinedArrays[i]);
            setResult = setTuple(0, data, tokens);
          }
        }
        else
        {
          auto data = m_DataStructure.getDataRefAs<Float32Array>(parentPath.createChildPath(dataArrayName));
          setResult = setTuple(0, data, tokens);
        }
        if(setResult.invalid())
        {
          return setResult;
        }
      }
    }

    // Just read and skip since this is probably a preflight, or we are just reading through the file to find the interesting data
    if(!m_Allocate)
    {
      std::string line;
      usize totalLinesToRead = (arrayTupleSize - 1) * tupleLineCount;
      for(usize i = 0; i < totalLinesToRead; i++)
      {
        std::getline(m_InStream, line);
        m_LineCount++;
      }
    }
    else
    {
      // Read each tuple's data starting at the second tuple
      for(usize tupleIndex = 1; tupleIndex < arrayTupleSize; tupleIndex++)
      {
        if(m_Filter->getCancel())
        {
          return {};
        }
        std::vector<std::string> tokens;

        // Read the data in groups of each tuple to build up the tokens
        for(int32_t compLine = 0; compLine < tupleLineCount; compLine++)
        {
          // Now read the line
          std::string compLineData;
          std::getline(m_InStream, compLineData);
          m_LineCount++;
          usize offset = (compLine == 0 ? 1 : 0);
          compLineData = StringUtilities::trimmed(compLineData);
          compLineData = StringUtilities::simplified(compLineData);
          auto subTokens = StringUtilities::split(compLineData, ' ');
          tokens.insert(tokens.end(), subTokens.begin() + static_cast<int64>(offset), subTokens.end());
        }

        Result<> setResult;
        if(dataArrayName == "USRNOD")
        {
          for(usize i = 0; i < m_UserDefinedVariables.size(); i++)
          {
            auto data = m_DataStructure.getDataRefAs<Float32Array>(m_UserDefinedArrays[i]);
            setResult = setTuple(0, data, tokens);
          }
        }
        else
        {
          auto data = m_DataStructure.getDataRefAs<Float32Array>(parentPath.createChildPath(dataArrayName));
          setResult = setTuple(tupleIndex, data, tokens);
        }
        if(setResult.invalid())
        {
          return setResult;
        }
      }
    }
    return {};
  }

  Result<> readVertexCoordinates(IGeometry::SharedVertexList* vertex, usize numVerts)
  {
    std::string buf;
    std::vector<std::string> tokens; /* vector to store the split data */

    // Read or Skip past all the vertex data
    for(usize i = 0; i < numVerts; i++)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }

      tokens = getNextLineTokens();

      try
      {
        vertex->operator[](3 * i) = std::stof(tokens[1]);
      } catch(const std::exception& e)
      {
        std::string msg = fmt::format("Error at line {}: Unable to convert vertex coordinate {}'s 1st string value \"{}\" to float.  Threw standard exception with text: \"{}\"",
                                      StringUtilities::number(m_LineCount), StringUtilities::number(i + 1), tokens[1], e.what());
        return MakeErrorResult(-2001, std::move(msg));
      }

      try
      {
        vertex->operator[](3 * i + 1) = std::stof(tokens[2]);
      } catch(const std::exception& e)
      {
        std::string msg = fmt::format("Error at line {}: Unable to convert vertex coordinate {}'s 2nd string value \"{}\" to float.  Threw standard exception with text: \"{}\"",
                                      StringUtilities::number(m_LineCount), StringUtilities::number(i + 1), tokens[2], e.what());
        return MakeErrorResult(-2002, std::move(msg));
      }

      vertex->operator[](3 * i + 2) = 0.0f;
    }

    return {};
  }

  Result<> readQuadGeometry(IGeometry::MeshIndexArrayType& quads, usize numCells)
  {
    std::string buf;
    std::vector<std::string> tokens; /* vector to store the split data */

    for(usize i = 0; i < numCells; i++)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }

      tokens = getNextLineTokens();

      // Subtract one from the node number because DEFORM starts at node 1, and we start at node 0
      try
      {
        quads[4 * i] = std::stoi(tokens[1]) - 1;
      } catch(const std::exception& e)
      {
        std::string msg =
            fmt::format("Error at line {}: Unable to convert quad {}'s 1st string value \"{}\" to integer.  Threw standard exception with text: \"{}\"", m_LineCount, (i + 1), tokens[1], e.what());
        return MakeErrorResult(-2004, msg);
      }

      try
      {
        quads[4 * i + 1] = std::stoi(tokens[2]) - 1;
      } catch(const std::exception& e)
      {
        std::string msg =
            fmt::format("Error at line {}: Unable to convert quad {}'s 2nd string value \"{}\" to integer.  Threw standard exception with text: \"{}\"", m_LineCount, (i + 1), tokens[2], e.what());
        return MakeErrorResult(-2005, msg);
      }

      try
      {
        quads[4 * i + 2] = std::stoi(tokens[3]) - 1;
      } catch(const std::exception& e)
      {
        std::string msg =
            fmt::format("Error at line {}: Unable to convert quad {}'s 3rd string value \"{}\" to integer.  Threw standard exception with text: \"{}\"", m_LineCount, (i + 1), tokens[3], e.what());
        return MakeErrorResult(-2006, msg);
      }

      try
      {
        quads[4 * i + 3] = std::stoi(tokens[4]) - 1;
      } catch(const std::exception& e)
      {
        std::string msg =
            fmt::format("Error at line {}: Unable to convert quad {}'s 4th string value \"{}\" to integer.  Threw standard exception with text: \"{}\"", m_LineCount, (i + 1), tokens[4], e.what());
        return MakeErrorResult(-2007, msg);
      }
    }

    return {};
  }

  [[nodiscard]] Result<> readDataForObject()
  {
    bool quadHit = false;
    bool vertHit = false;

    while(m_InStream.peek() != EOF)
    {
      if(m_Filter->getCancel())
      {
        return {};
      }
      bool isWord = false;
      std::vector<std::string> tokens;
      // Read the line. This line _Should_ be the start of "section" of data.
      {
        std::string buf;
        std::getline(m_InStream, buf);
        isWord = (buf[0] > 64 /*@ character */ && buf[0] < 91);
        buf = StringUtilities::trimmed(buf);
        buf = StringUtilities::simplified(buf);
        tokens = StringUtilities::split(buf, ' ');
        m_LineCount++;
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
        vertHit = true;
        usize numVerts;
        Result<> result = parse_ull(tokens.at(2), numVerts);
        if(result.invalid())
        {
          m_Filter->updateProgress(result.errors().data()->message);
          continue;
        }

        // Set the number of vertices and then create vertices array and resize vertex attr mat.
        m_Filter->updateProgress(fmt::format("DEFORM Data File: Number of Vertex Points = {}", StringUtilities::number(numVerts)));
        m_Cache.vertexAttrMatTupleCount = numVerts;

        if(m_Allocate)
        {
          // Grab vertex list from quad geom
          IGeometry::SharedVertexList* vertex = m_DataStructure.getDataAs<QuadGeom>(m_QuadGeomPath)->getVertices();

          auto vertexResult = readVertexCoordinates(vertex, numVerts);
          if(vertexResult.invalid())
          {
            return vertexResult;
          }
        }
      }
      else if(word == k_CellTitle)
      {
        quadHit = true;
        usize numCells;
        Result<> result = parse_ull(tokens.at(2), numCells);
        if(result.invalid())
        {
          m_Filter->updateProgress(result.errors().data()->message);
          continue;
        }

        // Set the number of cells and then create cells array and resize cell attr mat.
        m_Filter->updateProgress(fmt::format("DEFORM Data File: Number of Quad Cells = {}", StringUtilities::number(numCells)));
        m_Cache.cellAttrMatTupleCount = numCells;

        if(m_Allocate)
        {
          auto& quadGeom = m_DataStructure.getDataRefAs<QuadGeom>(m_QuadGeomPath);
          quadGeom.setSpatialDimensionality(2);
          IGeometry::MeshIndexArrayType& quads = quadGeom.getFacesRef();
          auto quadResult = readQuadGeometry(quads, numCells);
          if(quadResult.invalid())
          {
            return quadResult;
          }
        }
      }
      else if(tokens.size() >= 3 && (vertHit || quadHit))
      {
        // This is most likely the beginning of a data array
        std::string dataArrayName = tokens.at(0);
        usize tupleCount;
        Result<> tupResult = parse_ull(tokens.at(2), tupleCount);
        if(tupResult.invalid())
        {
          m_Filter->updateProgress(tupResult.errors().data()->message);
          continue;
        }

        Result<> readInResult;
        if(tupleCount == m_Cache.vertexAttrMatTupleCount)
        {
          m_Filter->updateProgress(fmt::format("Reading Vertex Data: {}", dataArrayName));

          readInResult = readDataArray(m_VertexAMPath, dataArrayName, tupleCount);
        }
        else if(tupleCount == m_Cache.cellAttrMatTupleCount)
        {
          m_Filter->updateProgress(fmt::format("Reading Cell Data: {}", dataArrayName));

          readInResult = readDataArray(m_CellAMPath, dataArrayName, tupleCount);
        }
        else
        {
          // If verbose, dump the word, number of tuples, and some warning saying that it doesn't have the right number of tuples
          // for either vertex or cell arrays.

          // This data is not able to be read.  Display a status message that explains why, based on what information we have available.
          if(!vertHit && quadHit)
          {
            std::string msg = fmt::format(
                "Unable to read data: {}.  Its tuple size ({}) doesn't match the correct number of tuples to be a cell array ({}), and the vertex tuple count has not been read yet.  Skipping...",
                dataArrayName, tupleCount, m_Cache.cellAttrMatTupleCount);
            m_Filter->updateProgress(msg);
          }
          else if(vertHit && !quadHit)
          {
            std::string msg = fmt::format(
                "Unable to read data: {}.  Its tuple size ({}) doesn't match the correct number of tuples to be a vertex array ({}), and the cell tuple count has not been read yet.  Skipping...",
                dataArrayName, tupleCount, m_Cache.vertexAttrMatTupleCount);
            m_Filter->updateProgress(msg);
          }
          else
          {
            std::string msg = fmt::format("Unable to read data: {}.  Its tuple size ({}) doesn't match the correct number of tuples to be either a vertex array or cell array.  Skipping...",
                                          dataArrayName, tupleCount);
            m_Filter->updateProgress(msg);
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
};
} // namespace

// -----------------------------------------------------------------------------
ReadDeformKeyFileV12::ReadDeformKeyFileV12(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadDeformKeyFileV12InputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Cache(FileCache{})
{
}

// -----------------------------------------------------------------------------
ReadDeformKeyFileV12::~ReadDeformKeyFileV12() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadDeformKeyFileV12::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void ReadDeformKeyFileV12::updateProgress(const std::string& progressMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progressMessage});
}

// -----------------------------------------------------------------------------
FileCache& ReadDeformKeyFileV12::getCache()
{
  return m_Cache;
}

// -----------------------------------------------------------------------------
Result<> ReadDeformKeyFileV12::operator()(bool allocate)
{
  /*
   * IF allocate is false DO NOT TOUCH DATA STRUCTURE IN ANY WAY
   *
   * If allocate is false then we are here in preflight meaning we
   * DO NOT have a working DataStructure and that the DataPaths that
   * have been passed in within inputValues are not valid as they have
   * not been created.
   */
  std::ifstream inStream(m_InputValues->InputFilePath, std::ios_base::binary);
  if(!inStream.is_open())
  {
    return MakeErrorResult(-2013, fmt::format("Unable to open the provided file to read at path : {}", m_InputValues->InputFilePath.string()));
  }

  IOHandler handler = IOHandler(this, m_DataStructure, inStream, m_InputValues->QuadGeomPath, m_InputValues->VertexAMPath, m_InputValues->CellAMPath, allocate);

  // Read from the file
  return handler.readDEFORMFile();
}
