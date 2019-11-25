/* global_def.h  --- inpaintBCT 
 * Copyright (C) 2013 Thomas März (maerz@maths.ox.ac.uk)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//#include "mex.h"
#include <math.h>
//#include <limits>

#define Inf         INFINITY     // defined in math.h of GNU C Lib, but not that of Visual C++ 
// #define Inf         std::numeric_limits<double>::infinity()
#define min(a,b)    ((a)<(b)?(a):(b))
#define max(a,b)    ((a)>(b)?(a):(b))
#define round(a)    (int)((a) + 0.5)
#define sign(a)     ((a) > 0 ? 1 : ((a) < 0 ? -1 : 0))


#define DT_IOP_INPAINT_BCT_THRESH_MIN        0.0
#define DT_IOP_INPAINT_BCT_THRESH_MAX      255.0
#define DT_IOP_INPAINT_BCT_THRESH_STEP       0.1
#define DT_IOP_INPAINT_BCT_THRESH_DEFAULT    0.0


#define DT_IOP_INPAINT_BCT_EPSILON_MIN       1.0
#define DT_IOP_INPAINT_BCT_EPSILON_MAX     100.0
#define DT_IOP_INPAINT_BCT_EPSILON_STEP      0.1
#define DT_IOP_INPAINT_BCT_EPSILON_DEFAULT   5.0

#define DT_IOP_INPAINT_BCT_KAPPA_MIN         0.0
#define DT_IOP_INPAINT_BCT_KAPPA_MAX       100.0
#define DT_IOP_INPAINT_BCT_KAPPA_STEP        0.01
#define DT_IOP_INPAINT_BCT_KAPPA_DEFAULT    25.0

#define DT_IOP_INPAINT_BCT_SIGMA_MIN         0.0
#define DT_IOP_INPAINT_BCT_SIGMA_MAX       100.0
#define DT_IOP_INPAINT_BCT_SIGMA_STEP        0.001
#define DT_IOP_INPAINT_BCT_SIGMA_DEFAULT     1.414213562373095 // sqrt(2.0)

#define DT_IOP_INPAINT_BCT_RHO_MIN           0.001
#define DT_IOP_INPAINT_BCT_RHO_MAX         100.0
#define DT_IOP_INPAINT_BCT_RHO_STEP          0.001
#define DT_IOP_INPAINT_BCT_RHO_DEFAULT       5.0



typedef enum Label {INSIDE, KNOWN, BAND, TO_INPAINT} Label;

typedef struct hItem
{
	double T;
	Label flag;
	int hpos;
	int i;
	int j;
} hItem;

typedef struct Data
{    
  // image info
  int rows;
  int cols;
  int channels;
  int size;
  double *Image;
  double *MImage;
  
  // data domain info
  double *Domain;
  double *MDomain;
  
  // time info
  hItem **heap;
  hItem *Tfield;
  double *ordered_points;
  int nof_points2inpaint;
  
  // parameters
  int radius;
  double epsilon;
  double kappa;
  double sigma;
  double rho;
  double thresh;
  double delta_quant4;
  double *convex;
  
  // smoothing kernels and buffer
  int lenSK1;
  int lenSK2;
  double *SKernel1;
  double *SKernel2;
  double *Shelp;
  
  // inpaint buffer
  double *Ihelp;
  
  // flags
  int ordergiven;
  int guidance;
  int inpaint_undefined;
  
  // extension
  double *GivenGuidanceT;
} Data;

typedef struct Heap  
{
	int size;
	hItem **heap;
  Data *pdata;
} Heap;

// functions in inpainting_func
void InpaintImage(Data *data);


// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
