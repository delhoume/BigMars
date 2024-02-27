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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

// fdelhoume 2023
// builds whole mars image (tiled) from all 3960 individual tiffs

// TODO -imagefolder in argments
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

int 
main(int argc, char *argv[]) {
        bool useJPEG = true;
    int argsstart = 1;
  if (argc == 1) {
    std::cout << "Usage: buildmarsimage -folder <folder>  <out.tif> <cols> <rows>" << std::endl;
    std::cout << "       folder is source TIFFs location" << std::endl;       
    std::cout << "       cols 1..90" << std::endl;   
    std::cout << "       rows 1..44" << std::endl;
    std::cout << "       fdelhoume 2024" << std::endl;
    return 1;
  }
char folder[128];
strcpy(folder, DEFAULT_FOLDER);
for (int arg = 1; arg < argc; ++arg) {
    if (!strcmp("-folder", argv[arg])) {
        strcpy(folder, argv[arg + 1]);
        argsstart += 2;
        arg++;
    } else if (!strcmp("-zip", argv[arg])) {
        useJPEG = false;
        argsstart++;
    }
}
  unsigned int srcwidth = 47420;
  unsigned int srcheight = 47420;
  const char* outfilename = argv[argsstart];
  unsigned int numsrcx = atoi(argv[argsstart + 1]);
  unsigned int numsrcy = atoi(argv[argsstart + 2]);

  if (numsrcx > 90) numsrcx = 90;
  if (numsrcx < 1) numsrcx = 1;
  if (numsrcy > 44) numsrcy = 44;
  if (numsrcy < 1) numsrcy = 1;

  int stepx = 4;
  int startx = -stepx * (numsrcx / 2); 
  int stepy = -4;
  int starty = -stepy * ((numsrcy  -1 )/ 2); 

    unsigned int tilewidth = 512;
    unsigned int tileheight = tilewidth;

unsigned int imagewidth = numsrcx * srcwidth;
  unsigned int imageheight = numsrcy * srcheight;

    unsigned int numtilesx = imagewidth / tilewidth;
	if (imagewidth % tilewidth)
		++numtilesx;

    unsigned int numtilesy = imageheight / tileheight;
	if (imageheight % tileheight)
		++numtilesy;

         // try to allocate a complete row of tiles !! 
  unsigned int full_tile_width = numtilesx * tilewidth;

	unsigned char *full_tile_data = new unsigned char[full_tile_width * tileheight];
    if (!full_tile_data) {
      std::cout << "failed allocating " << full_tile_width * tileheight << " bytes" << std::endl;
      return 1;
    }

	unsigned char *tile_data = new unsigned char[tilewidth * tileheight];
     if (!full_tile_data) {
      std::cout << "failed allocating " << tilewidth * tileheight << " bytes" << std::endl;
      return 1;
    }

std::cout << std::setprecision(2);
std::cout << "Starting from " << to_string(startx) << "  " << to_string(starty) <<  std::endl;
std::cout << outfilename << " " << to_string(imagewidth) << "x" << to_string(imageheight) << std::endl;
  // create dst image, open as BigTIFF (obviously)
  TIFF *tifout = TIFFOpen(outfilename, "w8");
  TIFFSetField(tifout, TIFFTAG_IMAGEWIDTH, imagewidth);
  TIFFSetField(tifout, TIFFTAG_IMAGELENGTH, imageheight);

  TIFFSetField(tifout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tifout, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  	TIFFSetField(tifout, TIFFTAG_TILEWIDTH, tilewidth);
	TIFFSetField(tifout, TIFFTAG_TILELENGTH, tileheight);

	std::cout << "final size: " << imagewidth << "x" << imageheight << std::endl;
	std::cout << "tile size: " << tilewidth << "x" << tileheight << std::endl;
	std::cout << "num tiles: "<< numtilesx << "x" <<  numtilesy << std::endl;

 if (useJPEG) {
        unsigned int quality = 80;
        TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
        TIFFSetField(tifout, TIFFTAG_JPEGQUALITY, quality);
    } else {
        TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
        TIFFSetField(tifout, TIFFTAG_ZIPQUALITY, Z_BEST_SPEED);
        // very important for performance for large images !
        TIFFSetField(tifout, TIFFTAG_PREDICTOR, PREDICTOR_NONE);
    }

  TIFFSetField(tifout, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, 1);
  // estimation of remaining time
  auto stime = std::chrono::system_clock::now();
 
  TIFF **tifrow = new TIFF *[numsrcx];
  for (unsigned int t = 0; t < numsrcx; ++t)
    tifrow[t] = 0;
  
  unsigned int tilex = 0;
  unsigned int tiley = 0;
  unsigned int current_src_row = 0; // index of current bigpart (0 to numsrcy)
  unsigned int current_tile_row = 0; // index of current y  tile (0 to numtilesy)
    unsigned int current_tile_y = 0; // index in current tile (0 to tileheight)
  unsigned int current_src_strip = 0; // strip in current bigpart (0 to srcheight)


  for (unsigned int y = 0; y < imageheight; ++y) {
    if (current_src_strip == 0) {
        std::cout << std::endl << "loading new sources for input " << current_src_row << " at line " << y << std::endl;
      
        for (unsigned int xx = 0; xx < numsrcx; ++xx) {
            TIFFClose(tifrow[xx]);
            const char *name = getName(starty + current_src_row * stepy, startx + xx * stepx, folder);
            std::cout << "Loading " << name << std::endl;
            tifrow[xx] = TIFFOpen(name, "r");
            if (tifrow[xx] == 0) {
                std::cout << std::endl << "Tile " << name << " not found" << std::endl;
            } 
        }
        current_src_row++;
    }
    // fill one line at current tile y position from current y position in input row
    //std::cout << "taking strip " << current_src_strip << " in " << current_src_row - 1 << " to strip " << current_tile_y << " in tile " << current_tile_row << std::endl;
 #pragma omp parallel for
    for (unsigned int xx = 0; xx < numsrcx; ++xx) {
        unsigned char* pos = full_tile_data +  current_tile_y * full_tile_width + xx * srcwidth;
        if (tifrow[xx]) {
            tsize_t rb = TIFFReadEncodedStrip(tifrow[xx], current_src_strip, pos, -1);
        } else {
            memset(pos, current_tile_y % 255, srcwidth);
        }
    }
       
    current_src_strip++;
    if (current_src_strip >= srcheight) 
        current_src_strip = 0;

    // save eventually a row of tiles when complete or last y row
     current_tile_y++;
    if ((current_tile_y >= tileheight) || (y == (imageheight - 1))) { 
        std::cout << "writing tile row " << (current_tile_row + 1) << " of " << numtilesy << " at line " << (y + 1) << " of " << imageheight << " for " << current_src_row;
            const auto ctime = chrono::system_clock::now();
        const auto differenceFromStart = std::chrono::duration_cast<std::chrono::seconds>(ctime - stime).count();

        float rate = differenceFromStart / (float)y;
        unsigned int remainingSecs = (imageheight - y) * rate;
    
        std::cout  << "|elapsed " << displayDuration(differenceFromStart);
        std::cout << "|predicted remaining time " << displayDuration(remainingSecs) <<    "                           \r";
        std::cout.flush();

        // char buffer[32];
        // sprintf(buffer, "debug/row_%d.jpg", current_tile_row);
        // stbi_write_jpg(buffer, (int) full_tile_width, (int)tileheight, 1, full_tile_data, 80);
        for (unsigned int xt = 0; xt < numtilesx; ++xt) {
            // now copy from full to single
			unsigned char *startx = full_tile_data + xt * tilewidth;
			for (unsigned int crow = 0; crow < tileheight; ++crow) {
				memcpy(tile_data + crow * tilewidth, startx + full_tile_width * crow, tilewidth);
			}
             TIFFWriteEncodedTile(tifout,
                                TIFFComputeTile(tifout,
                                                    xt * tilewidth,
                                                    current_tile_row * tileheight,
                                                    0, 0),
                                                    tile_data, tilewidth * tileheight);
        }
        current_tile_row++;
        current_tile_y = 0;
    }}
    std::cout << std::endl;
    for (unsigned int t = 0; t < numsrcx; ++t)
        TIFFClose(tifrow[t]);

    TIFFClose(tifout);
    delete[] full_tile_data;
    delete[] tile_data;
    return 0;
  }
