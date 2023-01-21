
<div align="center">
<img src="./charts/largelogo.png" height="250" alt="drawing"/>
</div>

## Introduction
The GPGC algorithm is a novel  quad-tree set-partition-coding algorithm that is designed to use information entropy as a means to analyze data, allowing for smarter encoding. 
It is specialized for rasterized geospatial data in a GeoTIFF or DTED format, facilitating transfer and storage of high precision LIDAR data for uses in automated vehicles, air-ground collision avoidance,
robotics, and precision agriculture.
<div align="center">
<img src="./charts/gif/transform.gif" height="500" alt="drawing"/>
<figcaption>GPGC Quadrisection Compression Animated</figcaption>
</div>

GPGC is a fully open-source binary format that outperforms similar lossy geospatial data compression algorithms in both efficiency and accuracy
- **Mathematically ensured to be artifact-proof.** Geospatial rasters are not like images, where small artifacts and discrepancies can be overlooked. GPGC takes major strides to ensure
the integrity of data by working only with linear transforms and partitioning, as well as setting both manual and automation maximum error thresholds.
- **An efficiency only matched by JPEG.** The codec is able to hold its own with even the most sophisticated and mature codecs such as JPEG and WEBP. At high qualities
they are often almost identical. The only difference? Smart entropy-coding in GPGC allows storage to be diverted to critical areas.
- **Linear complexity decoding** GPGC's novel, stack-based decoder is believed to be the theoretically most efficient depth-first search of a quad tree structure, reconstructing
encoded binary at unbelievable speeds.
- **Controllable Quality** The compression takes in both an expected standard deviation (sigma) of errors and a maximum entropy per pixel (mu). This fine-grained control
allows users to throttle compression ratios based on their own usage.

## A compression algorithm to *drive* the world
To clone the repository
```
git clone --recurse-submodules https://github.com/quothbonney/gpgc.git
```

