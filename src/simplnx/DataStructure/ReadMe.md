# simplnx::DataStructure

## Adding DataObject types
When adding DataObject types, additional IDataFactories need to be created for each IDataIOManager subclass. Complex provides the HDF5 version of IDataIOManager and IDataFactory, but plugins providing additional formats will need to be updated to enable reading and writing to the new types.

## Adding IO formats

simplnx comes with HDF5 readers and writers by default. To add additional IO formats, the plugin developer will require additional classes to tell simplnx how to read and write the DataObjects using the specified format.

Required subclasses:
- IDataFactory for each concrete DataObject type
- IDataIOManager for the format
  * Add IDataFactory subclasses to the IO Manager

In addition, a DataStructure reader and writer should be provided for the new format for easy IO.

Once these classes are created, the DataIOManager needs to be added to the plugin so that simplnx has access to it and can find it using the format name.
