
#include <bitset>
#include <iostream>


// encode blobs of any grayscale value 0-254 (value of 255 is considered as background)


class BlobEncodation
{


public:


	enum encode_t {
		PLAIN_DIRECTION_ONE_PER_BYTE,
		PLAIN_DIRECTION_PACKED_FOUR_BITS,
		PLAIN_DIRECTION_PACKED_THREE_BITS,
		RELATIVE_DIRECTION_ONE_PER_BYTE,
		RELATIVE_DIRECTION_PACKED_FOUR_BITS,
		RELATIVE_DIRECTION_PACKED_THREE_BITS,
		RELATIVE_DIRECTION_HUFF_APRIORI,
		RELATIVE_DIRECTION_HUFF_SIMPLE//,
		//RELATIVE_DIRECTION_HUFF_FULL
	};

	enum direction_t { PLAIN_DIRECTION, RELATIVE_DIRECTION };
	enum packing_t { ONE_PER_BYTE, PACKED_FOUR_BITS, PACKED_THREE_BITS, HUFF_APRIORI, HUFF_SIMPLE, HUFF_FULL, HUFF };


	void encode(const zor::GrayImage8u &original_image, std::vector<unsigned char> &encoded, encode_t encode_type, unsigned char background)
	{
		int width = original_image.width(), height = original_image.height();

		std::vector<unsigned char> colors;
		std::vector<zor::Vector2i> coords;
		std::vector<unsigned char> region_grows; // 0 = region grow and chain, 1 = just region grow, 7 = just chain
		std::vector< std::vector<unsigned char> > chains;

		for (int gray_value = 0; gray_value < 256; ++gray_value)
		{
			if (gray_value == background)
				continue;

			zor::GrayImage8u binary_image;
			int count_black = isolate_gray_value(original_image, gray_value, binary_image);
			if (count_black == 0)
				continue;

			int count_gray = mark_inner_pixels(binary_image);
			count_black -= count_gray;

			int count_region_grow = 0, count_chain = 0;

			// first do gray pixels which have a black neighbor (not behind)
			for (int i = 0; i < binary_image.num_pixels(); ++i)
			{
				if (binary_image.pixel(i) == 127) // guaranteed to not be on border
				{
					// if zero anywhere besides backward neighbor
					if (binary_image.pixel(i+1) == 0 || binary_image.pixel(i+1-width) == 0 ||
						binary_image.pixel(i-width) == 0 || binary_image.pixel(i-1-width) == 0 ||
						binary_image.pixel(i-1+width) == 0 ||
						binary_image.pixel(i+width) == 0 || binary_image.pixel(i+1+width) == 0)
					{
						// make sure no black pixel behind (x-1, y)
						if (binary_image.pixel(i-1) != 0)
						{
							int x = i % width;
							int y = i / width;

							colors.push_back(gray_value);
							coords.push_back(zor::Vector2i(x, y));
							region_grows.push_back(0); // region grow and chain

							count_region_grow += region_grow(binary_image, zor::Vector2i(x, y), 255);

							std::vector<unsigned char> chain;
							count_chain += create_chain_freeman8(binary_image, zor::Vector2i(x, y), chain);
							chains.push_back(chain);
						}
					}
				}
			}

			// remaining gray pixels with no black neighbor
			for (int i = 0; i < binary_image.num_pixels(); ++i)
			{
				if (binary_image.pixel(i) == 127)
				{
					int x = i % width;
					int y = i / width;

					colors.push_back(gray_value);
					coords.push_back(zor::Vector2i(x, y));
					region_grows.push_back(1); // region grow only

					count_region_grow += region_grow(binary_image, zor::Vector2i(x, y), 255);

					chains.push_back(std::vector<unsigned char>());
				}
			}

			// remaining black pixels
			for (int i = 0; i < binary_image.num_pixels(); ++i)
			{
				if (binary_image.pixel(i) == 0)
				{
					int x = i % width;
					int y = i / width;

					colors.push_back(gray_value);
					coords.push_back(zor::Vector2i(x, y));
					region_grows.push_back(7); // chain only

					std::vector<unsigned char> chain;
					count_chain += create_chain_freeman8(binary_image, zor::Vector2i(x, y), chain) + 1;
					chains.push_back(chain);
				}
			}

			if (count_region_grow != count_gray || count_chain != count_black)
				printf("PROBLEM - counts not equal\n");
		}

		direction_t direction_type;
		packing_t packing_type;

		encoded.clear();

		switch(encode_type)
		{
			case PLAIN_DIRECTION_ONE_PER_BYTE:
				encoded.push_back(1);
				direction_type = PLAIN_DIRECTION;
				packing_type = ONE_PER_BYTE;
				break;
			case PLAIN_DIRECTION_PACKED_FOUR_BITS:
				encoded.push_back(2);
				direction_type = PLAIN_DIRECTION;
				packing_type = PACKED_FOUR_BITS;
				break;
			case PLAIN_DIRECTION_PACKED_THREE_BITS:
				encoded.push_back(3);
				direction_type = PLAIN_DIRECTION;
				packing_type = PACKED_THREE_BITS;
				break;
			case RELATIVE_DIRECTION_ONE_PER_BYTE:
				encoded.push_back(4);
				direction_type = RELATIVE_DIRECTION;
				packing_type = ONE_PER_BYTE;
				break;
			case RELATIVE_DIRECTION_PACKED_FOUR_BITS:
				encoded.push_back(5);
				direction_type = RELATIVE_DIRECTION;
				packing_type = PACKED_FOUR_BITS;
				break;
			case RELATIVE_DIRECTION_PACKED_THREE_BITS:
				encoded.push_back(6);
				direction_type = RELATIVE_DIRECTION;
				packing_type = PACKED_THREE_BITS;
				break;
			case RELATIVE_DIRECTION_HUFF_APRIORI:
				encoded.push_back(7);
				direction_type = RELATIVE_DIRECTION;
				packing_type = HUFF_APRIORI;
				break;
			case RELATIVE_DIRECTION_HUFF_SIMPLE:
				encoded.push_back(8);
				direction_type = RELATIVE_DIRECTION;
				packing_type = HUFF_SIMPLE;
				break;
		}

		encoded.push_back(width / 256);
		encoded.push_back(width % 256);
		encoded.push_back(height / 256);
		encoded.push_back(height % 256);

		encoded.push_back(background); // background gray color

		// terminator is 180 degree turn (difference between previous and current direction is 4)

		if (colors.size() != coords.size() || colors.size() != region_grows.size() || colors.size() != chains.size())
			printf("PROBLEM - sizes not equal\n");

		int prev_color = -1;

		for (size_t i = 0; i < colors.size(); ++i)
		{
			// encode color if different from previous
			int color = colors[i];
			if (color != prev_color) {
				encoded.push_back(color);
				prev_color = color;
			} 

			// encode coord
			if (width > 256) {
				encoded.push_back(coords[i].x() / 256);
				encoded.push_back(coords[i].x() % 256);
			} else
				encoded.push_back(coords[i].x());

			if (height > 256) {
				encoded.push_back(coords[i].y() / 256);
				encoded.push_back(coords[i].y() % 256);
			} else
				encoded.push_back(coords[i].y());

			// if last coord of this color (or last coord at all), repeat coord to indicate end
			if (i == colors.size()-1 || colors[i+1] != colors[i])
			{
				if (width > 256) {
					encoded.push_back(coords[i].x() / 256);
					encoded.push_back(coords[i].x() % 256);
				} else
					encoded.push_back(coords[i].x());

				if (height > 256) {
					encoded.push_back(coords[i].y() / 256);
					encoded.push_back(coords[i].y() % 256);
				} else
					encoded.push_back(coords[i].y());
			}
		}

		// encode end of header (0)
		encoded.push_back(background);

		// encode region growing info and directions

		if (direction_type == RELATIVE_DIRECTION)
		{
			// change from plain directions to relative directions
			for (size_t i = 0; i < chains.size(); ++i) {
				int prev = 0;
				for (size_t c = 0; c < chains[i].size(); ++c) {
					int val = prev - chains[i][c];
					if (val < 0)
						val += 8;
					prev = chains[i][c];
					chains[i][c] = val;
				}
			}
		}

		
		std::vector<unsigned char> bits_plain, bits_packed;

		// prepare bits to encode (rgio grow codes + chain directions
		for (size_t i = 0; i < chains.size(); ++i)
		{
			bits_plain.push_back(region_grows[i]);
			for (size_t c = 0; c < chains[i].size(); ++c)
				bits_plain.push_back(chains[i][c]);
		}

		if (packing_type == ONE_PER_BYTE)
			bits_packed = bits_plain;
		else if (packing_type == PACKED_FOUR_BITS)
			pack_four_bits(bits_plain, bits_packed);
		else if (packing_type == PACKED_THREE_BITS)
			pack_three_bits(bits_plain, bits_packed);
		else if (packing_type == HUFF_APRIORI) {
			unsigned char lengths[8], patterns[8];
			lengths[0] = 1;	patterns[0] = 0;
			lengths[1] = 2;	patterns[1] = 2;
			lengths[2] = 4; patterns[2] = 14;
			lengths[3] = 6;	patterns[3] = 62;
			lengths[4] = 7;	patterns[4] = 127;
			lengths[5] = 7;	patterns[5] = 126;
			lengths[6] = 5;	patterns[6] = 30;
			lengths[7] = 3;	patterns[7] = 6;
			pack_huff_dictionary(bits_plain, bits_packed, lengths, patterns);
			for (int i = 0; i < 8; ++i)
				encoded.push_back(patterns[i]);
		}
		else if (packing_type == HUFF_SIMPLE)
		{
			unsigned char lengthsTemplate[8], patternsTemplate[8];
			lengthsTemplate[0] = 1;	patternsTemplate[0] = 0;
			lengthsTemplate[1] = 2;	patternsTemplate[1] = 2;
			lengthsTemplate[2] = 3;	patternsTemplate[2] = 6;
			lengthsTemplate[3] = 4; patternsTemplate[3] = 14;
			lengthsTemplate[4] = 5;	patternsTemplate[4] = 30;
			lengthsTemplate[5] = 6;	patternsTemplate[5] = 62;
			lengthsTemplate[6] = 7;	patternsTemplate[6] = 126;
			lengthsTemplate[7] = 7;	patternsTemplate[7] = 127;

			// get counts of values to encode (0-7)
			int counts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
			for (size_t i = 0; i < bits_plain.size(); ++i)
				counts[bits_plain[i]]++;

			// sort by counts_relative
			unsigned char lengths[8], patterns[8];
			for (int i = 0; i < 8; ++i) {
				int biggest_index = 0;
				int biggest_value = counts[0];
				for (int j = 1; j < 8; ++j)
					if (counts[j] > biggest_value) {
						biggest_value = counts[j];
						biggest_index = j;
					}
				lengths[biggest_index] = lengthsTemplate[i];
				patterns[biggest_index] = patternsTemplate[i];
				counts[biggest_index] = -1;
			}

			pack_huff_dictionary(bits_plain, bits_packed, lengths, patterns);
			for (int i = 0; i < 8; ++i)
				encoded.push_back(patterns[i]);
		}
		
		for (size_t i = 0; i < bits_packed.size(); ++i)
			encoded.push_back(bits_packed[i]);
	}




	bool decode(const std::vector<unsigned char> &encoded, zor::GrayImage8u &reconstructed_image)
	{
		if (encoded.size() < 5)
			return false;

		encode_t encode_type;
		direction_t direction_type;
		packing_t packing_type;

		switch(encoded[0])
		{
			case 1:
				encode_type = PLAIN_DIRECTION_ONE_PER_BYTE;
				direction_type = PLAIN_DIRECTION;
				packing_type = ONE_PER_BYTE;
				break;
			case 2:
				encode_type = PLAIN_DIRECTION_PACKED_FOUR_BITS;
				direction_type = PLAIN_DIRECTION;
				packing_type = PACKED_FOUR_BITS;
				break;
			case 3:
				encode_type = PLAIN_DIRECTION_PACKED_THREE_BITS;
				direction_type = PLAIN_DIRECTION;
				packing_type = PACKED_THREE_BITS;
				break;
			case 4:
				encode_type = RELATIVE_DIRECTION_ONE_PER_BYTE;
				direction_type = RELATIVE_DIRECTION;
				packing_type = ONE_PER_BYTE;
				break;
			case 5:
				encode_type = RELATIVE_DIRECTION_PACKED_FOUR_BITS;
				direction_type = RELATIVE_DIRECTION;
				packing_type = PACKED_FOUR_BITS;
				break;
			case 6:
				encode_type = RELATIVE_DIRECTION_PACKED_THREE_BITS;
				direction_type = RELATIVE_DIRECTION;
				packing_type = PACKED_THREE_BITS;
				break;
			case 7:
				encode_type = RELATIVE_DIRECTION_HUFF_APRIORI;
				direction_type = RELATIVE_DIRECTION;
				packing_type = HUFF;
				break;
			case 8:
				encode_type = RELATIVE_DIRECTION_HUFF_SIMPLE;
				direction_type = RELATIVE_DIRECTION;
				packing_type = HUFF;
				break;
		}

		int width = encoded[1]*256 + encoded[2];
		int height = encoded[3]*256 + encoded[4];
		reconstructed_image.resize(width, height);
		
		unsigned char background = encoded[5];
		reconstructed_image.fill(background);

		std::vector<unsigned char> colors;
		std::vector<zor::Vector2i> coords;

		int encode_index = 6;
		unsigned char color = encoded[encode_index++];

		while (color != background)
		{
			int prev_x = -1, prev_y = -1;

			// read coord
			while (encode_index < (int)encoded.size())
			{
				int x, y;

				if (width > 256) {
					x = encoded[encode_index++] * 256;
					x += encoded[encode_index++];
				} else
					x = encoded[encode_index++];

				if (height > 256) {
					y = encoded[encode_index++] * 256;
					y += encoded[encode_index++];
				} else
					y = encoded[encode_index++];

				if (x == prev_x && y == prev_y)
					break;

				colors.push_back(color);
				coords.push_back(zor::Vector2i(x, y));

				prev_x = x;
				prev_y = y;
			}

			color = encoded[encode_index++];
		}

		// if using Huffman type coding, read the eight bit patterns
		unsigned char patterns[8];
		if (packing_type == HUFF) {
			for (int i = 0; i < 8; ++i)
				patterns[i] = encoded[encode_index++];
		}

		std::vector<unsigned char> bits_packed, bits_plain;
		for (size_t i = encode_index; i < encoded.size(); ++i)
			bits_packed.push_back(encoded[i]);

		if (packing_type == ONE_PER_BYTE)
			bits_plain = bits_packed;
		else if (packing_type == PACKED_FOUR_BITS)
			unpack_four_bits(bits_packed, bits_plain);
		else if (packing_type == PACKED_THREE_BITS)
			unpack_three_bits(bits_packed, bits_plain);
		else if (packing_type == HUFF)
			unpack_huff_dictionary(bits_packed, bits_plain, patterns);

		// for each coord, read region grow value and then chain (if any)
		std::vector<unsigned char> region_grows;
		std::vector< std::vector<unsigned char> > chains;

		int index_bits = 0;
		for (size_t i = 0; i < coords.size(); ++i)
		{
			unsigned char region_grow_value = bits_plain[index_bits++];
			region_grows.push_back(region_grow_value);

			if (region_grow_value != 1)
			{
				std::vector<unsigned char> chain;
				int prev = 0;
				while (true) {
					int val = bits_plain[index_bits++];
					if (direction_type == RELATIVE_DIRECTION) {
						val = prev - val;
						if (val < 0)
							val += 8;
					}
					chain.push_back(val);

					// terminate when previous direction minus current direction is 4 (that is, backwards to previous position)
					int terminate = prev - val;
					if (terminate < 0)
						terminate += 8;
					if (terminate == 4)
						break;
					prev = val;
				}
				chains.push_back(chain);
			}
			else
				chains.push_back(std::vector<unsigned char>()); // no chain coding
		}

		// place chains in image
		for (size_t i = 0; i < coords.size(); ++i)
			if (region_grows[i] == 0 || region_grows[i] == 7)
				place_chain_freeman8(reconstructed_image, coords[i], chains[i], colors[i]);

		// region grow
		for (size_t i = 0; i < coords.size(); ++i)
			if (region_grows[i] == 0 || region_grows[i] == 1) {
				reconstructed_image.pixel(coords[i].x(), coords[i].y()) = background; // in case coord was part of chaining
				region_grow(reconstructed_image, coords[i], colors[i]);
			}

		return true;
	}




private:




	// all pixels with specified gray value are set to 0
	// all other pixels are set to 255
	// INPUT: grayscale image and specified value
	// OUTPUT: binary image (all pixels set to 0 or 255)
	int isolate_gray_value(               // RETURNS number of pixels of value
		const zor::GrayImage8u &gray_image, // original grayscale image
		unsigned char value,              // value in image to binarize as 0
		zor::GrayImage8u &binary_image)     // resulting binarized image
	{
		binary_image.resize(gray_image.width(), gray_image.height());
		int count = 0;
		for (int i = 0; i < gray_image.num_pixels(); ++i) {
			if (gray_image.pixel(i) == value) {
				binary_image.pixel(i) = 0;
				count++;
			} else
				binary_image.pixel(i) = 255;
		}
		return count;
	}




	// mark all pixels which are inside a blob as 127
	// (i.e., pixel is 0 and all four neighbors are 0)
	// INPUT: binary grayscale image (all pixels set to 0 or 255)
	// OUTPUT: ternary grayscale image (all pixels set to 0, 127, or 255)
	int mark_inner_pixels(zor::GrayImage8u &image)
	{
		int count = 0;
		for (int y = 1; y < image.height()-1; ++y)
			for (int x = 1; x < image.width()-1; ++x)
				if (image.pixel(x, y) == 0)
					if (image.pixel(x-1, y) != 255 && image.pixel(x+1, y) != 255 &&
						image.pixel(x, y-1) != 255 && image.pixel(x, y+1) != 255) {
							image.pixel(x, y) = 127;
							count++;
					}
		return count;
	}




	int region_grow(zor::GrayImage8u &image, zor::Vector2i coord, unsigned char set_value)
	{
		unsigned char val = image.pixel(coord.x(), coord.y());
		if (val == set_value)
			return 0;

		int count = 0;
		std::queue<zor::Vector2i> seeds;
		seeds.push(coord);

		while (!seeds.empty())
		{
			coord = seeds.front();
			seeds.pop();
			int x = coord.x();
			int y = coord.y();

			if (image.pixel(x, y) == val)
			{
				count++;
				image.pixel(x, y) = set_value;

				if (x > 0 && image.pixel(x-1, y) == val)
					seeds.push(zor::Vector2i(x-1, y));
				if (x < image.width()-1 && image.pixel(x+1, y) == val)
					seeds.push(zor::Vector2i(x+1, y));
				if (y > 0 && image.pixel(x, y-1) == val)
					seeds.push(zor::Vector2i(x, y-1));
				if (y < image.height()-1 && image.pixel(x, y+1) == val)
					seeds.push(zor::Vector2i(x, y+1));
			}
		}

		return count;
	}




	int create_chain_freeman8(zor::GrayImage8u &image, zor::Vector2i coord, std::vector<unsigned char> &chain)
	{
		chain.clear();
		//chain.push_back(0); // starting direction

		//       2
		//   3   ^   1
		//     \ | /
		//  4 <-- --> 0
		//     / | \
		//   5   V   7
		//       6

		int x = coord.x();
		int y = coord.y();
		int count = -1;

		while (x >= 0 && y >= 0)
		{
			image.pixel(x, y) = 255;
			count++;

			if (x < image.width()-1 && image.pixel(x+1, y) == 0) {
				x = x + 1;
				chain.push_back(0);
			}
			else if (x < image.width()-1 && y > 0 && image.pixel(x+1, y-1) == 0) {
				x = x + 1;
				y = y - 1;
				chain.push_back(1);
			}
			else if (y > 0 && image.pixel(x, y-1) == 0) {
				y = y - 1;
				chain.push_back(2);
			}
			else if (x > 0 && y > 0 && image.pixel(x-1, y-1) == 0) {
				x = x - 1;
				y = y - 1;
				chain.push_back(3);
			}
			else if (x > 0 && image.pixel(x-1, y) == 0) {// && count > 0) { // make sure first isn't backwards!
				x = x - 1;
				chain.push_back(4);
			}
			else if (x > 0 && y < image.height()-1 && image.pixel(x-1, y+1) == 0) {
				x = x - 1;
				y = y + 1;
				chain.push_back(5);
			}
			else if (y < image.height()-1 && image.pixel(x, y+1) == 0) {
				y = y + 1;
				chain.push_back(6);
			}
			else if (x < image.width()-1 && y < image.height()-1 && image.pixel(x+1, y+1) == 0) {
				x = x + 1;
				y = y + 1;
				chain.push_back(7);
			}
			//else if (x > 0 && image.pixel(x-1, y) == 0) {
			//	x = x - 1;
			//	chain.push_back(4);
			//}
			else {
				x = -1;
				y = -1;
			}
		}

		// if first "real" chain value is 4, change starting direction to 1
		//if (chain.size() > 1 && chain[1] == 4)
		//	chain[0] = 1;

		// add terminator value where previous - current = 4 (or -4)
		if (chain.size() == 0)
			chain.push_back(4);
		else {
			int terminate = chain[chain.size()-1] - 4;
			if (terminate < 0)
				terminate += 8;
			chain.push_back(terminate);
		}

		return count;
	}




	// ignore last one
	void place_chain_freeman8(zor::GrayImage8u &image, zor::Vector2i coord, std::vector<unsigned char> &chain, unsigned char color)
	{
		int x = coord.x(), y = coord.y();
		image.pixel(x, y) = color;

		for (size_t i = 0; i < chain.size()-1; ++i)
		{
			switch (chain[i]) {
			case 0:
				x = x + 1;
				break;
			case 1:
				x = x + 1;
				y = y - 1;
				break;
			case 2:
				y = y - 1;
				break;
			case 3:
				x = x - 1;
				y = y - 1;
				break;
			case 4:
				x = x - 1;
				break;
			case 5:
				x = x - 1;
				y = y + 1;
				break;
			case 6:
				y = y + 1;
				break;
			case 7:
				x = x + 1;
				y = y + 1;
				break;
			}

			image.pixel(x, y) = color;
		}
	}




	void pack_four_bits(const std::vector<unsigned char> &bits_plain, std::vector<unsigned char> &bits_packed)
	{
		bits_packed.clear();
		int size = bits_plain.size() - (bits_plain.size() % 2);
		for (int i = 0; i < size; i += 2) {
			unsigned char upper = bits_plain[i] << 4;
			unsigned char lower = bits_plain[i+1];
			bits_packed.push_back(upper | lower);
		}
		if (bits_plain.size() % 2 == 1) {
			unsigned char upper = bits_plain[size] << 4;
			bits_packed.push_back(upper);
		}
	}




	void unpack_four_bits(const std::vector<unsigned char> &bits_packed, std::vector<unsigned char> &bits_plain)
	{
		bits_plain.clear();
		for (size_t i = 0; i < bits_packed.size(); ++i) {
			unsigned char upper = bits_packed[i] & 0xF0;
			upper = upper >> 4;
			unsigned char lower = bits_packed[i] & 0x0F;
			bits_plain.push_back(upper);
			bits_plain.push_back(lower);
		}
	}


	public:

	template <int bytes>
	void print_bits(int value, char *before, char *after)
	{
		std::bitset<bytes*8> b(value);
		std::cout << before << b << after;
	}



	void pack_three_bits(const std::vector<unsigned char> &bits_plain, std::vector<unsigned char> &bits_packed)
	{
		std::vector<unsigned char> bits = bits_plain;
		while (bits.size() % 8 != 0)
			bits.push_back(0);

		for (size_t i = 0; i < bits.size(); i += 8)
		{
			int bits0 = bits[i];
			int bits1 = bits[i+1];
			int bits2 = bits[i+2];
			int bits3 = bits[i+3];
			int bits4 = bits[i+4];
			int bits5 = bits[i+5];
			int bits6 = bits[i+6];
			int bits7 = bits[i+7];
			
			bits0 <<= 21;
			bits1 <<= 18;
			bits2 <<= 15;
			bits3 <<= 12;
			bits4 <<= 9;
			bits5 <<= 6;
			bits6 <<= 3;

			int combined = bits0;
			combined |= bits1;
			combined |= bits2;
			combined |= bits3;
			combined |= bits4;
			combined |= bits5;
			combined |= bits6;
			combined |= bits7;

			//print_bits<3>(combined, "", "\n");

			unsigned char byte2 = combined & 0xFF;
			combined >>= 8;
			unsigned char byte1 = combined & 0xFF;
			combined >>= 8;
			unsigned char byte0 = combined & 0xFF;

			//print_bits<1>(byte0, "", "");
			//print_bits<1>(byte1, "", "");
			//print_bits<1>(byte2, "", "\n");

			bits_packed.push_back(byte0);
			bits_packed.push_back(byte1);
			bits_packed.push_back(byte2);
		}
	}




	void unpack_three_bits(const std::vector<unsigned char> &bits_packed, std::vector<unsigned char> &bits_plain)
	{
		bits_plain.clear();

		for (size_t i = 0; i < bits_packed.size(); i += 3)
		{
			unsigned char byte0 = bits_packed[i];
			unsigned char byte1 = bits_packed[i+1];
			unsigned char byte2 = bits_packed[i+2];

			int combined = byte0;
			combined <<= 8;
			combined |= byte1;
			combined <<= 8;
			combined |= byte2;

			int bits7 = combined & 7;
			combined >>= 3;
			int bits6 = combined & 7;
			combined >>= 3;
			int bits5 = combined & 7;
			combined >>= 3;
			int bits4 = combined & 7;
			combined >>= 3;
			int bits3 = combined & 7;
			combined >>= 3;
			int bits2 = combined & 7;
			combined >>= 3;
			int bits1 = combined & 7;
			combined >>= 3;
			int bits0 = combined & 7;

			bits_plain.push_back(bits0);
			bits_plain.push_back(bits1);
			bits_plain.push_back(bits2);
			bits_plain.push_back(bits3);
			bits_plain.push_back(bits4);
			bits_plain.push_back(bits5);
			bits_plain.push_back(bits6);
			bits_plain.push_back(bits7);
		}
	}




	void pack_huff_dictionary(const std::vector<unsigned char> &bits_plain, std::vector<unsigned char> &bits_packed,
		const unsigned char lengths[8], const unsigned char patterns[8])
	{
		// original byte values only 0-7

		bits_packed.clear();
		int count = -8;
		unsigned int bits = 0;

		// shift one bit at a time to make room for new bit pattern to add at end
		// if during shift reach 8 shifts, upper byte now full, take it and add to output
		// start count at -8

		for (size_t i = 0; i < bits_plain.size(); ++i)
		{
			//print_bits<2>(bits, "before shift: ", "\n");
			int size = lengths[bits_plain[i]];
			while (size > 0) {
				bits <<= 1;
				size--;
				count++;
				if (count == 8) {
					unsigned int upper = bits & 0xFF00;
					upper >>= 8;
					bits_packed.push_back(upper & 0xFF);
					count = 0;
				}
			}
			//print_bits<2>(bits, " after shift: ", "\n");
			bits |= patterns[bits_plain[i]];
			//print_bits<2>(bits, " after added: ", "\n");
		}

		// at end, do enough shifts to bring total to eight
		// place upper and lower bytes into output
		if (count <= 0) {
			bits <<= (-count);
			bits_packed.push_back(bits & 0xFF);
		}
		else {
			bits <<= (8 - count);
			unsigned int upper = bits & 0xFF00;
			upper >>= 8;
			bits_packed.push_back(upper & 0xFF);
			bits_packed.push_back(bits & 0xFF);
		}

		//for (size_t i = 0; i < bits_packed.size(); ++i)
		//	print_bits<1>(bits_packed[i], "", " ");
		//printf("\n");
	}




	void unpack_huff_dictionary(const std::vector<unsigned char> &bits_packed, std::vector<unsigned char> &bits_plain,
		const unsigned char patterns[8])
	{
		bits_plain.clear();

		unsigned char pos = 128;
		unsigned char bits = 0;
		int byte = 0;

		while (byte < (int)bits_packed.size())
		{
			unsigned char val = bits_packed[byte] & pos;
			bits <<= 1;
			if (val != 0)
				bits |= 1;

			for (int i = 0; i < 8; ++i)
				if (bits == patterns[i]) {
					bits_plain.push_back(i);
					bits = 0;
				}

			pos >>= 1;
			if (pos == 0) {
				pos = 128;
				byte++;
			}
		}
	}




};

