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

#include <GL/freeglut.h>
#include <GL/glew.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cuda_gl_interop.h>
#include <cutil.h>
#include <cutil_gl_inline.h>
#include <cutil_inline.h>
#include <driver_functions.h>
#include <rendercheck_gl.h>

const char *sSDKsample = "Realtime raytracing";

const cudaExtent volumeSize = make_cudaExtent(32, 32, 32);

const unsigned int width = 512;
const unsigned int height = 512;

// Kinect
short motorPos = 0;
PDWORD rgb32_data = 0;
PUSHORT depth32_data = 0;
const unsigned int kinect_width = 640;
const unsigned int kinect_height = 480;

// OpenGL
GLuint pbo; // OpenGL pixel buffer object
struct cudaGraphicsResource
    *cuda_pbo_resource; // CUDA Graphics Resource (to transfer PBO)

bool linearFiltering = true;
bool animate = true;

unsigned int *d_output = NULL;

#define MAX(a, b) ((a > b) ? a : b)

extern "C" void setTextureFilterMode(bool bLinearFilter);
extern "C" void initCuda(const unsigned char *h_volume, cudaExtent volumeSize,
                         int width, int height);
extern "C" void finalizeCuda();
#if 1
extern "C" void render_kernel(unsigned int *d_output, int imageW, int imageH,
                              float3 eye, float3 sphere, float radius,
                              float reflect, PDWORD bkground, PUSHORT depth,
                              float time);
#else
extern "C" void render_kernel(float3 sphere, float radius, float reflect,
                              unsigned int *d_output);
#endif

// Testing
float3 eye, sphere;
float radius;
float step = 1.0f;
float reflect = 0.0f;
float time = 0.0f;
int tick = 0;

// render image using CUDA
void render() {
  // map PBO to get CUDA device pointer
  cutilSafeCall(cudaGraphicsMapResources(1, &cuda_pbo_resource, 0));
  size_t num_bytes;
  cutilSafeCall(cudaGraphicsResourceGetMappedPointer(
      (void **)&d_output, &num_bytes, cuda_pbo_resource));

// call CUDA kernel, writing results to PBO
#if 1
  render_kernel(d_output, width, height, eye, sphere, radius, reflect,
                rgb32_data, depth32_data, time);
#else
  render_kernel(sphere, radius, reflect, d_output);
#endif
  cutilCheckMsg("kernel failed");
  cutilSafeCall(cudaGraphicsUnmapResources(1, &cuda_pbo_resource, 0));
}

// display results using OpenGL (called by GLUT)
void display() {
  render();

  // display results
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glRasterPos2i(0, 0);
  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);

  // long t = GetTickCount();
  glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  // t = GetTickCount() - t;
  // printf("%d ms\n", t);

  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  glutSwapBuffers();
  glutReportErrors();

  time += 0.1f;
}

void idle() {}

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
  case 'a':
    eye.x -= step;
    break;
  case 'd':
    eye.x += step;
    break;
  case 'w':
    eye.y += step;
    break;
  case 's':
    eye.y -= step;
    break;
  case 'q':
    eye.z -= step;
    break;
  case 'e':
    eye.z += step;
    break;

  case '4':
    sphere.x -= step;
    break;
  case '6':
    sphere.x += step;
    break;
  case '8':
    sphere.y += step;
    break;
  case '2':
    sphere.y -= step;
    break;
  case '7':
    sphere.z -= step;
    break;
  case '9':
    sphere.z += step;
    break;

  case '1':
    reflect -= 0.05f;
    reflect = (reflect < 0.0f) ? 0.0f : reflect;
    break;
  case '3':
    reflect += 0.05f;
    reflect = (reflect > 1.0f) ? 1.0f : reflect;
    break;

  case '-':
    radius -= 1.0f;
    break;
  case '+':
    radius += 1.0f;
    break;

  case 27:
    exit(0);
    break;
  default:
    break;
  }
  glutPostRedisplay();
}

void reshape(int x, int y) {
  glViewport(0, 0, x, y);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
}

void cleanup() { finalizeCuda(); }

void initGLBuffers() {
  // create pixel buffer object
  glGenBuffersARB(1, &pbo);
  glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
  glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,
                  width * height * sizeof(GLubyte) * 4, 0, GL_STREAM_DRAW_ARB);
  // glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

  // register this buffer object with CUDA
  cutilSafeCall(cudaGraphicsGLRegisterBuffer(&cuda_pbo_resource, pbo,
                                             cudaGraphicsMapFlagsWriteDiscard));
}

void initGL(int *argc, char **argv) {
  // initialize GLUT callback functions
  glutInit(argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutInitWindowSize(width, height);
  glutCreateWindow("CUDA Raytracer");
  // glutFullScreen();
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutReshapeFunc(reshape);
  glutIdleFunc(display);

  eye.x = 0.0f;
  eye.y = 0.0f;
  eye.z = -30.0f;

#if 1
  sphere.x = 0.0f;
  sphere.y = 0.0f;
  sphere.z = -5.0f;
  radius = 2.0f;
#else
  sphere.x = 233;
  sphere.y = 290;
  sphere.z = 0;
  radius = 100;
#endif

  glewInit();
  if (!glewIsSupported("GL_VERSION_2_0 GL_ARB_pixel_buffer_object")) {
    fprintf(stderr, "Required OpenGL extensions missing.");
    exit(-1);
  }
}

// General initialization call for CUDA Device
int chooseCudaDevice(int argc, char **argv, bool bUseOpenGL) {
  int result = 0;
  if (bUseOpenGL) {
    result = cutilChooseCudaGLDevice(argc, argv);
  } else {
    result = cutilChooseCudaDevice(argc, argv);
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
  printf("[%s]\n", sSDKsample);

  // First initialize OpenGL context, so we can properly set the GL for CUDA.
  // This is necessary in order to achieve optimal performance with OpenGL/CUDA
  // interop.
  initGL(&argc, argv);

  // use command-line specified CUDA device, otherwise use device with highest
  // Gflops/s
  chooseCudaDevice(argc, argv, true);

  // OpenGL buffers
  initGLBuffers();

  size_t size = volumeSize.width * volumeSize.height * volumeSize.depth;
  unsigned char *h_volume =
      (unsigned char *)malloc(size); // loadRawFile(path, size);

  initCuda(h_volume, volumeSize, width, height);
  free(h_volume);

  atexit(cleanup);

  glutMainLoop();

  cudaDeviceReset();
  cutilExit(argc, argv);

  return 0;
}
