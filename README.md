This project is guide for creating an single TIFF file and a Deep Zoom structure that will allow viewing interactively
a gigantic (42678000 x 2086480 pixels, that's a lot, at 89 terapixels).

The final TIFF image weights about 1 Terabyte, includes pre-computed sublevels and is tiled to allow even modest computers
to be able to view, zoom unzoom pan the full image.

This specially crafted pyramidal tiled TIFF can be opened with my Open Source software for Windows https://github.com/delhoume/vliv .
The generated Deep Zoom structure can be served using any Http server and visualized with any browser thanks to the OpenSeaDragon project :
https://openseadragon.github.io/

Thge source for the data is source from https://murray-lab.caltech.edu/CTX/index.html

3960 ZIP files (tiles) are available, each containing information and data for a part of the surface that covers 4 square degree.
We will only use the TIFF that holds pixel data (greyscale 1 byte / per pixel), has a 47420x47420 pixel size, and is uncompressed.
A single pixel covers about 5 meters.

They are available at https://murray-lab.caltech.edu/CTX/V01/tiles/ 

The first step towards the final image is to download the 3960 zip files.
This requires about 8 terabytes of disk space, I bought a 12To WD MyBook to store them.

The scripts/download_all.sh script will download all, thanks to their HTTP server supporting Accept-Ranges.
You can stop at any moment and downloading will resume when you relaunch it.
The download is split in 4 parts because I did not find how to create a single cUrl command for the files pattern.

Files are named using longitudes from -180 to 176  and latitudes from -88 to 84 in 4 increment

>curl -C -   "https://murray-lab.caltech.edu/CTX/V01/tiles/MurrayLab_GlobalCTXMosaic_V01_E[000-176:4]_N[00-84:4].zip" -O
>curl -C -   "https://murray-lab.caltech.edu/CTX/V01/tiles/MurrayLab_GlobalCTXMosaic_V01_E[000-176:4]_N-[04-88:4].zip" -O
>curl -C -   "https://murray-lab.caltech.edu/CTX/V01/tiles/MurrayLab_GlobalCTXMosaic_V01_E-[004-180:4]_N[00-84:4].zip" -O
>curl -C -   "https://murray-lab.caltech.edu/CTX/V01/tiles/MurrayLab_GlobalCTXMosaic_V01_E-[004-180:4]_N-[04-88:4].zip" -O


After a while, depending on your Internet connection speed, you should have the 3960 files. It took me about one month.
