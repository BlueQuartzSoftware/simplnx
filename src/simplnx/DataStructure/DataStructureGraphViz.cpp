/**
 * @file DataStructureGraphViz.cpp
 * @brief Implements GraphViz hierarchy export for DataStructure.
 */

#include "DataStructure.hpp"

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/DataStructure/DataStructureExportUtilities.hpp"

#include <ostream>
#include <ranges>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace nx::core
{
namespace
{
/**
 * @brief Writes a DataObject name as a quoted GraphViz label.
 * @param outputStream Stream that receives the escaped label text.
 * @param label Unescaped label text.
 */
void writeGraphVizLabel(std::ostream& outputStream, std::string_view label)
{
  for(const unsigned char character : label)
  {
    switch(character)
    {
    case '\\':
      outputStream << "\\\\";
      break;
    case '"':
      outputStream << "\\\"";
      break;
    case '\n':
      outputStream << "\\n";
      break;
    case '\r':
      outputStream << "\\r";
      break;
    case '\t':
      outputStream << "\\t";
      break;
    case '{':
    case '}':
    case '|':
    case '<':
    case '>':
      outputStream << '\\' << static_cast<char>(character);
      break;
    default:
      outputStream << (character < 0x20 || character == 0x7F ? '?' : static_cast<char>(character));
      break;
    }
  }
}

/**
 * @brief Writes the stable GraphViz identifier for a DataObject.
 * @param outputStream Stream that receives the identifier.
 * @param identifier DataObject identifier.
 */
void writeGraphVizNodeId(std::ostream& outputStream, DataObject::IdType identifier)
{
  outputStream << "\"node_" << identifier << '"';
}

} // namespace

void DataStructure::exportHierarchyAsGraphViz(std::ostream& outputStream) const
{
  outputStream << "digraph DataGraph {\n"
               << "\tlabelloc =\"t\"\n"
               << "\trankdir=LR;\n"
               << "\tlabel=\"DataStructure Hierarchy\"\n"
               << "\tfontcolor=\"#FFFFFA\"\n"
               << "\tfontsize=12\n"
               << "\tgraph [splines=true bgcolor=\"#242627\"]\n"
               << "\tnode [shape=record style=\"filled\" fillcolor=\"#1D7ECD\" fontsize=12 fontcolor=\"#FFFFFA\"]\n"
               << "\tedge [dir=front arrowtail=empty style=\"\" color=\"#FFFFFA\"]\n\n";

  const std::vector<const DataObject*> rootObjectPtrs = data_structure_export::GetSortedObjectPointers(m_RootGroup);
  std::vector<const DataObject*> pendingObjectPtrs;
  pendingObjectPtrs.reserve(rootObjectPtrs.size());
  for(const DataObject* objectPtr : std::ranges::reverse_view(rootObjectPtrs))
  {
    pendingObjectPtrs.emplace_back(objectPtr);
  }

  std::unordered_set<DataObject::IdType> expandedIds;
  expandedIds.reserve(m_DataObjects.size());
  while(!pendingObjectPtrs.empty())
  {
    const DataObject* objectPtr = pendingObjectPtrs.back();
    pendingObjectPtrs.pop_back();
    if(!expandedIds.insert(objectPtr->getId()).second)
    {
      continue;
    }

    writeGraphVizNodeId(outputStream, objectPtr->getId());
    outputStream << " [label=\"";
    writeGraphVizLabel(outputStream, objectPtr->getName());
    outputStream << "\"];\n";

    const auto* groupPtr = dynamic_cast<const BaseGroup*>(objectPtr);
    if(groupPtr == nullptr)
    {
      continue;
    }

    const std::vector<const DataObject*> childObjectPtrs = data_structure_export::GetSortedObjectPointers(groupPtr->getDataMap());
    for(const DataObject* childPtr : childObjectPtrs)
    {
      writeGraphVizNodeId(outputStream, objectPtr->getId());
      outputStream << " -> ";
      writeGraphVizNodeId(outputStream, childPtr->getId());
      outputStream << ";\n";
    }
    for(const DataObject* childPtr : std::ranges::reverse_view(childObjectPtrs))
    {
      pendingObjectPtrs.emplace_back(childPtr);
    }
  }

  outputStream << "}\n";
}
} // namespace nx::core
