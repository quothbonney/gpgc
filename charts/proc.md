0. Mosaic Preprocessing
	- Raster is divided into bands of compressible fragments with size 2^n that are compressed individually into distinct tree structures and decoded in bands to fill up entire raster 
1. Interpreting Raster Data
	- Develop C++ object `gpgc_partition` that handles regions of data within mosaic fragment passed by pointer and size from a full raster held in memory
	- Determine linear algebra of performant planar regression fitting (settled on LU decomposition of partition matrix)
	- Store orthogonal vector as IEE754 Half Precision floats in struct 64-bit leaf-node structure `gpgc_vector`
2. Regression Analysis
	- Absolute value of differentials accumulated into unsigned long integer and distributed across points
	- Z-score calculated with supplied compression parameter sigma, expecting standard deviation of acceptable differentials
	- Indefinite integral is approximated from z score, manipulating normal probability density function
	- Average information entropy per point calculated via Shannon's entropy formula
3. Recursive Quadrisection in Tree Structure
	- Average information entropy evaluated against compression parameter zeta
	- If entropy exceeds zeta, it is recursively quadrisected to four partitions of size 2^{n-1}
	- If entropy does not exceed zeta, the `gpgc_vector` structure is bitpacked into a 64-bit integer that is bitshifted into a bytestream that is subsequently encoded into a final .gpgc file
	- Partitions of size 2 forced to be encoded as leaf nodes, finishing tree encoding of each fragment and repeat
4. Stack Decoding
	- Stack structure decoder inserts sizes at the top of the queue and pops them when the branch of tree structure is complete, allowing fastest possible O(n) linear time decoding of tree structure
	- After tree is decoded, `gpgc_vector` orthogonal vector coefficients complete decoded raster with planar coefficients to form reconstruction
5. Analysis
	- 135 USGS 1-arcsecond continental rasters randomly selected for benchmarking between algorithms
	- Decoded rasters compared to their original rasters. Compression ratio and differences are recorded
	- 25, 50, 75 percentiles are recorded for each algorithm in compression ratio and inaccuracy (error)



