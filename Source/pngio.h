#ifndef _PNG_IO_H_
#define _PNG_IO_H_


#include <stdint.h>
#include <stdio.h>


#define pngio_malloc				malloc
#define pngio_free					free


#define PNG_FORMAT_INVALID			0
#define PNG_FORMAT_STANDARD			1
#define PNG_FORMAT_APPLE			2


#define PNG_IMAGE_NONE				0
#define PNG_IMAGE_OPTIMIZE_FOR_IOS	1
#define PNG_IMAGE_PREMULTIPLY_ALPHA	2
#define PNG_IMAGE_FLIP_VERTICAL		4


#ifdef __cplusplus
#include <iostream>
extern "C" {
#endif


struct png_pixel
{
	uint8_t r, g, b, a;
};
typedef struct png_pixel png_pixel;


struct png_image
{
	uint32_t   width;
	uint32_t   height;
	uint8_t  * data;
	
	#ifdef __cplusplus
	png_image( void );
	~png_image( void );
	bool load( std::istream & stream, uint32_t flags = PNG_IMAGE_NONE );
	bool save( std::ostream & stream, uint32_t flags = PNG_IMAGE_NONE );
	bool load( const std::string & path, uint32_t flags = PNG_IMAGE_NONE );
	bool save( const std::string & path, uint32_t flags = PNG_IMAGE_NONE );
	void set_pixel( uint32_t x, uint32_t y, png_pixel pixel );
	png_pixel get_pixel( uint32_t x, uint32_t y );
	uint8_t * take( void );
	#endif
};
typedef struct png_image png_image;


void png_image_init ( png_image * image );
void png_image_alloc( png_image * image, uint32_t width, uint32_t height );
void png_image_free ( png_image * image );
uint8_t * png_image_take( png_image * image );

uint8_t png_image_load( png_image * image, FILE * file, uint32_t flags );
uint8_t png_image_save( png_image * image, FILE * file, uint32_t flags );

uint8_t png_image_load_path( png_image * image, const char * path, uint32_t flags );
uint8_t png_image_save_path( png_image * image, const char * path, uint32_t flags );

void png_image_set_pixel( png_image * image, uint32_t x, uint32_t y, png_pixel pixel );
png_pixel png_image_get_pixel( png_image * image, uint32_t x, uint32_t y );


#ifdef __cplusplus
}
#endif

#endif