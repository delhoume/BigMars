#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <chrono>

/*
MacOS buid:
 dependencies : brew install g++ libtiff jpegturbo xz
              : https://github.com/nothings/stb/blob/master/stb_image_resize2.h
              : https://github.com/nothings/stb/blob/master/stb_image.h
              : https://github.com/nothings/stb/blob/master/stb_image_write.h

g++-14 -std=c++11 -O2 -I /opt/homebrew/include -I . build_mosaic.cpp -o build_mosaic  -L /opt/homebrew/lib -ltiff -lturbojpeg -lz-ng -lz -lzstd -llzma

generated tiffs can be displayed using https://github.com/delhoume/vliv or https://github.com/delhoume/qshowtiff
  

*/
using namespace std;

extern "C" {
#include <tiffio.h>
#include <zlib.h>
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize2.h>

// delhoume@gmail.com 2024

#if !defined(DEFAULT_FOLDER)
#define DEFAULT_FOLDER "."
#endif

int repeat = 0;

const char *getName(int row, int col, int columns, const char* folder = DEFAULT_FOLDER) {
static char buff[256];
  int frame = col + row * columns;
  if (repeat > 0) frame = frame % repeat;
const char *format = "%s/image%d.jpg";
 snprintf(buff, sizeof(buff), format, folder, frame);
  FILE* f = fopen(buff, "r");
  if (f) {
    fclose(f);
    return buff;
  } 

  // buff[0] = 0;
  return buff;
}

std::string displayDuration(int seconds) {
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int sseconds = seconds % 60;
  int days = hours / 24;
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
    std::cout << "Usage: build_mosaic -out <result> -dims <width> <height> -folder <source_image_folder> -cols <cols> -count <num_images> -tilesize <size> -border <border> -keepratio yes" << std::endl;
    std::cout << "       creates a mosaic image from possibly huge number of individual images with minimal memory" << std::endl; 
    std::cout << "       images must follow \"image%d.jpg\" pattern" << std::endl;      
    std::cout << "       default values:" << std::endl;      
    std::cout << "          -dims 1000 1000    (the dimension of images in final mosaic)" << std::endl;         
    std::cout << "          -folder .          (the folder in which source images reside)" << std::endl;         
    std::cout << "          -out out.tif" << std::endl;         
    std::cout << "          -cols 10" << std::endl;         
    std::cout << "          -count 100         (the number of source images)" << std::endl;         
    std::cout << "          -tilesize 512" << std::endl;      
    std::cout << "          -border 5" << std::endl;         
    std::cout << "          -tilesize 512" << std::endl;     
    std::cout << "          -keepratio yes     (keep source images original ratio or stretch to -dims)" << std::endl;   
    std::cout << "       examples:" << std::endl;      
    std::cout << "         build_mosaic -out mosaic.tif -dims 1000 1000 -folder images -cols 100 -count 10000 -border 10" << std::endl;
    std::cout << "          -> generates a (possibly empty if there are no source images in \"images\") 100100 x 100100 out.tif image" << std::endl;
    std::cout << "       delhoume@gmail.com 2024" << std::endl;
    return 1;
  }
 
  	TIFFOpenOptions *opts = TIFFOpenOptionsAlloc();
	TIFFOpenOptionsSetMaxSingleMemAlloc(opts, 0);

int dstwidth = 1000;
int dstheight = 1000;
int border = 5;
int columns = 10;
int count = 100;
int tilewidth = 512;
const char* outfilename = "out.tif";
bool keepratio =  true;

 
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
    } else if (!strcmp("-dims", argv[arg])) {
        dstwidth = atoi(argv[arg + 1]);
        dstheight = atoi(argv[arg + 2]);
        argsstart += 3;
    } else if (!strcmp("-out", argv[arg])) {
        outfilename = argv[arg + 1];
        argsstart += 2;
    } else if (!strcmp("-cols", argv[arg])) {
        columns = atoi(argv[arg + 1]);
        argsstart += 2;
    } else if (!strcmp("-count", argv[arg])) {
        count =  atoi(argv[arg + 1]);
        argsstart += 2;
    } else if (!strcmp("-border", argv[arg])) {
        border =  atoi(argv[arg + 1]);
        argsstart += 2;
    } else if (!strcmp("-tilesize", argv[arg])) {
        tilewidth =  atoi(argv[arg + 1]);
        argsstart += 2;
    } else if (!strcmp("-repeat", argv[arg])) { // test
        repeat =  atoi(argv[arg + 1]);
        argsstart += 2;
    } else if (!strcmp("-keepratio", argv[arg])) { // test
        if(!strcmp("yes", argv[arg + 1]))
             keepratio= false;
        argsstart += 2;
    }
}


int numsrcx = columns; 
int numsrcy = count / numsrcx;
if (numsrcy < 1) numsrcy = 1;


   int samples_per_pixel = 3;

   int dstwidth_with_border = dstwidth + border * 2;
   int dstheight_with_border = dstheight + border * 2;


  int tileheight = tilewidth;

  int imagewidth = numsrcx * dstwidth_with_border;
  int imageheight = numsrcy * dstheight_with_border;

    int numtilesx = imagewidth / tilewidth;
	if (imagewidth % tilewidth)
		++numtilesx;

    int numtilesy = imageheight / tileheight;
	if (imageheight % tileheight)
		++numtilesy;

         // try to allocate a complete row of tiles !!
    int full_tile_width = numtilesx * tilewidth;

	auto full_tile_data = new unsigned char[full_tile_width * tileheight * samples_per_pixel];
	auto tile_data = new unsigned char[tilewidth * tileheight * samples_per_pixel];

    std::cout << std::setprecision(2);
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

  TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel);

	std::cout << "final size: " << imagewidth << "x" << imageheight << std::endl;
	std::cout << "tile size: " << tilewidth << "x" << tileheight << std::endl;
	std::cout << "num tiles: "<< numtilesx << "x" <<  numtilesy << std::endl;
    std::cout << "compression: "<< (useJPEG ? "JPEG" : "Deflate") << std::endl;
 if (useJPEG) {
        int quality = 75;
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

 auto sources  = new unsigned char*[numsrcx];
  for (int t = 0; t < numsrcx; ++t)
    sources[t] = 0;

  int tilex = 0;
  int tiley = 0;
  int current_src_row = 0; // index of current bigpart (0 to numsrcy)
  int current_tile_row = 0; // index of current y  tile (0 to numtilesy)
  int current_tile_y = 0; // index in current tile (0 to tileheight)
  int current_src_strip = 0; // strip in current bigpart (0 to dstheight)

  unsigned char background_color[4] = { 0x88, 0x88, 0x88, 0x00};

  for (int y = 0; y < imageheight; ++y) {
    if (current_src_strip == 0) {
        std::cout << std::endl << "loading " << numsrcx << " new sources for input row " << current_src_row 
        << " at line " << y << "/" << imageheight << std::endl;
      for (int xx = 0; xx < numsrcx; ++xx) {
           if (sources[xx]) stbi_image_free(sources[xx]);

            unsigned char* frame = new unsigned char[dstwidth_with_border * dstheight_with_border * samples_per_pixel];
            // fill with background
            for (int idx = 0; idx < dstwidth_with_border * dstheight_with_border; ++idx)
                memcpy(frame + idx * samples_per_pixel, background_color, samples_per_pixel);         
            const char *name = getName(current_src_row, xx, numsrcx,  folder);
            int xi, yi, ni; 
            unsigned char* orig = stbi_load(name, &xi, &yi, &ni, 3);
                if (orig != nullptr) {
                      float ratio = xi / (float)yi;
                      int newwidth = dstwidth;
                    int newheight = dstheight;
                    if (keepratio == true) {
                        if (xi > yi) // keep width
                            newheight = dstheight / ratio;
                        else
                            newwidth = dstwidth * ratio;
                     }
                     // std::cout << ratio << " " << xi << " " << yi <<  " " << newwidth << " " << newheight  << std:: endl;
                    unsigned char* scaled = stbir_resize_uint8_linear(orig, xi, yi, 0, nullptr, newwidth, newheight, 0, (stbir_pixel_layout)3);
                    stbi_image_free(orig);
                    // center in frame
                    int offsety = abs((newheight - dstheight) / 2);
                    int offsetx = abs((newwidth -  dstwidth) / 2);
                    unsigned char* starty = frame + (border + offsety) * dstwidth_with_border * samples_per_pixel;
                    for (int y = 0; y < newheight; y++) {
                        memcpy(starty + (border + offsetx) * samples_per_pixel, scaled + y * newwidth * samples_per_pixel, newwidth * samples_per_pixel);
                        starty += dstwidth_with_border * samples_per_pixel;
                    }
                    stbi_image_free(scaled);
                    std::cout << ".";        std::cout.flush();
              }  else {
                std::cout << "x";        std::cout.flush();
            } 

            sources[xx] = frame;         
        }
        std::cout << std::endl;
        std::cout.flush();
        current_src_row++;
    }
 //     std::cout << "current src strip " << current_src_strip <<  std::endl;
    // fill one line at current tile y position from current y position in input row
    // TODO: add an optional border between images
    for (int xx = 0; xx < numsrcx; ++xx) {
        unsigned char* pos = full_tile_data +  current_tile_y * full_tile_width * samples_per_pixel + xx * dstwidth_with_border * samples_per_pixel;
         if (sources[xx]) {
           memcpy(pos, sources[xx] + dstwidth_with_border * samples_per_pixel * current_src_strip, dstwidth_with_border * samples_per_pixel);
        } 
    }

    current_src_strip++;
    if (current_src_strip >= dstheight_with_border)
        current_src_strip = 0;

    // save eventually a row of tiles when complete or last y row
     current_tile_y++;
    if ((current_tile_y >= tileheight) || (y == (imageheight - 1))) {
        std::cout << "writing tile row " << (current_tile_row + 1) << " of " << numtilesy << " at line " << (y + 1) << " of " << imageheight << " for " << current_src_row;
            const auto ctime = chrono::system_clock::now();
        const auto differenceFromStart = std::chrono::duration_cast<std::chrono::seconds>(ctime - stime).count();

        float rate = differenceFromStart / (float)y;
        int remainingSecs = floor((float)(imageheight - y) * rate);

        std::cout << std::endl;
        // std::cout  << "|elapsed " << displayDuration(differenceFromStart);
        // std::cout << "|predicted remaining time " << displayDuration(remainingSecs) << std::endl;
        // // char buffer[32];
        // sprintf(buffer, "debug/row_%d.jpg", current_tile_row);
        // stbi_write_jpg(buffer, (int) full_tile_width, (int)tileheight, bpp, full_tile_data, 80);
        for (int xt = 0; xt < numtilesx; ++xt) {
            // now copy from full to single
            unsigned char *startx = full_tile_data + xt * tilewidth * samples_per_pixel;
            for (int crow = 0; crow < tileheight; ++crow) {
              memcpy(tile_data + crow * tilewidth * samples_per_pixel, startx + full_tile_width * samples_per_pixel * crow, tilewidth * samples_per_pixel);
            }
             TIFFWriteEncodedTile(tifout,
                                TIFFComputeTile(tifout,
                                                    xt * tilewidth,
                                                    current_tile_row * tileheight,
                                                    0, 0),
                                                    tile_data, tilewidth * tileheight * samples_per_pixel);
        }
        current_tile_row++;
        current_tile_y = 0;
    }}
    for (int t = 0; t < numsrcx; ++t) {
        if (sources[t]) stbi_image_free(sources[t]);
    } 
    delete[] sources;
    TIFFFlush(tifout);
 
    TIFFClose(tifout);
    delete[] full_tile_data;
    delete[] tile_data;
    return 0;
  }
