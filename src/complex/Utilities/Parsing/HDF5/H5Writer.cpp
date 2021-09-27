#include "H5Writer.hpp"

#include "complex/Utilities/Parsing/HDF5/H5.hpp"

#include "H5Support.hpp"

using namespace complex;

H5::ErrorType H5::Writer::Generic::writeStringAttribute(H5::IdType locationID, const std::string& objectName, const std::string& attributeName, const std::string& data)
{
  H5O_info_t objectInfo{};

  /* Get the type of object */
  herr_t returnError = H5Oget_info_by_name(locationID, objectName.c_str(), &objectInfo, H5P_DEFAULT);
  if(returnError >= 0)
  {
    /* Open the object */
    hid_t objectID = Support::OpenId(locationID, objectName, objectInfo.type);
    if(objectID >= 0)
    {
      /* Create the attribute */
      hid_t attributeType = H5Tcopy(H5T_C_S1);
      if(attributeType >= 0)
      {
        usize attributeSize = data.size(); /* extra null term */
        herr_t error = H5Tset_size(attributeType, attributeSize);
        if(error < 0)
        {
          std::cout << "Error Setting H5T Size" << std::endl;
          returnError = error;
        }
        if(error >= 0)
        {
          error = H5Tset_strpad(attributeType, H5T_STR_NULLTERM);
          if(error < 0)
          {
            std::cout << "Error adding a null terminator." << std::endl;
            returnError = error;
          }
          if(error >= 0)
          {
            hid_t attributeSpaceID = H5Screate(H5S_SCALAR);
            if(attributeSpaceID >= 0)
            {
              /* Verify if the attribute already exists */
              int32 hasAttribute = Support::FindAttribute(objectID, attributeName);
              /* The attribute already exists, delete it */
              if(hasAttribute == 1)
              {
                error = H5Adelete(objectID, attributeName.c_str());
                if(error < 0)
                {
                  std::cout << "Error Deleting Attribute '" << attributeName << "' from Object '" << objectName << "'" << std::endl;
                  returnError = error;
                }
              }
              if(error >= 0)
              {
                /* Create and write the attribute */
                hid_t attributeID = H5Acreate(objectID, attributeName.c_str(), attributeType, attributeSpaceID, H5P_DEFAULT, H5P_DEFAULT);
                if(attributeID >= 0)
                {
                  error = H5Awrite(attributeID, attributeType, data.c_str());
                  if(error < 0)
                  {
                    std::cout << "Error Writing String attribute." << std::endl;

                    returnError = error;
                  }
                }
                H5S_CLOSE_H5_ATTRIBUTE(attributeID, error, returnError);
              }
              H5S_CLOSE_H5_DATASPACE(attributeSpaceID, error, returnError);
            }
          }
        }
        H5S_CLOSE_H5_TYPE(attributeType, error, returnError);
      }
      else
      {
        // returnError = attributeType;
      }
      /* Close the object */
      herr_t error = Support::CloseId(objectID, objectInfo.type);
      if(error < 0)
      {
        std::cout << "Error Closing Object Id" << std::endl;
        returnError = error;
      }
    }
  }
  return returnError;
}
