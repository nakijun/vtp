// (c) by Stefan Roettger

#ifndef PNGBASE_H
#define PNGBASE_H

unsigned char *decompressPNGimage(unsigned char *data,unsigned int bytes,int *width,int *height,int *components);
void compressPNGimage(unsigned char *image,int width,int height,int components,unsigned char **data,unsigned int *bytes,float gamma=0.0f);

#endif
