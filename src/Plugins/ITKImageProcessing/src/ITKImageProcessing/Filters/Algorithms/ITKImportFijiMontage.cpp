#include "ITKImportFijiMontage.hpp"

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "complex/Common/Array.hpp"
#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Montage/GridTileIndex.hpp"
#include "complex/DataStructure/Montage/GridMontage.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Utilities/MontageUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
const int32 k_Tolerance = 100;
const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const Uuid k_ColorToGrayScaleFilterId = *Uuid::FromString("d938a2aa-fee2-4db9-aa2f-2c34a9736580");
const FilterHandle k_ColorToGrayScaleFilterHandle(k_ColorToGrayScaleFilterId, k_ComplexCorePluginId);

template <bool GenerateCache = true>
class IOHandler
{
public:
  IOHandler(ITKImportFijiMontage* filter, DataStructure& dataStructure, const ITKImportFijiMontageInputValues* inputs)
  : m_Filter(filter)
  , m_DataStructure(dataStructure)
  , m_InputValues(inputs)
  , m_Allocate(inputs->allocate)
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
    m_InStream = std::ifstream(m_InputValues->inputFilePath, std::ios_base::binary);
    if(!m_InStream.is_open())
    {
      return MakeErrorResult(-2013, fmt::format("Unable to open the provided file to read at path : {}", m_InputValues->inputFilePath.string()));
    }

    if(!m_Allocate)
    {
      FillCache();
    }
    else
    {
      return ReadImages();
    }

    return {};
  }

private:
  // member variables
  ITKImportFijiMontage* m_Filter;
  DataStructure& m_DataStructure;
  std::ifstream m_InStream;
  const bool m_Allocate;
  FijiCache& m_Cache;
  const ITKImportFijiMontageInputValues* m_InputValues;

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

    // Example line to be parsed: slice_12.tif; ; (471.2965233276666, -0.522608066434236)
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
      bound.Filepath = fs::path(m_InputValues->inputFilePath.parent_path().string() + fs::path::preferred_separator + tokens[0]);

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
      bound.Origin = Point3Df(x, y, 0.0f);
      bound.Spacing = FloatVec3(1.0f, 1.0f, 1.0f);
      m_Cache.bounds.push_back(bound);
    }

    FindTileIndices();
  }

  // -----------------------------------------------------------------------------
  void FindTileIndices()
  {
    std::vector<int32> xValues(m_Cache.bounds.size());
    std::vector<int32> yValues(m_Cache.bounds.size());

    for(usize i = 0; i < m_Cache.bounds.size(); i++)
    {
      xValues[i] = static_cast<int32>(m_Cache.bounds[i].Origin[0]);
      yValues[i] = static_cast<int32>(m_Cache.bounds[i].Origin[1]);
    }

    std::map<int32, std::vector<usize>> avg_indices = MontageUtilities::Burn(k_Tolerance, xValues);
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

    avg_indices = MontageUtilities::Burn(k_Tolerance, yValues);
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

    m_Cache.maxCol = 0;
    m_Cache.maxRow = 0;
    for(auto& bound : m_Cache.bounds)
    {
      if(bound.Row < m_InputValues->rowMontageLimits[0] || bound.Row > m_InputValues->rowMontageLimits[1] || bound.Col < m_InputValues->columnMontageLimits[0] ||
         bound.Col > m_InputValues->columnMontageLimits[1])
      {
        continue;
      }

      m_Cache.maxCol = std::max(bound.Col, m_Cache.maxCol);
      m_Cache.maxRow = std::max(bound.Row, m_Cache.maxRow);
    }
  }

  // -----------------------------------------------------------------------------
  void FillCache()
  {
    // This next function will set the FileName (Partial), Row, Col for each "bound" object
    ParseConfigFile();

    Point3Df minCoord = {std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max(), std::numeric_limits<float32>::max()};

    int32 rowCountPadding = MontageUtilities::CalculatePaddingDigits(m_Cache.maxRow);
    int32 colCountPadding = MontageUtilities::CalculatePaddingDigits(m_Cache.maxCol);
    int32 charPaddingCount = std::max(rowCountPadding, colCountPadding);

    // Get the meta information from disk for each image
    for(auto& bound : m_Cache.bounds)
    {
      if(bound.Row < m_InputValues->rowMontageLimits[0] || bound.Row > m_InputValues->rowMontageLimits[1] || bound.Col < m_InputValues->columnMontageLimits[0] ||
         bound.Col > m_InputValues->columnMontageLimits[1])
      {
        continue;
      }

      std::stringstream dcNameStream;
      dcNameStream << m_InputValues->imagePrefix << bound.Filepath.filename().string().substr(0, bound.Filepath.filename().string().find(bound.Filepath.extension().string()));;
      dcNameStream << "r" << std::setw(charPaddingCount) << std::right << std::setfill('0') << bound.Row;
      dcNameStream << std::setw(1) << "c" << std::setw(charPaddingCount) << bound.Col;

      bound.ImageDataProxy = DataPath({m_InputValues->montageName, dcNameStream.str(), m_InputValues->cellAMName, m_InputValues->imageDataArrayName});

      {
        minCoord[0] = std::min(bound.Origin[0], minCoord[0]);
        minCoord[1] = std::min(bound.Origin[1], minCoord[1]);
        minCoord[2] = 0.0f;
      }
    }

    std::stringstream ss;
    ss << "Tile Column(s): " << m_Cache.maxCol + 1 << "  Tile Row(s): " << m_Cache.maxRow + 1 << "  Image Count: " << m_Cache.maxCol * m_Cache.maxRow;

    Point3Df overrideOrigin = minCoord;

    // Now adjust the origin/spacing if needed
    if(m_InputValues->changeOrigin)
    {
      overrideOrigin = m_InputValues->origin;
      Point3Df delta = {minCoord[0] - overrideOrigin[0], minCoord[1] - overrideOrigin[1], minCoord[2] - overrideOrigin[2]};
      for(auto& bound : m_Cache.bounds)
      {
        std::transform(bound.Origin.begin(), bound.Origin.end(), delta.begin(), bound.Origin.begin(), std::minus<>());
      }
    }

    ss << "\nOrigin: " << overrideOrigin[0] << ", " << overrideOrigin[1] << ", " << overrideOrigin[2];
    ss << "\nSpacing: "
       << "1.0"
       << ", "
       << "1.0"
       << ", "
       << "1.0";
    ss << "\n"
       << "Imported Columns: " << m_Cache.maxCol + 1 << "  Imported Rows: " << m_Cache.maxRow + 1 << "  Imported Image Count: " << ((m_Cache.maxCol + 1) * (m_Cache.maxRow + 1));
    m_Cache.montageInformation = ss.str();
  }

  // -----------------------------------------------------------------------------
  Result<> ReadImages()
  {
    Result<> outputResult = {};

    for(const auto& bound : m_Cache.bounds)
    {
      if(bound.Row < m_InputValues->rowMontageLimits[0] || bound.Row > m_InputValues->rowMontageLimits[1] || bound.Col < m_InputValues->columnMontageLimits[0] ||
         bound.Col > m_InputValues->columnMontageLimits[1])
      {
        continue;
      }

      m_Filter->sendUpdate(("Importing" + bound.Filepath.filename().string()));

      // Instantiate the Image Import Filter to actually read the image into a data array
      {
        // execute image import filter
        auto* filterListPtr = Application::Instance()->getFilterList();
        auto imageImportFilter = filterListPtr->createFilter(FilterTraits<ITKImageReader>::uuid);
        if(nullptr == imageImportFilter.get())
        {
          continue;
        }

        // This same filter was used to preflight so as long as nothing changes on disk this really should work....
        Arguments imageImportArgs;
        imageImportArgs.insertOrAssign(ITKImageReader::k_FileName_Key, std::make_any<fs::path>(bound.Filepath));
        imageImportArgs.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, std::make_any<DataPath>(bound.ImageDataProxy.getParent().getParent()));
        imageImportArgs.insertOrAssign(ITKImageReader::k_CellDataName_Key, std::make_any<std::string>(bound.ImageDataProxy.getParent().getTargetName()));
        imageImportArgs.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, std::make_any<DataPath>(bound.ImageDataProxy));

        auto result = imageImportFilter->execute(m_DataStructure, imageImportArgs).result;
        if(result.invalid())
        {
          outputResult = MergeResults(outputResult, result);
          continue;
        }
      }

      auto* image = m_DataStructure.getDataAs<ImageGeom>(bound.ImageDataProxy.getParent().getParent());
      image->setUnits(m_InputValues->lengthUnit);
      image->setOrigin(bound.Origin);
      image->setSpacing(bound.Spacing);

      // Now transfer the image data from the actual image data read from disk into our existing Attribute Matrix
      if(m_InputValues->convertToGrayScale)
      {
        auto* filterListPtr = Application::Instance()->getFilterList();
        if(!filterListPtr->containsPlugin(k_ComplexCorePluginId))
        {
          return MakeErrorResult(-18542, "ComplexCore was not instantiated in this instance, so color to grayscale is not a valid option.");
        }
        auto grayScaleFilter = filterListPtr->createFilter(k_ColorToGrayScaleFilterHandle);
        if(nullptr == grayScaleFilter.get())
        {
          continue;
        }

        // This same filter was used to preflight so as long as nothing changes on disk this really should work....
        Arguments colorToGrayscaleArgs;
        colorToGrayscaleArgs.insertOrAssign("conversion_algorithm", std::make_any<ChoicesParameter::ValueType>(0));
        colorToGrayscaleArgs.insertOrAssign("color_weights", std::make_any<VectorFloat32Parameter::ValueType>(m_InputValues->colorWeights));
        colorToGrayscaleArgs.insertOrAssign("input_data_array_vector", std::make_any<std::vector<DataPath>>(std::vector<DataPath>{bound.ImageDataProxy}));
        colorToGrayscaleArgs.insertOrAssign("output_array_prefix", std::make_any<std::string>("gray"));

        // Run grayscale filter and process results and messages
        auto result = grayScaleFilter->execute(m_DataStructure, colorToGrayscaleArgs).result;
        if(result.invalid())
        {
          outputResult = MergeResults(outputResult, result);
          continue;
        }

        // deletion of non-grayscale array
        DataObject::IdType id;
        { // scoped for safety since this reference will be nonexistent in a moment
          auto& oldArray = m_DataStructure.getDataRefAs<IDataArray>(bound.ImageDataProxy);
          id = oldArray.getId();
        }
        m_DataStructure.removeData(id);

        // rename grayscale array to reflect original
        {
          auto& gray = m_DataStructure.getDataRefAs<IDataArray>(bound.ImageDataProxy.getParent().createChildPath("gray" + bound.ImageDataProxy.getTargetName()));
          if(!gray.canRename(bound.ImageDataProxy.getTargetName()))
          {
            return MakeErrorResult(-18543, fmt::format("Unable to rename the grayscale array to {}", bound.ImageDataProxy.getTargetName()));
          }
          gray.rename(bound.ImageDataProxy.getTargetName());
        }
      }

      auto& gridMontage = m_DataStructure.getDataRefAs<GridMontage>(DataPath({m_InputValues->montageName}));

      GridTileIndex gridIndex = gridMontage.getTileIndex(bound.Row, bound.Col);
      // Set the montage's DataContainer for the current index
      gridMontage.setGeometry(&gridIndex, image);
    }

    return outputResult;
  }
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
  return IOHandler(this, m_DataStructure, m_InputValues)();
}
