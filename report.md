# Data Compression Course Project: Phase 1 Report
**Team Member:** mzaid-1
**Date:** April 24, 2026

## 1. Introduction
This report documents the implementation of Phase 1 for the BZip2 compression algorithm. This stage focuses on the foundational pipeline: Block Management, Run-Length Encoding (RLE-1), and Burrows-Wheeler Transform (BWT).

## 2. System Architecture
The system follows a modular design where each stage of the pipeline is isolated:
- **Block Division**: Manages large files by splitting them into fixed-size buffers.
- **RLE-1**: Reduces redundant sequences to prepare for BWT.
- **BWT Matrix Transform**: Clusters similar characters to improve entropy for future stages.

## 3. Implementation Details

### 3.1 Block Management
We implemented a `BlockManager` that allocates memory on the heap per block. 
- **Configuration**: Block size is fully configurable via `config.ini` (100KB to 900KB).
- **File Handling**: Supports safe read/write of binary streams.

### 3.2 RLE-1 (Run-Length Encoding)
We implemented the real BZip2 standard for RLE-1. 
- **Algorithm**: Runs of 4-259 characters are encoded as 4 literals followed by a count byte.
- **Rationale**: This prevents the "expansion problem" found in naive RLE when processing random data.

### 3.3 Burrows-Wheeler Transform (BWT)
- **Forward**: Implemented using an index-sorting approach. We sort all cyclic rotations of the input block lexicographically using `qsort`. 
- **Inverse**: Implemented the efficient **LF-Mapping** algorithm ($O(N)$ time and space), which avoids the $O(N^2)$ memory requirement of full matrix reconstruction.

## 4. Performance Evaluation
Initial testing shows that the BWT transform is the most computationally intensive part of Phase 1 due to the sorting overhead. The LF-mapping inverse transform provides excellent restoration speed.

## 5. Conclusion
Phase 1 is fully functional and verified. The pipeline successfully transforms data into a clustered state ready for MTF and Huffman coding in the subsequent stages.
