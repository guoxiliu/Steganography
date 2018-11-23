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
	image(const image &img) {	// copy constructor
		init(img.xres, img.yres, img.channels);
		copy(img.pixels, img.pixels+xres*yres*channels, pixels);
	}

private:
	// Private initialization function for constructors.
	void init(int width, int height, int depth) {
		xres = width;
		yres = height;
		channels = depth;
		pixels = new unsigned char[xres*yres*depth];
	}
} oldImg, hiddenImg, newImg;

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
	img.channels = 4;

	// Create memory space to store pixels.
	img.pixels = new unsigned char[img.xres*img.yres*4];
	unsigned char* tmp = new unsigned char[img.xres*img.yres*spec.nchannels];

	// Read image data into pixels.
	in->read_image(TypeDesc::UINT8, &tmp[0]);

	// Flip the image vertically so we can view it.
	for (int row = 0; row < img.yres; row++)
		for (int col = 0; col < img.xres; col++) 
			for (int ch = 0; ch < spec.nchannels; ch++) 
				img.pixels[(row*img.xres+col)*4+ch] = tmp[((img.yres-1-row)*img.xres+col)*spec.nchannels+ch];

	// Try to convert the image file to RGBA format.
	if (spec.nchannels == 1) 
		for (int row = 0; row < img.yres; row++) 
			for (int col = 0; col < img.xres; col++) 
				for (int ch = 1; ch < 4; ch++) 
					// Copy the pxiel value of the first channel to other 3 channels.
					img.pixels[(row*img.xres+col)*4+ch] = img.pixels[(row*img.xres+col)*4];

	else if (spec.nchannels == 3)
		for (int row = 0; row < img.yres; row++) 
			for (int col = 0; col < img.xres; col++) 
				// Simply set the last channel (alpha value) to 255.
				img.pixels[(row*img.xres+col)*4+4-1] = 255;

	// Print some useful information of the image file.
	cout << "Read image file " << filename << " successfully!" << endl;
	cout << "Image width: " << img.xres << ", height: " << img.yres << ", channels: " << spec.nchannels << endl << endl;

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
	unsigned char* pixmap = new unsigned char[xres*yres*4];

	// Need to flip the pixels.
	for (int row = 0; row < yres; row++)
		for (int col = 0; col < xres; col++) 
			for (int ch = 0; ch < 4; ch++) 
				pixmap[(row*xres+col)*4+ch] = tmp[((yres-1-row)*xres+col)*4+ch];

	// Open a file for writing the image.
	ImageSpec spec(xres, yres, 4, TypeDesc::UINT8);
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
	
	cout << "Write image " << filename << " successfully!" << endl << endl;

	// Release the memory to avoid memory leaks.
	ImageOutput::destroy(out);
}

/**
 * Hide one pixel into another one using given number of bits.
 */
unsigned char hide_pixel(unsigned char pixel1, unsigned char pixel2, int n) {
	unsigned char binVal1[8], binVal2[8], retVal = 0;
	int i = 0;

	// Initialize each element of the array to be 0 at first.
	for (i = 0; i < 8; i++) {
		binVal1[i] = 0;
		binVal2[i] = 0;
	}
	
	// Calculate the binary representation of the two pixels.
	for (i = 7; pixel1 != 0; i--) {
		binVal1[i] = pixel1 % 2;
		pixel1 /= 2;
	}
	for (i = 7; pixel2 != 0; i--) {
		binVal2[i] = pixel2 % 2;
		pixel2 /= 2;
	}

	// Merge two binary numbers. 
	for (i = 0; i < 8-n; i++) {
		retVal = retVal * 2 + binVal1[i];
	}
	for (i = 0; i < n; i++) {
		retVal = retVal * 2 + binVal2[i];
	}

	return retVal;
}

/**
 * Extract the hidden pixel from the given pixel.
 */
unsigned char extract_pixel(unsigned char pixel, int n) {
	unsigned char binVal1[8], binVal2[8], retVal = 0;
	int i = 0;

	// Initialize each element of the array to be 0 at first.
	for (i = 0; i < 8; i++) {
		binVal1[i] = 0;
		binVal2[i] = 0;
	}
	
	// Calculate the binary representation of the two pixels.
	for (i = 7; pixel != 0; i--) {
		binVal1[i] = pixel % 2;
		pixel /= 2;
	}

	// Extract the last n-bit number from the pixel.
	for (i = 0; i < n; i++) 
		binVal2[i] = binVal1[8-n+i];

	for (i = 0; i < 8; i++)
		retVal = retVal * 2 + binVal2[i];

	return retVal;
}

/**
 * Hide one image into a cover image.
 */
void hide_image(image &img1, image &img2) {
	if (img2.xres > img1.xres || img2.yres > img1.yres) {
		cerr << "The dimension of cover image should be larger than the image to hide!" << endl;
		exit(1);
	}
	
	int num;
	cout << ">> Please type in the number of bits you want to use: ";
	cin >> num;

	for (int row = 0; row < img2.yres; row++) {
		for (int col = 0; col < img2.xres; col++) {
			for (int ch = 0; ch < 4; ch++) {
				int position1 = (row*img1.xres+col) * 4 + ch;
				int position2 = (row*img2.xres+col) * 4 + ch;
				img1.pixels[position1] = hide_pixel(img1.pixels[position1], img2.pixels[position2], num);
			}
		}
	}

	// Save the width and height of the hidden image.
	img1.pixels[0] = (unsigned char)((float)img2.xres/img1.xres * 255.0);
	img1.pixels[1] = (unsigned char)((float)img2.yres/img1.yres * 255.0);

	cout << "Finished hiding the image!" << endl << endl;
}

/**
 * Extract the hidden image from the merged image.
 */
void extract_image(image &img1, image &img2) {
	img2.xres = (int) (img1.pixels[0]/255.0 * img1.xres); 
	img2.yres = (int) (img1.pixels[1]/255.0 * img1.yres);
	img2.pixels = new unsigned char[img2.xres*img2.yres*4];

	int num;
	cout << ">> Please type in the number of bits used: ";
	cin >> num;

	for (int row = 0; row < img2.yres; row++) {
		for (int col = 0; col < img2.xres; col++) {
			for (int ch = 0; ch < 4; ch++) {
				int position1 = (row*img1.xres+col) * 4 + ch;
				int position2 = (row*img2.xres+col) * 4 + ch;
				img2.pixels[position2] = extract_pixel(img1.pixels[position1], num);
			}
		}
	}

	cout << "Finished extracting the hidden image!" << endl << endl;
} 

/**
 * Display callback function: clear the screen and display the image if showImage 
 * is true, or show a normal black window otherwise.
 */
void display_func() {
	glClearColor(0, 0, 0, 0);			// the background color
	glClear(GL_COLOR_BUFFER_BIT);		// clear window to background color
	glRasterPos2i(0, 0);			// set the position for drawing
	glDrawPixels(newImg.xres, newImg.yres, GL_RGBA, GL_UNSIGNED_BYTE, newImg.pixels);
	glFlush();
}

/**
 * Keyboard callback function: 'r' read given image file, 'q' or ESC quit, 
 * 'w' write the framebuffer to an image file.
 */ 
void keyboard_func(unsigned char key, int x, int y) {
	switch(key) {
		// 'H': hide a new image into current displayed image.
		case 'h':
		case 'H':
		{
			char filename[50];
			cout << ">> Please type in the image filename you want to hide: ";
			cin >> filename;
			read_image(filename, hiddenImg);
			hide_image(newImg, hiddenImg);
			glutPostRedisplay();
			break;
		}

		// 'E': extract the hidden image from the current displayed image.
		case 'e':
		case 'E':
			extract_image(newImg, hiddenImg);
			newImg = hiddenImg;
			glutReshapeWindow(newImg.xres, newImg.yres);
			glutPostRedisplay();
			break;

		// 'R': revoke the operation and display the original image.
		case 'r':
		case 'R':
			newImg = image(oldImg);
			glutPostRedisplay();
			break;

		// 'W': write image to given file name.
		case 'w':
		case 'W':
		{
			char filename[50];
			cout << ">> Please type in the filename to save the current image: ";
			cin >> filename;
			write_image(filename, newImg);
			break;
		}
			
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
	int w = newImg.xres, h = newImg.yres;
	double ratio = 1.0;

	if (width < newImg.xres) {
		ratio = (double)width/newImg.xres;
	}
	if (height < newImg.yres) {
		double tmp = (double)height/newImg.yres;
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

	// Determine whether the user passes an argument to the program.
	if (argc > 1) {
		read_image(argv[1], oldImg);
		newImg = image(oldImg);
		glutInitWindowSize(newImg.xres, newImg.yres);
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
