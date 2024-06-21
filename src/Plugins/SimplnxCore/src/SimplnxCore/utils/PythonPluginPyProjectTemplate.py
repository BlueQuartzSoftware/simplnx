[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[tool.hatch.build.targets.wheel]
packages = ["src/#PLUGIN_NAME#"]

[project]
name = "simplnx.#PLUGIN_NAME#"
version = "1.0.0"
requires-python = ">=3.10"
dependencies = [
  "numpy"
]

[project.entry-points."simplnx.plugins"]
plugin = "#PLUGIN_NAME#"