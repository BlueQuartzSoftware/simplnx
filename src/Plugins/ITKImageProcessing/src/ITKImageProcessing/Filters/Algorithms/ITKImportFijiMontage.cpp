#include "ITKImportFijiMontage.hpp"

#include "complex/Common/Array.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/StringUtilities.hpp"
#include "complex/Utilities/MontageUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
const std::string k_DCName = "ZenInfo";
const std::string k_AMName = "CellAM";
const std::string k_AAName = "ImageData";
const std::string k_GrayScaleTempArrayName = "gray_scale_temp";
const std::string k_MontageInfoReplaceKeyword = "@UPDATE_ROW_COLUMN@";

template <bool GenerateCache = true>
class IOHandler
{
public:
  IOHandler(ITKImportFijiMontage* filter, DataStructure& dataStructure, fs::path& inFile, const DataPath& quadGeomPath, const DataPath& vertexAMPath, const DataPath& cellAMPath,
            const bool allocate)
  : m_Filter(filter)
  , m_DataStructure(dataStructure)
  , m_InFile(inFile)
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

  [[nodiscard]] Result<> operator()()
  {
    m_InStream = std::ifstream(m_InFile, std::ios_base::binary);
    if(!m_InStream.is_open())
    {
      return MakeErrorResult(-2013, fmt::format("Unable to open the provided file to read at path : {}", m_InFile.string()));
    }

    if(!m_Allocate)
    {
      FillCache();
    }
    else
    {
      ReadImages();
    }
  }

private:
  // member variables
  ITKImportFijiMontage* m_Filter;
  DataStructure& m_DataStructure;
  fs::path& m_InFile;
  std::ifstream m_InStream;
  const DataPath& m_QuadGeomPath;
  const DataPath& m_VertexAMPath;
  const DataPath& m_CellAMPath;
  const bool m_Allocate;
  FijiCache& m_Cache;
  usize m_LineCount = 0;
  std::vector<std::string> m_UserDefinedVariables = {};
  std::vector<DataPath> m_UserDefinedArrays = {};

  std::string m_MontageName = "Zen Montage";
  int32 m_LengthUnit = 6;
  Vec2<int32> m_ColumnMontageLimits = {0, 0};
  Vec2<int32> m_RowMontageLimits = {0, 0};
  Vec2<int32> m_MontageStart = {0, 0};
  Vec2<int32> m_MontageEnd = {0, 0};
  DataPath m_DataContainerPath = {};
  std::string m_CellAttributeMatrixName = "";
  std::string m_ImageDataArrayName = "";
  bool m_ConvertToGrayScale = false;
  FloatVec3 m_ColorWeights = FloatVec3(0.2125f, 0.7154f, 0.0721f);
  bool m_ChangeOrigin = false;
  FloatVec3 m_Origin = FloatVec3(0.0f, 0.0f, 0.0f);
  bool m_ChangeSpacing = false;
  FloatVec3 m_Spacing = FloatVec3(1.0f, 1.0f, 1.0f);

  int32 m_RowCount = -1;
  int32 m_ColumnCount = -1;
  int32 m_Tolerance = 100;

  // -----------------------------------------------------------------------------
  void ParseConfigFile()
  {
    bool dimFound = false;
    bool dataFound = false;

    std::string line;
    while(std::getline(m_InStream, line))
    {
      line = StringUtilities::trimmed(line);

      if(StringUtilities::starts_with(line, "dim =")) // found the dimensions
      {
        dimFound = true;
        // Should check that the value = 2
      }
      if(StringUtilities::starts_with(line, "# Define the image coordinates"))
      {
        // Found the start of the data
        dataFound = true;
        break;
      }
      if(StringUtilities::starts_with(line, "#")) // comment line
      {
        continue;
      }
    }

    if(!dimFound || !dataFound)
    {
      return;
    }

    // slice_12.tif; ; (471.2965233276666, -0.522608066434236)
    while(std::getline(m_InStream, line))
    {
      line = StringUtilities::trimmed(line);
      if(line.empty())
      {
        continue;
      }
      std::vector<std::string> tokens = StringUtilities::split(line, ';');
      if(tokens.size() != 3)
      {
        continue;
      }
      BoundsType bound;
      bound.Filename = fs::path(m_InFile.parent_path().string() + fs::path::preferred_separator + tokens[0]);

      std::string coords = StringUtilities::trimmed(tokens[2]);
      coords = StringUtilities::replace(coords, "(", "");
      coords = StringUtilities::replace(coords, ")", "");
      tokens = StringUtilities::split(coords, ',');
      if(tokens.size() != 2)
      {
        continue;
      }
      float32 x = std::stof(tokens[0]);
      float32 y = std::stof(tokens[1]);
      bound.Origin = FloatVec3(x, y, 0.0f);
      bound.Spacing = FloatVec3(1.0f, 1.0f, 1.0f);
      m_Cache.bounds.push_back(bound);
    }

    FindTileIndices(m_Tolerance);
  }

  // -----------------------------------------------------------------------------
  void FindTileIndices(int32 tolerance)
  {
    std::vector<int32> xValues(m_Cache.bounds.size());
    std::vector<int32> yValues(m_Cache.bounds.size());

    for(usize i = 0; i < m_Cache.bounds.size(); i++)
    {
      xValues[i] = static_cast<int32>(m_Cache.bounds[i].Origin[0]);
      yValues[i] = static_cast<int32>(m_Cache.bounds[i].Origin[1]);
    }

    std::map<int32, std::vector<usize>> avg_indices = MontageUtilities::Burn(tolerance, xValues);
    int32 index = 0;
    for(auto& iter : avg_indices)
    {
      const std::vector<usize>& indices = iter.second;
      for(const auto& i : indices)
      {
        m_Cache.bounds[i].Col = index;
      }
      index++;
    }
    m_ColumnCount = index;

    avg_indices = MontageUtilities::Burn(tolerance, yValues);
    index = 0;
    for(auto& iter : avg_indices)
    {
      const std::vector<usize>& indices = iter.second;
      for(const auto& i : indices)
      {
        m_Cache.bounds[i].Row = index;
      }
      index++;
    }
    m_RowCount = index;
  }

  // -----------------------------------------------------------------------------
  void FillCache(const fs::path& inputFile)
  {
    // This next function will set the FileName (Partial), Row, Col for each "bound" object
    ParseConfigFile(inputFile);

    FloatVec3 minCoord = {std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max()};
    FloatVec3 minSpacing = {std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max()};
    std::vector<ImageGeom> geometries;

    // Get the meta information from disk for each image
    for(auto& bound : m_Cache.bounds)
    {
      bound.LengthUnit = static_cast<IGeometry::LengthUnit>(m_LengthUnit);

      DataPath dap({::k_DCName, ::k_AMName, ::k_AAName});
      IFilter imageImportFilter = MontageImportHelper::CreateImageImportFilter(this, bound.Filename, dap);
      imageImportFilter->preflight();
      if(imageImportFilter->getErrorCode() < 0)
      {
        setErrorCondition(imageImportFilter->getErrorCode(), "Error Preflighting Image Import Filter.");
        continue;
      }

      DataContainerArray importImageDca = imageImportFilter->getDataContainerArray();
      {
        DataContainer fromDc = importImageDca->getDataContainer(::k_DCName);
        AttributeMatrix& fromCellAttrMat = fromDc->getAttributeMatrix(::k_AMName);
        IDataArray& fromImageData = fromCellAttrMat.getAttributeArray(::k_AAName);
        fromImageData.setName(m_ImageDataArrayName);
        bound.ImageDataProxy = fromImageData;

        // Set the Origin based on the values from the Fiji Config File
        // Set the spacing to the default 1,1,1
        ImageGeom& imageGeom = fromDc->getGeometryAs<ImageGeom>();
        bound.Dims = imageGeom.getDimensions();
        imageGeom.setSpacing(bound.Spacing);
        imageGeom.setOrigin(bound.Origin);
        minSpacing = imageGeom.getSpacing();
        //
        minCoord[0] = std::min(bound.Origin[0], minCoord[0]);
        minCoord[1] = std::min(bound.Origin[1], minCoord[1]);
        minCoord[2] = 0.0f;
        geometries.push_back(imageGeom);
      }

      // Execute color to grayscale filter
      if(m_ConvertToGrayScale)
      {
        IFilter grayScaleFilter = MontageImportHelper::CreateColorToGrayScaleFilter(this, dap, m_ColorWeights, ::k_GrayScaleTempArrayName);
        grayScaleFilter->setDataContainerArray(importImageDca);
        grayScaleFilter->preflight();
        if(grayScaleFilter->getErrorCode() < 0)
        {
          setErrorCondition(grayScaleFilter->getErrorCode(), "Error Preflighting Color to GrayScale filter");
          continue;
        }

        DataContainerArray colorToGrayDca = grayScaleFilter->getDataContainerArray();
        DataContainer fromDc = colorToGrayDca->getDataContainer(::k_DCName);
        AttributeMatrix& fromCellAttrMat = fromDc->getAttributeMatrix(::k_AMName);
        // Remove the RGB Attribute Array so we can rename the gray scale AttributeArray
        IDataArray& rgbImageArray = fromCellAttrMat->removeAttributeArray(::k_AAName);
        std::string grayScaleArrayName = ::k_GrayScaleTempArrayName + ::k_AAName;
        IDataArray& fromGrayScaleData = fromCellAttrMat->removeAttributeArray(grayScaleArrayName);
        fromGrayScaleData.setName(m_ImageDataArrayName);
        bound.ImageDataProxy = fromGrayScaleData;
      }

      m_Cache.maxCol = std::max(bound.Col, m_Cache.maxCol);
      m_Cache.maxRow = std::max(bound.Row, m_Cache.maxRow);
    }

    std::stringstream ss;
    ss << "Tile Column(s): " << m_ColumnCount - 1 << "  Tile Row(s): " << m_RowCount - 1 << "  Image Count: " << m_ColumnCount * m_RowCount;

    FloatVec3 overrideOrigin = minCoord;
    FloatVec3 overrideSpacing = minSpacing;

    // Now adjust the origin/spacing if needed
    if(m_ChangeOrigin || m_ChangeSpacing)
    {
      if(m_ChangeOrigin)
      {
        overrideOrigin = m_Origin;
      }
      if(m_ChangeSpacing)
      {
        overrideSpacing = m_Spacing;
      }
      FloatVec3 delta = {minCoord[0] - overrideOrigin[0], minCoord[1] - overrideOrigin[1], minCoord[2] - overrideOrigin[2]};
      for(usize i = 0; i < geometries.size(); i++)
      {
        ImageGeom imageGeom = geometries[i];
        BoundsType& bound = m_Cache.bounds.at(i);
        std::transform(bound.Origin.begin(), bound.Origin.end(), delta.begin(), bound.Origin.begin(), std::minus<>());
        imageGeom.setOrigin(bound.Origin); // Sync up the ImageGeom with the calculated values
        imageGeom.setSpacing(overrideSpacing);
      }
    }
    ss << "\nOrigin: " << overrideOrigin[0] << ", " << overrideOrigin[1] << ", " << overrideOrigin[2];
    ss << "\nSpacing: " << overrideSpacing[0] << ", " << overrideSpacing[1] << ", " << overrideSpacing[2];
    ss << ::k_MontageInfoReplaceKeyword;
    m_Cache.montageInformation = ss.str();
  }

  // -----------------------------------------------------------------------------
  void ReadImages()
  {
    int32 rowCountPadding = MontageUtilities::CalculatePaddingDigits(m_RowCount);
    int32 colCountPadding = MontageUtilities::CalculatePaddingDigits(m_ColumnCount);
    int32 charPaddingCount = std::max(rowCountPadding, colCountPadding);

    for(const auto& bound : m_Cache.bounds)
    {
      if(bound.Row < m_MontageStart[1] || bound.Row > m_MontageEnd[1] || bound.Col < m_MontageStart[0] || bound.Col > m_MontageEnd[0])
      {
        continue;
      }

      m_Filter->sendUpdate(("Importing" + bound.Filename.filename().string()));

      // Create our DataContainer Name using a Prefix and a rXXcYY format.
      std::stringstream dcNameStream;
      dcNameStream << "r" << std::setw(charPaddingCount) << std::right << std::setfill('0') << bound.Row;
      dcNameStream << std::setw(1) << "c" << std::setw(charPaddingCount) << bound.Col;

      // The DataContainer with a name based on the ROW & COLUMN indices is already created in the preflight
      // So is the Geometry
      ImageGeom& image = m_DataStructure->getDataRefAs<ImageGeom>();

      image.setUnits(static_cast<IGeometry::LengthUnit>(m_LengthUnit));

      // Create the Image Geometry
      SizeVec3 dims = image.getDimensions();

      std::vector<usize> tDims = {dims[0], dims[1], dims[2]};
      // The Cell AttributeMatrix is also already created at this point
      AttributeMatrix& cellAttrMat = image.getCellDataRef();
      // Instantiate the Image Import Filter to actually read the image into a data array
      DataPath dap({::k_DCName, ::k_AMName, m_ImageDataArrayName}); // This is just a temp path for the subfilter to use

      //execute image import filter
      IFilter imageImportFilter = MontageImportHelper::CreateImageImportFilter(this, bound.Filename, dap);
      if(nullptr == imageImportFilter.get())
      {
        continue;
      }
      // This same filter was used to preflight so as long as nothing changes on disk this really should work....
      imageImportFilter->execute();
      if(imageImportFilter->getErrorCode() < 0)
      {
        setErrorCondition(imageImportFilter->getErrorCode(), "Error Executing Image Import Filter.");
        continue;
      }
      // Now transfer the image data from the actual image data read from disk into our existing Attribute Matrix
      DataContainerArray importImageDca = imageImportFilter->getDataContainerArray();
      AttributeMatrix fromCellAttrMat = m_DataStructure.getDataRefAs<AttributeMatrix>(DataPath{::k_AMName});

      if(m_ConvertToGrayScale)
      {
        //Run grayscale filter and process results and messages
        #error implement;
//        AbstractFilter::Pointer grayScaleFilter = MontageImportHelper::CreateColorToGrayScaleFilter(this, dap, m_ColorWeights, Montage::k_GrayScaleTempArrayName);
//        grayScaleFilter->setDataContainerArray(importImageDca); // Use the Data Container array that was use for the import. It is set up and ready to go
//        connect(grayScaleFilter.get(), SIGNAL(messageGenerated(const AbstractMessage::Pointer&)), this, SIGNAL(messageGenerated(const AbstractMessage::Pointer&)));
//        grayScaleFilter->execute();
//        if(grayScaleFilter->getErrorCode() < 0)
//        {
//          setErrorCondition(grayScaleFilter->getErrorCode(), "Error Executing Color to GrayScale filter");
//          continue;
//        }

        DataContainerArray c2gDca = grayScaleFilter->getDataContainerArray();
        DataContainer c2gDc = c2gDca->getDataContainer(::k_DCName);
        AttributeMatrix c2gAttrMat = c2gDc->getAttributeMatrix(::k_AMName);

        std::string grayScaleArrayName = ::k_GrayScaleTempArrayName + m_ImageDataArrayName;

        IDataArray& rgbImageArray = c2gAttrMat->removeAttributeArray(::k_AAName);
        IDataArray& gray = c2gAttrMat->removeAttributeArray(grayScaleArrayName);
        gray.setName(m_ImageDataArrayName);
        cellAttrMat.addOrReplaceAttributeArray(gray);
      }
      else
      {
        // Copy the IDataArray (which contains the image data) from the temp data container array into our persistent data structure
        IDataArray& gray = fromCellAttrMat.removeAttributeArray(m_ImageDataArrayName);
        cellAttrMat.addOrReplaceAttributeArray(gray);
      }
    }
  }

  //  // -----------------------------------------------------------------------------
  //  std::string GetMontageInformation()
  //  {
  //    std::string info = m_MontageInformation;
  //    std::stringstream ss;
  //    int32 importedCols = m_MontageEnd[0] - m_MontageStart[0] + 1;
  //    int32 importedRows = m_MontageEnd[1] - m_MontageStart[1] + 1;
  //    ss << "\n"
  //       << "Imported Columns: " << importedCols << "  Imported Rows: " << importedRows << "  Imported Image Count: " << (importedCols * importedRows);
  //    info = StringUtilities::replace(info, Montage::k_MontageInfoReplaceKeyword, ss.str());
  //    return info;
  //  }
};
} // namespace

// -----------------------------------------------------------------------------
ITKImportFijiMontage::ITKImportFijiMontage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKImportFijiMontageInputValues* inputValues,
                                           FijiCache& cache)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Cache(cache)
{
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ITKImportFijiMontage::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
FijiCache& ITKImportFijiMontage::getCache()
{
  return m_Cache;
}

// -----------------------------------------------------------------------------
void ITKImportFijiMontage::sendUpdate(const std::string& message)
{
  m_MessageHandler({IFilter::Message::Type::Info, message});
}

// -----------------------------------------------------------------------------
Result<> ITKImportFijiMontage::operator()()
{
  /*
   * IF allocate is false DO NOT TOUCH DATA STRUCTURE IN ANY WAY
   *
   * If allocate is false then we are here in preflight meaning we
   * DO NOT have a working DataStructure and that the DataPaths that
   * have been passed in within inputValues are not valid as they have
   * not been created.
   */
  IOHandler handler = IOHandler(this, m_DataStructure, m_InputValues->InputFilePath, m_InputValues->QuadGeomPath, m_InputValues->VertexAMPath, m_InputValues->CellAMPath, m_InputValues->Allocate);

  // Read from the file
  return handler();
}
