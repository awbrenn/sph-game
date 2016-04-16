//
// Created by awbrenn on 1/20/16.
//
#include <cmath>
#include "CFDSolver.h"
#include "CFDUtility.h"
#include "iostream"


CFDSolver::CFDSolver(const int nx, const int ny, const float dx, const float Dt, int Nloops, int Oploops)
{
  Nx = nx;
  Ny = ny;
  Dx = dx;
  dt = Dt;
  brush_size = 11;
  source_brush = new float*[brush_size];
  for (int i = 0; i < brush_size; ++i) {source_brush[i] = new float[brush_size]; }
  InitializeBrushes();
  nloops = Nloops;
  oploops = Oploops;
  gravityX = 9.8f;
  gravityY = 0.0f;
  density1 = new float[Nx*Ny]();
  density2 = new float[Nx*Ny]();
  velocity1 = new float[Nx*Ny*2]();
  velocity2 = new float[Nx*Ny*2]();
  color1 = new float[Nx*Ny*3]();
  color2 = new float[Nx*Ny*3]();
  divergence = new float[Nx*Ny]();
  pressure = new float[Nx*Ny]();
  densitySourceField = 0;
  colorSourceField = 0;
  divergenceSourceField = 0;
}


CFDSolver::~CFDSolver()
{
  delete density1;
  delete density2;
  delete velocity1;
  delete velocity2;
  delete color1;
  delete color2;
  delete divergence;
  delete pressure;
}


const float CFDSolver::getDensity(int i, int j)
{
  if (i < Nx && i >=0 && j < Ny && j >= 0)
    return density1[dIndex(i,j)];
  else
    return 0.0;
}


const float CFDSolver::getVelocity(int i, int j, int c)
{
  if (i < Nx && i >=0 && j < Ny && j >= 0)
    return velocity1[vIndex(i,j,c)];
  else
    return 0.0;
}


const float CFDSolver::getColor(int i, int j, int c)
{
  if (i < Nx && i >=0 && j < Ny && j >= 0)
    return color1[cIndex(i,j,c)];
  else
    return 0.0;
}


const float CFDSolver::getPressure(int i, int j)
{
  if (i < Nx && i >=0 && j < Ny && j >= 0)
    return pressure[pIndex(i,j)];
  else
    return 0.0;
}


const float CFDSolver::getDivergence(int i, int j)
{
  if (i < Nx && i >=0 && j < Ny && j >= 0)
    return divergence[dIndex(i,j)];
  else
    return 0.0;
}


const float CFDSolver::InterpolateDensity(int i, int j, float w1, float w2, float w3, float w4)
{
  return getDensity(i    , j)     * w1 +
         getDensity(i + 1, j)     * w2 +
         getDensity(i    , j + 1) * w3 +
         getDensity(i + 1, j + 1) * w4 ;
}


const float CFDSolver::InterpolateVelocity(int i, int j, int c, float w1, float w2, float w3, float w4)
{
  return getVelocity(i    , j,     c) * w1 +
         getVelocity(i + 1, j,     c) * w2 +
         getVelocity(i,     j + 1, c) * w3 +
         getVelocity(i + 1, j + 1, c) * w4;
}


const float CFDSolver::InterpolateColor(int i, int j, int c, float w1, float w2, float w3, float w4)
{
  return getColor(i    , j,     c) * w1 +
         getColor(i + 1, j,     c) * w2 +
         getColor(i,     j + 1, c) * w3 +
         getColor(i + 1, j + 1, c) * w4;
}


void CFDSolver::bilinearlyInterpolate(const int ii, const int jj, const float x, const float y)
{
  // get index of sample
  const int i = (int) (x/Dx);
  const int j = (int) (y/Dx);

  // get weights of samples
  const float ax = std::abs(x/Dx - i);
  const float ay = std::abs(y/Dx - j);
  const float w1 = (1-ax) * (1-ay);
  const float w2 = ax * (1-ay);
  const float w3 = (1-ax) * ay;
  const float w4 = ax * ay;

  density2[dIndex(ii, jj)] = InterpolateDensity(i, j, w1, w2, w3, w4);

  velocity2[vIndex(ii, jj, 0)] = InterpolateVelocity(i, j, 0, w1, w2, w3, w4);
  velocity2[vIndex(ii, jj, 1)] = InterpolateVelocity(i, j, 1, w1, w2, w3, w4);

  color2[cIndex(ii, jj, 0)] = InterpolateColor(i, j, 0, w1, w2, w3, w4);
  color2[cIndex(ii, jj, 1)] = InterpolateColor(i, j, 1, w1, w2, w3, w4);
  color2[cIndex(ii, jj, 2)] = InterpolateColor(i, j, 2, w1, w2, w3, w4);
}


void CFDSolver::advect()
{
  float x, y;

  // advect each grid point
  for (int j=0; j<Ny; ++j)
  {
    for (int i=0; i<Nx; ++i)
    {
      x = i*Dx - velocity1[vIndex(i,j,0)]*dt;
      y = j*Dx - velocity1[vIndex(i,j,1)]*dt;
      bilinearlyInterpolate(i, j, x, y);
    }
  }

  swapFloatPointers(&density1, &density2);
  swapFloatPointers(&velocity1, &velocity2);
  swapFloatPointers(&color1, &color2);
}


void CFDSolver::addSourceColor()
{
  if (colorSourceField != 0)
  {
    for (int j=0; j<Ny; ++j)
    {
      for (int i=0; i<Nx; ++i)
      {
        color1[cIndex(i,j,0)] += colorSourceField[cIndex(i,j,0)];
        color1[cIndex(i,j,1)] += colorSourceField[cIndex(i,j,1)];
        color1[cIndex(i,j,2)] += colorSourceField[cIndex(i,j,2)];

        // clamp color values to 1.0f
        if (color1[cIndex(i,j,0)] > 1.0f)
          color1[cIndex(i,j,0)] = 1.0f;

        if (color1[cIndex(i,j,1)] > 1.0f)
          color1[cIndex(i,j,1)] = 1.0f;

        if (color1[cIndex(i,j,2)] > 1.0f)
          color1[cIndex(i,j,2)] = 1.0f;
      }
    }
    // re-initialize colorSourceField
    Initialize(colorSourceField, Nx*Ny*3, 0.0);
    colorSourceField = 0;
  }
}


void CFDSolver::addSourceDensity()
{
  if (densitySourceField != 0)
  {
    for (int j=0; j<Ny; ++j)
    {
      for (int i=0; i<Nx; ++i)
      {
        density1[dIndex(i,j)] += densitySourceField[dIndex(i,j)];
      }
    }
    // re-initialize densitySourceField
    Initialize(densitySourceField, Nx*Ny, 0.0);
    densitySourceField = 0;
  }
}


void CFDSolver::computeVelocity(float force_x, float force_y)
{
  for (int j=0; j<Ny; ++j)
  {
    for (int i=0; i<Nx; ++i)
    {
      velocity1[vIndex(i,j,0)] += (force_x * density1[dIndex(i,j)]*dt);
      velocity1[vIndex(i,j,1)] += (force_y * density1[dIndex(i,j)]*dt);
    }
  }
}


void CFDSolver::computeDivergence()
{
  int index;
  for (int j = 0; j < Ny; ++j)
  {
    for (int i = 0; i < Nx; ++i)
    {
      index = dIndex(i,j);
      divergence[index] = (getVelocity(i+1, j,   0) -
                                 getVelocity(i-1, j,   0)) / (2*Dx) +
                                (getVelocity(i,   j+1, 1) -
                                 getVelocity(i,   j-1, 1)) / (2*Dx);

      if (divergenceSourceField != 0)
        divergence[index] += divergenceSourceField[index];
    }
  }
  if (divergenceSourceField != 0) {
    // re-initialize colorSourceField
    Initialize(divergenceSourceField, Nx * Ny, 0.0);
    divergenceSourceField = 0;
  }

}


void CFDSolver::computePressure()
{
  Initialize(pressure, Nx*Ny, 0.0);

  for(int k = 0; k < nloops; ++k)
  {
    for (int j = 0; j < Ny; ++j)
    {
      for (int i = 0; i < Nx; ++i)
      {
        pressure[pIndex(i,j)] = ((getPressure(i+1, j)     +
                                   getPressure(i-1, j)    +
                                   getPressure(i,   j+1)  +
                                   getPressure(i,   j-1)) *
                                   0.25f) - ((Dx*Dx/4.0f) * getDivergence(i,j));
      }
    }
  }
}


void CFDSolver::computePressureForces(int i, int j, float* force_x, float* force_y)
{
  *force_x = (getPressure(i+1, j) - getPressure(i-1, j)) / (2*Dx);
  *force_y = (getPressure(i, j+1) - getPressure(i, j-1)) / (2*Dx);
}


void CFDSolver::computeVelocityBasedOnPressureForces()
{
  float force_x, force_y;

  for (int j = 0; j < Ny; ++j)
  {
    for (int i = 0; i < Nx; ++i)
    {
      computePressureForces(i, j, &force_x, &force_y);
      velocity1[vIndex(i,j,0)] -= force_x;
      velocity1[vIndex(i,j,1)] -= force_y;
    }
  }
}

void CFDSolver::sources()
{
  // compute sources
  computeVelocity(gravityX, gravityY);

  for (int i = 0; i < oploops; ++i)
  {
    computeDivergence();
    computePressure();
    computeVelocityBasedOnPressureForces();
  }
}


void CFDSolver::InitializeBrushes()
{
  int brush_width = (brush_size-1)/2;
  for( int j=-brush_width;j<=brush_width;j++ )
  {
    int jj = j + brush_width;
    float jfactor =  (float(brush_width) - (float)fabs(j) )/float(brush_width);
    for( int i=-brush_width;i<=brush_width;i++ )
    {
      int ii = i + brush_width;
      float ifactor =  (float(brush_width) - (float)fabs(i) )/float(brush_width);
      float radius = (float) ((jfactor * jfactor + ifactor * ifactor) / 2.0);
      source_brush[ii][jj] = powf(radius,0.5);
    }
  }
}



void CFDSolver::addSource( int x, int y, paint_mode mode) {
  int brush_width = (brush_size - 1) / 2;
  int xstart = x - brush_width;
  int ystart = y - brush_width;
  if (xstart < 0) { xstart = 0; }
  if (ystart < 0) { ystart = 0; }

  int xend = x + brush_width;
  int yend = y + brush_width;
  if (xend >= Nx) { xend = Nx - 1; }
  if (yend >= Ny) { yend = Ny - 1; }

  if (mode == PAINT_SOURCE) {
    for (int ix = xstart; ix <= xend; ix++) {
      for (int iy = ystart; iy <= yend; iy++) {
        int index = ix + Nx * (Ny - iy - 1);
        color1[3 * index] += source_brush[ix - xstart][iy - ystart];
        color1[3 * index + 1] += source_brush[ix - xstart][iy - ystart];
        color1[3 * index + 2] += source_brush[ix - xstart][iy - ystart];
        density1[index] += source_brush[ix - xstart][iy - ystart];
      }
    }
  }
}