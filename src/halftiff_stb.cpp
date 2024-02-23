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

// copying 4 corners
void CopytoTL(const unsigned char* source, unsigned char* dest, const unsigned int tilesize, const unsigned int spp) {
	const unsigned int halftilesize = tilesize / 2;
	const unsigned char* startxi = source;
	for (unsigned int y = 0; y < halftilesize; ++y) {
		const unsigned char* startxi = source + spp * y * halftilesize;
		unsigned char* startxo = dest + spp * y * tilesize;
		memcpy(startxo, startxi, halftilesize * spp);
	}
}

void CopytoTR(const unsigned char* source, unsigned char* dest, const unsigned int tilesize, const unsigned int spp) {
	const unsigned int halftilesize = tilesize / 2;
	for (unsigned int y = 0; y < halftilesize; ++y) {
		const unsigned char* startxi = source  + spp * y * halftilesize;
		unsigned char* startxo = dest + spp * y * tilesize + spp * halftilesize;
		memcpy(startxo, startxi, halftilesize * spp);
	}
}

void CopytoBL(const unsigned char* source, unsigned char* dest, const unsigned int tilesize, const unsigned int spp) {
	const unsigned int halftilesize = tilesize / 2;
	for (unsigned int y = 0; y < halftilesize; ++y) {
		const unsigned char* startxi = source  + spp * y * halftilesize;
		unsigned char* startxo = dest + spp * (y + halftilesize) * tilesize;
		memcpy(startxo, startxi, halftilesize * spp);
	}
}

void CopytoBR(const unsigned char* source, unsigned char* dest, const unsigned int tilesize, const unsigned int spp) {
	const unsigned int halftilesize = tilesize / 2;
	for (unsigned int y = 0; y < halftilesize; ++y) {	
		const unsigned char* startxi = source  + spp * y * halftilesize;
		unsigned char* startxo = dest + spp * (y + halftilesize) * tilesize + halftilesize * spp;
		memcpy(startxo, startxi, halftilesize * spp);
	}
}
 
int MYCheckTile(unsigned int col, unsigned int row, unsigned int numtilesx, unsigned int numtilesy) {
	int ret = 1;
	if (col >= numtilesx) {
//		cout << " out of range column " << col <<  " (" << numtilesx << ")" << endl;
		ret = 0;
	}
	if (row >= numtilesy) {
//		cout << " out of range row " << row <<  " (" << numtilesy << ")" << endl;
		ret = 0;
	}
	return ret;
}

int main(int argc, char* argv[]) {
//	TIFFSetWarningHandler(0);
 
    TIFF* tifin = TIFFOpen(argv[1], "rb");
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
#if 10
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
	if ((numtilesx == 1) || (numtilesy == 1)) {
		std::cout << "final image smaller than one tile, nothing to do" << endl;
		TIFFClose(tifin);
		return 1;
    }

	std::cout << "tilesize: " << tilewidth << " x " << tileheight << endl;
 	std::cout << "image tiled size: " << (numtilesx * tilewidth) << " x " << (numtilesx * tileheight)<< std::endl;
 	std::cout << "total tiles: " << ntiles << " (" << numtilesx << " x " << numtilesy << ")" << std::endl;
	
    unsigned int newwidth = (imagewidth + 1) >> 1;
    unsigned int newheight = (imageheight + 1) >> 1;
	if (newwidth < 1) newwidth = 1;	
	if (newheight < 1) newheight = 1;

 	#if 0
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
	
    unsigned char* toplefto = new unsigned char[halftiledatasize];
    unsigned char* toprighto = new unsigned char[halftiledatasize];
    unsigned char* bottomlefto = new unsigned char[halftiledatasize];
    unsigned char* bottomrighto = new unsigned char[halftiledatasize];
	
   TIFF* tifin_tr = TIFFOpen(argv[1], "rb");   
   TIFF* tifin_br = TIFFOpen(argv[1], "rb");   
   TIFF* tifin_bl = TIFFOpen(argv[1], "rb");
    for (unsigned int row = 0; row < newtilesy; ++row) {
		for (unsigned int col = 0; col < newtilesx; ++col) {
#pragma omp parallel sections num_threads(4)
   			 {
			// read all 4 corners
#pragma omp section
				{
					// top left
					if (MYCheckTile(col * 2, row * 2, numtilesx, numtilesy)) {
						ttile_t tilenum = TIFFComputeTile(tifin, col * 2 * tilewidth, row * 2 * tileheight, 0, 0);
						TIFFReadEncodedTile(tifin, tilenum, (uint32_t*)topleft, tiledatasize);
					} else {
						memset(topleft, 192, tilewidth * tileheight * spp);
					}
					// now scale them into newtiledata
					stbir_resize_uint8_linear(topleft, tilewidth, tileheight, 0, toplefto, halftilewidth, halftileheight, 0, (stbir_pixel_layout)spp);
					CopytoTL(toplefto, newtiledata, tilewidth, spp);  
				}
#pragma omp section
				 // top right
				{
				if (MYCheckTile(col * 2 + 1, row * 2, numtilesx, numtilesy)) {
					ttile_t tilenum = TIFFComputeTile(tifin_tr, (col * 2 + 1) * tilewidth, row * 2 * tileheight, 0, 0);
					TIFFReadEncodedTile(tifin_tr, tilenum,  (uint32_t*)topright, tiledatasize);
					} else {
						memset(topright, 64, tilewidth * tileheight * spp);
					}
					stbir_resize_uint8_linear(topright, tilewidth, tileheight, 0, toprighto, halftilewidth, halftileheight, 0, (stbir_pixel_layout)spp);
					CopytoTR(toprighto, newtiledata, tilewidth, spp);  
				}
#pragma omp section
				{
					// bottom left
					if (MYCheckTile(col * 2, row * 2 + 1, numtilesx, numtilesy)) {
					ttile_t tilenum = TIFFComputeTile(tifin_bl, col * 2 * tilewidth, (row * 2 + 1) * tileheight, 0, 0);
						TIFFReadEncodedTile(tifin_bl, tilenum, (uint32_t*)bottomleft, tiledatasize);
					} else {
						memset(bottomleft, 128, tilewidth * tileheight * spp);
					}
					stbir_resize_uint8_linear(bottomleft, tilewidth, tileheight, 0, bottomlefto, halftilewidth, halftileheight, 0, (stbir_pixel_layout)spp);
					CopytoBL(bottomlefto, newtiledata, tilewidth, spp);  
				}
#pragma omp section
				{
					// bottom right
					if (MYCheckTile(col * 2 + 1, row * 2 + 1, numtilesx, numtilesy)) {
					ttile_t tilenum = TIFFComputeTile(tifin_br, (col * 2 + 1) * tilewidth, (row * 2 + 1) * tileheight, 0, 0);
					TIFFReadEncodedTile(tifin_br, tilenum, (uint32_t*)bottomright, tiledatasize); 
					} else {
						memset(bottomright, 255, tilewidth * tileheight * spp);
					}	
					stbir_resize_uint8_linear(bottomright, tilewidth, tileheight, 0, bottomrighto, halftilewidth, halftileheight, 0, (stbir_pixel_layout)spp);
					CopytoBR(bottomrighto, newtiledata, tilewidth, spp);  
				}
		}
			if (DEBUG) {
				if (row == 0 && col == 0) {
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
					stbi_write_png(buffer, halftilewidth, halftileheight, spp, toplefto, 0);
					sprintf(buffer, "debug/toprightto_%d_%d.png", row, col);
					stbi_write_png(buffer, halftilewidth, halftileheight, spp, toprighto, 0);
					sprintf(buffer, "debug/bottomleftto_%d_%d.png", row, col);
					stbi_write_png(buffer, halftilewidth, halftileheight, spp, bottomlefto, 0);
					sprintf(buffer, "debug/bottomrightto_%d_%d.png", row, col);
					stbi_write_png(buffer, halftilewidth, halftileheight, spp, bottomrighto, 0);
					
					sprintf(buffer, "debug/newtile_%d_%d.png", row, col);
					stbi_write_png(buffer, tilewidth, tilewidth, spp, newtiledata, 0);
				}
			}

			// and save
			int tilenum = TIFFComputeTile(tifout, col * tilewidth, row * tileheight, 0, 0);
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
    delete [] toplefto;
    delete [] toprighto;
    delete [] bottomlefto;
    delete [] bottomrighto;
    return 0;
}
