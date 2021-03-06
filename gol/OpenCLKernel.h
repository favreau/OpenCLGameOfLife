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

#pragma once

#include <CL/opencl.h>

#include "DLL_API.h"
#include <stdio.h>
#include <string>
#include <windows.h>

const int gTextureWidth = 1920;
const int gTextureHeight = 1200;
const int gTextureDepth = 3;
const int gColorDepth = 4;

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
const int gVideoWidth = 640;
const int gVideoHeight = 480;
const int gKinectColorDepth = 2;
const int gDepthWidth = 320;
const int gDepthHeight = 240;

struct RecursiveInfo
{
    cl_int index;
    cl_float4 origin;
    cl_float4 target;
    cl_float4 color;
    cl_float4 ratio;
};

struct Material
{
    cl_float4 color;
    cl_float refraction;
    cl_int textured;
    cl_float transparency;
    cl_int textureId;
    cl_float4 specular;
};

struct Primitive
{
    cl_float4 center;
    // cl_float4 rotation;
    cl_float4 size;
    cl_int type;
    cl_int materialId;
    cl_float materialRatioX;
    cl_float materialRatioY;
};

struct Lamp
{
    cl_float4 center;
    cl_float4 color;
};

class GOL_API OpenCLKernel
{
public:
    OpenCLKernel(int platformId, int device, int nbWorkingItems, int draft);
    ~OpenCLKernel();

public:
    // ---------- Devices ----------
    void initializeDevice(int width, int height);
    void releaseDevice();

    void compileKernels(const KernelSourceType sourceType, const std::string &source, const std::string &ptxFileName,
                        const std::string &options);

public:
    // ---------- Rendering ----------
    void render(const unsigned int width, const unsigned int height, BYTE *bitmap, const float value);

public:
    // ---------- Textures ----------
    void setTexture(int index, BYTE *texture);

    long addTexture(const std::string &filename);

public:
    int getCLPlatformId() { return m_hPlatformId; };
    cl_context getCLContext() { return m_hContext; };
    cl_command_queue getCLQueue() { return m_hQueue; };

private:
    char *loadFromFile(const std::string &, size_t &);

private:
    // OpenCL Objects
    cl_device_id m_hDevices[100];
    int m_hPlatformId;
    cl_context m_hContext;
    cl_command_queue m_hQueue;
    cl_kernel m_hMainKernel;
    cl_uint m_computeUnits;
    cl_uint m_preferredWorkGroupSize;

private:
    // Host
    cl_mem m_hBitmap;
    cl_mem m_hBuffer;
    cl_mem m_hVideo;
    cl_mem m_hDepth;
    cl_mem m_hTextures;
    cl_int m_offset;
    cl_float m_timer;

private:
    BYTE *m_textures;
    bool m_texturedTransfered;
};
