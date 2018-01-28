#include "Framebuffer.h"

Framebuffer::Framebuffer()
{
	mWidth = 0;
	mHeight = 0;
	mColourBuffer = NULL;
}

Framebuffer::Framebuffer(int width, int height)
{
	InitFramebuffer(width, height);
}

Framebuffer::~Framebuffer()
{
	delete[] mColourBuffer;
}

void Framebuffer::InitFramebuffer(int width, int height)
{
	int size = width*height;
	mWidth = width;
	mHeight = height;

	mColourBuffer = new PixelRGBA[size];

	//memset(mColourBuffer, 0, size*sizeof(PixelRGBA));
}
