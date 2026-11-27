

#include <catch2/catch.hpp>

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"
#include "simplnx/unit_test/simplnx_test_dirs.hpp"

#include <fstream>
#include <map>
#include <sstream>
#include <string>

using namespace nx::core;

namespace
{
std::map<nx::core::Uuid, std::string> s_ParameterMap;

void AddParameterTrait(const char* parameterName, const char* uuid)
{
  const auto uuidResult = nx::core::Uuid::FromString(uuid);
  if(uuidResult.has_value())
  {
    s_ParameterMap[uuidResult.value()] = parameterName;
  }
}
} // namespace

static void GenerateParameterList()
{
  ::s_ParameterMap.clear();
  AddParameterTrait("simplnx.SeparatorParameter", "e6936d18-7476-4855-9e13-e795d717c50f");
  AddParameterTrait("simplnx.OEMEbsdScanSelectionParameter", "3935c833-aa51-4a58-81e9-3a51972c05ea");
  AddParameterTrait("simplnx.ReadH5EbsdFileParameter", "FAC15aa6-b367-508e-bf73-94ab6be0058b");
  AddParameterTrait("simplnx.NumericTypeParameter", "a8ff9dbd-45e7-4ed6-8537-12dd53069bce");
  AddParameterTrait("simplnx.StringParameter", "5d6d1868-05f8-11ec-9a03-0242ac130003");
  AddParameterTrait("simplnx.DataStoreFormatParameter", "cfd5c150-2938-42a7-b023-4a9288fb6899");
  AddParameterTrait("simplnx.MultiPathSelectionParameter", "b5632f4f-fc13-4234-beb2-8fd8820eb6b6");
  AddParameterTrait("simplnx.DataTypeParameter", "d31358d5-3253-4c69-aff0-eb98618f851b");
  AddParameterTrait("simplnx.EnsembleInfoParameter", "10d3924f-b4c9-4e06-9225-ce11ec8dff89");
  AddParameterTrait("simplnx.ArrayThresholdsParameter", "e93251bc-cdad-44c2-9332-58fe26aedfbe");
  AddParameterTrait("simplnx.CreateColorMapParameter", "7b0e5b25-564e-4797-b154-4324ef276bf0");
  AddParameterTrait("simplnx.DataObjectNameParameter", "fbc89375-3ca4-4eb2-8257-aad9bf8e1c94");
  AddParameterTrait("simplnx.NeighborListSelectionParameter", "ab0b7a7f-f9ab-4e6f-99b5-610e7b69fc5b");
  AddParameterTrait("simplnx.ChoicesParameter", "ee4d5ce2-9582-48fa-b182-8a766ce0feff");
  AddParameterTrait("simplnx.GeneratedFileListParameter", "aac15aa6-b367-508e-bf73-94ab6be0058b");
  AddParameterTrait("simplnx.DataPathSelectionParameter", "cd12d081-fbf0-46c4-8f4a-15e2e06e98b8");
  AddParameterTrait("simplnx.CalculatorParameter", "ba2d4937-dbec-5536-8c5c-c0a406e80f77");
  AddParameterTrait("simplnx.CropGeometryParameter", "32b03ebf-02a5-40c7-a41c-2380722caeb7");
  AddParameterTrait("simplnx.ReadCSVFileParameter", "4f6d6a33-48da-427a-8b17-61e07d1d5b45");

  AddParameterTrait("simplnx.MultiArraySelectionParameter", "d11e0bd8-f227-4fd1-b618-b6f16b259fc8");
  AddParameterTrait("simplnx.ArraySelectionParameter", "ab047a7f-f9ab-4e6f-99b5-610e7b69fc5b");
  AddParameterTrait("simplnx.DataGroupSelectionParameter", "bff3d4ac-04a6-5251-b178-4f83f7865074");
  AddParameterTrait("simplnx.AttributeMatrixSelectionParameter", "a3619d74-a1d9-4bc2-9e03-ca001d65b119");
  AddParameterTrait("simplnx.GeometrySelectionParameter", "3804cd7f-4ee4-400f-80ad-c5af17735de2");

  AddParameterTrait("simplnx.DataGroupCreationParameter", "bff2d4ac-04a6-5251-b188-4f83f7865074");
  AddParameterTrait("simplnx.ArrayCreationParameter", "ab047a7d-f81b-4e6f-99b5-610e7b69fc5b");

  AddParameterTrait("simplnx.FileSystemPathParameter", "f9a93f3d-21ef-43a1-a958-e57cbf3b2909");
  AddParameterTrait("simplnx.BoolParameter", "b6936d18-7476-4855-9e13-e795d717c50f");
  AddParameterTrait("simplnx.ReadHDF5DatasetParameter", "32e83e13-ee4c-494e-8bab-4e699df74a5a");
  AddParameterTrait("simplnx.Dream3dImportParameter", "170a257d-5952-4854-9a91-4281cd06f4f5");
  AddParameterTrait("simplnx.DynamicTableParameter", "eea76f1a-fab9-4704-8da5-4c21057cf44e");

  AddParameterTrait("simplnx.Int8Parameter", "cae73834-68f8-4235-b010-8bea87d8ff7a");
  AddParameterTrait("simplnx.UInt8Parameter", "6c3efeff-ce8f-47c0-83d1-262f2b2dd6cc");
  AddParameterTrait("simplnx.Int16Parameter", "44ae56e8-e6e7-4e4d-8128-dd3dc2c6696e");
  AddParameterTrait("simplnx.UInt16Parameter", "156a6f46-77e5-41d8-8f5a-65ba1da52f2a");
  AddParameterTrait("simplnx.Int32Parameter", "21acff45-a653-45db-a0d1-f43cd344b93a");
  AddParameterTrait("simplnx.UInt32Parameter", "e9521130-276c-40c7-95d7-0b4cb4f80649");
  AddParameterTrait("simplnx.Int64Parameter", "b2039349-bd3a-4dbb-93d2-b4b5c633e697");
  AddParameterTrait("simplnx.UInt64Parameter", "36d91b23-5500-4ed4-bdf3-d680f54ee5d1");
  AddParameterTrait("simplnx.Float32Parameter", "e4452dfe-2f70-4833-819e-0cbbec21289b");
  AddParameterTrait("simplnx.Float64Parameter", "f2a18fff-a095-47d7-b436-ede41b5ea21a");

  AddParameterTrait("simplnx.VectorInt8Parameter", "9f5f9683-e492-4a79-8378-79d727b2356a");
  AddParameterTrait("simplnx.VectorUInt8Parameter", "bff78ff3-35ef-482a-b3b1-df8806e7f7ef");
  AddParameterTrait("simplnx.VectorInt16Parameter", "43810a29-1a5f-4472-bec6-41de9ffe27f7");
  AddParameterTrait("simplnx.VectorUInt16Parameter", "2f1ba2f4-c5d5-403c-8b90-0bf60d2bde9b");
  AddParameterTrait("simplnx.VectorInt32Parameter", "d3188e18-e383-4727-ab32-88b5fda56ae8");
  AddParameterTrait("simplnx.VectorUInt32Parameter", "37322aa6-1a2f-4ecb-9aa1-8922d7ac1e49");
  AddParameterTrait("simplnx.VectorInt64Parameter", "4ceaffc1-7326-4f65-a33a-eae263dc22d1");
  AddParameterTrait("simplnx.VectorUInt64Parameter", "17309744-c4e8-4d1e-807e-e7012387f1ec");
  AddParameterTrait("simplnx.VectorFloat32Parameter", "88f231a1-7956-41f5-98b7-4471705d2805");
  AddParameterTrait("simplnx.VectorFloat64Parameter", "57cbdfdf-9d1a-4de8-95d7-71d0c01c5c96");
}

TEST_CASE("nx::core::FilterValidation::Filter Parameter Keys", "[simplnx][Filter]")
{
  unit_test::LoadPlugins();

  GenerateParameterList();

  auto* filterListPtr = Application::Instance()->getFilterList();
  const auto pluginListPtr = Application::Instance()->getPluginList();

  std::stringstream output;
  std::ofstream logFile(fmt::format("{}/human_names.log", unit_test::k_BinaryDir.view()), std::ios_base::trunc | std::ios_base::binary);
  std::set<std::string> uniqueSepNames;

  // Loop on each Plugin
  for(const auto& plugin : pluginListPtr)
  {
    const std::string plugName = plugin->getName();

    const auto& pluginFilterHandles = plugin->getFilterHandles();

    // Loop on each Filter
    for(const auto& filterHandle : pluginFilterHandles)
    {
      const std::string filterClassName = filterHandle.getClassName();
      IFilter::UniquePointerType filter = filterListPtr->createFilter(filterHandle);
      const auto& parameters = filter->parameters();

      if(nx::core::string_utilities::StartsWith(filterClassName, "Calculate") || nx::core::string_utilities::StartsWith(filterClassName, "Find") ||
         nx::core::string_utilities::StartsWith(filterClassName, "Generate") || nx::core::string_utilities::StartsWith(filterClassName, "Calculate"))
      {
        uniqueSepNames.insert(fmt::format("{},{}", plugName, filterClassName));
      }

      // Loop over each Parameter
      for(const auto& parameter : parameters)
      {
        auto const& paramValue = parameter.second;

        for(const auto& letter : paramValue->name())
        {
          if(::isupper(letter) != 0)
          {
            output << plugName << "->" << filter->name() << "->" << paramValue->name() << ". This parameter key has CAPITAL Letters. All parameter keys should be 'lower_snake_case' style\n";
            break;
          }
        }

        std::string pType = s_ParameterMap[paramValue->uuid()];
        if(!nx::core::string_utilities::EndsWith(parameter.first, "_path") &&
           (pType == "simplnx.ArraySelectionParameter" || pType == "simplnx.ArrayCreationParameter" || pType == "simplnx.AttributeMatrixSelectionParameter" ||
            pType == "simplnx.DataGroupCreationParameter" || pType == "simplnx.DataGroupSelectionParameter" || pType == "simplnx.DataPathSelectionParameter" ||
            pType == "simplnx.GeometrySelectionParameter" || pType == "simplnx.NeighborListSelectionParameter" || pType == "simplnx.DataGroupCreationParameter"))
        {
          output << plugName << "->" << filter->name() << "->" << paramValue->name() << " The parameter key should end with '_path' for a Parameter of Type " << pType << '\n';
        }

        if(pType == "simplnx.MultiArraySelectionParameter" && !nx::core::string_utilities::EndsWith(parameter.first, "s"))
        {
          output << plugName << "->" << filter->name() << "->" << paramValue->name() << " The parameter key should end with 's' for a Parameter of Type " << pType << '\n';
        }

        if(pType == "simplnx.DataObjectNameParameter" && !nx::core::string_utilities::EndsWith(parameter.first, "_name"))
        {
          output << plugName << "->" << filter->name() << "->" << paramValue->name() << " The parameter key should end with '_name' for a Parameter of Type " << pType << '\n';
        }
        if((pType == "simplnx.ChoicesParameter" || pType == "simplnx.NumericTypeParameter") && !nx::core::string_utilities::EndsWith(parameter.first, "_index"))
        {
          output << plugName << "->" << filter->name() << "->" << paramValue->name() << " The parameter key should end with '_index' for a Parameter of Type " << pType << '\n';
        }
      }
    }
  }

  for(const auto& name : uniqueSepNames)
  {
    logFile << "- " << name << "\n";
  }

  Application::DeleteInstance();
  REQUIRE(Application::Instance() == nullptr);

  if(!output.str().empty())
  {
    std::cout << output.str();
  }
  REQUIRE(output.str().empty() == true);
}

TEST_CASE("nx::core::FilterValidation::Filter Parameter Help Text", "[simplnx][Filter]")
{
  unit_test::LoadPlugins();

  GenerateParameterList();

  auto* filterListPtr = Application::Instance()->getFilterList();
  const auto pluginListPtr = Application::Instance()->getPluginList();

  std::stringstream output;

  // Loop on each Plugin
  for(const auto& plugin : pluginListPtr)
  {
    const std::string plugName = plugin->getName();

    const auto& pluginFilterHandles = plugin->getFilterHandles();

    // Loop on each Filter
    for(const auto& filterHandle : pluginFilterHandles)
    {
      const std::string filterClassName = filterHandle.getClassName();
      IFilter::UniquePointerType filter = filterListPtr->createFilter(filterHandle);

      const auto& parameters = filter->parameters();
      // Loop over each Parameter
      for(const auto& parameter : parameters)
      {
        auto const& paramValue = parameter.second;
        if(paramValue->helpText().empty())
        {
          output << plugName << "->" << filter->name() << "->" << paramValue->name() << ": Human Name: '" << paramValue->humanName() << "' The Help Text is empty\n";
        }
      }
    }
  }

  Application::DeleteInstance();
  REQUIRE(Application::Instance() == nullptr);

  if(!output.str().empty())
  {
    std::cout << output.str();
  }
  REQUIRE(output.str().empty() == true);
}

TEST_CASE("nx::core::FilterValidation::Filter Name", "[simplnx][Filter]")
{
  unit_test::LoadPlugins();

  GenerateParameterList();

  auto* filterListPtr = Application::Instance()->getFilterList();
  const auto pluginListPtr = Application::Instance()->getPluginList();

  std::stringstream output;

  // Loop on each Plugin
  for(const auto& plugin : pluginListPtr)
  {
    const std::string plugName = plugin->getName();

    const auto& pluginFilterHandles = plugin->getFilterHandles();

    // Loop on each Filter
    for(const auto& filterHandle : pluginFilterHandles)
    {
      const std::string filterClassName = filterHandle.getClassName();
      IFilter::UniquePointerType filter = filterListPtr->createFilter(filterHandle);
      if(!nx::core::string_utilities::EndsWith(filterClassName, "Filter"))
      {
        output << "- [ ] " << plugName << "->" << filter->name() << ". Filter class names should end with 'Filter'.\n";
      }
    }
  }

  Application::DeleteInstance();
  REQUIRE(Application::Instance() == nullptr);

  if(!output.str().empty())
  {
    std::cout << output.str();
  }
  REQUIRE(output.str().empty() == true);
}

TEST_CASE("nx::core::FilterValidation::Filter Version", "[simplnx][Filter]")
{
  unit_test::LoadPlugins();

  auto* filterListPtr = Application::Instance()->getFilterList();
  REQUIRE(filterListPtr->size() > 0);
  const auto pluginListPtr = Application::Instance()->getPluginList();
  // Loop on each Plugin
  for(const auto& plugin : pluginListPtr)
  {
    const std::string plugName = plugin->getName();

    const auto& pluginFilterHandles = plugin->getFilterHandles();

    // Loop on each Filter
    for(const auto& filterHandle : pluginFilterHandles)
    {
      IFilter::UniquePointerType filter = filterListPtr->createFilter(filterHandle);
      const int filterVersion = filter->parametersVersion();

      REQUIRE(filterVersion >= 1);
    }
  }

  Application::DeleteInstance();
  REQUIRE(Application::Instance() == nullptr);
}

TEST_CASE("nx::core::FilterValidation::Test Filter Clone", "[simplnx][Filter]")
{
  unit_test::LoadPlugins();

  auto* filterListPtr = Application::Instance()->getFilterList();
  REQUIRE(filterListPtr->size() > 0);
  const auto pluginListPtr = Application::Instance()->getPluginList();
  // Loop on each Plugin
  for(const auto& plugin : pluginListPtr)
  {
    const std::string plugName = plugin->getName();

    const auto& pluginFilterHandles = plugin->getFilterHandles();

    // Loop on each Filter
    for(const auto& filterHandle : pluginFilterHandles)
    {
      IFilter::UniquePointerType filter = filterListPtr->createFilter(filterHandle);
      IFilter::UniquePointerType clone = filter->clone();

      REQUIRE(filter->uuid() == clone->uuid());
    }
  }

  Application::DeleteInstance();
  REQUIRE(Application::Instance() == nullptr);
}
