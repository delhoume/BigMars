#include <iostream>
#include <fstream>


#include <tiffio.h>
#include <zlib.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize2.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

#define Z_BEST_COMPRESSION 9
#define Z_NORMAL_COMPRESSION 4

using namespace std;

#if !defined(DEBUG)
#define DEBUG false
#endif

// fdelhoume 2024
// halftiff full_image.tif half_image.tif

int main(int argc, char* argv[]) {
	TIFFOpenOptions *opts = TIFFOpenOptionsAlloc();
	TIFFOpenOptionsSetMaxSingleMemAlloc(opts, 0);// unlimited

// tiles out of range...
  TIFFSetWarningHandler(NULL);
  TIFFSetWarningHandlerExt(NULL);
   TIFFSetErrorHandler(NULL);
     TIFFSetErrorHandlerExt(NULL);

    TIFF* tifin = TIFFOpenExt(argv[1], "rb", opts);
	TIFFOpenOptionsFree(opts);
	if (!tifin) {
		std::cout << "could not open " << argv[1] << std::endl;
		return 1;
	}

    unsigned int imagewidth = 0;
    unsigned int imageheight = 0;
    unsigned int tilewidth = 0;
    unsigned int tileheight = 0;
	unsigned int bps = 0;
	unsigned int spp = 3;
	unsigned int photometric = PHOTOMETRIC_RGB;
	unsigned int compression;

    TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &imagewidth);
    TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &imageheight);
    TIFFGetField(tifin, TIFFTAG_TILEWIDTH, &tilewidth);
    TIFFGetField(tifin, TIFFTAG_TILELENGTH, &tileheight);
   	TIFFGetField(tifin, TIFFTAG_BITSPERSAMPLE, &bps);
	TIFFGetField(tifin, TIFFTAG_PHOTOMETRIC, &photometric);
	TIFFGetField(tifin, TIFFTAG_SAMPLESPERPIXEL, &spp);
	TIFFGetFieldDefaulted(tifin, TIFFTAG_COMPRESSION, &compression);

    // std::cout << "bits per sample: " << bps << endl;
    // std:: cout << "samples per pixel: " << spp << endl;
	std::cout << "original size:" << imagewidth << "x" << imageheight << endl;
    if (!TIFFIsTiled(tifin)) {
		std::cout << "image is not tiled, please convert using strips2tiled" << endl;
		TIFFClose(tifin);
		return 1;
    }
	int ntiles = TIFFNumberOfTiles(tifin); 
#if 1
	unsigned int 	numtilesx = imagewidth / tilewidth;
	if (imagewidth % tilewidth)
		++numtilesx;

	unsigned int numtilesy = imageheight / tileheight;
	if (imageheight % tileheight)
		++numtilesy;
#else
	unsigned int numtilesx = (imagewidth + tilewidth-1)/tilewidth;
	unsigned int numtilesy = (imageheight + tileheight-1)/tileheight;
#endif
	if ((numtilesx == 1) && (numtilesy == 1)) {
		std::cout << "final image smaller than one tile, nothing to do" << endl;
		TIFFClose(tifin);
		return 1;
    }

	std::cout << "tilesize: " << tilewidth << " x " << tileheight << endl;
 	std::cout << "image tiled size: " << (numtilesx * tilewidth) << " x " << (numtilesy * tileheight)<< std::endl;
 	std::cout << "total tiles: " << ntiles << " (" << numtilesx << " x " << numtilesy << ")" << std::endl;
	
    unsigned int newwidth = (imagewidth + 1) >> 1;
    unsigned int newheight = (imageheight + 1) >> 1;
	if (newwidth < 1) newwidth = 1;	
	if (newheight < 1) newheight = 1;

#if 1
	unsigned int 	newtilesx = newwidth / tilewidth;
	if (newwidth % tilewidth)
		++newtilesx;

	unsigned int newtilesy = newheight / tileheight;
	if (newheight % tileheight)
		++newtilesy;
		#else
	unsigned int newtilesx = (newwidth + tilewidth-1)/tilewidth;
	unsigned int newtilesy = (newheight + tileheight-1)/tileheight;
#endif
		
	std::cout << endl;
    std::cout << "new size: " << newwidth << " x " << newheight << endl;
    std::cout << "new tiles: " << newtilesx << " x " << newtilesy << endl;

    TIFF* tifout = TIFFOpen(argv[2], "w8");
    TIFFSetField(tifout, TIFFTAG_IMAGEWIDTH, newwidth);
    TIFFSetField(tifout, TIFFTAG_IMAGELENGTH, newheight);

    TIFFSetField(tifout, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tifout, TIFFTAG_BITSPERSAMPLE, bps);
    TIFFSetField(tifout, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  	TIFFSetField(tifout, TIFFTAG_PHOTOMETRIC, photometric);
	TIFFSetField(tifout, TIFFTAG_SAMPLESPERPIXEL, spp);

    TIFFSetField(tifout, TIFFTAG_TILEWIDTH, tilewidth);
    TIFFSetField(tifout, TIFFTAG_TILELENGTH, tileheight);

	TIFFSetField(tifout, TIFFTAG_COMPRESSION, compression);

	if (compression == COMPRESSION_ADOBE_DEFLATE || compression == COMPRESSION_DEFLATE) {
		unsigned int complevel;
		TIFFGetFieldDefaulted(tifin, TIFFTAG_ZIPQUALITY, &complevel);
		TIFFSetField(tifout, TIFFTAG_ZIPQUALITY, complevel);	
	} else if (compression == COMPRESSION_JPEG) {
 		unsigned int complevel;
		TIFFGetFieldDefaulted(tifin, TIFFTAG_JPEGQUALITY, &complevel);
		TIFFSetField(tifout, TIFFTAG_JPEGQUALITY, complevel);
//		 TIFFSetField(tifout, TIFFTAG_JPEGTABLESMODE, 0);
	}

	unsigned int tiledatasize = tilewidth * tileheight * spp;
	unsigned int halftiledatasize = tiledatasize / 4;

    unsigned char* newtiledata = new unsigned char[tiledatasize];
   
    unsigned char* topleft = new unsigned char[tiledatasize];
    unsigned char* topright = new unsigned char[tiledatasize];
    unsigned char* bottomleft = new unsigned char[tiledatasize];
    unsigned char* bottomright = new unsigned char[tiledatasize];
	
	unsigned int halftilewidth = tilewidth / 2;
	unsigned int halftileheight = tileheight / 2;
	
// because of openmp and multithread	
   TIFF* tifin_tr = TIFFOpen(argv[1], "rb");   
   TIFF* tifin_br = TIFFOpen(argv[1], "rb");   
   TIFF* tifin_bl = TIFFOpen(argv[1], "rb");
     for (unsigned int row = 0; row < newtilesy; ++row) {
		for (unsigned int col = 0; col < newtilesx; ++col) {
			// init new tiles
			memset(topleft, 192, tilewidth * tileheight * spp);
			memset(topright, 64, tilewidth * tileheight * spp);
			memset(bottomleft, 128, tilewidth * tileheight * spp);
			memset(bottomright, 255, tilewidth * tileheight * spp);
			ttile_t tilenum;
			// top left
			tilenum = TIFFComputeTile(tifin, col * 2 * tilewidth, row * 2 * tileheight, 0, 0);
			TIFFReadEncodedTile(tifin, tilenum, (uint32_t*)topleft, tiledatasize);
			// top right
			if ((col * 2 + 1) < numtilesx) {
				TIFFReadEncodedTile(tifin_tr, tilenum + 1,  (uint32_t*)topright, tiledatasize);
			}
			// last row in source might be out of bounds, so do not read...
			if ((row * 2 + 1) < numtilesy) {
				// bottom left
				tilenum = TIFFComputeTile(tifin_bl, col * 2 * tilewidth, (row * 2 + 1) * tileheight, 0, 0);
				TIFFReadEncodedTile(tifin_bl, tilenum, (uint32_t*)bottomleft, tiledatasize);
				// bottom right
				if ((col * 2 + 1) < numtilesx) {
					TIFFReadEncodedTile(tifin_br, tilenum + 1, (uint32_t*)bottomright, tiledatasize); 
				}
			} else {
			//	std::cout << "tile out of bounds (likely last row in source) " << row * 2 + 1 << " - "<< numtilesy << std::endl;
			}
			// now scale them directly into newtiledata
#pragma omp parallel sections num_threads(4)			
       { // topleft
			stbir_resize_uint8_linear(topleft, tilewidth, tileheight, 0, newtiledata, halftilewidth, halftileheight, tilewidth * spp, (stbir_pixel_layout)spp);
#pragma omp section		
			stbir_resize_uint8_linear(topright, tilewidth, tileheight, 0, newtiledata + spp * halftilewidth, halftilewidth, halftileheight, tilewidth * spp, (stbir_pixel_layout)spp);
			// middle left  
#pragma omp section
			stbir_resize_uint8_linear(bottomleft, tilewidth, tileheight, 0, newtiledata + tilewidth * halftileheight * spp,
			 				halftilewidth, halftileheight, tilewidth * spp, (stbir_pixel_layout)spp);
#pragma omp section			
			stbir_resize_uint8_linear(bottomright, tilewidth, tileheight, 0, newtiledata + tilewidth * halftileheight * spp 
			                  + spp * halftilewidth, halftilewidth, halftileheight, tilewidth * spp, (stbir_pixel_layout)spp);
		}
		if (DEBUG) { // && row == 0 && col == 0) {
				char buffer[128];	
				sprintf(buffer, "debug/toplefti_%d_%d.png", row, col);
				stbi_write_png(buffer, tilewidth, tileheight, spp, topleft, 0);
				sprintf(buffer, "debug/toprighti_%d_%d.png", row, col);
				stbi_write_png(buffer, tilewidth, tileheight, spp, topright, 0);
				sprintf(buffer, "debug/bottomlefti_%d_%d.png", row, col);
				stbi_write_png(buffer, 	tilewidth, tileheight, spp, bottomright, 0);	
				sprintf(buffer, "debug/bottomrighti_%d_%d.png", row, col);
				stbi_write_png(buffer, tilewidth, tileheight, spp, bottomleft, 0);

				sprintf(buffer, "debug/toplefto_%d_%d.png", row, col);
				stbi_write_png(buffer, halftilewidth, halftileheight, spp, newtiledata, tilewidth * spp);
				sprintf(buffer, "debug/toprightto_%d_%d.png", row, col);
				stbi_write_png(buffer, halftilewidth, halftileheight, spp, newtiledata + halftilewidth * spp, tilewidth * spp);

				unsigned char* startpoint = newtiledata + tilewidth * halftileheight * spp;

				sprintf(buffer, "debug/bottomleftto_%d_%d.png", row, col);
				stbi_write_png(buffer, halftilewidth, halftileheight, spp, startpoint, tilewidth * spp);
				sprintf(buffer, "debug/bottomrightto_%d_%d.png", row, col);
				stbi_write_png(buffer, halftilewidth, halftileheight, spp, startpoint + halftilewidth * spp, tilewidth * spp);
				
				sprintf(buffer, "debug/newtile_%d_%d.png", row, col);
				stbi_write_png(buffer, tilewidth, tilewidth, spp, newtiledata, 0);
			}
			// and save
			tilenum = TIFFComputeTile(tifout, col * tilewidth, row * tileheight, 0, 0);
			TIFFWriteEncodedTile(tifout, tilenum, newtiledata, tiledatasize);
		}
		std::cout << "writing tiles for row " << (row + 1) << " / " << newtilesy << "   \r";
		std::cout.flush();
	}
	std::cout << endl;
    TIFFClose(tifout);
    TIFFClose(tifin);
    delete [] newtiledata;
    delete [] topleft;
    delete [] topright;
    delete [] bottomleft;
    delete [] bottomright;

    return 0;
}
