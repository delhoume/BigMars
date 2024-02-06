#include <iostream>

#include <tiffio.h>


#define STB_SPRINTF_IMPLEMENTATION
#include "stb/stb_sprintf.h"

using namespace std;


/*
 Frederic Delhoume 2024
 check for presence and decodes all 3960 TIFF input files
*/

const char *zippedFolder = "ZippedTiffs";
const char *originalDataFolder = "OriginalData";
const char *originalTiffsFolder = "OriginalTiffs";

static char buffer1[128];
const char *getOriginalTiffName(int row, int col, const char *folder) {
  stbsp_sprintf(buffer1, "%s/MurrayLab_CTX_V01_E%s%03d_N%s%02d_Mosaic.tif",
                folder,
                (row < 0 ? "-" : ""), abs(row), (col < 0 ? "-" : ""), abs(col));
  return buffer1;
}
 
static char buffer2[128];
const char *getZippedName(int row, int col, const char *folder) {
  stbsp_sprintf(buffer2, "%s/MurrayLab_CTX_V01_E%s%03d_N%s%02d_Mosaic.tif",
                folder,
                (row < 0 ? "-" : ""), abs(row), (col < 0 ? "-" : ""), abs(col));
  return buffer2;
}

static char buffer3[128];
const char *getOriginalName(int row, int col, const char *folder) {
  stbsp_sprintf(buffer3, "MurrayLab_GlobalCTXMosaic_V01_E%s%03d_N%s%02d.zip",
                (row < 0 ? "-" : ""), abs(row), (col < 0 ? "-" : ""), abs(col));
  return buffer3;
}

int main(int argc, char *argv[]) {	
  unsigned int rows = 44;
  unsigned int cols = 90;
  int stepx = 4; 
  int stepy = -4;
  int currentrow = 84;
  int currentcol = -180;
  int allimages = rows * cols;

  unsigned int width = 47420;
  unsigned int height = 47420;
 

  unsigned int missing = 0;
  unsigned int verify = 0;
  
  unsigned char *rowdata = new unsigned char[width];
 
  cout << "Checking for all " << allimages << " files" << endl;
  unsigned int num = 0;
  for (int row = 0; row < rows; ++row, currentrow += stepy) {
    int currentcol = -180;
    for (int col = 0; col < cols; ++col, currentcol += stepx) {
      const char *zippedTifName = getZippedName(currentcol, currentrow, zippedFolder);

      FILE *filein = fopen(zippedTifName, "rb");
      if (filein == 0) {
              cout << zippedTifName << "  (" << num << " / " << allimages << ") missing" << endl;
        const char *originalZipName = getOriginalName(currentcol, currentrow, originalDataFolder);
        const char *originalTiffName = getOriginalTiffName(currentcol, currentrow, originalTiffsFolder);
  
        cout << "cd OriginalData" << endl
             << "curl -C - \"https://murray-lab.caltech.edu/CTX/V01/tiles/" << originalZipName << "\" -O" << endl
             << "cd .." << endl
             << "7zz e OriginalData/" << originalZipName << " -oOriginalTiffs \"*.tif\" -r -aos" << endl
             << "tiffcp -s -r 1 -c zip:p9 " << originalTiffName << " " << zippedTifName << endl
             << "rm -rf " << originalTiffName << endl;
        missing++;
          } else {
	if (verify) {
	  cout << zippedTifName << "  (" << num << " / " << allimages << ") present" << endl;
          unsigned int imagewidth;
          unsigned int imageheight;
          unsigned int rows_per_strip;

	  uint16_t bitspersample, samplesperpixel;

	  TIFF* tifin = TIFFOpen(zippedTifName, "r");
	  TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &imagewidth);
	  TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &imageheight);
	  TIFFGetField(tifin, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	  TIFFGetField(tifin, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
	  TIFFGetField(tifin, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);

	  for (unsigned int y = 0; y < height; ++y) {
            // check decode is fine
	    TIFFReadEncodedStrip(tifin, y, (tdata_t)rowdata, width);    
          }
	  TIFFClose(tifin);    
	}
      }
      fclose(filein);
      ++num;
    }
  }
  delete [] rowdata;
  if (missing > 0) {
    cout << missing << " missing" << endl;
  } else {
    cout << "none missing" << endl;
  }
  return 0;
}
