/* Copyright (c) 2011-2012, Cyrille Favreau
 * All rights reserved. Do not distribute without permission.
 * Responsible Author: Cyrille Favreau <cyrille_favreau@hotmail.com>
 *
 * This file is part of OpenCLGameOfLife
 * <https://github.com/favreau/OpenCLGameOfLife>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// OpenGL Graphics Includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Includes
#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <time.h>

#include "../GameOfLifeModule/OpenCLKernel.h"

// General Settings
const long REFRESH_DELAY = 1; // ms
const bool gUseKinect = false;

// Rendering window vars
const int nbIterations = 5;
const unsigned int draft = 1;
unsigned int window_width = unsigned int(512 * 1.0f);
unsigned int window_height = unsigned int(window_width * 9.f / 16.f);
const unsigned int window_depth = 4;

// Scene
int platform = 0;
int device = 0;

// Scene
float transparentColor = 0.1f;

#ifdef USE_KINECT
const float gSkeletonSize = 200.0;
const float gSkeletonThickness = 20.0;
#endif // USE_KINECT

// OpenGL
GLubyte *ubImage;
int previousFps = 0;

/**
--------------------------------------------------------------------------------
OpenGL
--------------------------------------------------------------------------------
*/

// GL functionality
void initgl(int argc, char **argv);
void display();
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void timerEvent(int value);
void createScene(int platform, int device);

// Helpers
void TestNoGL();
void Cleanup(int iExitCode);
void (*pCleanup)(int) = &Cleanup;

// Sim and Auto-Verification parameters
float anim = 0.0;
bool bNoPrompt = false;

// mouse controls
int mouse_old_x, mouse_old_y;
int mouse_buttons = 0;
float rotate_x = 0.0, rotate_y = 0.0;
float translate_z = -3.0;

// GameOfLife Module
OpenCLKernel *oclKernel = 0;
unsigned int *uiOutput = NULL;

float getRandomValue(int range, int safeZone, bool allowNegativeValues = true) {
  float value(static_cast<float>(rand() % range) + safeZone);
  if (allowNegativeValues) {
    value *= (rand() % 2 == 0) ? -1 : 1;
  }
  return value;
}

void idle() {}

void cleanup() {}

/*
--------------------------------------------------------------------------------
setup the window and assign callbacks
--------------------------------------------------------------------------------
*/
void initgl(int argc, char **argv) {
  size_t len(window_width * window_height * window_depth);
  ubImage = new GLubyte[len];
  memset(ubImage, 0, len);

  glutInit(&argc, (char **)argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

  glutInitWindowPosition(glutGet(GLUT_SCREEN_WIDTH) / 2 - window_width / 2,
                         glutGet(GLUT_SCREEN_HEIGHT) / 2 - window_height / 2);

  glutInitWindowSize(window_width, window_height);
  glutCreateWindow("OpenCL GameOfLife");

  glutDisplayFunc(display); // register GLUT callback functions
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutTimerFunc(REFRESH_DELAY, timerEvent, 1);
  return;
}

void TexFunc(void) {
  glEnable(GL_TEXTURE_2D);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  glTexImage2D(GL_TEXTURE_2D, 0, 3, window_width, window_height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, ubImage);

  glBegin(GL_QUADS);
  glTexCoord2f(1.0, 1.0);
  glVertex3f(-1.0, 1.0, 0.0);
  glTexCoord2f(0.0, 1.0);
  glVertex3f(1.0, 1.0, 0.0);
  glTexCoord2f(0.0, 0.0);
  glVertex3f(1.0, -1.0, 0.0);
  glTexCoord2f(1.0, 0.0);
  glVertex3f(-1.0, -1.0, 0.0);
  glEnd();

  glDisable(GL_TEXTURE_2D);
}

// Display callback
//*****************************************************************************
void display() {
  // clear graphics
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  char text[255];
  long t = GetTickCount();
  oclKernel->render(window_width, window_height, (BYTE *)ubImage,
                    transparentColor);
  t = GetTickCount() - t;
  sprintf(text, "OpenCL GameOfLife (%d Fps)", 1000 / ((t + previousFps) / 2));
  previousFps = t;

  TexFunc();
  glutSetWindowTitle(text);
  glFlush();

  glutSwapBuffers();
}

void timerEvent(int value) {
#if USE_KINECT
  oclKernel->updateSkeletons(
      0.0, gSkeletonSize - 200, -150.0, // Position
      gSkeletonSize,                    // Skeleton size
      gSkeletonThickness, 0,            // Default size and material
      gSkeletonThickness * 2.0f, 10,    // Head size and material
      gSkeletonThickness * 1.5f, 1,     // Hands size and material
      gSkeletonThickness * 1.8f, 10     // Feet size and material
      );
#endif // USE_KINEXT

#if 0
   //oclKernel->rotatePrimitive( 2, 10.f*cos(anim), 10.f*sin(anim), 0.f );
   for( int i(0); i<3; ++i )
   {
      activeSphereCenter[i].s[0] += activeSphereDirection[i].s[0];
      activeSphereCenter[i].s[1]  = -200 + activeSphereCenter[i].s[3] + ((i==0) ? 0 : activeSphereCenter[i].s[3]*fabs(cos(anim/2.f+i/2.f)));
      activeSphereCenter[i].s[2] += activeSphereDirection[i].s[2];
      oclKernel->setPrimitive( 
         activeSphereId+i, 
         activeSphereCenter[i].s[0], activeSphereCenter[i].s[1], activeSphereCenter[i].s[2], 
         activeSphereCenter[i].s[3], activeSphereCenter[i].s[3], 
         activeSphereMaterial[i], 1 ); 

      if( fabs(activeSphereCenter[i].s[0]) > (gRoomSize-activeSphereCenter[i].s[3])) activeSphereDirection[i].s[0] = -activeSphereDirection[i].s[0];
      if( fabs(activeSphereCenter[i].s[2]) > (gRoomSize-activeSphereCenter[i].s[3])) activeSphereDirection[i].s[2] = -activeSphereDirection[i].s[2];
   }
#endif // 0

  glutPostRedisplay();
  glutTimerFunc(REFRESH_DELAY, timerEvent, 0);
}

void createTextures() {
  // Textures
  std::stringstream str;
  str << "../textures/0";
  int i = 1 + rand() % 18;
  if (i < 10)
    str << "0";
  str << i;
  str << ".bmp";
  oclKernel->addTexture(str.str().c_str());
}

// Keyboard events handler
//*****************************************************************************
void keyboard(unsigned char key, int x, int y) {
  srand(static_cast<unsigned int>(time(NULL)));

  switch (key) {
  case 'R':
  case 'r': {
    // Reset scene
    delete oclKernel;
    oclKernel = 0;
    createScene(platform, device);
    createTextures();
    break;
  }
  case 'q':
  case 'Q': {
    transparentColor += 0.01f;
    transparentColor = (transparentColor > 1.f) ? 1.f : transparentColor;
    break;
  }
  case 'a':
  case 'A': {
    transparentColor -= 0.01f;
    transparentColor = (transparentColor < 0.f) ? 0.f : transparentColor;
    break;
  }
  case 'F':
  case 'f': {
    // Toggle to full screen mode
    glutFullScreen();
    break;
  }
  case '\033':
  case '\015':
  case 'X':
  case 'x': {
    // Cleanup up and quit
    bNoPrompt = true;
    Cleanup(EXIT_SUCCESS);
    break;
  }
  }
}

// Mouse event handlers
//*****************************************************************************
void mouse(int button, int state, int x, int y) {
  if (state == GLUT_DOWN) {
    mouse_buttons |= 1 << button;
  } else {
    if (state == GLUT_UP) {
      mouse_buttons = 0;
    }
  }
  mouse_old_x = x;
  mouse_old_y = y;
}

void motion(int x, int y) {
  switch (mouse_buttons) {
  case 1:
    break;
  case 2:
    break;
  case 4:
    break;
  }
}

// Function to clean up and exit
//*****************************************************************************
void Cleanup(int iExitCode) {
  // Cleanup allocated objects
  std::cout << "\nStarting Cleanup...\n\n" << std::endl;
  if (ubImage)
    delete[] ubImage;
  delete oclKernel;

  exit(iExitCode);
}

void createScene(int platform, int device) {
  srand(static_cast<unsigned int>(time(NULL)));

  oclKernel = new OpenCLKernel(platform, device, 128, draft);
  oclKernel->initializeDevice(window_width, window_height);
  oclKernel->compileKernels(kst_file, "../GameOfLifeModule/Kernel.cl", "", "");

#ifdef USE_KINECT
  nbPrimitives = oclKernel->addPrimitive(ptCamera);
  oclKernel->setPrimitive(nbPrimitives, 0, 100, gRoomSize - 10, 320, 240, 0, 1);
/*
nbPrimitives = oclKernel->addPrimitive( ptXYPlane );
oclKernel->setPrimitive( nbPrimitives, 0, 0, 0, 320, 240, 0, 1 );
*/
#endif // USE_KINECT
}

void main(int argc, char *argv[]) {
  std::cout << "---------------------------------------------------------------"
               "-----------------"
            << std::endl;
  std::cout << "Keys:" << std::endl;
  std::cout << "  s: add sphere" << std::endl;
  std::cout << "  y: add cylinder" << std::endl;
  std::cout << "  c: add cube" << std::endl;
  std::cout << "  p: add plan (single faced)" << std::endl;
  std::cout << "  l: add lamp" << std::endl;
  std::cout << "  r: reset scene" << std::endl;
  std::cout << "Mouse:" << std::endl;
  std::cout << "  left       : Zoom in/out" << std::endl;
  std::cout << "  middle     : Rotate" << std::endl;
  std::cout << "  right      : Translate" << std::endl;
  std::cout << "  shift+left : Depth of view" << std::endl;
  std::cout << std::endl;
  std::cout << "---------------------------------------------------------------"
               "-----------------"
            << std::endl;
  if (argc == 5) {
    std::cout << argv[1] << std::endl;
    sscanf_s(argv[1], "%d", &platform);
    sscanf_s(argv[2], "%d", &device);
    sscanf_s(argv[3], "%d", &window_width);
    sscanf_s(argv[4], "%d", &window_height);
  } else {
    std::cout << "Usage:" << std::endl;
    std::cout << "  OpenCLGameOfLifeTester.exe [platformId] [deviceId] "
                 "[WindowWidth] [WindowHeight]"
              << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  OpenCLGameOfLifeTester.exe 0 1 640 480" << std::endl;
    std::cout << std::endl;
    exit(1);
  }
  // First initialize OpenGL context, so we can properly set the GL for CUDA.
  // This is necessary in order to achieve optimal performance with OpenGL/CUDA
  // interop.
  initgl(argc, argv);

  // Create Scene
  createScene(platform, device);
  createTextures();

  atexit(cleanup);
  glutMainLoop();

  // Normally unused return path
  Cleanup(EXIT_SUCCESS);
}
