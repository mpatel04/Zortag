

#include <PixeLINKApi.h>
#pragma comment(lib, "PxLAPI40")


class PixelinkCamera
{

public:


	// Gets information about each camera that is connected.
	// Returns the number of connected cameras, or return -1 on error
	static int get_cameras_info(std::vector<CAMERA_ID_INFO> &cam_info)
	{
		cam_info.clear();

		// Query how many cameras there are
		U32 ncam;
		PXL_RETURN_CODE rc = PxLGetNumberCamerasEx(NULL, &ncam);
		if (!API_SUCCESS(rc))
			return -1;

		if (ncam == 0)
			return 0;
		
		// Declare a vector/array of CAMERA_ID_INFO structs, one for each camera
		cam_info.resize(ncam);

		// Don't forget to set the StructSize of the first element of the array
		cam_info[0].StructSize = sizeof(CAMERA_ID_INFO);
 
		rc = PxLGetNumberCamerasEx(&cam_info[0], &ncam);
		if (!API_SUCCESS(rc))
			return -1;

		return static_cast<int>(ncam);
	}


	PixelinkCamera() : _camera(NULL)
	{
		frame.resize(3000*3000*3);
		memset(&frame_descr, 0xA5, sizeof(FRAME_DESC));
	}


	void print_last_error()
	{
		// Request and display more information about the error
		ERROR_REPORT errorReport;
		PxLGetErrorReport(_camera, &errorReport);
		printf("\n");
		printf("Last function to return error was %s\n", errorReport.strFunctionName);
		printf("Last Error was 0x%X (%s)\n", errorReport.uReturnCode, errorReport.strReturnCode);
		printf("Description: %s\n", errorReport.strReport);
		printf("\n");
	}


	// Initialize the camera and set its pixel format to color
	// (pass 0 as serial number to get random camera)
	bool initialize(U32 serial_number)
	{
		/*
		 * Call PxLInitialize, passing in a serial number of 0, 
		 * meaning we want to talk to a (any) camera connected 
		 * to the system. If there's only one camera, we'll be 
		 * connected to it. If there's more than one camera, we 
		 * cannot know deterministically which camera we will be 
		 * connected to. 
		 * See the demo app getcamerainfo for an example of how to 
		 * enumerate all cameras and connect to a specific camera.
		 */
		PXL_RETURN_CODE rc = PxLInitialize(serial_number, &_camera);
		//printf("PxLInitialize   returned 0x%8.8X (%s)\n", rc,  (API_SUCCESS(rc)) ? "Success" : "Error");
		if (!API_SUCCESS(rc))
			return false;

		float params = PIXEL_FORMAT_BAYER8_GRBG;
		rc = PxLSetFeature(_camera, FEATURE_PIXEL_FORMAT, FEATURE_FLAG_MANUAL, 1, &params);
		return API_SUCCESS(rc);
	}


	// Uninitizlize the camera
	bool uninitialize()
	{
		/*
		 * If the initialization was successful, we need to call PxLUninitialize
		 * to tell the camera we're done with it.
		 * Note the use of the API_SUCCESS macro to check for success. 
		 * This is better than comparing to ApiSuccess.
		 */
		PXL_RETURN_CODE rc = PxLUninitialize(_camera);
		//printf("PxLUninitialize returned 0x%8.8X (%s)\n",  rc,  (API_SUCCESS(rc)) ? "Success" : "Error");
		return API_SUCCESS(rc);
	}


	// Start camera stream
	bool start_stream()
	{
		PXL_RETURN_CODE rc = PxLSetStreamState(_camera, START_STREAM);
		return API_SUCCESS(rc);
	}


	// Stop camera stream
	bool stop_stream()
	{
		PXL_RETURN_CODE rc = PxLSetStreamState(_camera, STOP_STREAM);
		return API_SUCCESS(rc);
	}


	// Get next image frame from camera
	bool get_next_frame(zor::ColorImageRGB24u &image)
	{
		frame_descr.uSize = sizeof(frame_descr);
		PXL_RETURN_CODE rc = PxLGetNextFrame(_camera, (U32)frame.size(), &frame[0], &frame_descr);
		int w = static_cast<int>(frame_descr.Roi.fWidth);
		int h = static_cast<int>(frame_descr.Roi.fHeight);
		if (!API_SUCCESS(rc) || w == 0 || h == 0) {
			image.resize(0, 0);
			return false;
		}

		//printf("GetNextFrame: frame count = %d, roi = %f x %f\n",
		//	frame_descr.uFrameNumber, frame_descr.Roi.fWidth, frame_descr.Roi.fHeight);
		//printf("pixel format: %f\n", frame_descr.PixelFormat.fValue);
		//printf("frame rate: %f\n", frame_descr.FrameRate.fValue);
				
		image.resize(w, h);
		U32 size = image.size_in_bytes();
		rc = PxLFormatImage(&frame[0], &frame_descr, IMAGE_FORMAT_RAW_RGB24, image.data(), &size);
		if (!API_SUCCESS(rc)) {
			image.resize(0, 0);
			return false;
		}

		// image is in "bitmap" format, so flip vertically and swap BGR to RGB
		zor::flip_image_vertical(image, image);
		for (int j = 0; j < image.num_pixels(); ++j)
			zor::swap(image.pixel(j).r(), image.pixel(j).b());
		return true;
	}


	/* Handle with which we interact with the camera */
	HANDLE _camera;

	// Declare a buffer that's large enough for any camera PixeLINK currently supports.
	std::vector<U8> frame;
	FRAME_DESC frame_descr;


};

