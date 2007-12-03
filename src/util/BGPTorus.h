/** \file BGPTorus.h
 *  Author: Abhinav S Bhatele
 *  Date created: May 21st, 2007  
 *  
 */

#ifndef _BGP_TORUS_H_
#define _BGP_TORUS_H_

#include "converse.h"

#if CMK_BLUEGENEP

#include "dcmf.h"

class BGPTorusManager {
  private:
    DCMF_Hardware_t bgp_hwt;
    int dimX;	// dimension of the allocation in X (no. of processors)
    int dimY;	// dimension of the allocation in Y (no. of processors)
    int dimZ;	// dimension of the allocation in Z (no. of processors)
    int dimNX;	// dimension of the allocation in X (no. of nodes)
    int dimNY;	// dimension of the allocation in Y (no. of nodes)
    int dimNZ;	// dimension of the allocation in Z (no. of nodes)
    int dimNT;  //dimension of the allocation in T (no. of processors per node)
   
    int torus[4];
    int procsPerNode;
    char *mapping;

  public:
    BGPTorusManager() {
      DCMF_Hardware(&bgp_hwt);
      dimNX = bgp_hwt.xSize;
      dimNY = bgp_hwt.ySize;
      dimNZ = bgp_hwt.zSize;
      dimNT = bgp_hwt.tSize;  //dimNT is a runtime option and not boot
			      //time and hence does not need to be
			      //corrected
 
      int numPes = CmiNumPes();
      int numNodes = numPes / dimNT;

      int max_t = 0;
      if(dimNX * dimNY * dimNZ != numNodes) {
        dimNX = dimNY = dimNZ = 0;
        int min_x, min_y, min_z;
        min_x = min_y = min_z = numPes;
        unsigned int tmp_t, tmp_x, tmp_y, tmp_z;      
        for(int c = 0; c < numPes; c++) {
	  DCMF_Messager_rank2torus(c, &tmp_x, &tmp_y, &tmp_z, &tmp_t);
	  
	  if(tmp_x > dimNX) dimNX = tmp_x;
          if(tmp_x < min_x) min_x = tmp_x;
	  if(tmp_y > dimNY) dimNY = tmp_y;
          if(tmp_y < min_y) min_y = tmp_y;
	  if(tmp_z > dimNZ) dimNZ = tmp_z;
          if(tmp_z < min_z) min_z = tmp_z;
	  if(tmp_t > max_t) max_t = tmp_t;
        }
	 
	dimNX = dimNX - min_x + 1;
	dimNY = dimNY - min_y + 1;
	dimNZ = dimNZ - min_z + 1;
      }

      dimX = dimNX;
      dimY = dimNY;
      dimZ = dimNZ;
 
      dimX = dimX * dimNT;	// assuming TXYZ
      procsPerNode = dimNT;

      torus[0] = bgp_hwt.xTorus;
      torus[1] = bgp_hwt.yTorus;
      torus[2] = bgp_hwt.zTorus;
      torus[3] = bgp_hwt.tTorus;
      
      mapping = getenv("BG_MAPPING");
    }

    ~BGPTorusManager() {
     }

    inline int getDimX() { return dimX; }
    inline int getDimY() { return dimY; }
    inline int getDimZ() { return dimZ; }

    inline int getDimNX() { return dimNX; }
    inline int getDimNY() { return dimNY; }
    inline int getDimNZ() { return dimNZ; }
    inline int getDimNT() { return dimNT; }

    inline int getProcsPerNode() { return procsPerNode; }

    inline int* isTorus() { return torus; }

    inline void rankToCoordinates(int pe, int &x, int &y, int &z) {
      x = pe % dimX;
      y = (pe % (dimX*dimY)) / dimX;
      z = pe / (dimX*dimY);
    }

    inline void rankToCoordinates(int pe, int &x, int &y, int &z, int &t) {
      if(mapping==NULL || (mapping!=NULL && mapping[0]=='X')) {
        x = pe % dimNX;
        y = (pe % (dimNX*dimNY)) / dimNX;
        z = (pe % (dimNX*dimNY*dimNZ)) / (dimNX*dimNY);
        t = pe / (dimNX*dimNY*dimNZ);
      } else {
        t = pe % dimNT;
        x = (pe % (dimNT*dimNX)) / dimNT;
        y = (pe % (dimNT*dimNX*dimNY)) / (dimNT*dimNX);
        z = pe / (dimNT*dimNX*dimNY);
      }
    }

    inline int coordinatesToRank(int x, int y, int z) {
      return x + (y + z*dimY) * dimX;
    }

    inline int coordinatesToRank(int x, int y, int z, int t) {
      if(mapping==NULL || (mapping!=NULL && mapping[0]=='X'))
        return x + (y + (z + t*dimNZ) * dimNY) * dimNX;
      else
        return t + (x + (y + z*dimNY) * dimNX) * dimNT;
    }
};

#endif // CMK_BLUEGENEP
#endif //_BGP_TORUS_H_
