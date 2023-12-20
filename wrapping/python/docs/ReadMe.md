# Creating Python Documentation with Sphinx

``` shell
conda create -n d3ddocs python=3.10 sphinx myst-parser sphinx-markdown-tables sphinx_rtd_theme
conda activate d3ddocs
cd simplnx/docs/
make html
```