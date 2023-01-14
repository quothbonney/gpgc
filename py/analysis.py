import gdal
import sys, os
import numpy as np
import decode
from matplotlib import cm

if __name__ == "__main__":
    path = "data/two/"
    output = "data/dotgpgc/"
    dir_list = os.listdir(path)
    
    for tif_file in dir_list:
        fname, _ = tif_file.split(".")
        outfile = output + fname + ".gpgc"
        infile = path + tif_file
        os.system(f"./Debug/gpgc {infile} {outfile} -z 0.5 -u 20")
        
