//     calcroom.C - fv10k1 package
//     Computes room reflection paths
//         
//     Copyright (C) 2001 Oleg Smirnov <smirnov@astron.nl>
//    
//     This program is free software; you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation; either version 2 of the License, or
//     (at your option) any later version.
// 
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
// 
//     You should have received a copy of the GNU General Public License
//     along with this program; if not, write to the Free Software
//     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Id: calcroom.C,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

const double RADEG = 180/M_PI,
             DEGRA = 1/RADEG;
const double vsound = 330;  // speed of sound
        
// room parameters and default values

double 
// room dimensions
    zsize = 5,
    xsize = zsize*1.60,
    ysize = zsize*2.33,
    
// source position
    xs1 = xsize/2-2,
    ys = 2,
    zs = 2,
// listener's position
    yl0 = ysize-2,
    zl0 = 2,
    
// wall absorption coeffs
    ax = .2,    // side walls
    ay1 = .2,   // front wall
    ay2 = .2,   // rear wall
    az1 = .5,   // floor
    az2 = .3;   // ceiling

double nrefl = 20;
double mindamp = 0.00005;
double maxdly = 80;      // 2*80 msec is all that fits into internal TRAM...
double maxpaths = 40;

// small math functions
double sqr( double x )
{
  return x*x;
}
    
double dist(double x1,double y1,double z1,double x2,double y2,double z2 )
{
  return sqrt(sqr(x1-x2)+sqr(y1-y2)+sqr(z1-z2));
}

// the path structure stores computed paths
typedef struct Path {
  char chan;
  int ix,iy,iz;
  double length,delay,damp,angle,fl,fr,rl,rr;
  int order;
} Path;

// comparison function for qsorting paths
int comparePaths( const void *p1,const void *p2 )
{
  double d = ((Path*)p1)->delay - ((Path*)p2)->delay;
  return d<0 ? -1 : ( d>0 ? 1 : 0);
}

const int MAXPATHS = 1000;

Path paths[MAXPATHS];

// Given apparent source direction (as dx,dy,ix,iy), computes a bearing 
// to the signal (relative to listener), and directional coeffs for 
// four speakers. The coeffs are my very naive attempt to reproduce ASD
// (apparent sound direction). The two speakers closest to the ASD
// are assigned coeffs of sin^2(a0) and cos^2(a0) (where a0 is the angle 
// relative to one of the speakers -- note that the sum of the coeffs is 
// always 1, and as the ASD gets closer to one of the speakers, its coeff
// goes to 1). This of course assumes speakers are due exactly to
// NW, NE, SW and SE of the listener.
// I deeply suspect that simple coeffs like this are pure bullshit, but 
// the proper way to do it would involve head transfer functions and other 
// crap that I don't feel like getting into just now...
// 
double computeAngle( double dx,double dy,int ix,int iy,
                      double &fl,double &fr,double &rl,double &rr)
{
  // apparent direction of signal for listener (0 is due right)
  double angle = atan2(dy*pow(-1,iy),dx*pow(-1,ix))*RADEG;
  double normang = angle + 360*(angle<0);
  // direction relative to FL speaker, normalized to 0-360
  double alr = normang-135;
  if( alr<0 ) alr+=360;
  // angle in sector between two active speakers
  double a0 = (alr-90*(int)(alr/90))*DEGRA;
  double c1 = sqr(cos(a0)), c2=1-c1;
  // figure out speaker coeffs
  fl=fr=rl=rr=0;
  if( alr<90 )
  { fl=c1; rl=c2; }
  else if( alr<180 )
  { rl=c1; rr=c2; }
  else if( alr<270 )
  { rr=c1; fr=c2; }
  else
  { fr=c1; fl=c2; }
  
  return angle;
}
    
int main( int argc,char *argv[] )
{ 
  zsize = 0;
  char nameout[256]="room.rp";
//  scan input parameters
  if( argc>1 )
  {
    FILE *f = fopen(argv[1],"rt");
    if( !f )
    {
      perror("Error opening input file");
      return 1;
    }
    char s[256]="";
    while( fgets(s,sizeof(s),f) )
    {
      char id[256];
      double val;
      if( sscanf(s,"%s %lf",id,&val)==2 )
      {
        #define MAPPARM(idd,var) else if( !strcasecmp(id,#idd) ) { \
              var=val; printf("     read %s: %f\n",#idd,val); \
            }
        if( 0 )
          return 0;
        MAPPARM(xsize,xsize)
        MAPPARM(ysize,ysize)
        MAPPARM(zsize,zsize)
        MAPPARM(xsrc,xs1)
        MAPPARM(ysrc,ys)
        MAPPARM(yl,yl0)
        MAPPARM(zsrc,zs)
        MAPPARM(zl,zl0)
        MAPPARM(aside,ax)
        MAPPARM(afront,ay1)
        MAPPARM(arear,ay2)
        MAPPARM(afloor,az1)
        MAPPARM(aceil,az2)
        MAPPARM(nr,nrefl)
        MAPPARM(mindamp,mindamp)
        MAPPARM(maxdly,maxdly)
        MAPPARM(maxpaths,maxpaths)
      }
      s[0]=0;
    }
    fclose(f);
    sprintf(nameout,"%s.rp",argv[1]);
  }
// set dependent parameters
  double xs2 = xsize-xs1;
  double xl0 = xsize/2;
  double r0 = dist(xs1,ys,zs,xl0,yl0,zl0);
// print everything
#define PRINTPARM(id,var,desc) printf("  %s: %f   " desc "\n",#id,var);
  printf("Room parameters:\n");
  PRINTPARM(xsize,xsize,"room width (X)");
  PRINTPARM(ysize,ysize,"room length (Y)");
  PRINTPARM(zsize,zsize,"room height (Z) - use 0 for 2D model");
  PRINTPARM(xsrc,xs1,"left source X position (right is symmetric)");
  PRINTPARM(ysrc,ys,"source Y position");
  PRINTPARM(yl,yl0,"listener Y position");
  PRINTPARM(zsrc,zs,"source Z position");
  PRINTPARM(zl,zl0,"listener Z position");
  PRINTPARM(ax,ax,"side wall dampening");
  PRINTPARM(ay1,ay1,"front wall dampening");
  PRINTPARM(ay2,ay2,"rear wall dampening");
  PRINTPARM(az1,az1,"floor dampening");
  PRINTPARM(az2,az2,"ceiling dampening");
  PRINTPARM(nr,nrefl,"max reflections computed");
  PRINTPARM(mindamp,mindamp,"dampening cut-off");
  PRINTPARM(maxdly,maxdly,"delay cut-off (in msec)");
  PRINTPARM(maxpaths,maxpaths,"max number of paths");
  FILE *fout = fopen(nameout,"wt");
  printf("Listener X position is %5.2fm\n",xl0);
  printf("Direct pathlength is %5.2fm\n",r0);
  int npath = 0;

    // if zsize=0, use 2D model 
  int iz0 = -(int)nrefl, iz1 = (int)nrefl;
  if( !zsize )
    iz0=iz1=0;
  // The algorithm works by considering a series of adjacent "virtual rooms"
  // to the west, south, top and bottom of the original room. A direct path from 
  // the source in the original room to a listener in one of the virtual rooms 
  // is equivalent to a reflected path, with the appropriate dampening and 
  // phase inversions taken into account.
  // Two simple assumptions make life somewhat easier:
  // (1) The original signal only travels "south" from source to listener.
  // (2) X(listener) is at room center, the sources' X(R) and X(L)
  //     are symmetrical to room center, and Y(R)=Y(L).
  //     What this means is that paths to the "east" from source L are the 
  //     inverse of paths "west" from source R, and vice versa. So we only 
  //     compute half the actual paths (the ones to the west), and the DSP 
  //     code can easily fill in the other half.
  for( int ix=0; ix<=nrefl && npath<=MAXPATHS; ix++ )       // virtual rooms west
    for( int iy=0; iy<=nrefl && npath<=MAXPATHS; iy++ )     // virtual rooms south
      for( int iz=iz0; iz<=iz1 && npath<=MAXPATHS; iz++ ) // virtual rooms up/down
      {
        if( !ix && !iy && !iz)  // skip direct path
          continue;
        int order = ix+iy+abs(iz);
//        if( ix+iy+iz > nrefl )  // skip if too many reflections
//          continue;
        // listener's position in "reflected" room
        // reflect each room along Y and Z; X remains symmetrical
        double xl = xsize*ix + xl0;
        double yl = ysize*iy + ((iy%2)?ysize-yl0:yl0);
        double zl = zsize*iz + ((iz%2)?zsize-zl0:zl0);
        // distance to listener
        double r1 = dist(xs1,ys,zs,xl,yl,zl);
        double r2 = dist(xs2,ys,zs,xl,yl,zl);
        // delays
        double dly1 = (r1-r0)/vsound;
        double dly2 = (r2-r0)/vsound;
        // work out total dampening coefficients:
        //   (r0/ri)^2 is signal decay along path
        double damp1 = sqr(r0/r1),damp2=sqr(r0/r2);
        //   (-ax)^ix is sidewall reflection dampening (each reflection
        //   inverts phase, hence the -1 sign)
        double xdamp = pow(ax-1,ix);
        damp1 *= xdamp; damp2 *= xdamp;
        //   (-ay)^iy is front/rear wall reflection dampening.
        //   iy/2 is the number of front reflections 
        //   (iy+1)/2 is the number of rear reflections 
        double ydamp = pow(ay1-1,iy/2) * pow(ay2-1,(iy+1)/2);
        damp1 *= ydamp; damp2 *= ydamp;
        //   (-az)^|iz| is wall/ceiling dampening.
        //   (iy+1)/2 is the number of rear reflections 
        double zdamp;
        if( iz<0 )
          zdamp = pow(az2-1,abs(iz)/2) * pow(az1-1,(abs(iz)+1)/2);
        else
          zdamp = pow(az1-1,iz/2) * pow(az2-1,(iz+1)/2);
        damp1 *= zdamp; damp2 *= zdamp;
        // store results, if dampening & delays are within limits 
        double fl,fr,rl,rr;
        if( fabs(damp1)>=mindamp && dly1<=maxdly/1000 )
        {
          double angle = computeAngle(xs1-xl,yl-ys,ix,iy,fl,fr,rl,rr);
          Path thispath = { 'L',ix,iy,iz,r1,dly1,damp1,angle,fl,fr,rl,rr,order};
          paths[npath++] = thispath;
        }
        if( fabs(damp2)>=mindamp && dly2<=maxdly/1000 )
        {
          double angle = computeAngle(xs2-xl,yl-ys,ix,iy,fl,fr,rl,rr);
          Path thispath = { 'R',ix,iy,iz,r2,dly2,damp2,angle,fl,fr,rl,rr,order};
          paths[npath++] = thispath;
        }
      }
  // now, if too many paths, sort them by delay and discard longer ones
  if( npath>(int)maxpaths )
  {
    printf("%d total paths found, trimming to %d\n",npath,(int)maxpaths);
    qsort(paths,npath,sizeof(Path),comparePaths);
    npath = (int)maxpaths;
  }
  // write them out
  for( int i=0; i<npath; i++ )
  {
    Path p = paths[i];
    fprintf(fout,"%c %10.6f %10.6f %10.6f %10.6f %10.6f %d\n",
        p.chan,p.delay,p.damp*p.fl,p.damp*p.fr,p.damp*p.rl,p.damp*p.rr,p.order);
    printf("Path %c%d(%d/%d/%d): %5.1f m, delay %6.2f ms., dampening %6.4f, angle %5.1f\n"
            "       %5.3f %5.3f\n       %5.3f %5.3f\n",
            p.chan,i,p.ix,p.iy,p.iz,p.length,p.delay*1e+3,p.damp,p.angle,
            p.fl,p.fr,p.rl,p.rr);
  }
  printf("%d paths written to file %s\n",npath,nameout);
  fclose(fout);
}
