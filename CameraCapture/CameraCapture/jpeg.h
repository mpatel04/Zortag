

extern "C" {
	#include "jpeg-6b/jpeglib.h"
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

