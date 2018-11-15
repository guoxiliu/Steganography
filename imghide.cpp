/*
   Final Project - Image Hiding 
 

   CPSC 6040		Guoxi Liu		11/15/2018

*/

#include <GL/gl.h>
#include <GL/glut.h>
#include <iostream>
#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING

using namespace std;

#define WIN_WIDTH 600			// width of initial black window
#define WIN_HEIGHT 600			// height of initial black window
#define CHANNELS 4				// RGBA format

// Define a structure to handle image data.
struct image {
	int xres, yres, channels;	// xres: width, yres: height
	unsigned char* pixels;		// image pixels

	// Constructors
	image() {}
	image(int width, int height) {
		init(width, height, 4);
	}
	image(int width, int height, int depth) {
		init(width, height, depth);
	}

private:
	// Private initialization function for constructors.
	void init(int width, int height, int depth) {
		xres = width;
		yres = height;
		channels = depth;
		pixels = new unsigned char[xres*yres*depth];
	}
} img, hiddenImg;

/** 
 * Read the image file in defined image structure, including width, height, 
 * channels, pixel values. If the number of channels is less than 4, we 
 * will try to covert it to RGBA format.
 */
void read_image(char* filename, image &img) {
	ImageInput *in = ImageInput::open(filename);
	if (!in) {
		cerr << "Could not create: " << geterror();
		exit(-1);
	}

	const ImageSpec &spec = in->spec();
	
	// Read image data into defined structure.
	img.xres = spec.width;
	img.yres = spec.height;
	img.channels = spec.nchannels;

	// Create memory space to store pixels.
	img.pixels = new unsigned char[img.xres*img.yres*CHANNELS];
	unsigned char* tmp = new unsigned char[img.xres*img.yres*img.channels];

	// Read image data into pixels.
	in->read_image(TypeDesc::UINT8, &tmp[0]);

	// Flip the image vertically so we can view it.
	for (int row = 0; row < img.yres; row++)
		for (int col = 0; col < img.xres; col++) 
			for (int ch = 0; ch < img.channels; ch++) 
				img.pixels[(row*img.xres+col)*CHANNELS+ch] = tmp[((img.yres-1-row)*img.xres+col)*img.channels+ch];

	// Try to convert the image file to RGBA format.
	if (img.channels == 1) 
		for (int row = 0; row < img.yres; row++) 
			for (int col = 0; col < img.xres; col++) 
				for (int ch = 1; ch < CHANNELS; ch++) 
					// Copy the pxiel value of the first channel to other 3 channels.
					img.pixels[(row*img.xres+col)*CHANNELS+ch] = img.pixels[(row*img.xres+col)*CHANNELS];

	else if (img.channels == 3)
		for (int row = 0; row < img.yres; row++) 
			for (int col = 0; col < img.xres; col++) 
				// Simply set the last channel (alpha value) to 255.
				img.pixels[(row*img.xres+col)*CHANNELS+CHANNELS-1] = 255;

	// Print some useful information of the image file.
	cout << "Read image file " << filename << "successfully!" << endl;
	cout << "Image width: " << img.xres << ", height: " << img.yres << ", channels: " << img.channels << endl;

	// Close the image file and release the memory.
	in->close();
	ImageInput::destroy(in);
}

/**
 * Write specific image struct to an image file with the given file name.
 */
void write_image(char* filename, const image &img) {
	// Create the oiio file handler for image.
	ImageOutput *out = ImageOutput::create(filename);
	if (!out) {
		cerr << "Could not create: " << geterror();
		exit(-1);
	}
	int xres = img.xres; 
	int yres = img.yres;
	unsigned char* tmp = img.pixels;
	unsigned char* pixmap = new unsigned char[xres*yres*CHANNELS];

	// Need to flip the pixels.
	for (int row = 0; row < yres; row++)
		for (int col = 0; col < xres; col++) 
			for (int ch = 0; ch < CHANNELS; ch++) 
				pixmap[(row*xres+col)*CHANNELS+ch] = tmp[((yres-row)*xres+col)*CHANNELS+ch];

	// Open a file for writing the image.
	ImageSpec spec(xres, yres, CHANNELS, TypeDesc::UINT8);
	if (!out->open(filename, spec)) {
		cerr << "Could not open " << filename << ", error = " << geterror() << endl;
		ImageOutput::destroy(out);
		return;
	};

	// Write the image to the file.
	if (!out->write_image(TypeDesc::UINT8, pixmap)) {
		cerr << "Could not write image to " << filename << ", error = " << geterror() << endl;
		ImageOutput::destroy(out);
		return;
	}

	// Close the image file after finish writing.
	if (!out->close()) {
		cerr << "Could not close " << filename << ", error = " << geterror() << endl;
		ImageOutput::destroy(out);
		return;
	}
	
	// Release the memory to avoid memory leaks.
	ImageOutput::destroy(out);
}

/**
 * Display callback function: clear the screen and display the image if showImage 
 * is true, or show a normal black window otherwise.
 */
void display_func() {
	glClearColor(0, 0, 0, 0);			// the background color
	glClear(GL_COLOR_BUFFER_BIT);		// clear window to background color
	glRasterPos2i(0, 0);			// set the position for drawing
	glDrawPixels(img.xres, img.yres, GL_RGBA, GL_UNSIGNED_BYTE, img.pixels);
	glFlush();
}

/**
 * Keyboard callback function: 'r' read given image file, 'q' or ESC quit, 
 * 'w' write the framebuffer to an image file.
 */ 
void keyboard_func(unsigned char key, int x, int y) {
	switch(key) {
		// 'H': 
		case 'h':
		case 'H':

			break;
		// 'E': 
		case 'e':
		case 'E':
			break;

		// 'W': write image to given file name.
		case 'w':
		case 'W':
			break;
			
		// 'Q': quit the program.
		case 'q':
		case 'Q':
		case 27:
			exit(0);
		
		// Other keys: just ignore it.
		default:
			return;
	}
}

/**
 * Reshape callback function: the image reamins centered if the window size is bigger
 * than it, and is scaled down to fit the window if the window size is smaller than it.
 */ 
void reshape_func(int width, int height) {
	int w = img.xres, h = img.yres;
	double ratio = 1.0;

	if (width < img.xres) {
		ratio = (double)width/img.xres;
	}
	if (height < img.yres) {
		double tmp = (double)height/img.yres;
		ratio = tmp<ratio ? tmp : ratio;
	}
	h *= ratio, w *= ratio;

	// Set the viewport to the entire window.
	glViewport((width-w)/2, (height-h)/2, width, height);
	// Scale down the image so it can fit the window.
	glPixelZoom(ratio, ratio);
	
	// Define the drawing coordinate system on the viewport.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, width, 0, height);
}

/**
 * Main entry for the program.
 */ 
int main(int argc, char* argv[])
{
	// Start up the glut utilities.
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);

	// Determine whether user passes an argument to the program.
	if (argc > 1) {
		read_image(argv[1], img);
		glutInitWindowSize(img.xres, img.yres);
		glutCreateWindow(argv[1]);
	} else {
		cerr << "Usage: " << argv[0] << " in.img" << endl;
		return -1;
	}

	// Set up callback functions here.
	glutDisplayFunc(display_func);
	glutKeyboardFunc(keyboard_func);
	glutReshapeFunc(reshape_func);
	
	// The function loops forever looking for events.
	glutMainLoop();
	return 0;
}
