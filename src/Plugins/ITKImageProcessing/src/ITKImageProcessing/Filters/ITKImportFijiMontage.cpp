#include "ITKImportFijiMontage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateDataGroupAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

namespace
{
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ITKImportFijiMontage::generateCache()
{
  // This next function will set the FileName (Partial), Row, Col for each "bound" object
  std::vector<BoundsType> bounds = parseConfigFile(getInputFile());

  FloatVec3Type minCoord = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  FloatVec3Type minSpacing = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
  std::vector<ImageGeom::Pointer> geometries;

  // Get the meta information from disk for each image
  for(auto& bound : bounds)
  {
    // This will update the FileName to the absolutePath
    QFileInfo fi(getInputFile());
    QString absolutePath = fi.absolutePath() + QDir::separator() + bound.Filename;
    bound.Filename = absolutePath;
    bound.LengthUnit = static_cast<IGeometry::LengthUnit>(getLengthUnit());

    DataArrayPath dap(::k_DCName, ITKImageProcessing::Montage::k_AMName, ITKImageProcessing::Montage::k_AAName);
    AbstractFilter::Pointer imageImportFilter = MontageImportHelper::CreateImageImportFilter(this, bound.Filename, dap);
    imageImportFilter->preflight();
    if(imageImportFilter->getErrorCode() < 0)
    {
      setErrorCondition(imageImportFilter->getErrorCode(), "Error Preflighting Image Import Filter.");
      continue;
    }

    DataContainerArray::Pointer importImageDca = imageImportFilter->getDataContainerArray();
    {
      DataContainer::Pointer fromDc = importImageDca->getDataContainer(::k_DCName);
      AttributeMatrix::Pointer fromCellAttrMat = fromDc->getAttributeMatrix(ITKImageProcessing::Montage::k_AMName);
      IDataArray::Pointer fromImageData = fromCellAttrMat->getAttributeArray(ITKImageProcessing::Montage::k_AAName);
      fromImageData->setName(getImageDataArrayName());
      bound.ImageDataProxy = fromImageData;

      // Set the Origin based on the values from the Fiji Config File
      // Set the spacing to the default 1,1,1
      ImageGeom::Pointer imageGeom = fromDc->getGeometryAs<ImageGeom>();
      bound.Dims = imageGeom->getDimensions();
      imageGeom->setSpacing(bound.Spacing);
      imageGeom->setOrigin(bound.Origin);
      minSpacing = imageGeom->getSpacing();
      //
      minCoord[0] = std::min(bound.Origin[0], minCoord[0]);
      minCoord[1] = std::min(bound.Origin[1], minCoord[1]);
      minCoord[2] = 0.0f;
      geometries.push_back(imageGeom);
    }

    if(getConvertToGrayScale())
    {
      DataArrayPath daPath(::k_DCName, ITKImageProcessing::Montage::k_AMName, ITKImageProcessing::Montage::k_AAName);
      AbstractFilter::Pointer grayScaleFilter = MontageImportHelper::CreateColorToGrayScaleFilter(this, daPath, getColorWeights(), ITKImageProcessing::Montage::k_GrayScaleTempArrayName);
      grayScaleFilter->setDataContainerArray(importImageDca);
      grayScaleFilter->preflight();
      if(grayScaleFilter->getErrorCode() < 0)
      {
        setErrorCondition(grayScaleFilter->getErrorCode(), "Error Preflighting Color to GrayScale filter");
        continue;
      }

      DataContainerArray::Pointer colorToGrayDca = grayScaleFilter->getDataContainerArray();
      DataContainer::Pointer fromDc = colorToGrayDca->getDataContainer(::k_DCName);
      AttributeMatrix::Pointer fromCellAttrMat = fromDc->getAttributeMatrix(ITKImageProcessing::Montage::k_AMName);
      // Remove the RGB Attribute Array so we can rename the gray scale AttributeArray
      IDataArray::Pointer rgbImageArray = fromCellAttrMat->removeAttributeArray(ITKImageProcessing::Montage::k_AAName);
      QString grayScaleArrayName = ITKImageProcessing::Montage::k_GrayScaleTempArrayName + ITKImageProcessing::Montage::k_AAName;
      IDataArray::Pointer fromGrayScaleData = fromCellAttrMat->removeAttributeArray(grayScaleArrayName);
      fromGrayScaleData->setName(getImageDataArrayName());
      bound.ImageDataProxy = fromGrayScaleData;
    }

    d_ptr->m_MaxCol = std::max(bound.Col, d_ptr->m_MaxCol);
    d_ptr->m_MaxRow = std::max(bound.Row, d_ptr->m_MaxRow);
  }

  QString montageInfo;
  QTextStream ss(&montageInfo);
  ss << "Tile Column(s): " << m_ColumnCount - 1 << "  Tile Row(s): " << m_RowCount - 1 << "  Image Count: " << m_ColumnCount * m_RowCount;

  FloatVec3Type overrideOrigin = minCoord;
  FloatVec3Type overrideSpacing = minSpacing;

  // Now adjust the origin/spacing if needed
  if(getChangeOrigin() || getChangeSpacing())
  {

    if(getChangeOrigin())
    {
      overrideOrigin = m_Origin;
    }
    if(getChangeSpacing())
    {
      overrideSpacing = m_Spacing;
    }
    FloatVec3Type delta = {minCoord[0] - overrideOrigin[0], minCoord[1] - overrideOrigin[1], minCoord[2] - overrideOrigin[2]};
    for(size_t i = 0; i < geometries.size(); i++)
    {
      ImageGeom::Pointer imageGeom = geometries[i];
      BoundsType& bound = bounds.at(i);
      std::transform(bound.Origin.begin(), bound.Origin.end(), delta.begin(), bound.Origin.begin(), std::minus<>());
      imageGeom->setOrigin(bound.Origin); // Sync up the ImageGeom with the calculated values
      imageGeom->setSpacing(overrideSpacing);
    }
  }
  ss << "\nOrigin: " << overrideOrigin[0] << ", " << overrideOrigin[1] << ", " << overrideOrigin[2];
  ss << "\nSpacing: " << overrideSpacing[0] << ", " << overrideSpacing[1] << ", " << overrideSpacing[2];
  ss << ITKImageProcessing::Montage::k_MontageInfoReplaceKeyword;
  d_ptr->m_MontageInformation = montageInfo;
  setBoundsCache(bounds);
}

// -----------------------------------------------------------------------------
void ITKImportFijiMontage::generateDataStructure()
{
  std::vector<BoundsType>& bounds = d_ptr->m_BoundsCache;

  DataContainerArray::Pointer dca = getDataContainerArray();

  GridMontage::Pointer gridMontage = GridMontage::New(getMontageName(), m_RowCount, m_ColumnCount);

  int32_t rowCountPadding = MetaXmlUtils::CalculatePaddingDigits(m_RowCount);
  int32_t colCountPadding = MetaXmlUtils::CalculatePaddingDigits(m_ColumnCount);
  int charPaddingCount = std::max(rowCountPadding, colCountPadding);

  for(const auto& bound : bounds)
  {
    if(bound.Row < m_MontageStart[1] || bound.Row > m_MontageEnd[1] || bound.Col < m_MontageStart[0] || bound.Col > m_MontageEnd[0])
    {
      continue;
    }

    // Create our DataContainer Name using a Prefix and a rXXcYY format.
    QString dcName = getDataContainerPath().getDataContainerName();
    QTextStream dcNameStream(&dcName);
    dcNameStream << "r";
    dcNameStream.setFieldWidth(charPaddingCount);
    dcNameStream.setFieldAlignment(QTextStream::AlignRight);
    dcNameStream.setPadChar('0');
    dcNameStream << bound.Row;
    dcNameStream.setFieldWidth(0);
    dcNameStream << "c";
    dcNameStream.setFieldWidth(charPaddingCount);
    dcNameStream << bound.Col;

    // Create the DataContainer with a name based on the ROW & COLUMN indices
    DataContainer::Pointer dc = dca->createNonPrereqDataContainer(this, dcName);
    if(getErrorCode() < 0)
    {
      continue;
    }

    // Create the Image Geometry
    ImageGeom::Pointer image = ImageGeom::CreateGeometry(dcName);
    image->setDimensions(bound.Dims);
    image->setOrigin(bound.Origin);
    image->setSpacing(bound.Spacing);
    image->setUnits(bound.LengthUnit);

    dc->setGeometry(image);

    GridTileIndex gridIndex = gridMontage->getTileIndex(bound.Row, bound.Col);
    // Set the montage's DataContainer for the current index
    gridMontage->setDataContainer(gridIndex, dc);

    using StdVecSizeType = std::vector<size_t>;
    // Create the Cell Attribute Matrix into which the image data would be read
    AttributeMatrix::Pointer cellAttrMat = AttributeMatrix::New(bound.Dims.toContainer<StdVecSizeType>(), getCellAttributeMatrixName(), AttributeMatrix::Type::Cell);
    dc->addOrReplaceAttributeMatrix(cellAttrMat);
    cellAttrMat->addOrReplaceAttributeArray(bound.ImageDataProxy);
  }
  getDataContainerArray()->addOrReplaceMontage(gridMontage);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ITKImportFijiMontage::readImages()
{
  std::vector<BoundsType>& bounds = d_ptr->m_BoundsCache;
  // Import Each Image
  DataContainerArray::Pointer dca = getDataContainerArray();

  //  int imageCountPadding = MetaXmlUtils::CalculatePaddingDigits(bounds.size());
  int32_t rowCountPadding = MetaXmlUtils::CalculatePaddingDigits(m_RowCount);
  int32_t colCountPadding = MetaXmlUtils::CalculatePaddingDigits(m_ColumnCount);
  int charPaddingCount = std::max(rowCountPadding, colCountPadding);

  for(const auto& bound : bounds)
  {
    if(bound.Row < m_MontageStart[1] || bound.Row > m_MontageEnd[1] || bound.Col < m_MontageStart[0] || bound.Col > m_MontageEnd[0])
    {
      continue;
    }

    QString msg;
    QTextStream out(&msg);
    out << "Importing " << bound.Filename;
    notifyStatusMessage(msg);
    // Create our DataContainer Name using a Prefix and a rXXcYY format.
    QString dcName = getDataContainerPath().getDataContainerName();
    QTextStream dcNameStream(&dcName);
    dcNameStream << "r";
    dcNameStream.setFieldWidth(charPaddingCount);
    dcNameStream.setFieldAlignment(QTextStream::AlignRight);
    dcNameStream.setPadChar('0');
    dcNameStream << bound.Row;
    dcNameStream.setFieldWidth(0);
    dcNameStream << "c";
    dcNameStream.setFieldWidth(charPaddingCount);
    dcNameStream << bound.Col;

    // The DataContainer with a name based on the ROW & COLUMN indices is already created in the preflight
    DataContainer::Pointer dc = dca->getDataContainer(dcName);
    // So is the Geometry
    ImageGeom::Pointer image = dc->getGeometryAs<ImageGeom>();

    image->setUnits(static_cast<IGeometry::LengthUnit>(m_LengthUnit));

    // Create the Image Geometry
    SizeVec3Type dims = image->getDimensions();
    // FloatVec3Type origin = image->getOrigin();
    // FloatVec3Type spacing = image->getSpacing();

    std::vector<size_t> tDims = {dims[0], dims[1], dims[2]};
    // The Cell AttributeMatrix is also already created at this point
    AttributeMatrix::Pointer cellAttrMat = dc->getAttributeMatrix(getCellAttributeMatrixName());
    // Instantiate the Image Import Filter to actually read the image into a data array
    DataArrayPath dap(::k_DCName, ITKImageProcessing::Montage::k_AMName, getImageDataArrayName()); // This is just a temp path for the subfilter to use
    AbstractFilter::Pointer imageImportFilter = MontageImportHelper::CreateImageImportFilter(this, bound.Filename, dap);
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
    DataContainerArray::Pointer importImageDca = imageImportFilter->getDataContainerArray();
    DataContainer::Pointer fromDc = importImageDca->getDataContainer(::k_DCName);
    AttributeMatrix::Pointer fromCellAttrMat = fromDc->getAttributeMatrix(ITKImageProcessing::Montage::k_AMName);
    // IDataArray::Pointer fromImageData = fromCellAttrMat->getAttributeArray(getImageDataArrayName());

    if(getConvertToGrayScale())
    {
      AbstractFilter::Pointer grayScaleFilter = MontageImportHelper::CreateColorToGrayScaleFilter(this, dap, getColorWeights(), ITKImageProcessing::Montage::k_GrayScaleTempArrayName);
      grayScaleFilter->setDataContainerArray(importImageDca); // Use the Data COntainer array that was use for the import. It is setup and ready to go
      connect(grayScaleFilter.get(), SIGNAL(messageGenerated(const AbstractMessage::Pointer&)), this, SIGNAL(messageGenerated(const AbstractMessage::Pointer&)));
      grayScaleFilter->execute();
      if(grayScaleFilter->getErrorCode() < 0)
      {
        setErrorCondition(grayScaleFilter->getErrorCode(), "Error Executing Color to GrayScale filter");
        continue;
      }

      DataContainerArray::Pointer c2gDca = grayScaleFilter->getDataContainerArray();
      DataContainer::Pointer c2gDc = c2gDca->getDataContainer(::k_DCName);
      AttributeMatrix::Pointer c2gAttrMat = c2gDc->getAttributeMatrix(ITKImageProcessing::Montage::k_AMName);

      QString grayScaleArrayName = ITKImageProcessing::Montage::k_GrayScaleTempArrayName + getImageDataArrayName();
      // IDataArray::Pointer fromGrayScaleData = fromCellAttrMat->getAttributeArray(grayScaleArrayName);

      IDataArray::Pointer rgbImageArray = c2gAttrMat->removeAttributeArray(ITKImageProcessing::Montage::k_AAName);
      IDataArray::Pointer gray = c2gAttrMat->removeAttributeArray(grayScaleArrayName);
      gray->setName(getImageDataArrayName());
      cellAttrMat->addOrReplaceAttributeArray(gray);
    }
    else
    {
      // Copy the IDataArray (which contains the image data) from the temp data container array into our persistent data structure
      IDataArray::Pointer gray = fromCellAttrMat->removeAttributeArray(getImageDataArrayName());
      cellAttrMat->addOrReplaceAttributeArray(gray);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ITKImportFijiMontage::flushCache()
{
  setTimeStamp_Cache(QDateTime());

  d_ptr->m_InputFile_Cache = "";
  d_ptr->m_DataContainerPath = DataArrayPath();
  d_ptr->m_CellAttributeMatrixName = "";
  d_ptr->m_ImageDataArrayName = "";
  d_ptr->m_ChangeOrigin = false;
  d_ptr->m_ChangeSpacing = false;
  d_ptr->m_ConvertToGrayScale = false;
  d_ptr->m_Origin = FloatVec3Type(0.0f, 0.0f, 0.0f);
  d_ptr->m_Spacing = FloatVec3Type(1.0f, 1.0f, 1.0f);
  d_ptr->m_ColorWeights = FloatVec3Type(0.2125f, 0.7154f, 0.0721f);
  d_ptr->m_MaxCol = 0;
  d_ptr->m_MaxRow = 0;
  d_ptr->m_BoundsCache.clear();
}

// -----------------------------------------------------------------------------
QString ITKImportFijiMontage::getMontageInformation()
{
  QString info = d_ptr->m_MontageInformation;
  QString montageInfo;
  QTextStream ss(&montageInfo);
  int32_t importedCols = m_MontageEnd[0] - m_MontageStart[0] + 1;
  int32_t importedRows = m_MontageEnd[1] - m_MontageStart[1] + 1;
  ss << "\n"
     << "Imported Columns: " << importedCols << "  Imported Rows: " << importedRows << "  Imported Image Count: " << (importedCols * importedRows);
  info = info.replace(ITKImageProcessing::Montage::k_MontageInfoReplaceKeyword, montageInfo);
  return info;
}

// -----------------------------------------------------------------------------
void ITKImportFijiMontage::findTileIndices(int32_t tolerance, std::vector<ITKImportFijiMontage::BoundsType>& bounds)
{
  std::vector<int32_t> xValues(bounds.size());
  std::vector<int32_t> yValues(bounds.size());

  for(size_t i = 0; i < bounds.size(); i++)
  {
    xValues[i] = bounds.at(i).Origin[0];
    yValues[i] = bounds.at(i).Origin[1];
  }

  std::map<int32_t, std::vector<size_t>> avg_indices = MontageImportHelper::Burn(tolerance, xValues);
  int32_t index = 0;
  for(auto& iter : avg_indices)
  {
    const std::vector<size_t>& indices = iter.second;
    for(const auto& i : indices)
    {
      bounds.at(i).Col = index;
    }
    index++;
  }
  m_ColumnCount = index;

  avg_indices = MontageImportHelper::Burn(tolerance, yValues);
  index = 0;
  for(auto& iter : avg_indices)
  {
    const std::vector<size_t>& indices = iter.second;
    for(const auto& i : indices)
    {
      bounds.at(i).Row = index;
    }
    index++;
  }
  m_RowCount = index;
}

// -----------------------------------------------------------------------------
std::vector<ITKImportFijiMontage::BoundsType> ITKImportFijiMontage::parseConfigFile(const QString& filePath)
{
  QString contents;

  // Read the Source File
  QFile source(filePath);
  source.open(QFile::ReadOnly);
  contents = source.readAll();
  source.close();

  QStringList list = contents.split(QRegExp("\\n"));
  QStringListIterator sourceLines(list);
  bool dimFound = false;
  bool dataFound = false;

  while(sourceLines.hasNext())
  {
    QString line = sourceLines.next().trimmed();

    if(line.startsWith("dim =")) // found the dimensions
    {
      dimFound = true;
      // Should check that the value = 2
    }
    if(line.startsWith("# Define the image coordinates"))
    {
      // Found the start of the data
      dataFound = true;
      break;
    }
    if(line.startsWith("#")) // comment line
    {
      continue;
    }
  }

  std::vector<ITKImportFijiMontage::BoundsType> bounds;
  if(!dimFound || !dataFound)
  {
    return bounds;
  }

  // slice_12.tif; ; (471.2965233276666, -0.522608066434236)
  while(sourceLines.hasNext())
  {
    QString line = sourceLines.next().trimmed();
    if(line.isEmpty())
    {
      continue;
    }
    QStringList tokens = line.split(";");
    if(tokens.count() != 3)
    {
      continue;
    }
    ITKImportFijiMontage::BoundsType bound;
    bound.Filename = tokens.at(0);

    QString coords = tokens.at(2).trimmed();
    coords = coords.replace("(", "");
    coords = coords.replace(")", "");
    tokens = coords.split(",");
    if(tokens.count() != 2)
    {
      continue;
    }
    float x = tokens.at(0).toFloat();
    float y = tokens.at(1).toFloat();
    bound.Origin = FloatVec3Type(x, y, 0.0f);
    bound.Spacing = FloatVec3Type(1.0f, 1.0f, 1.0f);
    bounds.push_back(bound);
  }

  findTileIndices(m_Tolerance, bounds);

  return bounds;
}
}

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKImportFijiMontage::name() const
{
  return FilterTraits<ITKImportFijiMontage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKImportFijiMontage::className() const
{
  return FilterTraits<ITKImportFijiMontage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImportFijiMontage::uuid() const
{
  return FilterTraits<ITKImportFijiMontage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImportFijiMontage::humanName() const
{
  return "ITK Import Fiji Montage";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImportFijiMontage::defaultTags() const
{
  return {"IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ITKImportFijiMontage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Fiji Configuration File", "", fs::path("<default file to read goes here>"), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<StringParameter>(k_MontageName_Key, "Name of Created Montage", "", "SomeString"));
  params.insert(std::make_unique<VectorInt32Parameter>(k_ColumnMontageLimits_Key, "Montage Column Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<VectorInt32Parameter>(k_RowMontageLimits_Key, "Montage Row Start/End [Inclusive, Zero Based]", "", std::vector<int32>(2), std::vector<std::string>(2)));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "", 0, ChoicesParameter::Choices{"Option 1", "Option 2", "Option 3"}));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Change Origin", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ConvertToGrayScale_Key, "Convert To GrayScale", "", false));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ColorWeights_Key, "Color Weighting", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_DataContainerPath_Key, "DataContainer Prefix", "", DataPath{}));
  params.insert(std::make_unique<ArrayCreationParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "", DataPath{}));
  params.insert(std::make_unique<StringParameter>(k_ImageDataArrayName_Key, "Image DataArray Name", "", "SomeString"));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, true);
  params.linkParameters(k_ConvertToGrayScale_Key, k_ColorWeights_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImportFijiMontage::clone() const
{
  return std::make_unique<ITKImportFijiMontage>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImportFijiMontage::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pChangeOriginValue = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pConvertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // If the filter needs to pass back some updated values via a key:value string:string set of values
  // you can declare and update that string here.
  // These variables should be updated with the latest data generated for each variable during preflight.
  // These will be returned through the preflightResult variable to the
  // user interface. You could make these member variables instead if needed.
  std::string montageInformation;

  // If this filter makes changes to the DataStructure in the form of
  // creating/deleting/moving/renaming DataGroups, Geometries, DataArrays then you
  // will need to use one of the `*Actions` classes located in complex/Filter/Actions
  // to relay that information to the preflight and execute methods. This is done by
  // creating an instance of the Action class and then storing it in the resultOutputActions variable.
  // This is done through a `push_back()` method combined with a `std::move()`. For the
  // newly initiated to `std::move` once that code is executed what was once inside the Action class
  // instance variable is *no longer there*. The memory has been moved. If you try to access that
  // variable after this line you will probably get a crash or have subtle bugs. To ensure that this
  // does not happen we suggest using braces `{}` to scope each of the action's declaration and store
  // so that the programmer is not tempted to use the action instance past where it should be used.
  // You have to create your own Actions class if there isn't something specific for your filter's needs
  // These are some proposed Actions based on the FilterParameters used. Please check them for correctness.
  {
    auto createDataGroupAction = std::make_unique<CreateDataGroupAction>(pDataContainerPathValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  // Store the preflight updated value(s) into the preflightUpdatedValues vector using
  // the appropriate methods.
  // These values should have been updated during the preflightImpl(...) method
  preflightUpdatedValues.push_back({"MontageInformation", montageInformation});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKImportFijiMontage::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pMontageNameValue = filterArgs.value<StringParameter::ValueType>(k_MontageName_Key);
  auto pColumnMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_ColumnMontageLimits_Key);
  auto pRowMontageLimitsValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_RowMontageLimits_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pChangeOriginValue = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pConvertToGrayScaleValue = filterArgs.value<bool>(k_ConvertToGrayScale_Key);
  auto pColorWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  auto pDataContainerPathValue = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_ImageDataArrayName_Key);

  return {};
}
} // namespace complex
