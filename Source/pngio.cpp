#include "pngio.h"
#include "libpng/png.h"
#include <stdlib.h>


static void png_read_file_data( png_structp readPtr, png_bytep data, png_size_t size ) 
{
	FILE * file = (FILE *) png_get_io_ptr( readPtr );
	fread( (char *) data, size, 1, file );
}


static void png_write_file_data( png_structp writePtr, png_bytep data, png_size_t size ) 
{
	FILE * file = (FILE *) png_get_io_ptr( writePtr );
	fwrite( (char *) data, size, 1, file );
}


static void png_flush_file_data( png_structp writePtr ) 
{
	FILE * file = (FILE *) png_get_io_ptr( writePtr );
	fflush( file );
}


static uint32_t png_read_file_format( FILE * file ) 
{
	char sign[8];
	if (fread( sign, 8, 1, file ) != 1)
	{
		return PNG_FORMAT_INVALID;
	}
	
	if (png_sig_cmp( (png_bytep) sign, 0, 8 ) != 0)
	{
		return PNG_FORMAT_INVALID;
	}
	
	if (fseek( file, 12, SEEK_SET ) != 0)
	{
		return PNG_FORMAT_INVALID;
	}
	
	char cgbi[4];
	if (fread( cgbi, 4, 1, file ) != 1)
	{
		return PNG_FORMAT_INVALID;
	}
	
	if (fseek( file, 8, SEEK_SET ) != 0)
	{
		return PNG_FORMAT_INVALID;
	}
	
	if (strncmp( cgbi, "CgBI", 4 ) == 0)
	{
		return PNG_FORMAT_APPLE;
	}

	return PNG_FORMAT_STANDARD;
}


void png_image_init( png_image * image )
{
	image->width = 0;
	image->height = 0;
	image->data = NULL;
}


void png_image_alloc( png_image * image, uint32_t width, uint32_t height )
{
	image->width = width;
	image->height = height;
	image->data = (uint8_t *) pngio_malloc( width * height * 4 );
}


void png_image_free( png_image * image )
{
	if (image->data != NULL) 
	{
		pngio_free( image->data );
		image->data = NULL;
	}
	image->width = 0;
	image->height = 0;
}


uint8_t * png_image_take( png_image * image )
{
	uint8_t * data = image->data;
	image->width = 0;
	image->height = 0;
	image->data = NULL;
	return data;
}


static uint8_t png_image_is_empty( const png_image * image )
{
	return (image->data == NULL || image->width == 0 || image->height == 0);
}


static void png_read_swap_transform( png_structp ptr, png_row_infop row_info, png_bytep row_data ) 
{
	png_pixel * p = (png_pixel *) row_data;
	png_pixel * e = p + row_info->width;
	for (; p != e; p++)
	{
		png_byte r = p->r;
		png_byte b = p->b;
		p->r = b;
		p->b = r;
	}
}


static void png_read_premultiply_transform( png_structp ptr, png_row_infop row_info, png_bytep row_data ) 
{
	png_pixel * p = (png_pixel *) row_data;
	png_pixel * e = p + row_info->width;
	for (; p != e; p++)
	{		
		png_byte a = p->a ? p->a : 1;
		p->r = (p->r * a) / 0xFF;
		p->g = (p->g * a) / 0xFF;
		p->b = (p->b * a) / 0xFF;
	}
}


static void png_read_swap_and_unpremultiply_transform( png_structp ptr, png_row_infop row_info, png_bytep row_data ) 
{
	png_pixel * p = (png_pixel *) row_data;
	png_pixel * e = p + row_info->width;
	for (; p != e; p++)
	{		
		png_byte a = p->a ? p->a : 1;
		png_byte r = (p->r * 0xFF) / a;
		png_byte g = (p->g * 0xFF) / a;
		png_byte b = (p->b * 0xFF) / a;
		p->r = b;
		p->g = g;
		p->b = r;
	}
}


static void png_write_swap_and_premultiply_transform( png_structp ptr, png_row_infop row_info, png_bytep row_data ) 
{
	png_pixel * p = (png_pixel *) row_data;
	png_pixel * e = p + row_info->width;
	for (; p != e; p++)
	{		
		png_byte a = p->a;
		png_byte r = (p->r * a) / 0xFF;
		png_byte g = (p->g * a) / 0xFF;
		png_byte b = (p->b * a) / 0xFF;
		p->r = b;
		p->g = g;
		p->b = r;
	}
}


static int png_read_user_chunk( png_structp readPtr, png_unknown_chunkp chunk ) 
{
	return 1;
}


static void pngio_error( const char * error )
{
	printf( "ERROR: %s\n", error );
}


//static void png_user_error( png_structp, png_const_charp msg )
//{
//} 


static uint8_t png_read( png_structp readPtr, png_image * image, uint32_t flags )
{
//	png_set_error_fn( readPtr, NULL, png_user_error, NULL );

	png_infop infoPtr = png_create_info_struct( readPtr );
	if (!infoPtr) 
	{
		pngio_error( "Couldn't initialize PNG info struct." );
		png_destroy_read_struct( & readPtr, NULL, NULL );
		return 0;
	}
	
	if (setjmp( png_jmpbuf( readPtr ) ))
	{
		pngio_error( "An error occured while reading the PNG file." );
		png_destroy_read_struct( & readPtr, & infoPtr, NULL );
		png_image_free( image );
		return 0;
	}

	png_set_sig_bytes( readPtr, 8 );
	
	#ifdef PNG_APPLE_MODE_SUPPORTED 
	if (png_get_apple_mode())
	{
		png_set_keep_unknown_chunks( readPtr, PNG_HANDLE_CHUNK_ALWAYS, NULL, 0 );
		png_set_read_user_chunk_fn( readPtr, NULL, png_read_user_chunk );
	}
	#endif
	
	png_read_info( readPtr, infoPtr );
	
	png_uint_32 w = png_get_image_width( readPtr, infoPtr );
	png_uint_32 h = png_get_image_height( readPtr, infoPtr );
	png_uint_32 bitDepth = png_get_bit_depth( readPtr, infoPtr );
	png_uint_32 channels = png_get_channels( readPtr, infoPtr );
	png_uint_32 interlaceType = png_get_interlace_type( readPtr, infoPtr );
	png_uint_32 colorType = png_get_color_type( readPtr, infoPtr );
	
	switch (colorType) 
	{
		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb( readPtr );
			channels = 3;           
			break;
		case PNG_COLOR_TYPE_GRAY:
			if (bitDepth < 8)
			{
				png_set_expand_gray_1_2_4_to_8( readPtr );
				bitDepth = 8;
			}
			png_set_gray_to_rgb( readPtr );
			break;
	}
	
	if (png_get_valid( readPtr, infoPtr, PNG_INFO_tRNS )) 
	{
		png_set_tRNS_to_alpha( readPtr );
		channels += 1;
	}
	else if (!(colorType & PNG_COLOR_MASK_ALPHA)) 
	{
		png_set_add_alpha( readPtr, 0xff, PNG_FILLER_AFTER );
	}
	
	if (bitDepth == 16)
	{
		png_set_strip_16( readPtr );
	}

	#ifdef PNG_APPLE_MODE_SUPPORTED
	if (png_get_apple_mode())
	{
		if (flags & PNG_IMAGE_PREMULTIPLY_ALPHA)
		{
			png_set_read_user_transform_fn( readPtr, png_read_swap_transform );
		}
		else
		{
			png_set_read_user_transform_fn( readPtr, png_read_swap_and_unpremultiply_transform );
		}
		png_set_user_transform_info( readPtr, NULL, bitDepth, channels );
		png_read_update_info( readPtr, infoPtr );
	}
	else
	#endif
	{
		if (flags & PNG_IMAGE_PREMULTIPLY_ALPHA)
		{
			png_set_read_user_transform_fn( readPtr, png_read_premultiply_transform );
		}
	}

	png_image_alloc( image, w, h );
	png_bytep p = image->data;
	
	const size_t passCount = interlaceType == PNG_INTERLACE_NONE ? 1 : png_set_interlace_handling( readPtr );
	const size_t bytesPerRow = w * 4;
	if (flags & PNG_IMAGE_FLIP_VERTICAL)
	{
		for (size_t pass = 0; pass < passCount; pass++)
		{
			for (size_t i = 0; i < h; i++) 
			{
				png_read_row( readPtr, p + (bytesPerRow * (h - i - 1)), NULL );
			}
		}
	}
	else
	{
//		png_bytep rp[h];
//		for (size_t i = 0; i < h; i++) 
//		{
//			rp[i] = p + (bytesPerRow * i);
//		}
//		png_read_image( readPtr, rp );
		for (size_t pass = 0; pass < passCount; pass++)
		{
			for (size_t i = 0; i < h; i++) 
			{
				png_read_row( readPtr, p + (bytesPerRow * i), NULL );
			}
		}
	}
	
	png_destroy_read_struct( & readPtr, & infoPtr, NULL );
	
	return 1;
}


static uint8_t png_write( png_structp writePtr, png_image * image, uint32_t flags )
{
	const uint32_t  h = image->height;
	const uint32_t  w = image->width;
	const uint8_t * p = image->data;
	const uint32_t  bitDepth = 8;
	const uint32_t  channels = 4;
	
	png_infop infoPtr = png_create_info_struct( writePtr );
	if (!infoPtr) 
	{
		pngio_error( "Couldn't initialize PNG info struct" );
		png_destroy_write_struct( & writePtr, NULL );
		return 0;
	}
	
	if (setjmp( png_jmpbuf( writePtr ) )) 
	{
		png_destroy_write_struct( & writePtr, & infoPtr );
		pngio_error( "An error occured while writing the PNG file." );
		return 0;
	}

	png_set_filter( writePtr, 0, PNG_FILTER_NONE );
	
	#ifdef PNG_APPLE_MODE_SUPPORTED
	if (png_get_apple_mode())
	{
		png_write_sig( writePtr );
		png_set_sig_bytes( writePtr, 8 );
		png_set_compression_window_bits( writePtr, -15 );
		png_set_write_user_transform_fn( writePtr, png_write_swap_and_premultiply_transform );
		png_set_user_transform_info( writePtr, NULL, bitDepth, channels );
	}
	#endif
	
	png_set_IHDR( writePtr, infoPtr, w, h, bitDepth, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );
	png_set_gAMA( writePtr, infoPtr, 0.45455 );
	png_set_cHRM( writePtr, infoPtr, 0.312700, 0.329, 0.64, 0.33, 0.3, 0.6, 0.15, 0.06 );
	png_set_sRGB( writePtr, infoPtr, 0);
	
	#ifdef PNG_APPLE_MODE_SUPPORTED
	if (png_get_apple_mode())
	{
		png_byte cname[] = { 'C', 'g', 'B', 'I', '\0' };
		png_byte cdata[] = { 0x50, 0x00, 0x20, 0x02 };
		png_write_chunk( writePtr, cname, cdata, 4 );
	}
	#endif

	png_write_info( writePtr, infoPtr );	

	const size_t bytesPerRow = w * 4;
	if (flags & PNG_IMAGE_FLIP_VERTICAL)
	{
		for (size_t i = 0; i < h; i++) 
		{
			png_write_row( writePtr, p + (bytesPerRow * (h - i - 1)) );
		}
	}
	else
	{
		for (size_t i = 0; i < h; i++) 
		{
			png_write_row( writePtr, p + (bytesPerRow * i) );
		}
	}
	
	png_write_end( writePtr, infoPtr );
	png_destroy_write_struct( & writePtr, & infoPtr );
	
	return 1;
}


uint8_t png_image_load( png_image * image, FILE * file, uint32_t flags )
{
	uint32_t format = png_read_file_format( file );
	if (format == PNG_FORMAT_INVALID)
	{
		pngio_error( "Not a valid PNG file." );
		return 0;
	}
	
	#ifdef PNG_APPLE_MODE_SUPPORTED 
	png_set_apple_mode( format == PNG_FORMAT_APPLE );
	#endif
	
	png_structp readPtr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if (!readPtr) 
	{
		pngio_error( "Couldn't initialize PNG read struct." );
		return 0;
	}
	
	png_set_read_fn( readPtr, (png_voidp) file, png_read_file_data );
	
	return png_read( readPtr, image, flags );
}



uint8_t png_image_save( png_image * image, FILE * file, uint32_t flags )
{
	if (png_image_is_empty( image ))
	{
		return 0;
	}

	#ifdef PNG_APPLE_MODE_SUPPORTED
	png_set_apple_mode( flags & PNG_IMAGE_OPTIMIZE_FOR_IOS );
	#endif
	
	png_structp writePtr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if (!writePtr) 
	{
		pngio_error( "Couldn't initialize PNG write struct." );
		return 0;
	}
	
	png_set_write_fn( writePtr, (png_voidp) file, png_write_file_data, png_flush_file_data );

	return png_write( writePtr, image, flags );
}


uint8_t png_image_load_path( png_image * image, const char * path, uint32_t flags )
{	
	FILE * ifile = fopen( path, "r" );
	if (!ifile) 
	{
		pngio_error( "Could not open file." );
		return false;
	}
	uint8_t result = png_image_load( image, ifile, flags );
	fclose( ifile );
	return result;
}



uint8_t png_image_save_path( png_image * image, const char * path, uint32_t flags )
{	
	FILE * ofile = fopen( path, "w" );
	if (!ofile) 
	{
		pngio_error( "Could not open file." );
		return false;
	}
	uint8_t result = png_image_save( image, ofile, flags );
	fclose( ofile );
	return result;
}


void png_image_set_pixel( png_image * image, uint32_t x, uint32_t y, png_pixel pixel )
{
	const uint32_t w = image->width;
	const uint32_t h = image->height;
	if (x < w && y < h)
	{
		((png_pixel *) image->data)[ (y * w) + x ] = pixel;
	}
}


png_pixel png_image_get_pixel( png_image * image, uint32_t x, uint32_t y )
{
	const uint32_t w = image->width;
	const uint32_t h = image->height;
	if (x < w && y < h)
	{
		return ((png_pixel *) image->data)[ (y * w) + x ];
	}
	static const png_pixel blank = { 0, 0, 0, 0 };
	return blank;
}



#ifdef __cplusplus

static void png_read_stream_data( png_structp readPtr, png_bytep data, png_size_t size ) 
{
	std::istream * stream = (std::istream *) png_get_io_ptr( readPtr );
	stream->read( (char *) data, size );
}


static void png_write_stream_data( png_structp writePtr, png_bytep data, png_size_t size ) 
{
	std::ostream * stream = (std::ostream *) png_get_io_ptr( writePtr );
	stream->write( (char *) data, size );
}


static void png_flush_stream_data( png_structp writePtr ) 
{
	std::ostream * stream = (std::ostream *) png_get_io_ptr( writePtr );
	stream->flush();
}


static uint32_t png_read_stream_format( std::istream & source ) 
{
	char sign[8];
	source.read( (char *) sign, 8 );
	if (!source.good()) return PNG_FORMAT_INVALID;
	
	if (png_sig_cmp( (png_bytep) sign, 0, 8 ) != 0)
	{
		return PNG_FORMAT_INVALID;
	}
	
	source.seekg( 12, std::ios_base::beg );
	if (!source.good()) return PNG_FORMAT_INVALID;
	
	char cgbi[4];
	source.read( cgbi, 4 );
	if (!source.good()) return PNG_FORMAT_INVALID;

	
	source.seekg( 8, std::ios_base::beg );
	if (!source.good()) return PNG_FORMAT_INVALID;
	
	if (strncmp( cgbi, "CgBI", 4 ) == 0)
	{
		return PNG_FORMAT_APPLE;
	}

	return PNG_FORMAT_STANDARD;
}


png_image::png_image( void )
{
	png_image_init( this );
}


png_image::~png_image( void )
{
	png_image_free( this );
}


bool png_image::load( std::istream & stream, uint32_t flags )
{
	uint32_t format = png_read_stream_format( stream );
	if (format == PNG_FORMAT_INVALID)
	{
		pngio_error( "Not a valid PNG file." );
		return 0;
	}
	
	#ifdef PNG_APPLE_MODE_SUPPORTED 
	png_set_apple_mode( format == PNG_FORMAT_APPLE );
	#endif
	
	png_structp readPtr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if (!readPtr) 
	{
		pngio_error( "Couldn't initialize PNG read struct." );
		return 0;
	}
	
	png_set_read_fn( readPtr, (png_voidp) & stream, png_read_stream_data );
	
	return png_read( readPtr, this, flags );
}


bool png_image::save( std::ostream & stream, uint32_t flags )
{
	if (png_image_is_empty( this ))
	{
		return 0;
	}

	#ifdef PNG_APPLE_MODE_SUPPORTED
	png_set_apple_mode( flags );
	#endif
	
	png_structp writePtr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if (!writePtr) 
	{
		pngio_error( "Couldn't initialize PNG write struct." );
		return 0;
	}
	
	png_set_write_fn( writePtr, (png_voidp) & stream, png_write_stream_data, png_flush_stream_data );

	return png_write( writePtr, this, flags );
}


bool png_image::load( const std::string & path, uint32_t flags )
{
	return png_image_load_path( this, path.c_str(), flags );
}


bool png_image::save( const std::string & path, uint32_t flags )
{
	return png_image_save_path( this, path.c_str(), flags );
}


void png_image::set_pixel( uint32_t x, uint32_t y, png_pixel pixel )
{
	png_image_set_pixel( this, x, y, pixel );
}


png_pixel png_image::get_pixel( uint32_t x, uint32_t y )
{
	return png_image_get_pixel( this, x, y );
}


uint8_t * png_image::take( void )
{
	return png_image_take( this );
}


#endif


// END
