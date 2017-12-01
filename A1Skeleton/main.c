/*******************************************************************
           Multi-Part Model Construction and Manipulation
********************************************************************/


#pragma warning (disable : 4996)

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include "Vector3D.h"
#include "CubeMesh.h"
#include "QuadMesh.h"
#include "Matrix3D.h"

const float PI = 3.1415926535897932384626433832795;
const int meshSize = 16;    // Default Mesh Size
const int vWidth = 650;     // Viewport width in pixels
const int vHeight = 500;    // Viewport height in pixels

static int shoulder = 0, elbow = 0; body = 0; cangle = 0; 
static double cameraZoom = 90; w = 500; h = 500;
static int currentButton;
static unsigned char currentKey;

static float x_pos = 0;
static float y_pos = 0;
static float z_pos = 0;
static int turn_angle = 0;

int oldX, oldY;
bool rotate = false;
float theta = 90, phi = 90;
float theta_cam = 90, phi_cam = 90;
float theta_wheels = 0;

// Lighting/shading and material properties for submarine - upcoming lecture - just copy for now

// Light properties
static GLfloat light_position0[] = { -6.0F, 12.0F, 0.0F, 1.0F };
static GLfloat light_position1[] = { 6.0F, 12.0F, 0.0F, 1.0F };
static GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat light_ambient[] = { 0.2F, 0.2F, 0.2F, 1.0F };

// Material properties
static GLfloat submarine_mat_ambient[] = { 0.4F, 0.2F, 0.0F, 1.0F };
static GLfloat submarine_mat_specular[] = { 0.1F, 0.1F, 0.0F, 1.0F };
static GLfloat submarine_mat_diffuse[] = { 0.9F, 0.5F, 0.0F, 1.0F };
static GLfloat submarine_mat_shininess[] = { 0.0F };

// A quad mesh representing the ground / sea floor 
static QuadMesh groundMesh;

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
typedef unsigned short ushort;
typedef unsigned long  ulong;

typedef struct RGB
{
	byte r, g, b;
} RGB;

typedef struct RGBpixmap
{
	int nRows, nCols;
	RGB *pixel;
} RGBpixmap;





RGBpixmap pix[10];

// Structure defining a bounding box, currently unused
//struct BoundingBox {
//    Vector3D min;
//    Vector3D max;
//} BBox;

// Prototypes for functions in this module
void initOpenGL(int w, int h);
void display(void);
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void mouseMotionHandler(int xMouse, int yMouse);
void keyboard(unsigned char key, int x, int y);
void functionKeys(int key, int x, int y);
void spinDisplay(int n);
void drawCircle(GLfloat x, GLfloat y, GLfloat radius);


//My functions
void createMap();

Vector3D ScreenToWorld(int x, int y);

void makeCheckerboard(RGBpixmap *p)
{
	long count = 0;
	int i, j, c;

	p->nRows = p->nCols = 64;
	p->pixel = (RGB *)malloc(3 * p->nRows * p->nCols);

	for (i = 0; i < p->nRows; i++)
		for (j = 0; j < p->nCols; j++)
		{
			c = (((i / 8) + (j / 8)) % 2) * 255;
			p->pixel[count].r = c;
			p->pixel[count].g = c;
			p->pixel[count++].b = 0;
		}
}



/**************************************************************************
*  fskip                                                                 *
*     Skips bytes in a file.                                             *
**************************************************************************/

void fskip(FILE *fp, int num_bytes)
{
	int i;
	for (i = 0; i < num_bytes; i++)
		fgetc(fp);
}


/**************************************************************************
*                                                                        *
*    Loads a bitmap file into memory.                                    *
**************************************************************************/

ushort getShort(FILE *fp) //helper function
{ //BMP format uses little-endian integer types
  // get a 2-byte integer stored in little-endian form
	char ic;
	ushort ip;
	ic = fgetc(fp); ip = ic;  //first byte is little one 
	ic = fgetc(fp);  ip |= ((ushort)ic << 8); // or in high order byte
	return ip;
}
//<<<<<<<<<<<<<<<<<<<< getLong >>>>>>>>>>>>>>>>>>>
ulong getLong(FILE *fp) //helper function
{  //BMP format uses little-endian integer types
   // get a 4-byte integer stored in little-endian form
	ulong ip = 0;
	char ic = 0;
	unsigned char uc = ic;
	ic = fgetc(fp); uc = ic; ip = uc;
	ic = fgetc(fp); uc = ic; ip |= ((ulong)uc << 8);
	ic = fgetc(fp); uc = ic; ip |= ((ulong)uc << 16);
	ic = fgetc(fp); uc = ic; ip |= ((ulong)uc << 24);
	return ip;
}


void readBMPFile(RGBpixmap *pm, char *file)
{
	FILE *fp;
	int numPadBytes, nBytesInRow;
	ulong fileSize;
	ushort reserved1;    // always 0
	ushort reserved2;     // always 0 
	ulong offBits; // offset to image - unreliable
	ulong headerSize;     // always 40
	ulong numCols; // number of columns in image
	ulong numRows; // number of rows in image
	ushort planes;      // always 1 
	ushort bitsPerPixel;    //8 or 24; allow 24 here
	ulong compression;      // must be 0 for uncompressed 
	ulong imageSize;       // total bytes in image 
	ulong xPels;    // always 0 
	ulong yPels;    // always 0 
	ulong numLUTentries;    // 256 for 8 bit, otherwise 0 
	ulong impColors;       // always 0 
	long count;
	char dum;

	/* open the file */
	if ((fp = fopen(file, "rb")) == NULL)
	{
		printf("Error opening file %s.\n", file);
		exit(1);
	}

	/* check to see if it is a valid bitmap file */
	if (fgetc(fp) != 'B' || fgetc(fp) != 'M')
	{
		fclose(fp);
		printf("%s is not a bitmap file.\n", file);
		exit(1);
	}

	fileSize = getLong(fp);
	reserved1 = getShort(fp);    // always 0
	reserved2 = getShort(fp);     // always 0 
	offBits = getLong(fp); // offset to image - unreliable
	headerSize = getLong(fp);     // always 40
	numCols = getLong(fp); // number of columns in image
	numRows = getLong(fp); // number of rows in image
	planes = getShort(fp);      // always 1 
	bitsPerPixel = getShort(fp);    //8 or 24; allow 24 here
	compression = getLong(fp);      // must be 0 for uncompressed 
	imageSize = getLong(fp);       // total bytes in image 
	xPels = getLong(fp);    // always 0 
	yPels = getLong(fp);    // always 0 
	numLUTentries = getLong(fp);    // 256 for 8 bit, otherwise 0 
	impColors = getLong(fp);       // always 0 

	if (bitsPerPixel != 24)
	{ // error - must be a 24 bit uncompressed image
		printf("Error bitsperpixel not 24\n");
		exit(1);
	}
	//add bytes at end of each row so total # is a multiple of 4
	// round up 3*numCols to next mult. of 4
	nBytesInRow = ((3 * numCols + 3) / 4) * 4;
	numPadBytes = nBytesInRow - 3 * numCols; // need this many
	pm->nRows = numRows; // set class's data members
	pm->nCols = numCols;
	pm->pixel = (RGB *)malloc(3 * numRows * numCols);//make space for array
	if (!pm->pixel) return; // out of memory!
	count = 0;
	dum;
	for (ulong row = 0; row < numRows; row++) // read pixel values
	{
		for (ulong col = 0; col < numCols; col++)
		{
			int r, g, b;
			b = fgetc(fp); g = fgetc(fp); r = fgetc(fp); //read bytes
			pm->pixel[count].r = r; //place them in colors
			pm->pixel[count].g = g;
			pm->pixel[count++].b = b;
		}
		fskip(fp, numPadBytes);
	}
	fclose(fp);
}


void setTexture(RGBpixmap *p, GLuint textureID)
{
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, p->nCols, p->nRows, 0, GL_RGB,
		GL_UNSIGNED_BYTE, p->pixel);
}

void myInit(void)
{

}




int main(int argc, char **argv)
{
    
	printf("Press 'F1' for controls!\n");
	// Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(vWidth, vHeight);
    glutInitWindowPosition(200, 30);
    glutCreateWindow("Assignment 3");

    // Initialize GL
    initOpenGL(vWidth, vHeight);

    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotionHandler);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(functionKeys);
	
	// My Functions
	createMap();

    // Start event loop, never returns
    glutMainLoop();
	myInit();

    return 0;
}

 

//Create Map
void createMap() {

	// width of whole, height of hole, x_pos, z_pos
	double map_features[6][4] = { { 0.03, 5, 13, -13 },
	{ 0.08, 9, -13, -13 },
	{ 0.04, 8, 13, 12 },
	{ 0.08, 3, 14, 1 },
	{ 0.1, 9, 12, -5 },
	{ 0.01, -4, 0, 0 },

	};

	for (int j = 0; j < sizeof(map_features) / sizeof(map_features[0]); j++) {

		//width
		double a = map_features[j][0];
		//height
		double b = map_features[j][1];
		//xpos
		double x_pos = map_features[j][2];
		//zpos
		double z_pos = map_features[j][3];

		for (int i = 0; i < (meshSize + 1)*(meshSize + 1); i++) {
			double x = groundMesh.vertices[i].position.x;
			double y = groundMesh.vertices[i].position.y;
			double z = groundMesh.vertices[i].position.z;

			double r = sqrt(pow((x - x_pos), 2) + pow((z - z_pos), 2));
			groundMesh.vertices[i].position.y += b*exp(-1 * a*pow(r, 2));
		}
	}
	ComputeNormalsQM(&groundMesh);
}



// Set up OpenGL. For viewport and projection setup see reshape(). */
void initOpenGL(int w, int h)
{
	
    // Set up and enable lighting
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
	
    glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
    glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    //glEnable(GL_LIGHT1);   // This light is currently off

    // Other OpenGL setup
    glEnable(GL_DEPTH_TEST);   // Remove hidded surfaces
    glShadeModel(GL_SMOOTH);   // Use smooth shading, makes boundaries between polygons harder to see 
    glClearColor(0.6F, 0.6F, 0.6F, 0.0F);  // Color and depth for glClear
    glClearDepth(1.0f);
    glEnable(GL_NORMALIZE);    // Renormalize normal vectors 
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);   // Nicer perspective

    // Set up ground/sea floor quad mesh
    Vector3D origin = NewVector3D(-16.0f, 0.0f, 16.0f);
    Vector3D dir1v = NewVector3D(1.0f, 0.0f, 0.0f);
    Vector3D dir2v = NewVector3D(0.0f, 0.0f, -1.0f);
    groundMesh = NewQuadMesh(meshSize);
    InitMeshQM(&groundMesh, meshSize, origin, 32.0, 32.0, dir1v, dir2v);


    Vector3D ambient = NewVector3D(0.0f, 0.05f, 0.0f);
    Vector3D diffuse = NewVector3D(0.4f, 0.8f, 0.4f);
    Vector3D specular = NewVector3D(0.04f, 0.04f, 0.04f);
    SetMaterialQM(&groundMesh, ambient, diffuse, specular, 1);

    // Set up the bounding box of the scene
    // Currently unused. You could set up bounding boxes for your objects eventually.
    //Set(&BBox.min, -8.0f, 0.0, -8.0);
    //Set(&BBox.max, 8.0f, 6.0,  8.0);

	//my init

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);

	readBMPFile(&pix[0], "..\\Grass01.bmp");
	setTexture(&pix[0], 2001);

	readBMPFile(&pix[1], "..\\apf_cemplankwall.bmp");
	setTexture(&pix[1], 2002);

	readBMPFile(&pix[2], "..\\camo.bmp");
	setTexture(&pix[2], 2003);

	readBMPFile(&pix[3], "..\\blue_camo.bmp");
	setTexture(&pix[3], 2004);

	readBMPFile(&pix[4], "..\\red_camo.bmp");
	setTexture(&pix[4], 2005);

	readBMPFile(&pix[5], "..\\mytire.bmp");
	setTexture(&pix[5], 2006);


	// Set up texture mapping assuming no lighting/shading 
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}


// Callback, called whenever GLUT determines that the window should be redisplayed
// or glutPostRedisplay() has been called.
void display(void)
{


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(cameraZoom, (GLdouble)w / h, 0.2, 40.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Set up the camera at position (0, 6, 22) looking at the origin, up along positive y axis
    //gluLookAt(0.0, 6.0, 22.0,  0.0, 0.0, 0.0,  0.0, 1.0, 0.0);

	GLdouble eyeX = 15*cos(phi_cam)*sin(theta_cam);
	GLdouble eyeY = 15*sin(phi_cam)*sin(theta_cam);
	GLdouble eyeZ = 15*cos(theta_cam);

	if(eyeY < 0){
		eyeY = 0;
	}


	gluLookAt(eyeX, eyeY, eyeZ, 0.0, 0.0, 0.0, 0, 1, 0);



    // Draw Submarine
	/*
    // Set submarine material properties
    glMaterialfv(GL_FRONT, GL_AMBIENT, submarine_mat_ambient);
    glMaterialfv(GL_FRONT, GL_SPECULAR, submarine_mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, submarine_mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SHININESS, submarine_mat_shininess);
	*/
    // Apply transformations to move submarine
    // ...

    // Apply transformations to construct submarine, modify this!
	
	/*
	glPushMatrix();
	glScalef(1,1,1);
	CubeMesh cube = newCube();
	drawCube(&cube);
	glPopMatrix();
	*/

	glBindTexture(GL_TEXTURE_2D, 2001); // top face of cube
										// Draw ground/sea floor
	DrawMeshQM(&groundMesh, meshSize);

	CubeMesh cube = newCube();

	
	glTranslatef(x_pos, y_pos, z_pos);
	glRotatef(turn_angle, 0.0, 1.0, 0.0);


	glBindTexture(GL_TEXTURE_2D, 2005); //texture

	glPushMatrix();
	glRotatef((GLfloat)body, 0.0, 1.0, 0);

	glPushMatrix();
	glRotatef((GLfloat)shoulder, 0.0, 0.0, 1.0);

	glBindTexture(GL_TEXTURE_2D, 2002); //texture
	//padle (pitch)
	glPushMatrix();
	glTranslatef(0.0, 3.0, 0.0);
	glRotatef((GLfloat)elbow, 0.0, 0.0, 1.0);
	glTranslatef(2.3, 0.0, 0.0);
	glScalef(1.2/2, 2/2, 1.03/2);
	drawCube(&cube);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 2005); //texture
	//past elbow (pitch)
    glPushMatrix();
	glTranslatef(1.5, 3.0, 0.0);
	glTranslatef(-1.5,0,0);
	glRotatef((GLfloat)elbow, 0.0, 0.0, 1.0);
	glTranslatef(1.5,0.0,0.0);
	glScalef(3.0/2, 0.5/2, 0.5/2);
	drawCube(&cube);
	glPopMatrix();

	//arm
    glPushMatrix();
	glRotatef(90, 0.0, 0.0, 1.0);
    glTranslatef(1.5, 0.0, 0.0);
    glScalef(3.0/2, 0.5/2, 0.5/2);
	drawCube(&cube);
    glPopMatrix();

	glPopMatrix();

	glPopMatrix();

	//Draw base
	glPushMatrix();
	glTranslatef(0.0, 0.0, 0.0);
	glScalef(0.5, 0.5, 0.5);
	drawCube(&cube);
	glPopMatrix();

	//Base Wide Part
	glPushMatrix();
	glTranslatef(0.0, -0.5, 0.0);
	glScalef(1.0, 0.3, 1.0);
	drawCube(&cube);
	glPopMatrix();

	//back-right tire
	glBindTexture(GL_TEXTURE_2D, 2006); //tire texture 

	glPushMatrix();
	GLUquadricObj *cyl;
	cyl = gluNewQuadric();
	glTranslatef(-0.8, -0.7, 1);
	glRotatef(theta_wheels, 0.0, 0.0, 1.0);
	gluQuadricTexture(cyl, GL_TRUE);
	gluCylinder(cyl, 0.5,0.5,0.5,32,32);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 2003); //black filling for tire

	glPushMatrix();
	glTranslatef(-0.81, -0.7, 1);
	drawCircle(0, 0, 0.48);
	glTranslatef(0.0, 0.0, 0.5);
	drawCircle(0, 0, 0.47);
	glPopMatrix();

	//front-right tire
	glBindTexture(GL_TEXTURE_2D, 2006); //tire texture 

	glPushMatrix();
	GLUquadricObj *cyl2;
	cyl2 = gluNewQuadric();
	glTranslatef(0.8, -0.7, 1);
	glRotatef(theta_wheels, 0.0, 0.0, 1.0);
	gluQuadricTexture(cyl2, GL_TRUE);
	gluCylinder(cyl2, 0.5,0.5,0.5,32,32);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 2003); //black filling for tire

	glPushMatrix();
	glTranslatef(0.81, -0.7, 1);
	drawCircle(0, 0, 0.48);
	glTranslatef(0.0, 0.0, 0.5);
	drawCircle(0, 0, 0.47);
	glPopMatrix();

	//front-left tire
	glBindTexture(GL_TEXTURE_2D, 2006); //tire texture 

	glPushMatrix();
	GLUquadricObj *cyl3;
	cyl3 = gluNewQuadric();
	glTranslatef(0.8, -0.7, -1.51);
	glRotatef(theta_wheels, 0.0, 0.0, 1.0);
	gluQuadricTexture(cyl3, GL_TRUE);
	gluCylinder(cyl3, 0.5,0.5,0.5,32,32);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 2003); //black filling for tire

	glPushMatrix();
	glTranslatef(0.81, -0.7, -1.51);
	drawCircle(0, 0, 0.48);
	glTranslatef(0.0, 0.0, 0.5);
	drawCircle(0, 0, 0.47);
	glPopMatrix();
	
	//back-left tire
	glBindTexture(GL_TEXTURE_2D, 2006); //tire texture 

	glPushMatrix();
	GLUquadricObj *cyl4;
	cyl4 = gluNewQuadric();
	glTranslatef(-0.8, -0.7, -1.51);
	glRotatef(theta_wheels, 0.0, 0.0, 1.0);
	gluQuadricTexture(cyl4, GL_TRUE);
	gluCylinder(cyl4, 0.5,0.5,0.5,32,32);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 2003); //black filling for tire

	glPushMatrix();
	glTranslatef(-0.81, -0.7, -1.51);
	drawCircle(0, 0, 0.48);
	glTranslatef(0.0, 0.0, 0.5);
	drawCircle(0, 0, 0.47);
	glPopMatrix();





    glutSwapBuffers();   // Double buffering, swap buffers
}



void drawCircle(GLfloat x, GLfloat y, GLfloat radius)
{
	int i;
	int triangleAmount = 1000; 
	GLfloat twicePi = 2.0f * PI;

	glEnable(GL_LINE_SMOOTH);
	glLineWidth(0.4);



	glBegin(GL_LINES);
	glColor4f(1.0, 0.0, 0.0, 1.0);

	for(i = 0; i <= triangleAmount; i++) {

		glVertex2f( x, y);
		glVertex2f(x + (radius * cos(i * twicePi / triangleAmount)), y + (radius * sin(i * twicePi / triangleAmount)));
	}

	glEnd();
}


// Callback, called at initialization and whenever user resizes the window.
void reshape(int w, int h)
{
    // Set up viewport, projection, then change to modelview matrix mode - 
    // display function will then set up camera and do modeling transforms.
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    
}

void spinDisplay(int n)
{

	theta += 18.0;
	if (theta > 360.0)
		theta -= 360.0;

	//theta_wheels += 18.0;
	if(n == 1)
		theta_wheels -=18;

	if(n == -1)
		theta_wheels +=18;

	if(theta_wheels > 360)
		theta_wheels -= 360;

	glutPostRedisplay();
	//glutTimerFunc(100, spinDisplay, 0);
}

// Callback, handles input from the keyboard, non-arrow keys
void keyboard(unsigned char key, int x, int y)
{
	switch (key) {

	case 's':   /*  s key rotates at shoulder  */
		if (shoulder >= 90) {
			break;
		}
		else {
			shoulder = (shoulder + 5) % 360;
		}
		glutPostRedisplay();
		break;
	case 'S':
		if (shoulder <= -90) {
			break;
		}
		else {
			shoulder = (shoulder - 5) % 360;
		}
		glutPostRedisplay();
		break;
	case 'e':  /*  e key rotates at elbow  */
		if (elbow >= 90) {
			break;

		}
		else {
			elbow = (elbow + 5) % 360;
		}
		glutPostRedisplay();
		break;
	case 'E':
		if (elbow <= -45) {
			break;
		}
		else {
			elbow = (elbow - 5) % 360;
		}
		glutPostRedisplay();
		break;

	case 'r':
		body = (body + 5) % 360;
		glutPostRedisplay();
		break;
	case 'R':
		body = (body - 5) % 360;
		glutPostRedisplay();
		break;

	case 'Z':
		cameraZoom = (cameraZoom + 5);
		glutPostRedisplay();
		break;
	case 'z':
		cameraZoom = (cameraZoom - 5);
		glutPostRedisplay();
		break;
	default:
		break;

	}
	

	//printf(" %lf %lf %lf \n ", vector.x, vector.y, vector.z);

    glutPostRedisplay();   // Trigger a window redisplay
}

// Callback, handles input from the keyboard, function and arrow keys
void functionKeys(int key, int x, int y)
{
    // Help key
    if (key == GLUT_KEY_F1)
    {
		printf("------------------ Controls ------------------\n\n");
		printf("Use 'S' &&  's' keys for shoulder pitch controls\n");
		printf("Use 'E' && 'e' keys for elbow pitch controls\n");
		printf("Use 'R' && 'r' keys for 'yaw' controls\n");
		printf("Use 'Z' && 'z' keys for 'yaw' controls\n");
		printf("Press and hold left mouse button for camera movement\n");
		printf("\n-------------------- END ---------------------\n");
    }

	else if (key == GLUT_KEY_LEFT) {
		turn_angle = (turn_angle + 5) % 360;
	}
	else if (key == GLUT_KEY_RIGHT) {
		turn_angle = (turn_angle - 5) % 360;
	}
	else if (key == GLUT_KEY_UP) {
		//y_pos = (y_pos + 0.5);
		glutTimerFunc(100, spinDisplay, 0);
		x_pos += 0.5*cos(PI / 180 * turn_angle);
		z_pos += 0.5*sin(PI / 180 * -turn_angle);
		spinDisplay(1);
		//glutTimerFunc(100, spinDisplay, 0);
	}
	else if (key == GLUT_KEY_DOWN) {
		//y_pos = (y_pos - 0.5);
		glutTimerFunc(100, spinDisplay, 0);
		x_pos -= 0.5*cos(PI / 180 * turn_angle);
		z_pos -= 0.5*sin(PI / 180 * -turn_angle);
		spinDisplay(-1);
		
	}
    // Do transformations with arrow keys
    //else if (...)   // GLUT_KEY_DOWN, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_LEFT
    //{
    //}

    glutPostRedisplay();   // Trigger a window redisplay
}


// Mouse button callback - use only if you want to 
void mouse(int button, int state, int x, int y)
{
    currentButton = button;

    switch (button)
    {
    case GLUT_LEFT_BUTTON:
        if (state == GLUT_DOWN)
        {
			rotate = false;
			if (button == GLUT_LEFT_BUTTON) {
				oldX = x;
				oldY = y;
				rotate = true;
			};

        }
        break;
    case GLUT_RIGHT_BUTTON:
        if (state == GLUT_DOWN)
        {
            ;
        }
        break;
    default:
        break;
    }

    glutPostRedisplay();   // Trigger a window redisplay
}


// Mouse motion callback - use only if you want to 
void mouseMotionHandler(int xMouse, int yMouse)
{
    if (currentButton == GLUT_LEFT_BUTTON)
    {
		if (rotate) {
			theta_cam += (xMouse - oldX)*0.01f;
			phi_cam += (yMouse - oldY)*0.01f;
		}
		oldX = xMouse;
		oldY = yMouse;
		glutPostRedisplay(); ;
    }

    glutPostRedisplay();   // Trigger a window redisplay
}


Vector3D ScreenToWorld(int x, int y)
{
    // you will need to finish this if you use the mouse
    return NewVector3D(0, 0, 0);
}




