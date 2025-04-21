
#include <iostream>
#include <vector>

#include "../../../ZorImageLib/zor.hpp"

#include "PixelinkCamera.hpp"
#include "jpeg.h"




#include "../include/glew.h"
#pragma comment(lib, "glew32.lib")

#include "../include/glut.h"
#pragma comment(lib, "glut32.lib")

#include <GL/gl.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")




int pause = 1450; //3500;
//char *directory = "D:/camera/";
std::string directory_base = "D:/camera";
#define USE_GUI




int ncam;
std::vector<PixelinkCamera> cameras;
std::vector<zor::ColorImageRGB24u> camera_images;
std::vector<GLuint> texture_ids;
zor::Timer timer;
bool first;
bool do_capture;
int height;
int win_width, win_height;








void display(void)
{
	glLoadIdentity();
	if (do_capture)
		glClearColor(0.f, 1.f - (timer.seconds()*1000/pause), 0.f, 0.f);
	else
		glClearColor(1.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glColor3d(1.0, 1.0, 1.0);
	glEnable(GL_TEXTURE_2D);

	for (int i = 0; i < ncam; ++i) {
		glBindTexture(GL_TEXTURE_2D, texture_ids[i]);
		glBegin(GL_QUADS);
			glTexCoord2d(0, 0);		glVertex2d(0+i, 1);	// top left
			glTexCoord2d(0, 1);		glVertex2d(0+i, 0);	// bottom left
			glTexCoord2d(1, 1);		glVertex2d(1+i, 0);	// bottom right
			glTexCoord2d(1, 0);		glVertex2d(1+i, 1);	// top right
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);

	glColor3d(0, 0, 0);

	glLineWidth(6.f);
	glBegin(GL_LINES);
		glVertex2d(0, 0);
		glVertex2d(0, 1);
		glVertex2d(ncam, 0);
		glVertex2d(ncam, 1);
	glEnd();
	
	glLineWidth(3.f);
	glBegin(GL_LINES);
		for (int i = 1; i < ncam; ++i) {
			glVertex2d(i, 0);
			glVertex2d(i, 1);
		}
		glVertex2d(0, 0);
		glVertex2d(ncam, 0);
		glVertex2d(0, 1);
		glVertex2d(ncam, 1);
	glEnd();
		
	glFlush();
	glutSwapBuffers();
}




void reshape(int width, int height)
{
	win_width = width;
	win_height = height;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, ncam, -0.166, 1.166);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glutPostRedisplay();
}




void keyboard(unsigned char key, int x, int y)
{
	if (key == 32)
	{
		if (do_capture)
			do_capture = false;
		else {
			do_capture = true;
			first = true;
		}
	}
	else if (key == 'r')
	{
		//glutReshapeWindow(ncam*height, height);
		glutReshapeWindow(win_width, win_width/ncam);
	}
}




void special(int key, int x, int y)
{
	if (ncam > 1)
	{
		if (key == GLUT_KEY_LEFT) {
			PixelinkCamera temp = cameras[0];
			cameras[0] = cameras[1];
			cameras[1] = temp;
		}
		else if (key == GLUT_KEY_RIGHT) {
			PixelinkCamera temp = cameras[ncam-1];
			cameras[ncam-1] = cameras[ncam-2];
			cameras[ncam-2] = temp;
		}
	}
	glutPostRedisplay();
}




void idle(void)
{
	SYSTEMTIME time;
	GetLocalTime(&time);
	//printf("Time: %04d-%02d-%02d %02d:%02d:%02d\n", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

	bool write = false;
	if (do_capture && (first || timer.seconds() * 1000 > pause)) {
		printf("\n");
		write = true;
		first = false;
		timer.start();
	}

	std::vector<bool> camera_ok(ncam);

	std::string directory;// = directory_base;
	if (write) {
		directory = directory_base;
		char date[16];
		sprintf_s(date, " %04d-%02d-%02d/", time.wYear, time.wMonth, time.wDay);
		directory.append(date);
		if (CreateDirectoryA(directory.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
			// good to go
		}
		else {
			printf("Output directory does not exist!\n");
		}
	}

	#pragma omp parallel for
	for (int i = 0; i < ncam; ++i)
	{
		camera_ok[i] = false;
		if (cameras[i].get_next_frame(camera_images[i]))
		{
			camera_ok[i] = true;
			if (write) {
				char filename[64];
				sprintf_s(filename, "%s%04d%02d%02d-%02d%02d%02d-%d.jpg", directory.c_str(), time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, i);
				bool result = write_jpeg_file(filename, camera_images[i], 80);
				if (result)
					printf("Image captured: %s\n", filename);
				else
					printf("Error writing image!\n");
			}
		}
		else
			cameras[i].print_last_error();
	}

	glEnable(GL_TEXTURE_2D);
	for (int i = 0; i < ncam; ++i) {
		if (!camera_ok[i])
			continue;
		glBindTexture(GL_TEXTURE_2D, texture_ids[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, camera_images[i].width(), camera_images[i].height(), 0, GL_RGB, GL_UNSIGNED_BYTE, camera_images[i].data());
	}
	glDisable(GL_TEXTURE_2D);
	
	glutPostRedisplay();
}




void cleanup(void)
{
	/* Stop the camera streams */
	for (int i = 0; i < ncam; ++i)
		if (!cameras[i].stop_stream())
			cameras[i].print_last_error();

	/* Uninitialize the cameras */
	for (int i = 0; i < ncam; ++i)
		if (!cameras[i].uninitialize())
			cameras[i].print_last_error();
}




int main(int argc, char* argv[])
{
	timer.start();
	first = true;
	do_capture = false;

	//if (CreateDirectoryA(directory, NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
	//	// good to go
	//}
	//else {
	//	printf("Output directory does not exist\n");
	//	return -1;
	//}

	std::vector<CAMERA_ID_INFO> cam_info;
	ncam = PixelinkCamera::get_cameras_info(cam_info);
	printf("Number of cameras: %d\n", ncam);

	cameras.resize(ncam);
	camera_images.resize(ncam);
	texture_ids.resize(ncam);

	/* Print the serial number of each camera */
	for (int i = 0; i < ncam; i++)
		printf("Camera %d | Serial Number: %d\n", i, cam_info[i].CameraSerialNum);

	/* Create and initialize the cameras */
	bool good = true;
	for (int i = 0; i < ncam; ++i) {
		if (!cameras[i].initialize(cam_info[i].CameraSerialNum)) {
			good = false;
			cameras[i].print_last_error();
		}
	}

	/* Start the camera streams */
	for (int i = 0; i < ncam; ++i) {
		if (!cameras[i].start_stream()) {
			good = false;
			cameras[i].print_last_error();
			break;
		}
		int trials = 0;
		for (trials = 0; trials < 3; ++trials) {
			if (cameras[i].get_next_frame(camera_images[i]))
				break;
			cameras[i].print_last_error();
			cameras[i].stop_stream();
			cameras[i].start_stream();
		}
		if (trials == 3)
			good = false;
	}

	atexit(cleanup);




#ifdef USE_GUI


	if (good)
	{
		glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
		int screen_width = glutGet(GLUT_SCREEN_WIDTH);
		int screen_height = glutGet(GLUT_SCREEN_HEIGHT);
		//printf("Screen: %d x %d\n", screen_width, screen_height);
		height = screen_width / (ncam+1);
		glutInitWindowSize(ncam*height, height);
		glutCreateWindow("Camera Capture");

		for (int i = 0; i < ncam; ++i)
			glGenTextures(1, &(texture_ids[i]));

		glutDisplayFunc(display);
		glutReshapeFunc(reshape);
		glutIdleFunc(idle);
		glutKeyboardFunc(keyboard);
		glutSpecialFunc(special);
		//glutMouseFunc(mouse);
		//glutMotionFunc(motion);

		glutMainLoop();
	}


#else
	

	/* do the capture */
	if (good)
	{
		std::vector<zor::ColorImageRGB24u> camera_images(ncam);
		SYSTEMTIME time;

		while (true)
		{
			GetLocalTime(&time);
			printf("\nThe local time is: %04d-%02d-%02d %02d:%02d:%02d\n", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

			#pragma omp parallel for
			for (int i = 0; i < ncam; ++i)
			{
				if (cameras[i].get_next_frame(camera_images[i])) {
					char filename[64];
					sprintf_s(filename, "%s%04d%02d%02d-%02d%02d%02d-%d.jpeg", directory, time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, i);
					write_jpeg_file(filename, camera_images[i], 80);
					printf("Image captured %d x %d: %s\n", camera_images[i].width(), camera_images[i].height(), filename);
				}
				else
					cameras[i].print_last_error();
			}

			printf("Waiting %d ms ...\n", pause);
			Sleep(pause);
		}
	}


#endif
	



	/* Stop the camera streams */
	for (int i = 0; i < ncam; ++i)
		if (!cameras[i].stop_stream())
			cameras[i].print_last_error();

	/* Uninitialize the cameras */
	for (int i = 0; i < ncam; ++i)
		if (!cameras[i].uninitialize())
			cameras[i].print_last_error();

	return 0;
}
