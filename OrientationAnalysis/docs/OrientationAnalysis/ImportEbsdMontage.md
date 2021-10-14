# ImportEbsdMontage #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| NO | InputFileListInfo | Input File List | <<<NOT_IMPLEMENTED>>> | EbsdMontageImportFilterParameter |
| YES | MontageName | Name of Created Montage | StringParameter::ValueType | StringParameter |
| YES | CellAttributeMatrixName | Cell Attribute Matrix Name | StringParameter::ValueType | StringParameter |
| YES | CellEnsembleAttributeMatrixName | Cell Ensemble Attribute Matrix Name | StringParameter::ValueType | StringParameter |
| YES | DefineScanOverlap | Define Type of Scan Overlap | ChoicesParameter::ValueType | ChoicesParameter |
| YES | ScanOverlapPixel | Pixel Overlap Value (X, Y) | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| YES | ScanOverlapPercent | Percent Overlap Value (X, Y) | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | GenerateIPFColorMap | Generate IPF Color Map | bool | BoolParameter |
| YES | CellIPFColorsArrayName | IPF Colors | StringParameter::ValueType | StringParameter |
