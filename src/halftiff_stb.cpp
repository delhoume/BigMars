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
 
int MYTIFFCheckTile(unsigned int tx, unsigned int ty, unsigned int tilesize, 
				unsigned int imagewidth, unsigned int imageheight) {
	if ((tx * tilesize) >= imagewidth) {
//		cout << " bad x tile " << tx <<  "(" << tilesize << " " << imagewidth << ")" << endl;
		return 0;
	}
	if ((ty * tilesize) >= imageheight) {
//		cout << " bad y tile " << ty <<  "(" << tilesize << " " << imageheight << ")" << endl;
		return 0;
	}
	return 1;
}

int main(int argc, char* argv[]) {
//	TIFFSetWarningHandler(0);
 
    TIFF* tifin = TIFFOpen(argv[1], "rb");

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

    std::cout << "original size:" << imagewidth << "x" << imageheight << endl;
    if (!TIFFIsTiled(tifin)) {
	std::cout << "image is not tiled, please convert using strips2tiled" << endl;
	return 1;
    }
    std::cout << "Tile size: " << tilewidth << " x " << tileheight << endl;
    unsigned int numtilesx = imagewidth / tilewidth;
    if (imagewidth % tilewidth) ++numtilesx;

    unsigned int numtilesy = imageheight / tileheight;
    if (imageheight % tileheight) ++numtilesy;

    unsigned int newwidth = (unsigned int)floor(imagewidth / 2.0);
    unsigned int newheight = (unsigned int)floor(imageheight / 2.0);
    unsigned int newtilesx = newwidth / tilewidth;
    if (newwidth % tilewidth) ++newtilesx;

    unsigned int newtilesy = newheight / tileheight;
    if (newheight % tileheight)	++newtilesy;

    std::cout << "bits per sample: " << bps << endl;
    std:: cout << "samples per pixel: " << spp << endl;
	std::cout << "photometric: ";
	switch (photometric) {
		case PHOTOMETRIC_MINISWHITE: { std::cout << "min is white"; break; }
		case PHOTOMETRIC_MINISBLACK: { std::cout << "min is black"; break; }
		case PHOTOMETRIC_RGB:        { std::cout << "rgb"; break; }
		case PHOTOMETRIC_PALETTE:    { std::cout << "palette"; break; }
		default:                     { std::cout << photometric; }
	}
		
	std::cout << endl;
    std::cout << "original tiles: " << numtilesx << "x" << numtilesy << endl;
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
		std::cout << "writing tiles for row " << row << " (of " << (newtilesy - 1) << ") ";
		for (unsigned int col = 0; col < newtilesx; ++col) {
#pragma omp parallel sections num_threads(4)
   			 {
			// read all 4 corners
#pragma omp section
				{
					// top left
					if (MYTIFFCheckTile(col * 2, row * 2, tilewidth, imagewidth, imageheight)) {
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
				if (MYTIFFCheckTile(col * 2 + 1, row * 2, tilewidth, imagewidth, imageheight)) {
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
					if (MYTIFFCheckTile(col * 2, row * 2 + 1,  tilewidth, imagewidth, imageheight)) {
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
					if (MYTIFFCheckTile(col * 2 + 1, row * 2 + 1,  tilewidth, imagewidth, imageheight)) {
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
			bool cond = true;
			unsigned char symb = 'I';
			if (newtilesy >= 20) {
				if (newtilesy >= 200) {
					cond = (col % 100) == 0;
					symb = 'C';
				} else {
					cond = (col  % 10) == 0;
					symb = 'X';
				}
			}
			if (cond) {
				std::cout << symb;
			}
			std::cout.flush();
		}
		std::cout << endl;
    }
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
