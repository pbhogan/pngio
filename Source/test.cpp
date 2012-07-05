#include <assert.h>
#include "pngio.h"
#define MIN( a, b ) ((a < b) ? a : b)


static uint8_t png_image_copy( const char * sourceFilePath, const char * targetFilePath, uint32_t saveFlags )
{
	png_image image;
	png_image_init( & image );
	uint8_t result = 0;
	if (png_image_load_path( & image, sourceFilePath, PNG_IMAGE_NONE ))
	{
		result = png_image_save_path( & image, targetFilePath, saveFlags );
	}	
	png_image_free( & image );
	return result;
}


static png_pixel make_pixel( uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF )
{
	png_pixel pixel = { r, g, b, a };
	return pixel;
}


static bool operator == ( png_pixel p1, png_pixel p2 )
{
	return (memcmp( & p1, & p2, sizeof(png_pixel) ) == 0);
}


static bool pixel_is( png_pixel pixel, uint8_t r, uint8_t g, uint8_t b, uint8_t a )
{
	if (pixel.r != r) printf( "Expected R = 0x%02X and got 0x%02X \n", r, pixel.r );
	if (pixel.g != g) printf( "Expected G = 0x%02X and got 0x%02X \n", g, pixel.g );
	if (pixel.b != b) printf( "Expected B = 0x%02X and got 0x%02X \n", b, pixel.b );
	if (pixel.a != a) printf( "Expected A = 0x%02X and got 0x%02X \n", a, pixel.a );
	return pixel == make_pixel( r, g, b, a );
}


static void test_24_bit_image( void )
{
	png_image image;

	assert( image.load( "../../Images/Test24.png" ) );
	assert( image.width  == 24 );
	assert( image.height == 24 );
	
	assert( pixel_is( image.get_pixel( 0,  0  ), 0xFF, 0x00, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  0  ), 0x00, 0xFF, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 0  ), 0x00, 0x00, 0xFF, 0xFF ) );
	assert( pixel_is( image.get_pixel( 0,  9  ), 0xFF, 0x00, 0x00, 0x80 ) );
	assert( pixel_is( image.get_pixel( 9,  9  ), 0x00, 0xFF, 0x00, 0x80 ) );
	assert( pixel_is( image.get_pixel( 16, 9  ), 0x00, 0x00, 0xFF, 0x80 ) );
	assert( pixel_is( image.get_pixel( 0,  16 ), 0xFF, 0x00, 0x00, 0x0D ) );
	assert( pixel_is( image.get_pixel( 9,  16 ), 0x00, 0xFF, 0x00, 0x0D ) );
	assert( pixel_is( image.get_pixel( 16, 16 ), 0x00, 0x00, 0xFF, 0x0D ) );
}


static void test_24_bit_image_flipped( void )
{
	png_image image;

	assert( image.load( "../../Images/Test24.png", PNG_IMAGE_FLIP_VERTICAL ) );
	assert( image.width  == 24 );
	assert( image.height == 24 );
	
	assert( pixel_is( image.get_pixel( 0,  16 ), 0xFF, 0x00, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  16 ), 0x00, 0xFF, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 16 ), 0x00, 0x00, 0xFF, 0xFF ) );
	assert( pixel_is( image.get_pixel( 0,  9  ), 0xFF, 0x00, 0x00, 0x80 ) );
	assert( pixel_is( image.get_pixel( 9,  9  ), 0x00, 0xFF, 0x00, 0x80 ) );
	assert( pixel_is( image.get_pixel( 16, 9  ), 0x00, 0x00, 0xFF, 0x80 ) );
	assert( pixel_is( image.get_pixel( 0,  0  ), 0xFF, 0x00, 0x00, 0x0D ) );
	assert( pixel_is( image.get_pixel( 9,  0  ), 0x00, 0xFF, 0x00, 0x0D ) );
	assert( pixel_is( image.get_pixel( 16, 0  ), 0x00, 0x00, 0xFF, 0x0D ) );
}


static void test_24_bit_image_premultiplied( void )
{
	png_image image;

	assert( image.load( "../../Images/Test24.png", PNG_IMAGE_PREMULTIPLY_ALPHA ) );
	assert( image.width  == 24 );
	assert( image.height == 24 );
	
	assert( pixel_is( image.get_pixel( 0,  0  ), 0xFF, 0x00, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  0  ), 0x00, 0xFF, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 0  ), 0x00, 0x00, 0xFF, 0xFF ) );
	assert( pixel_is( image.get_pixel( 0,  9  ), 0x80, 0x00, 0x00, 0x80 ) );
	assert( pixel_is( image.get_pixel( 9,  9  ), 0x00, 0x80, 0x00, 0x80 ) );
	assert( pixel_is( image.get_pixel( 16, 9  ), 0x00, 0x00, 0x80, 0x80 ) );
	assert( pixel_is( image.get_pixel( 0,  16 ), 0x0D, 0x00, 0x00, 0x0D ) );
	assert( pixel_is( image.get_pixel( 9,  16 ), 0x00, 0x0D, 0x00, 0x0D ) );
	assert( pixel_is( image.get_pixel( 16, 16 ), 0x00, 0x00, 0x0D, 0x0D ) );
}


static void test_24_bit_image_interlaced( void )
{
	png_image image;

	assert( image.load( "../../Images/Test24Interlaced.png" ) );
	assert( image.width  == 24 );
	assert( image.height == 24 );
	
	assert( pixel_is( image.get_pixel( 0,  0  ), 0xFF, 0x00, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  0  ), 0x00, 0xFF, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 0  ), 0x00, 0x00, 0xFF, 0xFF ) );
	assert( pixel_is( image.get_pixel( 0,  9  ), 0xFF, 0x00, 0x00, 0x80 ) );
	assert( pixel_is( image.get_pixel( 9,  9  ), 0x00, 0xFF, 0x00, 0x80 ) );
	assert( pixel_is( image.get_pixel( 16, 9  ), 0x00, 0x00, 0xFF, 0x80 ) );
	assert( pixel_is( image.get_pixel( 0,  16 ), 0xFF, 0x00, 0x00, 0x0D ) );
	assert( pixel_is( image.get_pixel( 9,  16 ), 0x00, 0xFF, 0x00, 0x0D ) );
	assert( pixel_is( image.get_pixel( 16, 16 ), 0x00, 0x00, 0xFF, 0x0D ) );
}


static void test_8_bit_image( void )
{
	png_image image;

	assert( image.load( "../../Images/Test8.png" ) );
	assert( image.width  == 24 );
	assert( image.height == 24 );
	
	assert( pixel_is( image.get_pixel( 0,  0  ), 0xFF, 0x00, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  0  ), 0x00, 0xFF, 0x00, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 0  ), 0x00, 0x00, 0xFF, 0xFF ) );
	assert( pixel_is( image.get_pixel( 0,  9  ), 0xFF, 0x7F, 0x7F, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  9  ), 0x7F, 0xFF, 0x7F, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 9  ), 0x7F, 0x7F, 0xFF, 0xFF ) );
	assert( pixel_is( image.get_pixel( 0,  16 ), 0xFF, 0xF2, 0xF2, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  16 ), 0xF2, 0xFF, 0xF2, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 16 ), 0xF2, 0xF2, 0xFF, 0xFF ) );
}


static void test_8_bit_image_grayscale( void )
{
	png_image image;

	assert( image.load( "../../Images/Test8Grayscale.png" ) );
	assert( image.width  == 24 );
	assert( image.height == 24 );
	
	assert( pixel_is( image.get_pixel( 0,  0  ), 0x55, 0x55, 0x55, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  0  ), 0xAA, 0xAA, 0xAA, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 0  ), 0x55, 0x55, 0x55, 0xFF ) );
	assert( pixel_is( image.get_pixel( 0,  9  ), 0xAA, 0xAA, 0xAA, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  9  ), 0xAA, 0xAA, 0xAA, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 9  ), 0xAA, 0xAA, 0xAA, 0xFF ) );
	assert( pixel_is( image.get_pixel( 0,  16 ), 0xF6, 0xF6, 0xF6, 0xFF ) );
	assert( pixel_is( image.get_pixel( 9,  16 ), 0xF6, 0xF6, 0xF6, 0xFF ) );
	assert( pixel_is( image.get_pixel( 16, 16 ), 0xF6, 0xF6, 0xF6, 0xFF ) );
}


static void test_image_apple( void )
{
	png_image image;

	assert( image.load( "../../Images/TestApple.png" ) );
	assert( image.width  == 114 );
	assert( image.height == 114 );
}


static void test_image_save( void )
{
	png_image image1, image2;

	assert( image1.load( "../../Images/Test24.png" ) );
	assert( image1.save( "../../Images/Save24.png" ) );
	assert( image2.load( "../../Images/Save24.png" ) );
	assert( image1.width  == image2.width );
	assert( image1.height == image2.height );
	
	for (int i = 0; i < image1.width; i++)
	{
		for (int j = 0; j < image1.height; j++)
		{
			assert( image1.get_pixel( i, j ) == image2.get_pixel( i, j ) );
		}
	}
}


int main( int argc, const char * argv[] )
{
	test_24_bit_image();
	test_24_bit_image_flipped();
	test_24_bit_image_premultiplied();
	test_24_bit_image_interlaced();
	test_8_bit_image();
	test_8_bit_image_grayscale();
	test_image_apple();
	test_image_save();
	
	return 0;
}

