#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <list>

#include "Rasterizer.h"

Rasterizer::Rasterizer(void)
{
	mFramebuffer = NULL;
	mScanlineLUT = NULL;
}

void Rasterizer::ClearScanlineLUT()
{
	Scanline *pScanline = mScanlineLUT;
	for (int y = 0; y < mHeight; y++)
	{
		(pScanline + y)->clear();
		(pScanline + y)->shrink_to_fit();
	}
}

unsigned int Rasterizer::ComputeOutCode(const Vector2 & p, const ClipRect& clipRect)
{
	unsigned int CENTRE = 0x0;
	unsigned int LEFT = 0x1;
	unsigned int RIGHT = 0x1 << 1;
	unsigned int BOTTOM = 0x1 << 2;
	unsigned int TOP = 0x1 << 3;
	unsigned int outcode = CENTRE;
	
	if (p[0] < clipRect.left)
		outcode |= LEFT;
	else if (p[0] >= clipRect.right)
		outcode |= RIGHT;

	if (p[1] < clipRect.bottom)
		outcode |= BOTTOM;
	else if (p[1] >= clipRect.top)
		outcode |= TOP;

	return outcode;
}

bool Rasterizer::ClipLine(const Vertex2d & v1, const Vertex2d & v2, const ClipRect& clipRect, Vector2 & outP1, Vector2 & outP2)
{
	// I have not implemented this but have had a go at coding how I think it would work
	const Vector2 p1 = v1.position;
	const Vector2 p2 = v2.position;
	unsigned int outcode1 = ComputeOutCode(p1, clipRect);
	unsigned int outcode2 = ComputeOutCode(p2, clipRect);

	outP1 = p1;
	outP2 = p2;
	
	bool draw = false;
	if (outcode1 == 0x0 && outcode2 == 0x0) // If both endpoints are within the window then no clipping is needed and we can draw
	{
		draw = true;
		return false; // Return false that no clipping was needed
	}
	else
	{
		if ((outcode1 && outcode2) != 0x0) // Check wether the regions both contain a 1 in the same place
		{
			return false; // If both endpoints are outside the window then we cannot clip and cannot draw
		}
		else
		{
			// Then we need clipping
			Vector2 pt1 = outP1;
			Vector2 pt2 = outP2;
			bool swap = false;
			bool foundIntersection = false;

			if (pt2[1] < pt1[1]) // Swap the points to iterate one way
			{
				swap = true;
				std::swap(pt1, pt2);
			}

			for (int y = pt1[1]; y < pt2[1]; y++)												// Loop from the smallest Y value to the largest Y value but dont include it
			{
				if (y > 0 && y <= (mHeight - 1))												// Look for within range of Y
				{
					int x = pt1[0] + (y - pt1[1]) * ((pt1[0] - pt2[0]) / (pt1[1] - pt2[1]));	// Calculate the X value

					if (x > 0 || x <= (mWidth - 1))												// Look for within rage of X
					{
						if (!foundIntersection)													// If we have not found the first intersection
						{
							// This will calculate only once
							foundIntersection = true;
							if (!swap) // Set new values of X and Y to the respective outPoint
							{
								outP1[0] = x;
								outP1[1] = y;
							}
							else
							{
								outP2[0] = x;
								outP2[1] = y;
							}
						}
						else 
						{							
							// This will keep reapplying until it cannot meet the bounds for either X or Y
							if (!swap)  // Set new values of X and Y to the respective outPoint
							{
								outP2[0] = x;
								outP2[1] = y;
							}
							else
							{
								outP1[0] = x;
								outP1[1] = y;
							}

							if (!draw)  // If we havn't set our ability to draw the line then set it as we have now found a second clipped point
								draw = true;
						}
					}
				}
			}
		}
	}

	return true; // Return true that the line has been clipped
}

void Rasterizer::WriteRGBAToFramebuffer(int x, int y, const Colour4 & colour)
{
	if ((x > 0 && x <= (mWidth - 1)) && (y > 0 && y <= (mHeight - 1))) // Added the bound restrictions for the frame buffer
	{
		PixelRGBA *pixel = mFramebuffer->GetBuffer();
		pixel[y*mWidth + x] = colour;
	}
}

Rasterizer::Rasterizer(int width, int height)
{
	// Initialise the rasterizer to its initial state
	mFramebuffer = new Framebuffer(width, height);
	mScanlineLUT = new Scanline[height];
	mWidth = width;
	mHeight = height;

	mBGColour.SetVector(0.0, 0.0, 0.0, 1.0);	//default bg colour is black
	mFGColour.SetVector(1.0, 1.0, 1.0, 1.0);    //default fg colour is white

	mGeometryMode = LINE;
	mFillMode = UNFILLED;
	mBlendMode = NO_BLEND;

	SetClipRectangle(0, mWidth, 0, mHeight);
}

Rasterizer::~Rasterizer()
{
	delete mFramebuffer;
	delete[] mScanlineLUT;
}

void Rasterizer::Clear(const Colour4& colour)
{
	PixelRGBA *pixel = mFramebuffer->GetBuffer();
	SetBGColour(colour);
	int size = mWidth*mHeight;
	for(int i = 0; i < size; i++)
	{
		// Fill all pixels in the framebuffer with background colour
		*(pixel + i) = mBGColour;
	}
}

void Rasterizer::DrawPoint2D(const Vector2& pt, int size)
{
	int x = pt[0];
	int y = pt[1];
	WriteRGBAToFramebuffer(x, y, mFGColour); // Write the main line to the buffer
}

Colour4 Rasterizer::LerpColour(const Colour4 start, const Colour4 end, const int step, const int lastStep, bool alpha) 
{
	float r = (end[0] - start[0]) * step / lastStep + start[0]; // Lerp r
	float g = (end[1] - start[1]) * step / lastStep + start[1]; // Lerp b
	float b = (end[2] - start[2]) * step / lastStep + start[2]; // Lerp g
	return (alpha) ? Colour4(r, g, b, (end[3] - start[3]) * step / lastStep + start[3]) : Colour4(r, g, b); // Give external option to return Lerp a (not needed in this assignment)
}

Colour4 Rasterizer::AlphaBlend(const Colour4 colour, const int x, const int y)
{
	if ((x > 0 && x <= (mWidth - 1)) && (y > 0 && y <= (mHeight - 1))) // Only alpha blend within the bounds to prevent further redundant calculations
	{
		int position = y * mFramebuffer->GetWidth() + x;						// Get the index of the disered point in the frame buffer
		Colour4 pColour = mFramebuffer->GetBuffer()[position];					// Obtain the current colour that exists at the point
		Colour4 newColour = (colour *  colour[3]) + pColour * (1 - colour[3]);	// Calculate new colour using the alpha
		return newColour;
	}

	return colour; // Return the colour if it did not meet the bounds requirement
}


void Rasterizer::DrawLine2D(const Vertex2d & v1, const Vertex2d & v2, int thickness)
{
	Vector2 pt1 = v1.position;
	Vector2 pt2 = v2.position;

	int dx = pt2[0] - pt1[0];
	int dy = pt2[1] - pt1[1];
	bool swap = (abs(dx) <= abs(dy)); // Do we need to flip the differential values?

	// Set the increments for the different octants
	int dx1 = DifferentialIncrement(dx);
	int dy1 = DifferentialIncrement(dy);
	int dx2 = swap ? 0 : dx1;
	int dy2 = swap ? dy1 : 0;

	// Use height and width based decisions from the dy and dx
	int deltaLarge = swap ? abs(dy) : abs(dx); // Longest differential
	int deltaSmall = swap ? abs(dx) : abs(dy); // Smallest differential 
	
	int epsilon = deltaLarge >> 1;			// Check whether n == half of l (avoid round off)
	for (int i = 0; i <= deltaLarge; i++)	// For every point on this line
	{
		Vector2 temp(pt1[0], pt1[1]);
		Colour4 colour = (mFillMode == INTERPOLATED_FILLED) ? LerpColour(v1.colour, v2.colour, i, deltaLarge, false) : v1.colour;	// Grab the colour respective to interpolation between the two disired points
		SetFGColour((mBlendMode == ALPHA_BLEND) ? AlphaBlend(colour, pt1[0], pt1[1]) : colour);										// Set the Foreground colour respective to alpha-blending on this pixel

		if (thickness > 1)	// Check wether a larger than default thickness is requested
		{
			int maxOffset = thickness / 2;												// Use the default rounding to int
			int start	= (swap) ? temp[0] - maxOffset : temp[1] - maxOffset;			// Set the smallest index for iteration resepctive of swapping the points due to large angle
			int end		= (swap) ? temp[0] + maxOffset : temp[1] + maxOffset;			// Set the largest index for iteration resepctive of swapping the points due to large angle
			for (int offset = 1; offset < maxOffset; offset++)
				for (int j = start; j < end; j++)
					DrawPoint2D((swap) ? Vector2(j, temp[1]) : Vector2(temp[0], j));	// Draw the point resepctive of swapping the points due to large angle
		}
		else
		{
			DrawPoint2D(temp); // Otherwuise simply draw the point here
		}

		epsilon += deltaSmall;							// Increase epsilon by the checked dy
		pt1[0] += (epsilon >= deltaLarge) ? dx1 : dx2;	// Increase X by calculated dx for the current iteration
		pt1[1] += (epsilon >= deltaLarge) ? dy1 : dy2;	// Increase y by calculated dy for the current iteration
		if (epsilon >= deltaLarge)						// When epsilon becomes greater than width then we subtract the width but continue to increase the point
			epsilon -= deltaLarge;
		
	}
}

void Rasterizer::DrawUnfilledPolygon2D(const Vertex2d * vertices, int count)
{
	// Use Test 3 (Press F3) to test solution.
	for (int i = 0; i < count; i++)
		DrawLine2D(vertices[i], vertices[(i < count - 1) ? (i + 1): 0]); // Draw a line between each counter-clockwise pair of polygon verticies handling the last and first index together

}


void Rasterizer::ScanlineFillPolygon2D(const Vertex2d * vertices, int count)
{
	// Use Test 6 (Press F6) to test solution

	ClearScanlineLUT();						// Clear the Scanlines before filling a new polygon
	ScanClip polygonClip { mHeight, 0 };	// Create a Y axis clipping plane the size of the screen
	for (int i = 0; i < count; i++)
	{
		int v2Index = (i + 1) % count;		// Create next index so we dont have to keep calculating it
		Vector2 pt1 = vertices[i].position;
		Vector2 pt2 = vertices[v2Index].position;

		if (pt1[1] < polygonClip.lowest)	// Clip scan to the smallest polygon vertex Y value
			polygonClip.lowest = (pt1[1] < 0) ? 0 : pt1[1];
		
		if (pt1[1] > polygonClip.highest)	// Clip scan the largest polygon vertex Y value
			polygonClip.highest = (pt1[1] > (mHeight - 1)) ? (mHeight - 1) : pt1[1];

		Colour4 colour1 = (pt2[1] < pt1[1]) ? vertices[v2Index].colour : vertices[i].colour; // Obtain Colour 1 respective of swapping
		Colour4 colour2 = (pt2[1] < pt1[1]) ? vertices[i].colour : vertices[v2Index].colour; // Obtain Colour 2 respective of swapping

		if (pt2[1] < pt1[1]) // Swap the points to iterate one way
			std::swap(pt1, pt2);
			
		for (int y = pt1[1]; y < pt2[1]; y++)	// Loop from the smallest Y value to the largest Y value but DONT include it
		{
			if (y > 0 && y < (mHeight - 1))		// Only store scans within the window
			{
				int x = pt1[0] + (y - pt1[1]) * ((pt1[0] - pt2[0]) / (pt1[1] - pt2[1]));																// Calculate the X value
				Colour4 colour = (mFillMode == INTERPOLATED_FILLED) ? LerpColour(colour1, colour2, (y - pt1[1]), (pt2[1] - pt1[1]), false) : colour1;	// Grab the colour depending on interpolation
				ScanlineLUTItem item{ colour, x };
				mScanlineLUT[y].push_back(item);
			}
		}
	}

	for (int y = polygonClip.lowest; y < polygonClip.highest; y++) // Loop from the bottom to the top of our polygon clip
	{
		if (!mScanlineLUT[y].empty())	// Check the scanline isn't empty
		{
			int lengthLUT = mScanlineLUT[y].size();
			if (lengthLUT % 2 == 0)		// Check we have an even number of LUT items
			{
				if (lengthLUT > 1)
					std::sort(mScanlineLUT[y].begin(), mScanlineLUT[y].end(), [](const ScanlineLUTItem& a, const ScanlineLUTItem& b) { return a.pos_x < b.pos_x; }); // Sort the LUT Items in ascending order by X position

				for (int i = 0; i < lengthLUT; i += 2) // Iterate in pairs and draw a line between of the created vertices
				{
					Vertex2d v1 = { mScanlineLUT[y][i].colour,		Vector2(mScanlineLUT[y][i].pos_x, y) };
					Vertex2d v2 = { mScanlineLUT[y][i + 1].colour,	Vector2(mScanlineLUT[y][i + 1].pos_x, y) };
					DrawLine2D(v1, v2);
				}
			}
		}
	}
}

void Rasterizer::ScanlineInterpolatedFillPolygon2D(const Vertex2d * vertices, int count)
{
	//Use Test 7 to test solution
	ScanlineFillPolygon2D(vertices, count); // The same implementation handles the Interpolated Fill
}

void Rasterizer::DrawCircle2D(const Circle2D & inCircle, bool filled)
{
	//Use Test 8 to test solution
	SetFGColour(inCircle.colour);
	float rSqrd = inCircle.radius * inCircle.radius;											// Create a varaible for the radius squared
	for (int y = -inCircle.radius; y <= inCircle.radius; y++)
	{
		for (int x = -inCircle.radius; x <= inCircle.radius; x++)
		{
			float mSqrd = x * x + y * y;														// Create a varaible for the magnitude squared
			if ((filled) ? mSqrd <= rSqrd : (mSqrd > CIRCLE_BOUND * rSqrd && mSqrd <= rSqrd))	// Check the magnitude squared against the radius squared with respect to filling and the circumference boundary	
				DrawPoint2D(Vector2(inCircle.centre[0] + x, inCircle.centre[1] + y));			// Draw the point in addition from the centre
		}
	}
}

Framebuffer *Rasterizer::GetFrameBuffer() const
{
	return mFramebuffer;
}
