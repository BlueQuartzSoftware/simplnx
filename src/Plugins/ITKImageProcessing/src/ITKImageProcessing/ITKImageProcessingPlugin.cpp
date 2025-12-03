#include "ITKImageProcessingPlugin.hpp"

#include "ITKImageProcessing/ITKImageProcessing_filter_registration.hpp"
#include "ITKImageProcessingLegacyUUIDMapping.hpp"

#include <itkBMPImageIOFactory.h>
#include <itkBioRadImageIOFactory.h>
#include <itkGE4ImageIOFactory.h>
#include <itkGE5ImageIOFactory.h>
#include <itkGiplImageIOFactory.h>
#include <itkJPEGImageIOFactory.h>
#include <itkMRCImageIOFactory.h>
#include <itkMetaImageIOFactory.h>
#include <itkNiftiImageIOFactory.h>
#include <itkNrrdImageIOFactory.h>
#include <itkPNGImageIOFactory.h>
#include <itkStimulateImageIOFactory.h>
#include <itkTIFFImageIOFactory.h>
#include <itkVTKImageIOFactory.h>

using namespace nx::core;

namespace
{
// Plugin Uuid
constexpr AbstractPlugin::IdType k_ID = *Uuid::FromString("115b0d10-ab97-5a18-88e8-80d35056a28e");
} // namespace

ITKImageProcessingPlugin::ITKImageProcessingPlugin()
: AbstractPlugin(k_ID, "ITKImageProcessing", "Filters that wrap the ITK Software library. ITK is located at https://github.com/InsightSoftwareConsortium/ITK", "BlueQuartz Software")
{
  std::vector<::FilterCreationFunc> filterFuncs = ::GetPluginFilterList();
  for(const auto& filterFunc : filterFuncs)
  {
    addFilter(filterFunc);
  }

  static bool s_IsRegistered = false;
  if(!s_IsRegistered)
  {
    RegisterITKImageIO();

    /*
      2025-12-02
      Tested with ITK 5.4.5 and libtiff 4.7.1
      Primarily affects DREAM3DNX built for conda distribution
      When ITKIOTIFF is built with an external libtiff, it misconfigures on Windows. It checks for the existence of TIFFFieldReadCount using CMake's check_type_size().
      check_type_size() uses try_compile() which uses sizeof(). It is against the standard to do `sizeof(func)`. The correct way is to do `sizeof(&func)`. gcc and clang
      allow the first version without -pedantic but msvc rejects it. This leads to a situation where ITK_TIFF_HAS_TIFFFieldReadCount is not defined but
      ITK_TIFF_HAS_TIFFField is. Then in itkTIFFImageIO.cxx, different code is selected to access the field name. For certain files, this can come back as nullptr.
      For the particular file we tested with, the field data type (7) was unsupported by ITK. Then we end up at the default case of a switch where ITK attempts to print
      out the field name and data type to say that it isn't supported. But since the field name is nullptr it crashes.
      ITK 6 looks to avoid this issue by increasing the minimum required libtiff version removing the need for workarounds.

      Here we disable the global warning display on Windows which prevents the nullptr from being accessed. This prevents *all* warnings from being printed,
      but simplnx will primarily be used with DREAM3DNX which, as a GUI application on Windows, doesn't have a terminal by default to print to anyways.
      Once ITK fixes this issue, this code can be removed.
    */
#ifdef _WIN32
    itk::Object::GlobalWarningDisplayOff();
#endif

    s_IsRegistered = true;
  }
}

ITKImageProcessingPlugin::~ITKImageProcessingPlugin() noexcept = default;

void ITKImageProcessingPlugin::RegisterITKImageIO()
{
  itk::JPEGImageIOFactory::RegisterOneFactory();
  itk::NrrdImageIOFactory::RegisterOneFactory();
  itk::PNGImageIOFactory::RegisterOneFactory();
  itk::TIFFImageIOFactory::RegisterOneFactory();
  itk::JPEGImageIOFactory::RegisterOneFactory();
  itk::BMPImageIOFactory::RegisterOneFactory();
  itk::MetaImageIOFactory::RegisterOneFactory();
  itk::NiftiImageIOFactory::RegisterOneFactory();
  itk::GiplImageIOFactory::RegisterOneFactory();
  itk::VTKImageIOFactory::RegisterOneFactory();
  itk::StimulateImageIOFactory::RegisterOneFactory();
  itk::BioRadImageIOFactory::RegisterOneFactory();
  itk::GE4ImageIOFactory::RegisterOneFactory();
  itk::GE5ImageIOFactory::RegisterOneFactory();
  itk::MRCImageIOFactory::RegisterOneFactory();
}

AbstractPlugin::SIMPLMapType ITKImageProcessingPlugin::getSimplToSimplnxMap() const
{
  return nx::core::k_SIMPL_to_ITKImageProcessing;
}

std::vector<std::string> ITKImageProcessingPlugin::GetList2DSupportedFileExtensions()
{
  return {".png", ".tif", ".jpg", ".jpeg", ".bmp", ".mha"};
}

SIMPLNX_DEF_PLUGIN(ITKImageProcessingPlugin)
