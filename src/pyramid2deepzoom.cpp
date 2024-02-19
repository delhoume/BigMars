#include <iostream>
#include <cstring>
using namespace std;

extern "C" {
#include <tiffio.h>
}


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

char tileBuffer[128];
const char* dziFile = "%sDeepZoom.dzi";
const char* tilePattern = "%sDeepZoom_files/%d/%d_%d.jpg";

const char* dziTemplateJSON = "{\n\
 \"Image\": {\n\
       \"xmlns\":    \"http://schemas.microsoft.com/deepzoom/2008\",\n\
        \"Format\":   \"jpg\",\n\
        \"Overlap\":  \"0\",\n\
        \"TileSize\": \"%d\",\n\
        \"minLevel\": \"%d\",\n\
        \"maxLevel\": \"%d\",\n\
        \"Size\": {\n\
            \"Width\":  \"%d\",\n\
            \"Height\": \"%d\",\n\
        }\
    }\
}";

const char* dziTemplateXML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<Image xmlns=\"http://schemas.microsoft.com/deepzoom/2008\"\n\
       Format=\"jpg\"\n\
       Overlap=\"0\"\n\
       TileSize=\"%d\"\n\
       minLevel=\"%d\"\n\
       maxLevel=\"%d\">\n\
    <Size Width=\"%d\"\n\
          Height=\"%d\"/>\n\
</Image>";

int 
main(int argc, char* argv[]) {
    TIFFSetErrorHandler(NULL);
    TIFFSetErrorHandlerExt(NULL);
   TIFFSetWarningHandler(NULL);
    TIFFSetWarningHandlerExt(NULL);
    TIFF* tifin = TIFFOpen(argv[1], "r");
    if (!tifin)
        return 1;

     // general info available
    uint16_t bitspersample, samplesperpixel;

    TIFFGetField(tifin, TIFFTAG_BITSPERSAMPLE, &bitspersample);
    TIFFGetField(tifin, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);

        //get directory
    unsigned int nDirectories = TIFFNumberOfDirectories(tifin);
    std::cout << "Directories: " << nDirectories << std::endl;
    // full image must be first, only one pass for now
    unsigned int maxlevel;
    unsigned int imagewidth;   
    unsigned int imageheight;
   unsigned int tilewidth;   
    unsigned int tileheight;
    TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &imagewidth);
    TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &imageheight);
    TIFFGetField(tifin, TIFFTAG_TILEWIDTH, &tilewidth);
    TIFFGetField(tifin, TIFFTAG_TILELENGTH, &tileheight);
    unsigned int filetype;
    TIFFGetFieldDefaulted(tifin, TIFFTAG_SUBFILETYPE, &filetype);
    const char* name  = argc > 2 ? argv[2] : 0;

    if (filetype == FILETYPE_REDUCEDIMAGE) {
    } else { //  compute levels
        unsigned int maxdimension = imagewidth;
        if (imageheight > imagewidth) maxdimension = imageheight;
        maxlevel = ceil(log(maxdimension)/log(2));
        std::cout << "Largest image: " << imagewidth << " " << imageheight << ": " << maxlevel << " levels" << std::endl;
         if (argc == 2) return 0;
        char outBuffer[1024];
        sprintf(outBuffer, dziFile, name);
        FILE* tp = fopen(outBuffer, "w");
        unsigned int len = sprintf(outBuffer, dziTemplateXML,  tilewidth, maxlevel - nDirectories + 1, maxlevel, imagewidth, imageheight);
        fwrite(outBuffer, 1, len, tp);
        fclose(tp);
    }

      for (unsigned int d = 0; d < nDirectories; ++d) {
        TIFFSetDirectory(tifin, d);
        TIFFGetField(tifin, TIFFTAG_IMAGEWIDTH, &imagewidth);
        TIFFGetField(tifin, TIFFTAG_IMAGELENGTH, &imageheight);
       if (TIFFIsTiled(tifin)) {
            unsigned int level = maxlevel - d;
            unsigned int alltiles = TIFFNumberOfTiles(tifin); 
            unsigned int numtilesx = (imagewidth + tilewidth-1)/tilewidth;
            unsigned int numtilesy = (imageheight + tileheight-1)/tileheight;
            unsigned int tileImageBufferSize = tilewidth * samplesperpixel * tileheight;
            unsigned char* tileImageBuffer = new unsigned char[tileImageBufferSize];
             std::cout << "Level " << level << " " << imagewidth << "x" << imageheight << std::endl;
             std::cout << "Tiles  " << numtilesx << "x" <<  numtilesy << " tiles total " << (numtilesx * numtilesy) << std::endl;
            for (unsigned row = 0; row < numtilesy; ++row) {
                for (unsigned int col = 0; col < numtilesx; ++col) {
                    TIFFReadEncodedTile(tifin, TIFFComputeTile(tifin, col * tilewidth, row * tileheight, 0, 0),
                        tileImageBuffer, tileImageBufferSize);
                    char outBuffer[128];
                    sprintf(outBuffer, tilePattern, name, level, col, row);
                    std::cout << "   tile " << outBuffer << " " << (row * numtilesx  + col + 1) << " / " << (numtilesx * numtilesy) << "    \r";
                    std::cout.flush();
                  stbi_write_jpg(outBuffer, tilewidth, tileheight, samplesperpixel, tileImageBuffer, 85);
                }
            }
            delete [] tileImageBuffer;
            std::cout << endl;
        }
    }
    std::cout << std::endl << "All done" << std::endl;
    TIFFClose(tifin);
    return 0;
}
