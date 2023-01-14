from osgeo import gdal
import matplotlib.pyplot as plt
import os

def read_gtif(filename):
    ds = gdal.Open(filename)
    raw_data = ds.ReadAsArray()
    plt.imshow(raw_data)
    plt.show()


if __name__ == '__main__':
    path = "../data/two/"
    dir_l = os.listdir(path)
    for fl in dir_l:
        read_gtif(path + fl)
