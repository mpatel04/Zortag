//
//  process.h
//  ZorSecure
//
//  Created by Joseph Marino on 4/8/13.
//  Copyright (c) 2013 Zortag. All rights reserved.
//

#ifndef ZorSecure_process_h
#define ZorSecure_process_h

#include "ellipse.h"


static void extractBubble(const zor::GrayImage8u &rawImage, const std::vector<zor::Vector2f> corners, zor::GrayImage8u &bubble,
                          zor::GrayImage8u &surround, std::vector<zor::GrayImage8u> *debugImages)
{
    //NSLog(@"before resample");
    zor::Vector2f oldPos[4], newPos[4];
    oldPos[0] = corners[0];     newPos[0].set(6, 35);
    oldPos[1] = corners[1];     newPos[1].set(6, 175);
    oldPos[2] = corners[2];     newPos[2].set(146, 175);
    oldPos[3] = corners[3];     newPos[3].set(146, 35);
    zor::GrayImage8u grayResampled;
	//resample(rawImage, oldPos, newPos, 384, 214, grayResampled);
	zor::transform_image(grayResampled, rawImage, oldPos, newPos, 384, 214, 255);
    if (debugImages != NULL)
        debugImages->push_back(grayResampled);
    //NSLog(@"after resample");

    zor::GrayImage8u grayBinary;
    threshold_otsu_block(grayResampled, 16, 0.45f, grayBinary); // was 0.7f, then 0.45f;
    if (debugImages != NULL)
        debugImages->push_back(grayBinary);

    zor::GrayImage8u grayEdge;
    //roughEdgesBlack(grayBinary, grayEdge);
	zor::edge_detect_binary_eight_neighbors(grayEdge, grayBinary, 0, 255, 0);
	//zor::edge_detect_binary_eight_neighbors_setup(grayEdge, grayBinary).run(0, 255, 0);
    if (debugImages != NULL)
        debugImages->push_back(grayEdge);
	grayEdge.fill_border(0);

    zor::GrayImage8u grayEdgeReduce;
    reduceEdges(grayEdge, grayEdgeReduce);
    if (debugImages != NULL)
        debugImages->push_back(grayEdgeReduce);

    float conicEquation[6];
    randomHoughEllipseConicEquation(grayEdgeReduce, conicEquation);

    float A = conicEquation[0];
    float B = conicEquation[1];
    float C = conicEquation[2];
    float D = conicEquation[3];
    float E = conicEquation[4];
    float F = conicEquation[5];
    // color ellipse in gray for debugging
    if (debugImages != NULL) {
        for (int y = 0; y < grayEdgeReduce.height(); ++y)
            for (int x = 0; x < grayEdgeReduce.width(); ++x) {
                float value = A*x*x + B*x*y + C*y*y + D*x + E*y + F;
                if (value < 0 && grayEdgeReduce.pixel(x, y) != 255)
                    grayEdgeReduce.pixel(x, y) = 100;
            }
        debugImages->push_back(grayEdgeReduce);
    }
    
    float parametricEquation[5];
    ellipse_conic_to_parametric(conicEquation, parametricEquation);

    //resampleBubble(grayResampled, parametricEquation, bubble);
	zor::Matrix3x3f transform = zor::create_transformation_ellipse_to_circle(parametricEquation, zor::Vector2f(255.f/2, 255.f/2), 128*1.2f);
	zor::transform_image(bubble, grayResampled, transform, 256, 256, 255);
    if (debugImages != NULL)
        debugImages->push_back(bubble);

	// new additions
	zor::Matrix3x3f transform2 = zor::create_transformation_ellipse_to_circle(parametricEquation, zor::Vector2f(255.f/2, 255.f/2), 128*0.9f);
	zor::transform_image(surround, grayResampled, transform2, 256, 256, 255);
}


static void extractBubble(const zor::GrayImage8u &rawImage, const zor::Vector2f corners[4], zor::GrayImage8u &bubble,
                          zor::GrayImage8u& surround, std::vector<zor::GrayImage8u> *debugImages)
{
    std::vector<zor::Vector2f> cornersVec(4);
    cornersVec[0] = corners[0];
    cornersVec[1] = corners[1];
    cornersVec[2] = corners[2];
    cornersVec[3] = corners[3];
    extractBubble(rawImage, cornersVec, bubble, surround, debugImages);
}


static void createComposite(const std::vector<const zor::GrayImage8u*> &images, zor::GrayImage8u &composite)
{
    if (images.size() == 0)
        return;
    
    composite.resize(images[0]->width(), images[0]->height());
    if (images.size() == 1) {
        for (int i = 0; i < composite.num_pixels(); ++i)
            composite.pixel(i) = images[0]->pixel(i);
        return;
    }
    
    int *sum = new int[composite.num_pixels()];
    for (int i = 0; i < composite.num_pixels(); ++i)
        sum[i] = 0;
    for (int n = 0; n < (int)images.size(); ++n)
        for (int i = 0; i < composite.num_pixels(); ++i)
            sum[i] += images[n]->pixel(i);
    for (int i = 0; i < composite.num_pixels(); ++i)
        composite.pixel(i) = (unsigned char)(sum[i] / images.size());    
    delete[] sum;
}


static void getSignature(const std::vector<const zor::GrayImage8u*> &bubbles, zor::GrayImage8u &signature,
                         std::vector<zor::GrayImage8u> *debugImages)
{
    zor::GrayImage8u composite;
    createComposite(bubbles, composite);
    if (debugImages != NULL)
        debugImages->push_back(composite);
    
    //threshold_otsu(composite, 0.65f, signature); // was 0.85f, then 0.7f
	//threshold_otsu_block(composite, 64, 0.35f, signature);
	// ORIGINAL ==> threshold_otsu_block(composite, 32, 0.45f, signature); // was 0.45f;
	threshold_otsu_block(composite, 64, 0.45f, signature); // was 0.45f;

    for (int y = 0; y < signature.height(); ++y)
        for (int x = 0; x < signature.width(); ++x)
            if (((x-128)*(x-128) + (y-128)*(y-128)) > 15625) // 128 -> 16384, 127 -> 16129, 126 -> 15876, 120 -> 14400
                signature.pixel(x, y) = 255;

	int num = 1;
	for (int i = 0; i < num; ++i)
		zor::erode_binary_image(signature, signature, 0, 255);
	for (int i = 0; i < num; ++i)
		zor::dilate_binary_image(signature, signature, 0);

    if (debugImages != NULL)
        debugImages->push_back(signature);
}


static void getSignature_new(const std::vector<const zor::GrayImage8u*> &bubbles, zor::GrayImage8u &signature,
                         std::vector<zor::GrayImage8u> *debugImages)
{
    zor::GrayImage8u composite;
    createComposite(bubbles, composite);
    if (debugImages != NULL)
        debugImages->push_back(composite);
    
    threshold_otsu(composite, 0.65f, signature); // was 0.85f

	zor::GrayImage8u grayEdge;
	zor::edge_detect_binary_eight_neighbors(grayEdge, signature, 0, 255, 0);
	grayEdge.fill_border(0);

	for (int y = 0; y < grayEdge.height(); ++y)
        for (int x = 0; x < grayEdge.width(); ++x) {
            if (((x-128)*(x-128) + (y-128)*(y-128)) < 15376)
                grayEdge.pixel(x, y) = 0;
			else if (((x-128)*(x-128) + (y-128)*(y-128)) > 18496)
				grayEdge.pixel(x, y) = 0;
		}

	/*
		120 -> 14400
		124 -> 15376
		125 -> 15625
		126 -> 15876
		127 -> 16129
		128 -> 16384
		130 -> 16900
		132 -> 17424
		134 -> 17956
		136 -> 18496
	*/

	//signature = grayEdge;
	
    float conicEquation[6], parametricEquation[5];
    randomHoughEllipseConicEquation(grayEdge, conicEquation);
    ellipse_conic_to_parametric(conicEquation, parametricEquation);
	
	zor::Matrix3x3f transform = zor::create_transformation_ellipse_to_circle(parametricEquation, zor::Vector2f(255.f/2, 255.f/2), 128);
	zor::transform_image(signature, signature, transform, 256, 256, 255);

	zor::GrayImage8u sig2;
	threshold_otsu(signature, 0.5f, sig2); // was 0.85f
	signature = sig2;
	//signature.setBorder(255);

    for (int y = 0; y < signature.height(); ++y)
        for (int x = 0; x < signature.width(); ++x)
            if (((x-128)*(x-128) + (y-128)*(y-128)) > 15875)
                signature.pixel(x, y) = 255;
	
	int num = 2;
	for (int i = 0; i < num; ++i)
		zor::erode_binary_image(signature, signature, 0, 255);
	for (int i = 0; i < num; ++i)
		zor::dilate_binary_image(signature, signature, 0);
	
    if (debugImages != NULL)
        debugImages->push_back(signature);
}


static void getSignatureOLD(const zor::GrayImage8u &bubble, zor::GrayImage8u &signature, std::vector<zor::GrayImage8u> *debugImages)
{
    //std::vector<const zor::GrayImage8u*> bubblePtrs;
    //bubblePtrs.push_back(&bubble);
    //getSignature(bubblePtrs, signature, debugImages);
    
    threshold_otsu(bubble, 0.85f, signature);
    //threshold_otsu_block(bubble, 8, 0.85f, signature);
    for (int y = 0; y < signature.height(); ++y)
        for (int x = 0; x < signature.width(); ++x)
            if (((x-128)*(x-128) + (y-128)*(y-128)) > 14400) // 128 -> 16384, 120 -> 14400
                signature.pixel(x, y) = 255;
    if (debugImages != NULL)
        debugImages->push_back(signature);
}

static void getSignature(const zor::GrayImage8u &bubble, zor::GrayImage8u &signature, std::vector<zor::GrayImage8u> *debugImages)
{
    zor::GrayImage8u mask(bubble.width(), bubble.height());
    for (int y = 0; y < bubble.height(); ++y)
        for (int x = 0; x < bubble.width(); ++x) {
            if (((x-128)*(x-128) + (y-128)*(y-128)) > 14400) // 128 -> 16384, 120 -> 14400, 124 -> 15376
                mask.pixel(x, y) = 0;
            else
                mask.pixel(x, y) = 255;
        }
    
    threshold_otsu(bubble, mask, 0.85f, signature);
}


// comparison by matching all non-white pixels within range
static void compareParticles(const zor::GrayImage8u &sig1, const zor::GrayImage8u &sig2, int radius, float ratios[2])
{
    if (sig1.width() != sig2.width() || sig1.height() != sig2.height())
       return;
    
    int offsetSize = (radius*2)+1, offsetIndex = 0;
    std::vector<zor::Vector2i> offsets(offsetSize*offsetSize);
    for (int x = -offsetSize/2; x <= offsetSize/2; ++x)
        for (int y = -offsetSize/2; y <= offsetSize/2; ++y)
            offsets[offsetIndex++].set(x, y);
    
    int count = 0, matched = 0;
    int borderSkip = offsetSize/2;
    for (int y = borderSkip; y < sig1.width()-borderSkip; ++y)
        for (int x = borderSkip; x < sig1.height()-borderSkip; ++x)
            if (sig1.pixel(x, y) != 255) {
				count++;
				for (int i = 0; i < offsetIndex; ++i)
					if (sig2.pixel(x+offsets[i].x(), y+offsets[i].y()) != 255) {
						matched++;
						break;
					}
			}
    ratios[0] = (float)matched / (float)count;
    
    count = 0;
    matched = 0;
    for (int y = borderSkip; y < sig2.width()-borderSkip; ++y)
        for (int x = borderSkip; x < sig2.height()-borderSkip; ++x)
            if (sig2.pixel(x, y) != 255) {
				count++;
				for (int i = 0; i < offsetIndex; ++i)
					if (sig1.pixel(x+offsets[i].x(), y+offsets[i].y()) != 255) {
						matched++;
						break;
					}
			}
    ratios[1] = (float)matched / (float)count;
}


/*
static void compareBubbles3D(const std::vector<const zor::GrayImage8u*> &bubbles, float results3D[3], std::vector<zor::GrayImage8u> *debugImages)
{
    if (bubbles.size() == 0)
        return;
    
    int width = bubbles[0]->width(), height = bubbles[0]->height();
    int numPixels = bubbles[0]->numPixels(), numImages = bubbles.size();
    
    std::vector<zor::GrayImage8u> bubbleImages(numImages);
    for (int n = 0; n < numImages; ++n)
        mean_filter(*bubbles[n], 5, bubbleImages[n]);
	
    int numOffsets = 4;
    zor::Vector2i offsets[4];
    offsets[0].set(-1, -1);
    offsets[1].set(0, -1);
    offsets[2].set(1, -1);
    offsets[3].set(-1, 0);
	
    std::vector<const zor::GrayImage8u*> bubbleImagesPtrs(numImages);
    for (size_t i = 0; i < numImages; ++i)
        bubbleImagesPtrs[i] = &bubbleImages[i];
    zor::GrayImage8u composite, sig;
    createComposite(bubbleImagesPtrs, composite);
    //threshold_otsu_block(composite, 16, 0.65f, sig);
    if (debugImages != NULL) {
        debugImages->push_back(composite);
        //debugImages->push_back(sig);
    }
    
    zor::GrayImage8u compositeFiltered;
    mean_filter(composite, 5, compositeFiltered);
    
    zor::GrayImage8u bubbleMask(width, height);
    for (int i = 0; i < numPixels; ++i)
        bubbleMask.pixel(i) = 0;
    
    //if (whole)
    {
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
                if (((x-128)*(x-128) + (y-128)*(y-128)) < 14400) // radius 99 -> 9801; radius 128 -> 16384; radius 126 -> 15876; radius 120 -> 14400
                    bubbleMask.pixel(x, y) = 255;
    }
    
    //else
    //{
    //    for (int i = 0; i < numPixels; ++i)
    //        if (sig.pixel(i) == 0)
    //            bubbleMask.values[i] = true;
    //    bubbleMask.dilate(3);
    //    for (int y = 0; y < height; ++y)
    //        for (int x = 0; x < width; ++x)
    //            if (((x-128)*(x-128) + (y-128)*(y-128)) > 14400) // radius 99 -> 9801; radius 128 -> 16384; radius 126 -> 15876; radius 120 -> 14400
    //                bubbleMask.values[y*width + x] = false;
    //}
     
    
    //if (images != null)
    //    images.add(bubbleMask.tozor::GrayImage8u());
    
    GLCM glcm;
    float contrastOriginals = 0.f;
    for (int i = 0; i < numImages; ++i) {
        float contrast = 0.f;
        for (int j = 0; j < numOffsets; ++j) {
            glcm.create(bubbleImages[i], bubbleMask, offsets[j]);
            contrast += glcm.contrast();
        }
        contrast /= numOffsets;
        contrastOriginals += contrast;
    }
    contrastOriginals /= numImages;
    
    float contrastComposite = 0.f, contrastCompositeFiltered = 0.f;
    for (int j = 0; j < numOffsets; ++j) {
        glcm.create(composite, bubbleMask, offsets[j]);
        contrastComposite += glcm.contrast();
        glcm.create(compositeFiltered, bubbleMask, offsets[j]);
        contrastCompositeFiltered += glcm.contrast();
    }
    contrastComposite /= numOffsets;
    contrastCompositeFiltered /= numOffsets;
    
    results3D[0] = contrastComposite / contrastOriginals;
    results3D[1] = contrastCompositeFiltered / contrastOriginals;
    results3D[2] = (results3D[0] + results3D[1]) / 2.f;
}
*/


static float compareShiftedOLD(const zor::GrayImage8u &image1, const zor::GrayImage8u &image2)
{
    if (image1.width() != image2.width() || image1.height() != image2.height())
        return 0;
    
    float matched1[10] = {0.f}, matched2[10] = {0.f};
    float count1 = 0.f, count2 = 0.f;
    
    for (int i = 10; i < image1.num_pixels()-10; ++i)
    {
        if (image1.pixel(i) != 255) {
            count1 += 1;
            for (int n = 0; n < 10; ++n)
                if (image2.pixel(i-n) != 255)
                    matched1[n] += 1;
        }
        if (image2.pixel(i) != 255) {
            count2 += 1;
            for (int n = 0; n < 10; ++n)
                if (image1.pixel(i+n) != 255)
                    matched2[n] += 1;
        }
    }
    
    for (int n = 0; n < 10; ++n) {
        matched1[n] /= count1;
        matched2[n] /= count2;
    }
    
    //for (int i = 0; i < 10; ++i)
    //    NSLog(@"%d: %f", i, matched[i]);
    
    float max1 = matched1[1], max2 = matched2[1];;
    for (int i = 2; i < 10; ++i) {
        if (matched1[i] > max1)
            max1 = matched1[i];
        if (matched2[i] > max2)
            max2 = matched2[i];
    }
    
    //NSLog(@"Shift Ratios: %f %f", max1/matched1[0], max2/matched2[0]);
    return ( (max1/matched1[0]) + (max2/matched2[0]) ) / 2.f;
}


static float compareShifted(const zor::GrayImage8u &image1, const zor::GrayImage8u &image2, bool left = true)
{
    if (image1.width() != image2.width() || image1.height() != image2.height())
        return 0;
    
    const int dist = 10;
    float matched1[dist] = {0.f};
    float count1 = 0.f;
    
    if (left)
    {
        for (int i = dist; i < image1.num_pixels()-dist; ++i)
        {
            if (image1.pixel(i) != 255) {
                count1 += 1;
                for (int n = 0; n < dist; ++n)
                    if (image2.pixel(i-n) != 255)
                        matched1[n] += 1;
            }
        }
    }
    else
    {
        for (int i = dist; i < image1.num_pixels()-dist; ++i)
        {
            if (image1.pixel(i) != 255) {
                count1 += 1;
                for (int n = 0; n < dist; ++n)
                    if (image2.pixel(i+n) != 255)
                        matched1[n] += 1;
            }
        }
    }
    
    for (int n = 0; n < dist; ++n) {
        matched1[n] /= count1;
    }
    
    //for (int i = 0; i < 10; ++i)
    //    NSLog(@"%d: %f", i, matched[i]);
    
    float max1 = matched1[2]; // was [2]
    int maxOffset = 2;
    for (int i = 3; i < dist; ++i) { // was i = 3
        if (matched1[i] > max1) {
            max1 = matched1[i];
            maxOffset = i;
        }
    }
    printf("  MaxIndex: %d", maxOffset);
    
    //NSLog(@"Shift Ratios: %f %f", max1/matched1[0], max2/matched2[0]);
    //return max1/matched1[0];
    
    float ret = max1 / matched1[0];
    
    float val = sqrtf((float)maxOffset) * ret * ret;
    printf("  New Shift Value: %g\n", val);
    
    //return ret;
    return val;
}


static float compareShifted2(const zor::GrayImage8u &image1, const zor::GrayImage8u &image2)
{
    if (image1.width() != image2.width() || image1.height() != image2.height())
        return 0;
    
    // for each black pixel in image1, see how far left to go to hit black pixel (up to image edge) in image2
    int numShiftedPixels = 0;
    int shiftAmount = 0;
    for (int i = 0; i < image1.num_pixels(); ++i)
    {
        if (image1.pixel(i) != 255)
        {
            int x = i % image1.width();
            int index = i;
            int count = 0;
            bool found = false;
            while (!found && x >= 0 && count < 10) {
                if (image2.pixel(index) != 255)
                    found = true;
                else {
                    count++;
                    x--;
                    index--;
                }
            }
            if (found && count >= 0) {
                shiftAmount += count;
                numShiftedPixels++;
            }
        }
    }
    
    printf("shift: %d / %d = %f", shiftAmount, numShiftedPixels, (float)shiftAmount/numShiftedPixels);
    return (float)shiftAmount / numShiftedPixels;
}


static void gammaCorrect(const zor::GrayImage8u &in, zor::GrayImage8u &out, float gamma)
{
    if (out.width() != in.width() || out.height() != in.height())
        out.resize(in.width(), in.height());
    
    for (int i = 0; i < in.num_pixels(); ++i) {
        float value = in.pixel(i);
        value /= 255;
        value = powf(value, gamma);
        value *= 255;
        out.pixel(i) = static_cast<unsigned char>(value);
    }
}


#endif
