/*******************************************************************
           Multi-Part Model Construction and Manipulation
********************************************************************/

using namespace std;

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gl/glut.h>
#include "QuadMesh.h"
#include "Vector3D.h"
#include "CubeMesh.h"
#include <cstdlib>
#include <vector>
#include <algorithm>


const int meshSize = 32;    // Default Mesh Size
const int vWidth = 800;     // Viewport width in pixels
const int vHeight = 600;    // Viewport height in pixels

static int currentButton;
static unsigned char currentKey;

// array of cube meshes
CubeMesh *a_mesh;
vector<CubeMesh> list_mesh;
int number = 0;
bool created = false;
bool translate = false;
bool scale = false;
bool height = false;
CubeMesh test;

float angle;
int nwidth = 20;
int nheight = 20;
float ballRadius;
Vector3D startRotationVector;
Vector3D currentRotationVector;
bool isRotating = false;
double rotationAngle;

Vector3D cameraPos = NewVector3D(0.0f, 6.0f, 22.0f);
Vector3D origin = NewVector3D(-16.0f, 0.0f, 16.0f);
Vector3D ray;



// Lighting/shading and material properties for drone - upcoming lecture - just copy for now

// Light properties
static GLfloat light_position0[] = { -6.0F, 12.0F, 0.0F, 1.0F };
static GLfloat light_position1[] = { 6.0F, 12.0F, 0.0F, 1.0F };
static GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat light_ambient[] = { 0.2F, 0.2F, 0.2F, 1.0F };

// Material properties
static GLfloat drone_mat_ambient[] = { 0.4F, 0.2F, 0.0F, 1.0F };
static GLfloat drone_mat_specular[] = { 0.1F, 0.1F, 0.0F, 1.0F };
static GLfloat drone_mat_diffuse[] = { 0.9F, 0.5F, 0.0F, 1.0F };
static GLfloat drone_mat_shininess[] = { 0.0F };

// A quad mesh representing the ground
static QuadMesh groundMesh;

// Structure defining a bounding box, currently unused
//struct BoundingBox {
//    Vector3D min;
//    Vector3D max;
//} BBox;

// Prototypes for functions in this module
void initOpenGL(int w, int h);
void exiting();
void display(void);
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void mouseMotionHandler(int xMouse, int yMouse);
void keyboard(unsigned char key, int x, int y);
void functionKeys(int key, int x, int y);
Vector3D ScreenToWorld(int x, int y);


int main(int argc, char **argv)
{
	a_mesh = new CubeMesh[20];

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(vWidth, vHeight);
    glutInitWindowPosition(200, 30);
    glutCreateWindow("Assignment 2");

    // Initialize GL
    initOpenGL(vWidth, vHeight);

    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotionHandler);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(functionKeys);

    // Start event loop, never returns
    glutMainLoop();

	std::atexit(exiting);

    return 0;
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

    // Set up ground quad mesh
	//half the size of the mesh
    Vector3D dir1v = NewVector3D(1.0f, 0.0f, 0.0f);
    Vector3D dir2v = NewVector3D(0.0f, 0.0f, -1.0f);

    groundMesh = NewQuadMesh(meshSize);
    InitMeshQM(&groundMesh, meshSize, origin, 32.0, 32.0, dir1v, dir2v);

	// Changes colour of ground (diffuse?)
    Vector3D ambient = NewVector3D(0.0f, 0.05f, 0.0f);
    Vector3D diffuse = NewVector3D(0.4f, 0.8f, 0.4f);
    Vector3D specular = NewVector3D(0.04f, 0.04f, 0.04f);
    SetMaterialQM(&groundMesh, ambient, diffuse, specular, 0.2);

	Subtract(&cameraPos, &origin, &ray);

    // Set up the bounding box of the scene
    // Currently unused. You could set up bounding boxes for your objects eventually.
    //Set(&BBox.min, -8.0f, 0.0, -8.0);
    //Set(&BBox.max, 8.0f, 6.0,  8.0);
}


// Callback, called whenever GLUT determines that the window should be redisplayed
// or glutPostRedisplay() has been called.
void display(void)
{
	// Clear all pixels
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	ballRadius = min((int)(nwidth / 2), (int)(nheight / 2));
	gluLookAt(ray.x*cosf(rotationAngle), ray.y, ray.x*-sinf(rotationAngle), 0, 0, 0, 0.0f, 1.0f, 0.0f);
	
	glPushMatrix();
	if (!list_mesh.empty())
	{
		glPushMatrix();
		CubeMesh current = list_mesh[number];
		drawCube(&current);
		glPopMatrix();
	}

	if (!list_mesh.empty())
	{
		vector<CubeMesh>::iterator it;

		for (it = list_mesh.begin(); it != list_mesh.end(); it++) {
			glPushMatrix();
			drawCube(&(*it));
			glPopMatrix();
		}
	}


    // Set drone material properties
    glMaterialfv(GL_FRONT, GL_AMBIENT, drone_mat_ambient);
    glMaterialfv(GL_FRONT, GL_SPECULAR, drone_mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, drone_mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SHININESS, drone_mat_shininess);

    // Draw ground mesh
    DrawMeshQM(&groundMesh, meshSize);

	glPopMatrix();

    glutSwapBuffers();   // Double buffering, swap buffers
}


// Callback, called at initialization and whenever user resizes the window.
void reshape(int w, int h)
{
    // Set up viewport, projection, then change to modelview matrix mode - 
    // display function will then set up camera and do modeling transforms.
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLdouble)w / h, 0.2, 40.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Set up the camera at position (0, 6, 22) looking at the origin, up along positive y axis
	gluLookAt(ray.x*-cosf(rotationAngle), ray.y, ray.x*sinf(rotationAngle), 0, 0, 0, 0.0f, 1.0f, 0.0f);
}

// Callback, handles input from the keyboard, non-arrow keys
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 't':
		translate = true;
		scale = false;
		height = false;
		break;
	case 's':
		scale = true;
		translate = false;
		height = false;
		break;
	case 'h':
		height = true;
		scale = false;
		translate = false;
		break;
	}
	

    glutPostRedisplay();   // Trigger a window redisplay
}

// Callback, handles input from the keyboard, function and arrow keys
void functionKeys(int key, int x, int y)
{
    // Help key
    if (key == GLUT_KEY_F1)
    {
		if (list_mesh.empty())
		{
			test = newCube();
			list_mesh.push_back(test);
		}
		else
		{
			test = newCube();
			list_mesh.push_back(test);
			number++;
			list_mesh[number - 1].selected = false;
		}
		
		
    }
    // Do transformations with arrow keys
    else if (key == GLUT_KEY_DOWN)   // GLUT_KEY_DOWN, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_LEFT
    {
		if (translate) {
			list_mesh[number].tz = list_mesh[number].tz + 1;
		}
		else if (height) {
			if (list_mesh[number].sfy > 0.5)
			{
				list_mesh[number].sfy = list_mesh[number].sfy - 0.5;
			}	
			else if (list_mesh[number].sfy <= 0.5 && list_mesh[number].sfy > 0.1) {
				list_mesh[number].sfy = list_mesh[number].sfy - 0.1;
				
			}
			else {
				list_mesh[number].sfy = 0.1;
			}

		}
		else if (scale) {
			list_mesh[number].sfz = list_mesh[number].sfz - 1;
			if (list_mesh[number].sfz < 1)
				list_mesh[number].sfz = 1;
		}
    }
	else if (key == GLUT_KEY_UP)
	{
		if (translate) {
			list_mesh[number].tz = list_mesh[number].tz - 1;
		}
		else if (height) {
			list_mesh[number].sfy = list_mesh[number].sfy + 0.5;
			if (list_mesh[number].sfy > 30) {
				list_mesh[number].sfy = 30;
			}
				
		}
		else if (scale)
		{
			list_mesh[number].sfz = list_mesh[number].sfz + 1;
		}
	}
	else if (key == GLUT_KEY_LEFT)
	{
		if (translate) {
			list_mesh[number].tx = list_mesh[number].tx - 1;

		}
		else if (scale) {
			list_mesh[number].sfx = list_mesh[number].sfx - 1;
			if (list_mesh[number].sfx < 1)
				list_mesh[number].sfx = 1;
		}
	}
	else if (key == GLUT_KEY_RIGHT)
	{
		if (translate) {
			list_mesh[number].tx = list_mesh[number].tx + 1;
		}
		else if (scale) {
			list_mesh[number].sfx = list_mesh[number].sfx + 1;
		}
	}

    glutPostRedisplay();   // Trigger a window redisplay
}


// Mouse button callback - use only if you want to 
void mouse(int button, int state, int x, int y)
{
    currentButton = button;

    switch (button)
    {
    case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN) {
			int newX = (x - (nwidth / 2));
			int newY = ((nheight / 2) - y);

			startRotationVector = ScreenToWorld(newX, newY);
			Normalize(&startRotationVector);

			currentRotationVector = startRotationVector;
			
		}
		else
		{
			
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
	int newX = (xMouse - (nwidth / 2));
	int newY = ((nheight / 2) - yMouse);

	currentRotationVector = ScreenToWorld(xMouse, yMouse);
	Normalize(&currentRotationVector);

	Vector3D calcVec;
	Subtract(&currentRotationVector, &startRotationVector, &calcVec);

	if (GetLength(&calcVec) > 1E-6)
	{
		angle = DotProduct(&currentRotationVector, &startRotationVector);
		angle > (1 - 1E-10) ? angle = 1.0 : angle = angle;
		rotationAngle = acos(angle) * 180.0f / (float)3.14;
		rotationAngle = rotationAngle * 0.1;
	}

    glutPostRedisplay();   // Trigger a window redisplay
}


Vector3D ScreenToWorld(int x, int y)
{
    // you will need to finish this if you use the mouse
   // return NewVector3D(0, 0, 0);

	int d = x * x + y * y;
	float squareRadius = ballRadius * ballRadius;
	if (d > squareRadius)
		return NewVector3D((float)x, (float)y, 0);
	else
		return NewVector3D((float)x, (float)y, sqrt(squareRadius - d));
}

void idle()
{

}

void exiting()
{
	delete[] a_mesh;
	list_mesh.clear();
}





