# YAPWR
Yet Another Partial Wave Repository: Code for extracting Dihadron Partial Waves at CLAS12


## Table of Contents

- [Description](#description)  
- [Usage](#usage)  
- [Contact](#contact)  

## Description

This repository manages a SLURM pipeline for studying dihadron partial waves at CLAS12. The goal of the pipeline goes beyond extracting the partial waves:

  - Produce kinematic plots of the datasets ($x$, $Q^2$, $p_T$, etc.)
  - Perform systematic errors analyses
  - Calculate bin centers
  - Generate LaTeX tables/figures for easy pasting into Overleaf


## Usage

The program is intended to be used on Jefferson Lab's `ifarm` , and run using their cluster manager `slurm`. 

```bash
./run_project.rb [--append] [--maxEntries N] [--maxFiles M] PROJECT_NAME CONFIG1 [CONFIG2 ...]
```

## Contact

Gregory Matousek (gamatousek@gmail.com)
   
