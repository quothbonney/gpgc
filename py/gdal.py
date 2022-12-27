from osgeo import gdal
from osgeo import osr
import sys
import numpy

if __name__ == '__main__':
    ds = gdal.Open(sys.argv[1])
    band = ds.GetRasterBand(1)
    nw_partition = band.ReadAsArray(0, 0, 512, 512);
    dst_filename = "out.tif" 

    fileformat = "GTiff"
    driver = gdal.GetDriverByName(fileformat)
    metadata = driver.GetMetadata()
    
    if metadata.get(gdal.DCAP_CREATE) == "YES":
        print("Driver {} supports Create() method.".format(fileformat))

    if metadata.get(gdal.DCAP_CREATECOPY) == "YES":
        print("Driver {} supports CreateCopy() method.".format(fileformat))

    dst_ds = driver.Create(dst_filename, xsize=512, ysize=512,
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


