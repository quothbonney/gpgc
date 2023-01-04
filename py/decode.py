import matplotlib.pyplot as plt
from matplotlib import cm
import numpy as np
import struct
import sys
import math
from osgeo import gdal

def read_gtif(filename):
    ds = gdal.Open(filename)
    raw_data = ds.ReadAsArray()
    return raw_data

def decompress(filename, sz):
    data = []
    vectors = []

    b = [0]
    x0 = [0]
    y0 = [0]

    with open(filename) as gpgc:
        for line in gpgc:
            words = line.split(" ")
            #data.append((int(words[3]), (float(words[0]), float(words[1]), int(words[2]))));
            max_size = sz;
            size = int(words[3])
            exp = max_size / size
            data.append(int(math.log2(exp)))
            vectors.append((float(words[0]), float(words[1]), float(words[2]), int(size)))

        
    index = 0
    while index < len(data):
        while b[index] < data[index]:
            b[index] = b[index] + 1
            for i in range(3):
                b.insert(index + 1, b[index])

            
            x0.insert(index + 1, x0[index] + (1 / (2 ** b[index])))
            x0.insert(index + 2, x0[index])
            x0.insert(index + 3, x0[index] + (1 / (2 ** b[index])))

            y0.insert(index + 1, y0[index])
            y0.insert(index + 2, y0[index] + (1 / (2 ** b[index])))
            y0.insert(index + 3, y0[index] + (1 / (2 ** b[index])))

        index += 1

    #print(x0)
    #print(y0)

    decompressed = np.zeros((max_size, max_size), dtype=int)

    for elem in range(len(x0)):
        x_o = int(x0[elem] * max_size)
        y_o = int(y0[elem] * max_size)
        #print(x_o, y_o)
        
        i, j, k, size = vectors[elem]
        for m in range(size):
            for n in range(size):
                altitude = (i * m) + (j * n) + k
                decompressed[y_o + m][x_o + n] = int(altitude)


    return decompressed


if __name__ == '__main__':

    decompressed = decompress(sys.argv[1], int(sys.argv[2]))
    original = read_gtif(sys.argv[3]);

    plt.imshow(decompressed, cmap=cm.terrain)
    plt.show()
    



