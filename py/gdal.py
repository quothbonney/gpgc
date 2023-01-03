from osgeo import gdal
from osgeo import osr
import sys
import os
import numpy

if __name__ == '__main__':
    input = sys.argv[1];
    size = int(sys.argv[2])
    output = sys.argv[3];

    ds = gdal.Open(input)
    band = ds.GetRasterBand(1)
    nw_partition = band.ReadAsArray(0, 0, size, size);
    dst_filename = output 
    
    os.system(f"touch {output}")
    fileformat = "GTiff"
    driver = gdal.GetDriverByName(fileformat)
    metadata = driver.GetMetadata()
    
    if metadata.get(gdal.DCAP_CREATE) == "YES":
        print("Driver {} supports Create() method.".format(fileformat))

    if metadata.get(gdal.DCAP_CREATECOPY) == "YES":
        print("Driver {} supports CreateCopy() method.".format(fileformat))

    dst_ds = driver.Create(dst_filename, xsize=size, ysize=size,
                    bands=1, eType=gdal.GDT_Byte)

    
    
    dst_ds.SetGeoTransform([444720, 30, 0, 3751320, 0, -30])
    srs = osr.SpatialReference()
    srs.SetUTM(11, 1)
    srs.SetWellKnownGeogCS("NAD27")
    dst_ds.SetProjection(srs.ExportToWkt())
    dst_ds.GetRasterBand(1).WriteArray(nw_partition)
    print("Done.")
    # Once we're done, close properly the dataset
    dst_ds = None
