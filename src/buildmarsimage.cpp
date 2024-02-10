#include <iostream>
#include <fstream>
#include <string>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <chrono>

using namespace std;

extern "C" {
#include <tiffio.h>
#include <zlib.h>
}

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

// fdelhoume 2023
// builds whole mars image (strip) from all 3960 individual tiffs

#define Z_MIDDLE_COMPRESSION 6

#if !defined(DEFAULT_FOLDER)
#define DEFAULT_FOLDER "."
#endif

static char buff[256];
const char *getName(int row, int col, const char* folder = DEFAULT_FOLDER) {
  snprintf(buff,
           sizeof(buff),
           "%s/ZippedTiffs/MurrayLab_CTX_V01_E%s%03d_N%s%02d_Mosaic.tif",
           folder,
           (col < 0 ? "-" : ""), abs(col),
           (row < 0 ? "-" : ""), abs(row));
  return buff;
}

std::string displayDuration(unsigned int seconds) {
  unsigned int hours = seconds / 3600;
  unsigned int minutes = (seconds % 3600) / 60;
  unsigned int sseconds = seconds % 60;
  unsigned int days = hours / 24;
  std::stringstream str;
  str << std::setfill('0');
  if (hours >= 24)
    str << std::setw(2) << days << "d";
  str << std::setw(2) << (hours % 24) << "h"
      << std::setw(2) << minutes << "m"
      << std::setw(2) << sseconds << "s";
  return str.str();
}

// Exponential moving Average to smooth variations on predicted remaining time
class Ema
{
public:
  Ema(float alpha) : _alpha(alpha) {}
  float filter(float input)
  {
    if (_hasInitial)
    {
      _output = _alpha * (input - _output) + _output;
    }
    else
    {
      _output = input;
      _hasInitial = true;
    }
    return _output;
  }
  void reset()
  {
    _hasInitial = true;
  }

private:
  bool _hasInitial = false;
  float _output = 0;
  float _alpha = 0; //  Smoothing factor, in range [0,1]. Higher the value - less smoothing (higher the latest reading impact)
};

int 
main(int argc, char *argv[]) {
    std::cout << "Usage: buildmarsimage <out.tif> <cols> <rows>" << std::endl;
    std::cout << "       cols 1..90" << std::endl;   
    std::cout << "       rows 1..44" << std::endl;
    std::cout << "       no arg means buildmarsimage mars_90_44_strip.tif 90 44" << std::endl;
    std::cout << "       fdelhoume 2024" << std::endl;

  unsigned int srcwidth = 47420;
  unsigned int srcheight = 47420;
  const char* outfilename = argc > 1 ? argv[1] : "mars_90_44_strip.tif";
  unsigned int numsrcx = argc > 2 ? atoi(argv[2]) : 90;
  unsigned int numsrcy = argc > 3 ? atoi(argv[3]) : 44;

  if (numsrcx > 90) numsrcx = 90;
  if (numsrcx < 1) numsrcx = 1;
  if (numsrcy > 44) numsrcy = 44;
  if (numsrcy < 1) numsrcy = 1;

  int stepx = 4;
  int startx = -stepx * (numsrcx / 2); 
  int stepy = -4;
  int starty = -stepy * (numsrcy / 2); 

  unsigned int dstwidth = numsrcx * srcwidth;
  unsigned int dstheight = numsrcy * srcheight;

  Ema smoothRemainingSeconds(0.1);
std::cout << std::setprecision(2);
std::cout << "Starting from " << to_string(startx) << "  " << to_string(starty) <<  std::endl;
std::cout << outfilename << " " << to_string(dstwidth) << "x" << to_string(dstheight) << std::endl;
  // create dst image, open as BigTIFF (obviously)
  TIFF *tifout = TIFFOpen(outfilename, "w8");
  TIFFSetField(tifout, TIFFTAG_IMAGEWIDTH, dstwidth);
  TIFFSetField(tifout, TIFFTAG_IMAGELENGTH, dstheight);

  TIFFSetField(tifout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tifout, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField(tifout, TIFFTAG_ROWSPERSTRIP, 1);

  // very little compression for fast output (at the cost of disk space)
  TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
  TIFFSetField(tifout, TIFFTAG_ZIPQUALITY, Z_BEST_SPEED);
  // very important for performance for large images !
  TIFFSetField(tifout, TIFFTAG_PREDICTOR, PREDICTOR_NONE);

  TIFFSetField(tifout, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, 1);
  // estimation of remaining time
  auto stime = std::chrono::system_clock::now();
  // one dst strip is all we need to allocate
  unsigned char *dstrow = new unsigned char[dstwidth];
  memset(dstrow, 0, dstwidth);

  TIFF **tifrow = new TIFF *[numsrcx];
  
  unsigned int y = 0;
  int currow = starty;
  for (unsigned int r = 0; r < numsrcy; ++r, currow += stepy) {
    int curcol = startx;
    for (unsigned int c = 0; c < numsrcx; ++c, curcol += stepx) {
      const char *name = getName(currow, curcol);
      std::cout << "Loading " << name << std::endl;
      tifrow[c] = TIFFOpen(name, "r");
      if (tifrow[c] == 0) {
        std::cout << "Tile " << name << "not found" << std::endl;
      }
    }
      std::cout << "Computing row " << (r + 1) << " / " << numsrcy << endl;;
      // write one row
      for (unsigned int h = 0; h < srcheight; ++h, ++y) {
        // for all cols
        const auto rtime = chrono::system_clock::now();
        // would doing this in parallel speed up things ?
//#pragma omp parallel for
        for (unsigned int col = 0; col < numsrcx; ++col) {
          // compute pos in dstrow
          unsigned char *posindst = dstrow + col * srcwidth;
          // read one strip tif at row col
          if (tifrow[col]) {
            tsize_t rb = TIFFReadEncodedStrip(tifrow[col], h, posindst, -1);
            if (rb > srcwidth) {
              // not very compatible width Open MP...
              std::cout << "error for " << getName(currow, curcol) << endl;
            } else if (rb != srcwidth) {
              std::cout << "read bytes " << rb << " and buffer size " << srcwidth << " differ " << std::endl;
            }
          } else {
            memset(posindst, 0, srcwidth);
          }
        }
        const auto btime = chrono::system_clock::now();
        // save strip at y final pos
        TIFFWriteEncodedStrip(tifout, y, dstrow, dstwidth);
        const auto ctime = chrono::system_clock::now();
        const auto differenceFromStart = std::chrono::duration_cast<std::chrono::seconds>(ctime - stime).count();
        if (differenceFromStart >= 5) {
          const auto differenceOneRowRead = std::chrono::duration_cast<std::chrono::milliseconds>(btime - rtime).count();
          const auto differenceOneRowWrite = std::chrono::duration_cast<std::chrono::milliseconds>(ctime - btime).count();
          const auto differenceOneRow = std::chrono::duration_cast<std::chrono::milliseconds>(ctime - rtime).count();
          double ppercent = y / (double)dstheight;

          unsigned int remainingFromCurrentPerf = differenceOneRow * (dstheight - y) / 1000;
          remainingFromCurrentPerf = (unsigned int)smoothRemainingSeconds.filter(remainingFromCurrentPerf);

          float rate = differenceFromStart / (float)y;
          unsigned int remainingSecs = (dstheight - y) * rate;

          std::cout << std::setfill(' ') << std::setw(6) << (y + 1) << " / " << dstheight
                    << " # " << std::setw(5) << (h + 1) << " / " << srcheight
                    << " # " << (int)(ppercent * 100) << "%"
                    << " # row N" << currow << " - " << "(" << (r + 1) << "/" << numsrcy << ")"
                    << " # speed "  << std::setw(4) << differenceOneRowRead << "r/"  
                    << std::setw(4) << differenceOneRowWrite << "w/" 
                    << std::setw(4) << differenceOneRow << "t/" 
                    << std::setw(4) << (unsigned int)(rate * 1000) << "a ms/row";
          std::cout << " # elapsed: " << displayDuration(differenceFromStart);
          std::cout << " #end :" << displayDuration(remainingSecs) << "/" <<  displayDuration(remainingFromCurrentPerf);
          std::cout << "           \r";
          std::cout.flush();
        }
      }
      for (unsigned int c = 0; c < numsrcx; ++c)  { // close all tiffs for current row
        if (tifrow[c])
          TIFFClose(tifrow[c]);
      }
      std::cout << std::endl;
    }
    TIFFClose(tifout);
    delete[] tifrow;
    delete[] dstrow;
    return 0;
  }
