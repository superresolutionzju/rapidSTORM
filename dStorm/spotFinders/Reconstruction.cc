// #include "spotFinders/Reconstruction.hh"
#include <CImg.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <stdio.h>

#include <stdlib.h>
#include <list>
#include <vector>
#include <cassert>
#include <iostream>
#include <iomanip>

#ifndef MINIMUM
	#define MINIMUM(A,B) ((A<B)?(A):(B))
#endif

#ifndef MAXIMUM
	#define MAXIMUM(A,B) ((A>B)?(A):(B))
#endif

//#define MEASURE_TIME
#ifdef MEASURE_TIME
#define TIME_MEASUREMENT(x)  x
#else
#define TIME_MEASUREMENT(x) 
#endif

using namespace std;
using namespace cimg_library;

namespace dStorm {

template <typename InstanceType>
bool firstIsBrighter( 
    const CImg<InstanceType>& a,
    const CImg<InstanceType>& b ) 
throw() 
{
    bool is = true;
    unsigned long sz = a.size();
    for (unsigned long i = 0; i < sz; i++)
        if ( a.data[i] < b.data[i] )
            is = false;
    return is;
}

template <typename InstanceType>
void add_border_to_image (CImg<InstanceType>& src) throw()
{
  for (unsigned int x = 0; x < src.width; x++)
    src(x,0) = src(x,src.height-1) = 0;
  for (unsigned int y = 0; y < src.height; y++)
    src(0,y) = src(src.width-1,y) = 0;

}

//-----------------------------------------------------------------------------
/* Fast Reconstruction Funktion v3.1

   Marko Tscherepanow 28.10.2004
   Changes for CImg: Steve Wolter, 12/2008
   
   L. Vincent, Morpological Grayscale Reconstruction in Image Analysis:
	 Applications and Efficient Algorithms, IEEE Trans. in Image Proc. 

   Input:  Gray level image and mask image of type IplImage*
   Return: Result of reconstruction by dilation, 
           image of type IplImage* 
    
*/
template <typename InstanceType>
void ReconstructionByDilationII(CImg<InstanceType>& src, 
    CImg<InstanceType>& mask,
    CImg<InstanceType>& target)
    throw()
{
	register int x,y,k;
	InstanceType d,q;
	CImg<InstanceType> temp_i, mask_i;	
	TIME_MEASUREMENT( clock_t t1 = clock(); clock_t t2; )
	
        assert( src.size() == mask.size() );
        assert( firstIsBrighter(mask, src) );
	
	add_border_to_image(src);
	add_border_to_image(mask);
	add_border_to_image(target);

	// neighbourhood group forward scan
	int  m[4] = {-(temp_i.width+1),-(temp_i.width),-(temp_i.width)+1,-1};
	// neighbourhood group backward scan
	int  n[4] = {temp_i.width+1,(temp_i.width),(temp_i.width)-1,1}; 
	//int  nr[4][2] = {{1,1},{0,1},{-1,1},{1,0}};
	// complete neighbourhood 
	int  nc[8]= {-(temp_i.width+1), -(temp_i.width), -(temp_i.width)+1,
                     -1, 1, (temp_i.width)-1, (temp_i.width), temp_i.width+1};
	//int  nc[8][2] = {{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}}; 
	    
        // forward reconstruction
	for ( y = 1; y < (int)src.height; y++)
          for ( x = 1; x < (int)src.width; x++)
        {    
            unsigned int base = y*src.width + x;
            
            // find global maximum of neighbourhood 
            d= src.data[base];
            for (k=0; k< 4;k++)
            {
                        // rand des bildes 0, daher kein randwertproblem
                        d=MAXIMUM(src.data[base+m[k]],d); 
            }
            
            // compare mask and maximum value of neighbourhood
            // write this value on position i,j
            
            d = MINIMUM(mask.data[base],d);
            target.data[base] = d;
        }
			
	list<int> fifo;  
			
	// backward reconstruction
	for(y=src.height-2;y>=0;y--)
            for(x=src.width-2;x>=0;x--)
            {    
                int base = y*src.width + x;
		    
                // find global maximum of neighbourhood 
                d=target.data[base];
                for (k=0; k< 4;k++)
                {
                    // rand des bildes 0, daher kein randwertproblem
                    d=MAXIMUM(target.data[base+n[k]],d);
                }
		    
                // compare mask and maximum value of neighbourhood
                // write this value on position i,j
		    		    
                d = MINIMUM(target.data[base],d);
                target.data[base]=d;

                for (k=0; k< 4;k++)
                {
                    q=target.data[base+n[k]];
                    if((q<d)&&(q<target.data[base+n[k]]))
                        fifo.push_back(base);
                } 	
            } 	
	
	while(!(fifo.empty()))
	{
            int base=fifo.front();
            fifo.pop_front();
	
            d=target.data[base];
            for (k=0; k< 8;k++) 		// complete neighbourhood
            { 	
                    int base_new=base+nc[k];
                    q=target.data[base_new];
                    if((q<d)&&(q!=mask.data[base_new]))
                    {
                        target.data[base_new]=
                            MINIMUM(d,mask.data[base_new]);  	
                        fifo.push_back(base_new);
                    } 
            }	
	}
	
	
#ifdef MEASURE_TIME
	t2=clock();
 	printf("time for reconstruction (v3.1): %fs\n", (float) (t2-t1)/1000000);
#endif
}

}
