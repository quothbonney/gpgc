from osgeo import gdal
import sys

def convert_gtiff_to_mrsid(input_file, output_file):
    # Open the input file
    in_ds = gdal.Open(input_file, gdal.GA_ReadOnly)
    if in_ds is None:
        print("Error: Unable to open input file")
        return

    # Create the output file
    driver = gdal.GetDriverByName("MrSID")
    out_ds = driver.CreateCopy(output_file, in_ds, 0, [])
    if out_ds is None:
        print("Error: Unable to create output file")
        return

    # Close the datasets
    in_ds = None
    out_ds = None

# Test the function
convert_gtiff_to_mrsid(sys.argv[1], "output.sid")
