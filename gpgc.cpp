#include <cstdint>
#include "gdal_priv.h"
#include <stdio.h>

typedef struct {
	uint16_t height, width;
	GDALRasterBand* rBand;
	double rHeaderData[6];
} gpgc_gdal_data;

gpgc_gdal_data* process_file(const char* filename) {
	GDALAllRegister();
	GDALRasterBand* poBand;
	GDALDataset* poDataset;
	gpgc_gdal_data working;
	
	poDataset = (GDALDataset*)GDALOpen(filename, GA_ReadOnly);
	poBand = poDataset->GetRasterBand(1);
	
	working.width = poBand->GetXSize();
	working.height= poBand->GetYSize();
	poDataset->GetGeoTransform(working.rHeaderData);
}

uint16_t** gpgc_read_16() {
	
}

int main(int argc, char *argv[]) {
	process_file(argv[1]);
}

