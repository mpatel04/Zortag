
#include <iostream>

#define DATAMATRIX_NODEBUG
#include "../../../../ZorImageLib/zor.hpp"

#include <sstream>
#include "../../../ZorSecureManufacturingUpload/ZorSecureManufacturingUpload/manufacturing_files_io.h"

#include "files_in_directory.h"
#include "E:/backup/zortag/programs/manufacturing/process_in_office/FindBarcodes/FindBarcodes/jpeg.h"
//#include "jpeg.h"
#include "process.h"
#include "BlobEncodation.h"
#include "quality_control.h"

#include <conio.h> // for getch()




//const char* barcode_directory = "F:/zortag/zortag-scanned-labels/offline/barcodes/barcodes 2017-06-13";
//const char* fingerprint_directory = "F:/zortag/zortag-scanned-labels/offline/fingerprints/fingerprints 2017-06-13";
//const char* fingerprint_raw_directory = "F:/zortag/zortag-scanned-labels/offline/fingerprints raw/fingerprints raw 2017-06-13";

std::string barcode_directory = "E:/offline machine images/barcodes/barcodes ";
std::string fingerprint_directory = "E:/offline machine images/fingerprints/fingerprints ";
std::string fingerprint_raw_directory = "E:/offline machine images/camerafingerprints raw/camerafingerprints raw ";




struct BarcodeInfo {
	std::string barcode;
	std::string filename;
	std::vector<zor::Vector2f> corners;
};




std::string make_alphanum(const std::string& str)
{
	std::string alphanum;
	for (size_t n = 0; n < str.length(); ++n)
		if (::isalnum(str.at(n)))
			alphanum.push_back(str.at(n));
	return alphanum;
}




void get_barcodes_OLD(const std::string& barcode_filename, BarcodeInfo& top, BarcodeInfo& bottom)
{
	top.filename.clear();
	bottom.filename.clear();

	const int BUFSIZE = 512;
	char str[BUFSIZE], str2[BUFSIZE];

	// read file
	std::ifstream infile(barcode_filename);
	while (infile.good())
	{
		// each line: barcode, "image path", tl.x, tl.y, bl.x, bl.y, br.x, br.y, tr.x, tr.y

		infile.getline(str, BUFSIZE, ' ');
		if (strlen(str) == 0)
			break;
		std::string barcode = str;

		infile.getline(str, BUFSIZE, '"');
		infile.getline(str, BUFSIZE, '"');
		std::string filename = str;

		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str2, BUFSIZE, ' ');
		zor::Vector2f topLeftCorner(atof(str), atof(str2));

		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str2, BUFSIZE, ' ');
		zor::Vector2f bottomLeftCorner(atof(str), atof(str2));

		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str2, BUFSIZE, ' ');
		zor::Vector2f bottomRightCorner(atof(str), atof(str2));

		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str2, BUFSIZE);
		zor::Vector2f topRightCorner(atof(str), atof(str2));

		/*
		std::cout << bcode << "\n" << filename << "\n" <<
			topLeftCorner.x() << " " << topLeftCorner.y() << "\n" <<
			bottomLeftCorner.x() << " " << bottomLeftCorner.y() << "\n" <<
			bottomRightCorner.x() << " " << bottomRightCorner.y() << "\n" <<
			topRightCorner.x() << " " << topRightCorner.y() << "\n\n";
		*/

		// if top image not found yet and this is a top image, save the information
		if (topLeftCorner.y() < 500 && top.filename.size() == 0) {
			top.barcode = barcode;
			top.filename = filename;
			top.corners.resize(4);
			top.corners[0] = topLeftCorner;
			top.corners[1] = bottomLeftCorner;
			top.corners[2] = bottomRightCorner;
			top.corners[3] = topRightCorner;
		}

		// if bottom image not found yet and this is a bottom image, save the information
		if (topLeftCorner.y() > 700 && topLeftCorner.y() < 1200 && bottom.filename.size() == 0) {
			bottom.barcode = barcode;
			bottom.filename = filename;
			bottom.corners.resize(4);
			bottom.corners[0] = topLeftCorner;
			bottom.corners[1] = bottomLeftCorner;
			bottom.corners[2] = bottomRightCorner;
			bottom.corners[3] = topRightCorner;
		}
	}
	infile.close();
}




void get_barcodes(const std::string& barcode_filename, BarcodeInfo& top, BarcodeInfo& bottom)
{
	top.filename.clear();
	bottom.filename.clear();

	const int BUFSIZE = 512;
	char str[BUFSIZE], str2[BUFSIZE];

	std::vector<BarcodeInfo> info;

	// read file
	std::ifstream infile(barcode_filename);
	while (infile.good())
	{
		// each line: barcode, "image path", tl.x, tl.y, bl.x, bl.y, br.x, br.y, tr.x, tr.y

		infile.getline(str, BUFSIZE, ' ');
		if (strlen(str) == 0)
			break;
		std::string barcode = str;

		infile.getline(str, BUFSIZE, '"');
		infile.getline(str, BUFSIZE, '"');
		std::string filename = str;

		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str2, BUFSIZE, ' ');
		zor::Vector2f topLeftCorner(atof(str), atof(str2));

		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str2, BUFSIZE, ' ');
		zor::Vector2f bottomLeftCorner(atof(str), atof(str2));

		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str2, BUFSIZE, ' ');
		zor::Vector2f bottomRightCorner(atof(str), atof(str2));

		infile.getline(str, BUFSIZE, ' ');
		infile.getline(str2, BUFSIZE);
		zor::Vector2f topRightCorner(atof(str), atof(str2));

		float d = (topRightCorner.y() - topLeftCorner.y()) * 1.3f;
		if (topRightCorner.y() + d > 1944)
			continue;
		bool addit = true;
		for (std::size_t i = 0; i < info.size(); ++i)
			if (std::abs(info[i].corners[0].y() - topLeftCorner.y()) < 100) {
				addit = false;
				break;
			}
		if (addit) {
			BarcodeInfo b;
			b.barcode = barcode;
			b.filename = filename;
			b.corners.resize(4);
			b.corners[0] = topLeftCorner;
			b.corners[1] = bottomLeftCorner;
			b.corners[2] = bottomRightCorner;
			b.corners[3] = topRightCorner;
			info.push_back(b);
		}

		/*
		std::cout << bcode << "\n" << filename << "\n" <<
			topLeftCorner.x() << " " << topLeftCorner.y() << "\n" <<
			bottomLeftCorner.x() << " " << bottomLeftCorner.y() << "\n" <<
			bottomRightCorner.x() << " " << bottomRightCorner.y() << "\n" <<
			topRightCorner.x() << " " << topRightCorner.y() << "\n\n";
		*/

		/*
		// if top image not found yet and this is a top image, save the information
		if (topLeftCorner.y() < 500 && top.filename.size() == 0) {
			top.barcode = barcode;
			top.filename = filename;
			top.corners.resize(4);
			top.corners[0] = topLeftCorner;
			top.corners[1] = bottomLeftCorner;
			top.corners[2] = bottomRightCorner;
			top.corners[3] = topRightCorner;
		}

		// if bottom image not found yet and this is a bottom image, save the information
		if (topLeftCorner.y() > 700 && topLeftCorner.y() < 1200 && bottom.filename.size() == 0) {
			bottom.barcode = barcode;
			bottom.filename = filename;
			bottom.corners.resize(4);
			bottom.corners[0] = topLeftCorner;
			bottom.corners[1] = bottomLeftCorner;
			bottom.corners[2] = bottomRightCorner;
			bottom.corners[3] = topRightCorner;
		}
		*/
	}

	if (info.size() >= 2)
	{
		if (info[0].corners[0].y() < info[1].corners[0].y()) {
			top = info[0];
			bottom = info[1];
		} else {
			top = info[1];
			bottom = info[0];
		}
	}

	infile.close();
}




int main(int argc, char* argv[])
{
	std::string date;
	std::cout << "Enter date: ";
	std::cin >> date;

	barcode_directory.append(date);
	fingerprint_directory.append(date);
	fingerprint_raw_directory.append(date);
	std::cout << barcode_directory << '\n' << fingerprint_directory << '\n' << fingerprint_raw_directory << '\n';

	std::vector<std::string> barcode_filenames;
	get_files_in_directory(barcode_directory, barcode_filenames);
	printf("Number of barcodes: %d\n", barcode_filenames.size());
	
	int count_passed = 0, count_failed = 0;

	for (int bar = 0; bar < barcode_filenames.size(); ++bar)
	{
		//if (bar > 0)
		//	break;

		std::cout << "\n" << bar << " / " << barcode_filenames.size() << "\n" << barcode_filenames[bar] << "\n";
		BarcodeInfo top, bottom;
		get_barcodes(barcode_filenames[bar], top, bottom);
		if (top.filename.size() == 0 || bottom.filename.size() == 0) {
			printf("  MISSING TOP OR BOTTOM, skipping\n");
			continue;
		}
		
		// read images
		zor::ColorImageRGB24u top_color_image, bottom_color_image;

		//printf("  %s\n", top.filename.c_str());
		//if (read_jpeg_file(top.filename, top_color_image))
		//	printf("    Size: %d x %d\n", top_color_image.width(), top_color_image.height());
		//else
		//	printf("    error reading jpeg!\n");

		//printf("  %s\n", bottom.filename.c_str());
		//if (read_jpeg_file(bottom.filename, bottom_color_image))
		//	printf("    Size: %d x %d\n", bottom_color_image.width(), bottom_color_image.height());
		//else
		//	printf("    error reading jpeg!\n");

		if (!read_jpeg_file(top.filename, top_color_image)) {
			printf("  Error reading %s\n", top.filename.c_str());
			continue;
		}
		if (!read_jpeg_file(bottom.filename, bottom_color_image)) {
			printf("  Error reading %s\n", bottom.filename.c_str());
			continue;
		}

		// convert color images to grayscale
		zor::GrayImage8u top_gray_image, bottom_gray_image;
		zor::convert_image_luminosity(top_gray_image, top_color_image);
		zor::convert_image_luminosity(bottom_gray_image, bottom_color_image);

		// extract the bubbles
		zor::GrayImage8u top_bubble, bottom_bubble, top_surround, bottom_surround;
		extractBubble(top_gray_image, top.corners, top_bubble, top_surround, NULL);
		extractBubble(bottom_gray_image, bottom.corners, bottom_bubble, bottom_surround, NULL);

		// get the fingerprints
		zor::GrayImage8u top_fingerprint, bottom_fingerprint;

		std::vector<const zor::GrayImage8u*> imagePtrs;
		imagePtrs.push_back(&top_bubble);
		getSignature(imagePtrs, top_fingerprint, NULL);

		imagePtrs.clear();
		imagePtrs.push_back(&bottom_bubble);
		getSignature(imagePtrs, bottom_fingerprint, NULL);

		// write the fingerprint images as debug
		std::string raw_filename_top = fingerprint_raw_directory;
		raw_filename_top += '/';
		raw_filename_top.append(make_alphanum(top.barcode));
		raw_filename_top.append("-top.pgm");
		zor::write_pgm(top_fingerprint, raw_filename_top);

		std::string raw_filename_bottom = fingerprint_raw_directory;
		raw_filename_bottom += '/';
		raw_filename_bottom.append(make_alphanum(bottom.barcode));
		raw_filename_bottom.append("-bottom.pgm");
		zor::write_pgm(bottom_fingerprint, raw_filename_bottom);

		raw_filename_top = fingerprint_raw_directory;
		raw_filename_top += '/';
		raw_filename_top.append(make_alphanum(top.barcode));
		raw_filename_top.append("_bubble-top.pgm");
		zor::write_pgm(top_bubble, raw_filename_top);

		raw_filename_bottom = fingerprint_raw_directory;
		raw_filename_bottom += '/';
		raw_filename_bottom.append(make_alphanum(bottom.barcode));
		raw_filename_bottom.append("_bubble-bottom.pgm");
		zor::write_pgm(bottom_bubble, raw_filename_bottom);

		// encode the fingerprint
		std::vector<unsigned char> encoded;
		BlobEncodation blobber;
		blobber.encode(bottom_fingerprint, encoded, BlobEncodation::RELATIVE_DIRECTION_HUFF_SIMPLE, 255);

		// compare the fingerprints and do quality control
		// (if problem, then 'continue;')
		if (quality_control(top.barcode, top_fingerprint, bottom.barcode, encoded)) {
			printf(", passed\n");
			++count_passed;
		} else {
			printf(", FAILED\n");
			++count_failed;
			continue;
		}

		// write the fingerprint
		std::string fingerprint_filename = fingerprint_directory;
		fingerprint_filename += '/';
		fingerprint_filename.append(make_alphanum(top.barcode));
		write_fingerprint(fingerprint_filename.c_str(), top.barcode, encoded);
	}

	printf("\n");
	printf("Total: %d\nPassed: %d\nFailed: %d\n", barcode_filenames.size(), count_passed, count_failed);
	
	std::cout << "Press any key to exit";
	getch();

	return 0;
}
