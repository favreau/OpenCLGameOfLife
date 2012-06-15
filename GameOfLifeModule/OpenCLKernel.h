/* 
 * OpenCL Raytracer
 * Copyright (C) 2011-2012 Cyrille Favreau <cyrille_favreau@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

/*
 * Author: Cyrille Favreau <cyrille_favreau@hotmail.com>
 *
 */

#pragma once

#include <CL/opencl.h>

#include "DLL_API.h"
#include <stdio.h>
#include <string>
#include <windows.h>
#if USE_KINECT
#include <MSR_nuiapi.h>
#endif // USE_KINECT

const int gTextureWidth  = 1920;
const int gTextureHeight = 1200;
const int gTextureDepth  = 3;
const int gColorDepth    = 4;

enum KernelSourceType
{
   kst_file,
   kst_string
};

enum PrimitiveType 
{
   ptSphere = 0,
   ptTriangle,
   ptCheckboard,
   ptCamera,
   ptXYPlane,
   ptYZPlane,
   ptXZPlane,
   ptCylinder
};

const int NO_MATERIAL = -1;

const int gKinectColorVideo = 4;
const int gVideoWidth       = 640;
const int gVideoHeight      = 480;
const int gKinectColorDepth = 2;
const int gDepthWidth       = 320;
const int gDepthHeight      = 240;

struct RecursiveInfo
{
   cl_int    index;
   cl_float4 origin; 
   cl_float4 target;
   cl_float4 color;
   cl_float4 ratio;
};

struct Material
{
   cl_float4 color;
   cl_float  refraction;
   cl_int    textured;
   cl_float  transparency;
   cl_int    textureId;
   cl_float4 specular;
};

struct Primitive
{
   cl_float4 center;
   //cl_float4 rotation;
   cl_float4 size;
   cl_int    type;
   cl_int    materialId;
   cl_float  materialRatioX;
   cl_float  materialRatioY;
};

struct Lamp
{
   cl_float4 center;
   cl_float4 color;
};

class GAMEOFLIFEMODULE_API OpenCLKernel
{
public:
   OpenCLKernel( int platformId, int device, int nbWorkingItems, int draft );
   ~OpenCLKernel();

public:
   // ---------- Devices ----------
   void initializeDevice(
      int        width, 
      int        height);
   void releaseDevice();

   void compileKernels(
      const KernelSourceType sourceType,
      const std::string& source, 
      const std::string& ptxFileName,
      const std::string& options);

public:
   // ---------- Rendering ----------
   void render(
      int   imageW, 
      int   imageH, 
      BYTE* bitmap,
      float value);

public:

   // ---------- Textures ----------
   void setTexture(
      int   index,
      BYTE* texture );

   long addTexture( 
      const std::string& filename );

#ifdef USE_KINECT
public:

   // ---------- Kinect ----------

   long updateSkeletons( 
      double center_x, double  center_y, double  center_z, 
      double size,
      double radius,       int materialId,
      double head_radius,  int head_materialId,
      double hands_radius, int hands_materialId,
      double feet_radius,  int feet_materialId);
#endif // USE_KINECT

public:

   int              getCLPlatformId() { return m_hPlatformId; };
   cl_context       getCLContext()    { return m_hContext; };
   cl_command_queue getCLQueue()      { return m_hQueue; };

private:

   char* loadFromFile( const std::string&, size_t&);

private:
   // OpenCL Objects
   cl_device_id     m_hDevices[100];
   int              m_hPlatformId;
   cl_context       m_hContext;
   cl_command_queue m_hQueue;
   cl_kernel        m_hMainKernel;
   cl_uint          m_computeUnits;
   cl_uint          m_preferredWorkGroupSize;

private:
   // Host
   cl_mem m_hBitmap;
   cl_mem m_hBuffer;
   cl_mem m_hVideo;
   cl_mem m_hDepth;
   cl_mem m_hTextures;
   cl_int m_offset;
  cl_float m_timer;

   // Kinect declarations
#ifdef USE_KINECT
private:
   HANDLE             m_skeletons;
   HANDLE             m_hNextDepthFrameEvent; 
   HANDLE             m_hNextVideoFrameEvent;
   HANDLE             m_hNextSkeletonEvent;
   HANDLE             m_pVideoStreamHandle;
   HANDLE             m_pDepthStreamHandle;
   NUI_SKELETON_FRAME m_skeletonFrame;

   long               m_skeletonsBody;
   long               m_skeletonsLamp;

#endif // USE_KINECT

private:
   BYTE*       m_textures;
   bool        m_texturedTransfered;
};
