# ImportAxioVisionV4Montage #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | InputFile | AxioVision XML File (_meta.xml) | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | MontageName | Name of Created Montage | StringParameter::ValueType | StringParameter |
| YES | ColumnMontageLimits | Montage Column Start/End [Inclusive, Zero Based] | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| YES | RowMontageLimits | Montage Row Start/End [Inclusive, Zero Based] | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| NO | MontageInformation | Montage Information | <<<NOT_IMPLEMENTED>>> | PreflightUpdatedValueFilterParameter |
| YES | ImportAllMetaData | Import All MetaData | bool | BoolParameter |
| YES | ChangeOrigin | Change Origin | bool | BoolParameter |
| YES | Origin | Origin | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | ChangeSpacing | Change Spacing | bool | BoolParameter |
| YES | Spacing | Spacing | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | ConvertToGrayScale | Convert To GrayScale | bool | BoolParameter |
| YES | ColorWeights | Color Weighting | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | DataContainerPath | DataContainer Prefix | DataPath | DataGroupCreationParameter |
| YES | CellAttributeMatrixName | Cell Attribute Matrix Name | StringParameter::ValueType | StringParameter |
| YES | ImageDataArrayName | Image DataArray Name | StringParameter::ValueType | StringParameter |
| YES | MetaDataAttributeMatrixName | MetaData AttributeMatrix Name | StringParameter::ValueType | StringParameter |
