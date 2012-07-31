#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "GL/glew.h"
#include <GL/glut.h>
#include "util/vec3.h"
#include "util/Matrix.h"



int sWidth = 800;
int sHeight = 600;

// Shadow map size
const int shadowMapSize = 512;
const int texWidth = 512;
const int texHeight = 512;

Vec3f camPos = Vec3f(-.5f, 3.5f, -2.5f);
Vec3f lightPos = Vec3f(2.0f, 4.0f, -2.0f);

//---------------
// FBO
GLuint fbo;
GLuint depthBuffer;

//---------------
// Textures
GLuint imgTexture;
GLuint shadowMapTexture;

//---------------
// Matrices
Matrix lightProjectionMatrix;
Matrix lightViewMatrix;
Matrix cameraProjectionMatrix;
Matrix cameraViewMatrix;
GLfloat lPM[16];
GLfloat lVM[16];
GLfloat cPM[16];
GLfloat cVM[16];
// [-1, 1] to [0,1]
GLfloat biasMatrix[16] = { 0.5f, 0.0f, 0.0f, 0.0f,
                           0.0f, 0.5f, 0.0f, 0.0f,
                           0.0f, 0.0f, 0.5f, 0.0f,
                           0.5f, 0.5f, 0.5f, 1.0f};

//-----------
// rotation
GLfloat curRot = 0.0;
GLfloat rotStep = 0.01;
GLfloat lightStep = 0.00;

// OSD
void drawText(int, int, void*, std::string);
void fps();


void reshape(int, int);

void calcMatrix()
{
   //----------------
   // Calculate the matrices
   glPushMatrix();

   glLoadIdentity();
   gluPerspective(45.0f, (float)sWidth/sHeight, 1.0f, 100.0f);
   glGetFloatv(GL_MODELVIEW_MATRIX, cPM);
   cameraProjectionMatrix = Matrix(cPM);

   glLoadIdentity();
   gluLookAt(  camPos.x, camPos.y, camPos.z,
               0.0f, 0.0f, 0.0f,
               0.0f, 1.0f, 0.0f);
   glGetFloatv(GL_MODELVIEW_MATRIX, cVM);
   cameraViewMatrix = Matrix(cVM);

   glLoadIdentity();
   //gluPerspective(45.0f, 1.0f, 1.0f, 100.0f);
   gluPerspective(45.0f, 1.0f, 2.0f, 8.0f);
   //gluPerspective(45.0f, (float)sWidth/sHeight, 1.0f, 100.0f);
   glGetFloatv(GL_MODELVIEW_MATRIX, lPM);
   lightProjectionMatrix = Matrix(lPM);

   glLoadIdentity();
   gluLookAt(  lightPos.x, lightPos.y, lightPos.z,
               0.0f, 0.0f, 0.0f,
               0.0f, 1.0f, 0.0f);
   glGetFloatv(GL_MODELVIEW_MATRIX, lVM);
   lightViewMatrix = Matrix(lVM);

   glPopMatrix();


}

void init()
{

   // initialize glew
   GLenum status = glewInit();
   if (status != GLEW_OK)
      printf("ERROR: OpenGL 2.0 not supported!\n");
   else
      printf("OpenGL 2.0 supported!\n");

   // TODO: CHECK FOR EXTENSION SUPPORT
   if (!GLEW_ARB_depth_texture || !GLEW_ARB_shadow)
   {
      printf("ARB_depth_texture and ARB_shadow not supported!\n");
      exit(1);
   }

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // Shading states
   glShadeModel(GL_SMOOTH);
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

   // Depth states
   glClearDepth(1.0f);
   glDepthFunc(GL_LEQUAL);
   glEnable(GL_DEPTH_TEST);

   glEnable(GL_CULL_FACE);
   //glCullFace(GL_BACK);
   glEnable(GL_NORMALIZE);

   // Create the shadow map texture
   glGenTextures(1, &shadowMapTexture);
   glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
   glTexImage2D(  GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0,
                  GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

   // setup the material
   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);

   GLfloat white[] = {1.0, 1.0, 1.0, 1.0};
   glMaterialfv(GL_FRONT, GL_SPECULAR, white);
   glMaterialf(GL_FRONT, GL_SHININESS, 16.0f);

   calcMatrix();


   reshape(sWidth, sHeight);
}

void initFBO()
{
   //if (!glewIsSupported("GL_FRAMEBUFFER_EXT GL_RENDERBUFFER_EXT GL_DEPTH_ATTACHMENT_EXT"))
   //{
   //   printf("ERROR: Framebuffer extensions not supported!\n");
   //   exit(1);
   //}

   // setup the FBO
   glGenFramebuffersEXT(1, &fbo);
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

   // create the render buffer for depth
   glGenRenderbuffersEXT(1, &depthBuffer);
   glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBuffer);
   glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, texWidth, texHeight);
   // bind the render buffer
   glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depthBuffer);

   // create the img texture
   glEnable(GL_TEXTURE_2D);
   glGenTextures(1, &imgTexture);
   glBindTexture(GL_TEXTURE_2D, imgTexture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, imgTexture, 0);
   glDisable(GL_TEXTURE_2D);

   // set the render targets
   GLenum mrt[] = {GL_COLOR_ATTACHMENT0_EXT};
   glDrawBuffers(3, mrt);

   GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
   if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
   {
      printf("ERROR: FBO status not complete!\n");
      exit(1);
   }

   // unbind the fbo
   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void drawScene()
{
   //------------------------
   // Draw the scene

   // draw the objects
   glPushMatrix();
   glScalef(0.5, 0.5, 0.5);
   glTranslatef(0.0, 1.0, 1.0);
      
      
      glRotatef(-90, 1.0, 0.0, 0.0);
      glColor4f(0.0, 0.0f, 1.0f, 1.0f);
      glutSolidCone(1.0, 2.0, 10, 10);
      glTranslatef(2.0, 3.0, 0.0);
      glColor3f(0.5, 0.5, 0.0);
      glPushMatrix();
         glRotatef(curRot, 0.0, 1.0, 0.0);
         glutSolidCube(1.0);
      glPopMatrix();
      glTranslatef(-4.0f, 0.0, 0.0);
      glColor3f(0.5, 0.0, 0.0);
      glutSolidSphere(0.5, 10, 10);
   glPopMatrix();

   // draw the ground
   glColor3f(1.0f, 0.0f, 0.0f);
   glPushMatrix();

   glScalef(1.0f, 0.05f, 1.0f);
   glutSolidCube(3.0f);

   glPopMatrix();
   //glTranslatef(0.0f, 1.0f, -3.0f);
   //glutSolidSphere(1.0, 10, 10);

   // draw the lights
   glPushMatrix();
      glTranslatef(lightPos.x, lightPos.y, lightPos.z);
      glColor3f(0.0, 1.0, 0.0);
      glutWireSphere(0.2, 5, 5);
   glPopMatrix();

   // draw the ground
   //glColor3f(1.0, 0.0, 0.0);
   //glTranslatef(0.0, -10.0, 0.0);
   //glutSolidCube(10);

/*
   	//Display lists for objects
	static GLuint spheresList=0, torusList=0, baseList=0;

	//Create spheres list if necessary
	if(!spheresList)
	{
		spheresList=glGenLists(1);
		glNewList(spheresList, GL_COMPILE);
		{
			glColor3f(0.0f, 1.0f, 0.0f);
			glPushMatrix();

			glTranslatef(0.45f, 1.0f, 0.45f);
			glutSolidSphere(0.2, 24, 24);

			glTranslatef(-0.9f, 0.0f, 0.0f);
			glutSolidSphere(0.2, 24, 24);

			glTranslatef(0.0f, 0.0f,-0.9f);
			glutSolidSphere(0.2, 24, 24);

			glTranslatef(0.9f, 0.0f, 0.0f);
			glutSolidSphere(0.2, 24, 24);

			glPopMatrix();
		}
		glEndList();
	}

	//Create torus if necessary
	if(!torusList)
	{
		torusList=glGenLists(1);
		glNewList(torusList, GL_COMPILE);
		{
			glColor3f(1.0f, 0.0f, 0.0f);
			glPushMatrix();

			glTranslatef(0.0f, 0.5f, 0.0f);
			glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
			glutSolidTorus(0.2, 0.5, 24, 48);

			glPopMatrix();
		}
		glEndList();
	}

	//Create base if necessary
	if(!baseList)
	{
		baseList=glGenLists(1);
		glNewList(baseList, GL_COMPILE);
		{
			glColor3f(0.0f, 0.0f, 1.0f);
			glPushMatrix();

			glScalef(1.0f, 0.05f, 1.0f);
			glutSolidCube(3.0f);

			glPopMatrix();
		}
		glEndList();
	}


	//Draw objects
	glCallList(baseList);
	glCallList(torusList);
	
	glPushMatrix();
	//glRotatef(angle, 0.0f, 1.0f, 0.0f);
	glCallList(spheresList);
	glPopMatrix();
   */
}

void renderTextures()
{
   calcMatrix();
   //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
   //-------------------
   // FIRST PASS - lights pov
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixf(lPM);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadMatrixf(lVM);

   glPushAttrib(GL_VIEWPORT_BIT);
   glViewport(0, 0, shadowMapSize, shadowMapSize);

   // draw only backfaces to remove precision error
   glCullFace(GL_FRONT);

   // use only flat shading
   glShadeModel(GL_FLAT);
   glColorMask(0, 0, 0, 0);

   drawScene();

   // read the depth buffer into the shadow map texture
   glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
   glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, shadowMapSize, shadowMapSize);

   // restore states
   glCullFace(GL_BACK);
   glShadeModel(GL_SMOOTH);
   glColorMask(1, 1, 1, 1);

   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glPopAttrib();


   //--------------------
   // SECOND PASS - draw scene darkened.. IE: shadow areas
   glClear(GL_DEPTH_BUFFER_BIT);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixf(cPM);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadMatrixf(cVM);

   glPushAttrib(GL_VIEWPORT_BIT);
   glViewport(0, 0, sWidth, sHeight);

   // Use dim light to represent shadowed areas
   GLfloat p[4];
   p[0] = lightPos.x;
   p[1] = lightPos.y;
   p[2] = lightPos.z;
   p[3] = 1.0;
   GLfloat ambient[] = {0.2f, 0.2f, 0.2f, 0.2f};
   GLfloat black[] = {0.0, 0.0, 0.0, 0.0};
   glLightfv(GL_LIGHT0, GL_POSITION, p);
   glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, ambient);
   glLightfv(GL_LIGHT0, GL_SPECULAR, black);
   glEnable(GL_LIGHT0);
   glEnable(GL_LIGHTING);

   drawScene();


   //-------------------
   // THIRD PASS - Draw with bright light
   GLfloat white[] = {1.0, 1.0, 1.0, 1.0};
   glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
   glLightfv(GL_LIGHT0, GL_SPECULAR, white);

   // Calculate the texture matrix
   // eye space to light clip space
   Matrix texMatrix = Matrix(biasMatrix)*lightProjectionMatrix*lightViewMatrix;
   GLfloat row0[4];
   GLfloat row1[4];
   GLfloat row2[4];
   GLfloat row3[4];
   texMatrix.getRow(0, row0); texMatrix.getRow(1, row1);
   texMatrix.getRow(2, row2); texMatrix.getRow(3, row3);

   // tex coord generation
   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
   glTexGenfv(GL_S, GL_EYE_PLANE, row0);
   glEnable(GL_TEXTURE_GEN_S);

   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
   glTexGenfv(GL_T, GL_EYE_PLANE, row1);
   glEnable(GL_TEXTURE_GEN_T);

   glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
   glTexGenfv(GL_R, GL_EYE_PLANE, row2);
   glEnable(GL_TEXTURE_GEN_R);

   glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
   glTexGenfv(GL_Q, GL_EYE_PLANE, row3);
   glEnable(GL_TEXTURE_GEN_Q);

   // bind the shadow map
   glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
   glEnable(GL_TEXTURE_2D);

   // enable shadow compare
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE);
   // true if not in shadow
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
   // store result as intensity
   glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);

   // discard all but alpha 1 values
   glAlphaFunc(GL_GEQUAL, 0.99f);
   glEnable(GL_ALPHA_TEST);

   drawScene();

   glDisable(GL_TEXTURE_2D);

   glDisable(GL_TEXTURE_GEN_S);
   glDisable(GL_TEXTURE_GEN_T);
   glDisable(GL_TEXTURE_GEN_R);
   glDisable(GL_TEXTURE_GEN_Q);

   glDisable(GL_LIGHTING);
   glDisable(GL_ALPHA_TEST);

   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopAttrib();

   //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);


   //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

   //glPushAttrib(GL_VIEWPORT_BIT);
   //glViewport(0, 0, texWidth, texHeight);
   //glMatrixMode(GL_PROJECTION);
   //glPushMatrix();
   //glLoadMatrixf(cPM);

   //glMatrixMode(GL_MODELVIEW);
   //glPushMatrix();
   //glLoadMatrixf(cVM);

   //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   ////glCullFace(GL_BACK);

   //p[4];
   //p[0] = lightPos.x;
   //p[1] = lightPos.y;
   //p[2] = lightPos.z;
   //p[3] = 1.0;
   //GLfloat ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
   //GLfloat diff[] = {0.5f, 0.5f, 0.5f, 1.0f};

   //glLightfv(GL_LIGHT0, GL_POSITION, p);
   //glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
   //glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
   //glEnable(GL_LIGHT0);
   //glEnable(GL_LIGHTING);

   //drawScene();

   //glDisable(GL_LIGHTING);

   //glPopMatrix();
   //glMatrixMode(GL_PROJECTION);
   //glPopMatrix();
   //glMatrixMode(GL_MODELVIEW);
   //glPopAttrib();

   //glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void drawQuadViewport(GLuint texID1, GLuint texID2, GLuint texID3, GLuint texID4)
{
   glEnable(GL_TEXTURE_2D);

   //-------------------------
   // Top Left
   glBindTexture(GL_TEXTURE_2D, texID1);
   glBegin(GL_QUADS);

      //glNormal3f( 0.0f, 0.0f, -1.0f);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(-99.0, 1.0);
      glTexCoord2f(1.0f, 0.0f); glVertex2f(-1.0, 1.0);
      glTexCoord2f(1.0f, 1.0f); glVertex2f(-1.0, 99.0);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(-99.0, 99.0);

   glEnd();

   //-------------------------
   // Top Right
   glBindTexture(GL_TEXTURE_2D, texID2);
   glBegin(GL_QUADS);

      //glNormal3f( 0.0f, 0.0f, -1.0f);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(1.0, 1.0);
      glTexCoord2f(1.0f, 0.0f); glVertex2f(99.0, 1.0);
      glTexCoord2f(1.0f, 1.0f); glVertex2f(99.0, 99.0);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(1.0, 99.0);

   glEnd();

   //-------------------------
   // Bottom Left
   glBindTexture(GL_TEXTURE_2D, texID3);
   glBegin(GL_QUADS);

      //glNormal3f( 0.0f, 0.0f, -1.0f);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(-99.0, -99.0);
      glTexCoord2f(1.0f, 0.0f); glVertex2f(-1.0, -99.0);
      glTexCoord2f(1.0f, 1.0f); glVertex2f(-1.0, -1.0);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(-99.0, -1.0);

   glEnd();

   //-------------------------
   // Top Left
   glBindTexture(GL_TEXTURE_2D, texID4);
   glBegin(GL_QUADS);

      //glNormal3f( 0.0f, 0.0f, -1.0f);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(1.0, -99.0);
      glTexCoord2f(1.0f, 0.0f); glVertex2f(99.0, -99.0);
      glTexCoord2f(1.0f, 1.0f); glVertex2f(99.0, -1.0);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(1.0, -1.0);

   glEnd();

   glDisable(GL_TEXTURE_2D);

}

void display()
{
   renderTextures();

   //glClearColor(0.2, 0.2, 0.2, 0.5);
   //glClearDepth(1.0f);
   //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   //glLoadIdentity();

   //glColor4f(1.0, 1.0, 1.0, 0.0);
   ////glDisable(GL_BLEND);
   //drawQuadViewport(imgTexture, shadowMapTexture, 0, 0);

   glColor3f(0.0, 1.0, 0.0);
   //fps();

   curRot += rotStep;
   if (lightPos.z >= 10.0f || lightPos.z <= -10.0f)
      lightStep *= -1.0;

   lightPos.z += lightStep;

   glutSwapBuffers();
}

void reshape(int w, int h)
{
   sWidth = w;
   sHeight = h;

   glViewport( 0, 0, sWidth, sHeight );
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(-100, 100, -100, 100);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

void keyboard( unsigned char key, int x, int y )
{
   switch (key)
   {
      // ESC
   case 27:
      exit(0);
      break;
   }
}

void idle() { glutPostRedisplay(); }

int main (int argc, char* argv[])
{
   glutInit(&argc, argv);
   glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
   glutInitWindowSize(sWidth, sHeight);
   glutCreateWindow( "Shadow Mapping test" );

   init();
   //initFBO();

   glutDisplayFunc( display );
   glutReshapeFunc( reshape );
   glutKeyboardFunc( keyboard );
   glutIdleFunc( idle );
   glutMainLoop();

   return 0;
}
