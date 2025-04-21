

float compare_fingerprints(const zor::GrayImage8u &target, const zor::GrayImage8u &compare, int level)
{
	if (target.width() != compare.width() || target.height() != compare.height())
		return 0.f;

	int width = target.width(), height = target.height(), num_pixels = target.num_pixels();
	int match = 0, count = 0;

	for (int y = level; y < height-level; ++y)
		for (int x = level; x < width-level; ++x)
			if (target.pixel(x, y) != 255) {
				count++;
				bool found = false;
				for (int dy = -level; dy <= level && !found; ++dy)
					for (int dx = -level; dx <= level && !found; ++dx)
						if (compare.pixel(x+dx, y+dy) == target.pixel(x, y))
							found = true;
				if (found)
					match++;
			}

	return (count == 0) ? 0 : (match*100.f) / count;
}




bool quality_control(const std::string& barcode_top, const zor::GrayImage8u& bubble_top, const std::string& barcode_previous, const std::vector<uint8_t>& fingerprint_previous)
{
	//printf("quality control start\n");
	//printf("%s %s\n", barcode_top.c_str(), barcode_previous.c_str());

	// if the barcodes are not the same, or either barcode is empty (i.e., no image captured),
	// then cannot verify for quality control, return false
	if (barcode_top.length() == 0 || barcode_previous.length() == 0 || ::strcmp(barcode_top.c_str(), barcode_previous.c_str()) != 0) {
		printf("  quality control returning false\n");
		return false;
	}

	zor::GrayImage8u bubble_previous;
	BlobEncodation blobber;
	blobber.decode(fingerprint_previous, bubble_previous);

	float compare1 = compare_fingerprints(bubble_top, bubble_previous, 2); // compare how the bottom matches to top
	float compare2 = compare_fingerprints(bubble_previous, bubble_top, 4); // compare top to bottom
	printf("  QC | Comparisons: %f %f, ", compare1, compare2);

	int count_pixels = 0;
	for (int k = 0; k < bubble_previous.num_pixels(); ++k)
		if (bubble_previous.pixel(k) != 255)
			count_pixels++;
	printf("Count pixels: %d", count_pixels);

	//if (compare1 < 0.9f || compare2 < 0.9f)
	if (compare1 < 90.f || compare2 < 90.f)
		return false;
	if (count_pixels < 2000)
		return false;

	// additions
	if (count_pixels < 3000)
		return false;
	if (count_pixels > 20000)
		return false;

	return true;
}

