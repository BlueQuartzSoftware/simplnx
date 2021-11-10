# ITKImportFijiMontage #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | InputFile | Fiji Configuration File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | MontageName | Name of Created Montage | StringParameter::ValueType | StringParameter |
| YES | ColumnMontageLimits | Montage Column Start/End [Inclusive, Zero Based] | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| YES | RowMontageLimits | Montage Row Start/End [Inclusive, Zero Based] | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| YES | LengthUnit | Length Unit | ChoicesParameter::ValueType | ChoicesParameter |
| YES | ChangeOrigin | Change Origin | bool | BoolParameter |
| YES | Origin | Origin | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | ConvertToGrayScale | Convert To GrayScale | bool | BoolParameter |
| YES | ColorWeights | Color Weighting | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | DataContainerPath | DataContainer Prefix | DataPath | DataGroupCreationParameter |
| YES | CellAttributeMatrixName | Cell Attribute Matrix Name | DataPath | ArrayCreationParameter |
| YES | ImageDataArrayName | Image DataArray Name | StringParameter::ValueType | StringParameter |
