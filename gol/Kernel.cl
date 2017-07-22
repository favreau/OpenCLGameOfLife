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

// Textures
__constant int gTextureWidth  = 1920;
__constant int gTextureHeight = 1080;
__constant int gTextureDepth  = 3;

__constant int gVideoColor  = 4;
__constant int gVideoWidth  = 640;
__constant int gVideoHeight = 480;

__constant int gDepthColor  = 2;
__constant int gDepthWidth  = 320;
__constant int gDepthHeight = 240;

__constant int   gStep = 1;

int pixelPower( float4 pixel, float limit )
{
	return( ((pixel.x+pixel.y+pixel.z)/3.f)>limit ) ? 0 : 1;
}


// ________________________________________________________________________________
void makeOpenGLColor( 
	float4         color, 
	__global char* bitmap, 
	int            index)
{
	int mdc_index = index*4; 

	color.x = (color.x>1.f) ? 1.f : color.x;
	color.y = (color.y>1.f) ? 1.f : color.y; 
	color.z = (color.z>1.f) ? 1.f : color.z;
	color.w = (color.w>1.f) ? 1.f : color.w;

	color.x = (color.x<0.f) ? 0.f : color.x;
	color.y = (color.y<0.f) ? 0.f : color.y; 
	color.z = (color.z<0.f) ? 0.f : color.z;
	color.w = (color.w<0.f) ? 0.f : color.w;

	unsigned char r = color.w*color.x*256.f;
	unsigned char g = color.w*color.y*256.f;
	unsigned char b = color.w*color.z*256.f;
	unsigned char a = color.w*256.f;

	bitmap[mdc_index  ] = r; // Red
	bitmap[mdc_index+1] = g; // Green
	bitmap[mdc_index+2] = b; // Blue
	bitmap[mdc_index+3] = a; // Alpha
}

void gameOfLife(
	int              x,
	int              y,
	int              width,
	int              height,
	__global char*   bitmap,
	__global float4* buffer,
	__global char*   video,
	__global char*   depth,
	__global char*   textures,
	int              offset,
	float            limit,
	float            timer)
{
	int index = y*width+x;

	float4 black = 0;
	float4 bitmapColor;
	bitmapColor.x = ((unsigned char)textures[index*gTextureDepth+0])/256.f;
	bitmapColor.y = ((unsigned char)textures[index*gTextureDepth+1])/256.f;
	bitmapColor.z = ((unsigned char)textures[index*gTextureDepth+2])/256.f;
	bitmapColor.w = 1.f;

	int outputSize = height*width;

	if( offset == -1 ) 
	{
		buffer[index] = black;
		buffer[index+outputSize] = bitmapColor;
		makeOpenGLColor( bitmapColor, bitmap, index ); 
	}
	else
	{
		if( x>gStep && x<width-gStep && y>gStep && y<height-gStep ) 
		{
			int offsetIndex =    ( offset == 0 ) ? 0 : outputSize;
			int notOffsetIndex = ( offset == 0 ) ? outputSize : 0;

			makeOpenGLColor( buffer[offsetIndex+index], bitmap, index ); 

			int indexTop         = (y-gStep)*width + x;
			int indexTopRight    = (y-gStep)*width + x+gStep;
			int indexRight       = y*width         + x+gStep;
			int indexBottomRight = (y+gStep)*width + x+gStep;
			int indexBottom      = (y+gStep)*width + x;
			int indexBottomLeft  = (y+gStep)*width + x-gStep;
			int indexLeft        = y*width         + x-gStep;
			int indexTopLeft     = (y-gStep)*width + x-gStep;

			int sum = 0;

			sum = sum + pixelPower(buffer[offsetIndex+indexTop],limit);
			sum = sum + pixelPower(buffer[offsetIndex+indexTopRight],limit);
			sum = sum + pixelPower(buffer[offsetIndex+indexRight],limit);
			sum = sum + pixelPower(buffer[offsetIndex+indexBottomRight],limit);
			sum = sum + pixelPower(buffer[offsetIndex+indexBottom],limit);
			sum = sum + pixelPower(buffer[offsetIndex+indexBottomLeft],limit);
			sum = sum + pixelPower(buffer[offsetIndex+indexLeft],limit);
			sum = sum + pixelPower(buffer[offsetIndex+indexTopLeft],limit);

			if( sum < 1 ) 
			{
				// dying
				buffer[notOffsetIndex+index] = buffer[offsetIndex+index];
				buffer[notOffsetIndex+index].w -= 0.002f; 
				if( buffer[notOffsetIndex+index].w <= 0.f ) 
				{
					buffer[notOffsetIndex] = black;
				}
			}
			else
			{
				if( sum > 7 ) 
				{
					// dead
					buffer[notOffsetIndex+index] = black;
				}
				else 
				{
					// alive
					buffer[notOffsetIndex+index] = bitmapColor;
				}
			}
		}
	}
}

void average(
	int              x,
	int              y,
	int              width,
	int              height,
	__global char*   bitmap,
	__global float4* buffer,
	__global char*   video,
	__global char*   depth,
	__global char*   textures,
	int              offset,
	float            limit,
	float            timer)
{
	int index = y*width+x;

	float4 black = 0;
	float4 bitmapColor;
	bitmapColor.x = ((unsigned char)textures[index*gTextureDepth+0])/256.f;
	bitmapColor.y = ((unsigned char)textures[index*gTextureDepth+1])/256.f;
	bitmapColor.z = ((unsigned char)textures[index*gTextureDepth+2])/256.f;
	bitmapColor.w = 1.f;

	int outputSize = height*width;

	if( offset == -1 ) 
	{
		// Initialization
		buffer[index] = bitmapColor;
		buffer[index+outputSize] = bitmapColor;
		makeOpenGLColor( bitmapColor, bitmap, index ); 
	}
	else
	{
		int offsetIndex =    ( offset == 0 ) ? 0 : outputSize;
		int notOffsetIndex = ( offset == 0 ) ? outputSize : 0;

		if( x>gStep && x<width-gStep && y>gStep && y<height-gStep ) 
		{
			makeOpenGLColor( buffer[offsetIndex+index], bitmap, index ); 

			int indexTop         = (y-gStep)*width + x;
			int indexTopRight    = (y-gStep)*width + x+gStep;
			int indexRight       = y*width         + x+gStep;
			int indexBottomRight = (y+gStep)*width + x+gStep;
			int indexBottom      = (y+gStep)*width + x;
			int indexBottomLeft  = (y+gStep)*width + x-gStep;
			int indexLeft        = y*width         + x-gStep;
			int indexTopLeft     = (y-gStep)*width + x-gStep;

			float4 sum = buffer[offsetIndex+indexTop];
			sum += buffer[offsetIndex+indexTopRight];
			sum += buffer[offsetIndex+indexRight];
			sum += buffer[offsetIndex+indexBottomRight];
			sum += buffer[offsetIndex+indexBottom];
			sum += buffer[offsetIndex+indexBottomLeft];
			sum += buffer[offsetIndex+indexLeft];
			sum += buffer[offsetIndex+indexTopLeft];

			sum /= 8.f;
			buffer[offsetIndex+index] = sum;
		}
		else
		{
			buffer[notOffsetIndex+index] = black;
			buffer[offsetIndex+index] = black;
		}
	}
}

/**
* ________________________________________________________________________________
* Main Kernel!!!
* ________________________________________________________________________________
*/
__kernel void main_kernel(
	int              width,
	int              height,
	__global char*   bitmap,
	__global float4* buffer,
	__global char*   video,
	__global char*   depth,
	__global char*   textures,
	int              offset,
	float            limit,
	float            timer)
{
	gameOfLife( get_global_id(0), get_global_id(1), width, height, bitmap, buffer, video, depth, textures, offset, limit, timer );
	//average( get_global_id(0), get_global_id(1), width, height, bitmap, buffer, video, depth, textures, offset, limit, timer );
}
