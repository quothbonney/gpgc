import matplotlib.pyplot as plt
from matplotlib import image
from matplotlib import cm
import numpy as np
import struct
import sys
import math
from osgeo import gdal
import itertools as it
import PIL


def read_gtif(filename):
    ds = gdal.Open(filename)
    raw_data = ds.ReadAsArray()
    return raw_data


def create_offsets(sizes):
    x0 = [0]
    y0 = [0]
    b = [0]

    index = 0
    while index < len(sizes):
        while b[index] < sizes[index]:
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

    return [x0, y0]


def post_processing(xorigins, yorigins, sizes, mmap, max_size):
    for i in range(len(xorigins)):
        mmap[int(yorigins[i] * max_size)][int(xorigins[i] * max_size)] = 500


def decompress(filename, sz):
    sizes = []
    data = []
    vectors = []

    block = -1

    with open(filename) as gpgc:
        for line in gpgc:
            words = line.split(" ")

            # data.append((int(words[3]), (float(words[0]), float(words[1]), int(words[2]))));
            max_size = sz;
            size = int(words[3])
            exp = max_size / size
            data.append(int(math.log2(exp)))
            vectors.append((float(words[0]), float(words[1]), float(words[2]), size))

    decompressed = np.zeros((max_size, max_size), dtype=int)

    x0, y0 = create_offsets(data)
    for elem in range(len(x0)):
        x_o = int(x0[elem] * max_size)
        y_o = int(y0[elem] * max_size)
        # print(x_o, y_o)

        i, j, k, size = vectors[elem]
        for m in range(size):
            for n in range(size):
                altitude = (i * m) + (j * n) + k
                decompressed[y_o + m][x_o + n] = int(altitude)

    return decompressed


if __name__ == '__main__':
    decompressed = decompress(sys.argv[1], int(sys.argv[2]))
    # original = read_gtif(sys.argv[3]);
    # ate_bicubic(decompressed);

    im = plt.imshow(decompressed, cmap=cm.terrain)

    plt.colorbar(im)

    plt.show()




