

extern "C" {
	#include "jpeglib.h"
}
//#pragma comment(lib, "jpeg-6b/libjpeg.lib")
#pragma comment(lib, "libjpeg.lib")
// C/C++ -> Code Generation -> Runtime Library -> Multi-threaded (/MT)
// (use static linking instead of dynamic to go with libjpeg.lib)




bool write_file(const char *filename, const char *data, int length)
{
	std::ofstream outfile;
	outfile.open(filename, std::ios::binary);
	if (!outfile.good())
		return false;
	outfile.write(data, length);
	outfile.flush();
	outfile.close();
	if (!outfile.good())
		return false;
	return true;
}




bool write_jpeg_file(const char *filename, const zor::ColorImageRGB24u &image, int quality)
{
	// http://andrewewhite.net/wordpress/2008/09/02/very-simple-jpeg-writer-in-c-c

	FILE* outfile;
	errno_t result = fopen_s(&outfile, filename, "wb");

	if (result != 0)
		return false;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
 
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);
 
	cinfo.image_width      = image.width();
	cinfo.image_height     = image.height();
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	/*set the quality [0..100]  */
	jpeg_set_quality (&cinfo, quality, true);
	jpeg_start_compress(&cinfo, true);

	JSAMPROW row_pointer;          /* pointer to a single row */
 
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer = (JSAMPROW) &image.data()[cinfo.next_scanline*image.width()];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);

	result = fclose(outfile);

	if (result != 0)
		return false;
	return true;
}




/* setup the buffer but we did that in the main function */
void init_buffer(jpeg_compress_struct* cinfo) {}
 
/* what to do when the buffer is full; this should almost never
 * happen since we allocated our buffer to be big to start with
 */
boolean empty_buffer(jpeg_compress_struct* cinfo) {
	return TRUE;
}
 
/* finalize the buffer and do any cleanup stuff */
void term_buffer(jpeg_compress_struct* cinfo) {}


int write_jpeg_buffer(std::vector<uint8_t> &jpeg, const zor::ColorImageRGB24u &image, int quality)
{
	// http://www.andrewewhite.net/wordpress/2010/04/07/simple-cc-jpeg-writer-part-2-write-to-buffer-in-memory/

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr       jerr;
	struct jpeg_destination_mgr dmgr;
 
	/* create our in-memory output buffer to hold the jpeg */
	//JOCTET *out_buffer   = new JOCTET[image.width() * image.height() *3];
	jpeg.resize(image.num_pixels()*3);
 
	/* here is the magic */
	dmgr.init_destination    = init_buffer;
	dmgr.empty_output_buffer = empty_buffer;
	dmgr.term_destination    = term_buffer;
	dmgr.next_output_byte    = (JOCTET*)jpeg.data(); //out_buffer;
	dmgr.free_in_buffer      = image.num_pixels() *3;
 
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
 
	/* make sure we tell it about our manager */
	cinfo.dest = &dmgr;
 
	cinfo.image_width      = image.width();
	cinfo.image_height     = image.height();
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;
 
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality (&cinfo, quality, true);
	jpeg_start_compress(&cinfo, true);
 
	JSAMPROW row_pointer;
	uint8_t *buffer = (uint8_t*)image.data();
 
	/* main code to write jpeg data */
	while (cinfo.next_scanline < cinfo.image_height) { 		
		row_pointer = (JSAMPROW) &buffer[cinfo.next_scanline * image.width() * 3];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}
	jpeg_finish_compress(&cinfo);

	int size = cinfo.dest->next_output_byte - (JOCTET*)jpeg.data();
	jpeg.resize(size);
	return size;
}




bool read_jpeg_file(const std::string& filename, zor::ColorImageRGB24u& image)
{
	// http://www.aaronmr.com/en/2010/03/test/

	/* these are standard libjpeg structures for reading(decompression) */
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	/* libjpeg data structure for storing one row, that is, scanline of an image */
	JSAMPROW row_pointer[1];

	FILE *infile = fopen(filename.c_str(), "rb" );
	unsigned long location = 0;
	int i = 0;

	if ( !infile )
	{
		printf("Error opening jpeg file %s\n!", filename );
		return false;
	}
	
	/* here we set up the standard libjpeg error handler */
	cinfo.err = jpeg_std_error( &jerr );
	
	/* setup decompression process and source, then read JPEG header */
	jpeg_create_decompress( &cinfo );
	
	/* this makes the library read from infile */
	jpeg_stdio_src( &cinfo, infile );
	
	/* reading the image header which contains image information */
	jpeg_read_header( &cinfo, TRUE );
	
	/* Uncomment the following to output image information, if needed. */
	/*--
	printf( "JPEG File Information: \n" );
	printf( "Image width and height: %d pixels and %d pixels.\n", cinfo.image_width, cinfo.image_height );
	printf( "Color components per pixel: %d.\n", cinfo.num_components );
	printf( "Color space: %d.\n", cinfo.jpeg_color_space );
	--*/
	
	/* Start decompression jpeg here */
	jpeg_start_decompress( &cinfo );

	/* allocate memory to hold the uncompressed image */
	//raw_image = (unsigned char*)malloc( cinfo.output_width*cinfo.output_height*cinfo.num_components );
	if (cinfo.num_components != 3)
		return false;
	image.resize(cinfo.output_width, cinfo.output_height);
	unsigned char *raw_image = reinterpret_cast<unsigned char*>( image.data() );
	
	/* now actually read the jpeg into the raw buffer */
	row_pointer[0] = (unsigned char *)malloc( cinfo.output_width*cinfo.num_components );
	
	/* read one scan line at a time */
	while( cinfo.output_scanline < cinfo.image_height )
	{
		jpeg_read_scanlines( &cinfo, row_pointer, 1 );
		for (i = 0; i < cinfo.image_width*cinfo.num_components; ++i)
			raw_image[location++] = row_pointer[0][i];
	}
	
	/* wrap up decompression, destroy objects, free pointers and close open files */
	jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );
	free(row_pointer[0]);
	fclose(infile);
	
	/* yup, we succeeded! */
	return true;
}

