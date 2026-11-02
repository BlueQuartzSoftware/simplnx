# ImageProcessing Real-World Examples

This folder contains 16 independent DREAM3D-NX tutorials (05–20) and three inherited examples (02–04). The suite demonstrates all 92 filters in the ImageProcessing plugin.

The examples focus on materials science and additive manufacturing. They include XCT pore analysis, powder-bed inspection, melt-pool segmentation, particle separation, morphology, distance metrology, image projections, calibration, and industrial image formats.

## Source folders and companion files

Each numbered example has its own source folder, using underscores instead of spaces (for example, `05_AM_XCT_Porosity_Segmentation`). Tutorials 05–20 contain three files with the same base name; inherited examples 02–04 contain only a `.d3dpipeline`:

- `.d3dpipeline` is the authoritative DREAM3D-NX GUI pipeline.
- `.yaml` contains structured industry, input, output, filter, parameter, citation, and MCP metadata. It uses the JSON-compatible subset of YAML 1.2 so the existing C++ test code can parse it without a new dependency.
- `.md` explains the industrial problem, each processing step, parameter rationale, output interpretation, failure modes, adaptation choices, and scientific basis.

`PythonGeneration.yaml` is at the suite root. Each pipeline YAML references it. The file tells an LLM, MCP server, developer, or test how to generate temporary Python from the authoritative pipeline. Persistent Python sidecars are not stored.

The runtime copy preserves the numbered folders under `pipelines/ImageProcessing`. Installation flattens the 19 pipelines, the 16 pairs of tutorial companions, and `PythonGeneration.yaml` into the installed `ImageProcessing` directory.

## Input data

The pipelines read from `Data/ImageProcessing_Examples`. CMake downloads and verifies `ImageProcessing_Examples_v1.tar.gz` before it copies the directory into the runtime data folder.

The archive `Provenance.json` file identifies each source, checksum, transformation, data type, dimension, spacing, unit, and consuming pipeline. The archive prefers original paper data, then compatible paper-linked data, then deterministic paper-inspired synthesis. Synthetic inputs are not experimental measurements.

`Generators/GenerateImageProcessingExampleData.py` and `Generators/PaperInputTargets.json` are stored in the archive. They record the fixed seed, paper visual targets, target traits, generated metrics, and known differences. The generator source is also maintained under `test/DataGeneration` with deterministic unit tests.

## Outputs

Each pipeline writes to its own directory under:

`Data/Output/ImageProcessing_Examples/`

Each pipeline writes a final `.dream3d` file. Selected pipelines also write TIFF, PNG, or MHA previews.

## Run a pipeline

Run a pipeline from the application binary directory. This example runs the AM XCT workflow:

```bash
./nxrunner --execute "pipelines/ImageProcessing/05_AM_XCT_Porosity_Segmentation/05_AM_XCT_Porosity_Segmentation.d3dpipeline"
```

For an installed package, use `pipelines/ImageProcessing/05_AM_XCT_Porosity_Segmentation.d3dpipeline` instead.

The pipelines use relative paths. Start `nxrunner` or DREAM3D-NX from the application binary directory so `Data` resolves to the runtime data folder.

## Generate and run Python

Use `simplnx_utilities.generate_python_pipeline()` as specified by `PythonGeneration.yaml`. Write the generated code to a temporary file, then run it from the application binary directory with the Python executable from the DREAM3D-NX Anaconda environment.

The temporary program uses the same relative input and output paths as the GUI pipeline. It stops with a `RuntimeError` if a filter reports an error. Regenerate it after any pipeline change.

## DREAM3D-NX bookmarks

DREAM3D-NX scans the runtime `pipelines` folder at launch. The application shows these examples under:

`pipelines -> ImageProcessing -> numbered example folder` in a build tree, or `pipelines -> ImageProcessing` in an installed package

## Coverage

The tutorials are numbered `05` through `20` after the inherited `02` through `04` examples.

`FilterCoverage.json` assigns each registered ImageProcessing filter to one enabled pipeline step. `ExamplePipelineCoverageTest.cpp` compares the manifest with the CMake filter list and the pipeline JSON files.

The same test requires a complete three-file source folder for every tutorial and rejects persistent `.py` sidecars. It compares YAML step order and UUIDs with the pipeline, checks all required narrative sections and workflow-first citations, and validates the suite-level Python-generation reference. Separate tests run the GUI pipelines and registered Python examples and verify citation metadata through DOI or authoritative paper URLs. The `test/GenerateAndRunExamplePipelines.py` helper generates and executes temporary Python for the 16 tutorials; it is not currently registered with CTest.

## Storage modes

The same pipeline files run in the normal in-core build and the OOC build. OOC validation requires the registered disk-backed SimplnxOoc store. A preset name alone does not prove OOC execution.
