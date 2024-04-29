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

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

// fdelhoume 2023

// TODO -imagefolder in argments
#if !defined(DEFAULT_FOLDER)
#define DEFAULT_FOLDER "."
#endif

static char buff[256];
const char *getName(int row, int col, int columns, const char* folder = DEFAULT_FOLDER) {
  snprintf(buff, sizeof(buff), "%s/frames/f%.6d.png", folder, col + row * columns);
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
    std::cout << "Usage: build_barry_lydon -folder <folder>  <out.tif> <cols> <rows>" << std::endl;
    std::cout << "       folder is source PNGs location" << std::endl;       
    std::cout << "       cols 1..200" << std::endl;   
    std::cout << "       rows 1..200" << std::endl;
    std::cout << "       delhoume@gmail.com 2024" << std::endl;
    return 1;
  }

  	TIFFOpenOptions *opts = TIFFOpenOptionsAlloc();
	TIFFOpenOptionsSetMaxSingleMemAlloc(opts, 0);

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


  unsigned int srcwidth = 1920;
  unsigned int srcheight = 1080;
  unsigned int bpp = 3;


  const char* outfilename = argv[argsstart];
  unsigned int numsrcx = atoi(argv[argsstart + 1]);
  unsigned int numsrcy = atoi(argv[argsstart + 2]);


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

	unsigned char *full_tile_data = new unsigned char[full_tile_width * tileheight * bpp];
    if (!full_tile_data) {
      std::cout << "failed allocating " << full_tile_width * tileheight * bpp << " bytes" << std::endl;
      return 1;
    }
	unsigned char *tile_data = new unsigned char[tilewidth * tileheight * bpp];
     if (!full_tile_data) {
      std::cout << "failed allocating " << tilewidth * tileheight * bpp << " bytes" << std::endl;
      return 1;
    }

std::cout << std::setprecision(2);
std::cout << outfilename << " " << to_string(imagewidth) << "x" << to_string(imageheight) << std::endl;
  // create dst image, open as BigTIFF (obviously)
  TIFF *tifout = TIFFOpenExt(outfilename, "w8", opts);
  TIFFOpenOptionsFree(opts);
	
  TIFFSetField(tifout, TIFFTAG_IMAGEWIDTH, imagewidth);
  TIFFSetField(tifout, TIFFTAG_IMAGELENGTH, imageheight);

  TIFFSetField(tifout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tifout, TIFFTAG_BITSPERSAMPLE, 8);
  TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tifout,TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  	TIFFSetField(tifout, TIFFTAG_TILEWIDTH, tilewidth);
	TIFFSetField(tifout, TIFFTAG_TILELENGTH, tileheight);

  TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, bpp);

	std::cout << "final size: " << imagewidth << "x" << imageheight << std::endl;
	std::cout << "tile size: " << tilewidth << "x" << tileheight << std::endl;
	std::cout << "num tiles: "<< numtilesx << "x" <<  numtilesy << std::endl;
    std::cout << "compression: "<< (useJPEG ? "JPEG" : "Deflate") << std::endl;
 if (useJPEG) {
        unsigned int quality = 75;
        TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
        TIFFSetField(tifout, TIFFTAG_JPEGQUALITY, quality);
        TIFFSetField(tifout, TIFFTAG_JPEGTABLESMODE, 0);
    } else {
        TIFFSetField(tifout, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
        TIFFSetField(tifout, TIFFTAG_ZIPQUALITY, Z_BEST_SPEED);
        // very important for performance for large images !
        TIFFSetField(tifout, TIFFTAG_PREDICTOR, PREDICTOR_NONE);
    }

  // estimation of remaining time
  auto stime = std::chrono::system_clock::now();
 
 unsigned char** sources  = new unsigned char*[numsrcx];
  for (unsigned int t = 0; t < numsrcx; ++t)
    sources[t] = 0;
  
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
           if (sources[xx]) stbi_image_free(sources[xx]);
            const char *name = getName(current_src_row, xx, numsrcx, folder);
            std::cout << ".";
            int xi, yi, ni;
            sources[xx] = stbi_load(name, &xi, &yi, &ni, 0);
            if (sources[xx] == 0) {
                std::cout << std::endl << "Tile " << name << " not found" << std::endl;
            } 
//           stbi_write_jpg("toto.jpg", xi, yi, ni, sources[xx], 80);
        }
        current_src_row++;
    }
    // fill one line at current tile y position from current y position in input row
    for (unsigned int xx = 0; xx < numsrcx; ++xx) {
        unsigned char* pos = full_tile_data +  current_tile_y * full_tile_width * bpp + xx * srcwidth * bpp;
        if (sources[xx]) {
           memcpy(pos, sources[xx] + 1920 * bpp * current_src_strip, 1920 * bpp);
        } else {
   //         memset(pos, ((y % 256) + (xx % 256)) % 256, srcwidth);
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
        // stbi_write_jpg(buffer, (int) full_tile_width, (int)tileheight, bpp, full_tile_data, 80);
        for (unsigned int xt = 0; xt < numtilesx; ++xt) {
            // now copy from full to single
			unsigned char *startx = full_tile_data + xt * tilewidth * bpp;
			for (unsigned int crow = 0; crow < tileheight; ++crow) {
				memcpy(tile_data + crow * tilewidth * bpp, startx + full_tile_width * bpp * crow, tilewidth * bpp);
			}
             TIFFWriteEncodedTile(tifout,
                                TIFFComputeTile(tifout,
                                                    xt * tilewidth,
                                                    current_tile_row * tileheight,
                                                    0, 0),
                                                    tile_data, tilewidth * tileheight * bpp);
        }
        current_tile_row++;
        current_tile_y = 0;
    }}
    std::cout << std::endl;
    for (unsigned int t = 0; t < numsrcx; ++t) {
        if (sources[t]) stbi_image_free(sources[t]);
    } 
    delete[] sources;
    TIFFFlush(tifout);
 
    TIFFClose(tifout);
    delete[] full_tile_data;
    delete[] tile_data;
    return 0;
  }
