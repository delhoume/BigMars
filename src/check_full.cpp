#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <tiffio.h>

using namespace std;

/*
 Frederic Delhoume 2024
 check for full image data by reading wholee image strip by strip 
*/

bool silent = false;

int main(int argc, char *argv[]) {
  int success = 1;
unsigned int imagewidth;
  unsigned int imageheight;
  unsigned int rows_per_strip;

uint16_t bitspersample, samplesperpixel;

TIFF* tifin = TIFFOpen(argv[1], "r");
TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &imagewidth);
TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &imageheight);
TIFFGetField(tifin, TIFFTAG_BITSPERSAMPLE, &bitspersample);
TIFFGetField(tifin, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
TIFFGetField(tifin, TIFFTAG_ROWSPERSTRIP, &rows_per_strip);
 
 std::cout << "image : " << std::endl
	 << "  size: " << imagewidth << " x " << imageheight << endl
	 << "  rows per strip: " << rows_per_strip << endl
	 << "  samples per pixel: " << samplesperpixel << endl
	 << "  bits per sample: " << bitspersample << endl; 

std::cout << argv[1];
if (TIFFIsTiled(tifin)) {
  std::cout << " is tiled" << std::endl;
  return 1;
}

tsize_t datasize = imagewidth * rows_per_strip * samplesperpixel;
  unsigned char *rawdata = new unsigned char[datasize];
unsigned char *data = new unsigned char[datasize];
unsigned int nstrips = TIFFNumberOfStrips(tifin);

  std::cout << " checking all " << imageheight << " strips" << endl;
  unsigned int strip = 0;
    for (unsigned int y = 0; y < imageheight; y += rows_per_strip, ++strip) {
         tsize_t readRawBytes = TIFFReadRawStrip(tifin, strip, rawdata, datasize);
           if (readRawBytes == -1 || readRawBytes > datasize) {    
                std::cout << "Error reading raw data strip " << y << " " << readRawBytes << " " << datasize << std::endl;
                std::stringstream filename; filename << "debug/raw_" << y << ".zst";
                ofstream output(filename.str(), ios::binary);
                output.write((const char*)rawdata, readRawBytes);
                output.close();
             }  
 
        //tsize_t readBytes = TIFFReadEncodedStrip(tifin, strip, data, datasize);
   /*
          
    
           std::string filename2("raw_"); filename2 += to_string(strip); filename2 += "_enc.dec";
            ofstream output2(filename2, std::ios::binary);
            output2.write((const char*)data, datasize);
            output2.close(); 
*/
 // more efficient
         unsigned int ret = TIFFReadFromUserBuffer(tifin, 0, rawdata, readRawBytes, data, datasize);
         if (ret == 0) {
            std::cout << "Error decoding data strip " << strip <<  " " << y << " " << readRawBytes << " " << datasize << std::endl;
            std::stringstream filename; filename << "debug/raw2_" << y << ".zst";
            ofstream output(filename.str(), ios::binary);
            output.write((const char*)rawdata, readRawBytes);
            output.close(); 
          }
          std::cout << "    " << (y + 1) << " / " << nstrips << "    \r";  std::cout.flush();
        }
    std::cout << endl << argv[1] << " Done" << endl;
    TIFFClose(tifin);
    delete [] rawdata; 
    delete [] data;
  return 0; // success
}
