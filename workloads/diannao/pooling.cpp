#include <iostream>
#include <string>
#include "dnn.hpp"
#include <stdlib.h>
#include <stdio.h>


using namespace std;

#if SB
#include "pool2x2l4avg.h"
#include "pool4x4l2avg.h"
#endif

#include "sim_timing.h"

#define AVG 1

//Problem Size
#ifndef Ny //if Ny is undefined, then assume nothing is defined
  #define Ny 32
  #define Nx 32
  
  #define Kx 4
  #define Ky 4
  //#define Ni 100  //Input Layers == Ouptut Layers
  #define Ni 128
#endif

//slide increment
#ifndef Sy
  #define Sx 1
  #define Sy 1
#endif

#ifndef Tii //Tiling Sizes:
  #define Tii 64
  #define Ti  16
  #define Ty  16
  #define Tx  16
#endif

#define NYPAD (Ny+Ky)
#define NXPAD (Nx+Kx)

#define NYSCL (Ny/Sy)
#define NXSCL (Nx/Sx)


void fill_pooling(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni],
                  VTYPE (&neuron_n1)[NYSCL][NXSCL][Ni],
                  VTYPE (&neuron_n2)[NYSCL][NXSCL][Ni]) {
  int total=0;
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        //neuron_i[yy][xx][ni] = xx+yy+ni;
        neuron_i[yy][xx][ni] = rand() &0x3FFF;

        //neuron_i[yy][xx][ni] = 1;
      }
    }
  }
  //takes too long....
  for(int yy = 0; yy < NYSCL; ++yy) {
    for(int xx = 0; xx < NXSCL; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_n1[yy][xx][ni] = 0;
        neuron_n2[yy][xx][ni] = 0;
      }
    }
  }
}

int pooling_layer_blocked(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni],
                           VTYPE (&neuron_n)[NYSCL][NXSCL][Ni]) {
  int c=0;

  VTYPE value[Ni]={0};
  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int iii = 0; iii < Ni; iii += Tii) {
        // — Original code — (excluding ii loop)
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += Sy) {
          int xout = xx/Sx;
          for (int x = xx; x < xx + Tx; x += Sx) {
    
            for (int ii = iii; ii < iii + Tii; ii += Ti) {
              for (int i = ii; i < ii + Ti; i++) {
                value[i] = 0;
              }
    
              for (int ky = 0; ky < Ky; ky++) {
                for (int kx = 0; kx < Kx; kx++) {
                  //c++;
                  for (int i = ii; i < ii + Ti; i++) {
                    #ifdef AVG
                    value[i] += neuron_i[ky + y][kx + x][i];
                    #else
                    value[i] = max(value[i], neuron_i[ky + y][kx + x][i]);
                    #endif
                  }
                }
              }

              for (int i = ii; i < ii + Ti; i++) {
                #ifdef AVG
                neuron_n[yout][xout][i] = value[i] / (Kx * Ky);
                #else
                neuron_n[yout][xout][i] = value[i];
                #endif
              }
            }
            xout++;
          }
          yout++;
        }
      }
    }
  }
  return c;
}

void pooling_layer(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni],
                   VTYPE (&neuron_n)[NYSCL][NXSCL][Ni]) {
  VTYPE value[Ni]={0};
  // — Original code —
  int yout = 0;
  for (int y = 0; y < Ny; y += Sy) {
    int xout = 0;
    for (int x = 0; x < Nx; x += Sx) {
      for (int i = 0; i < Ni; i++) {
        value[i]=0;
      }

      for (int ky = 0; ky < Ky; ky++) {
        for (int kx = 0; kx < Kx; kx++) {
          for (int i = 0; i < Ni; i++) {
            #ifdef AVG
            value[i] += neuron_i[ky + y][kx + x][i];
            #else
            value[i] = max(value[i], neuron_i[ky + y][kx + x][i]);
            #endif
          }
        }
      }

      for (int i = 0; i < Ni; i++) {
        #ifdef AVG
        neuron_n[yout][xout][i] = value[i] / (Kx * Ky);
        #else
        neuron_n[yout][xout][i] = value[i];
        #endif
      }
      xout++;
    }
    yout++;
  }
}

#if SB

int pooling_layer_blocked_sb_4x4_sx1_sy1(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni],
                                         VTYPE (&neuron_n)[NYSCL][NXSCL][Ni]) {
  int c=0;

  int pipedepth=16;
  int pipedepth_bytes=(pipedepth*8);

  SB_CONFIG(pool4x4l2avg_config, pool4x4l2avg_size); 

  VTYPE value[Ni]={0};
  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      //cout << dec << "\n yy: " << yy << " xx: " << xx << "\n";


      for (int iii = 0; iii < Ni; iii += pipedepth*4) {
        // — Original code — (excluding ii loop)
        int yout = yy/Sy;
        
        for (int y = yy; y < yy + Ty; y += 2) { // two rows at a time
          //int xout = xx/Sx;
            //upper -- xx + Tx
            //lower - xx

          //cout << dec << "\n yy: " << yy << " xx: " << xx << " iii: " << iii << "\n";

          //First three loops produce garbage

          SB_CONST(P_pool4x4l2avg_Xa, 0, 2*pipedepth*1); //Initialize garbage inputs
          SB_CONST(P_pool4x4l2avg_Xb, 0, 2*pipedepth*1);
          SB_CONST(P_pool4x4l2avg_Xc, 0, 2*pipedepth*1);

          SB_DMA_READ(&neuron_i[y+0][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+3,P_pool4x4l2avg_R0);
          SB_DMA_READ(&neuron_i[y+1][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+3,P_pool4x4l2avg_R1);
          SB_DMA_READ(&neuron_i[y+2][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+3,P_pool4x4l2avg_R2);
          SB_DMA_READ(&neuron_i[y+3][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+3,P_pool4x4l2avg_R3);
          SB_DMA_READ(&neuron_i[y+4][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+3,P_pool4x4l2avg_R4);

          //each rec has a slightly different number of iterations
          SB_RECURRENCE(P_pool4x4l2avg_Oa,P_pool4x4l2avg_Xa,2*pipedepth*(Tx+2));  
          SB_RECURRENCE(P_pool4x4l2avg_Ob,P_pool4x4l2avg_Xb,2*pipedepth*(Tx+2));
          SB_RECURRENCE(P_pool4x4l2avg_Oc,P_pool4x4l2avg_Xc,2*pipedepth*(Tx+2));

          SB_GARBAGE(P_pool4x4l2avg_O0,pipedepth*3);
          SB_GARBAGE(P_pool4x4l2avg_O1,pipedepth*3);

          SB_DMA_WRITE(P_pool4x4l2avg_O0,Ni*sizeof(VTYPE),pipedepth_bytes,Tx,&neuron_n[y+0][xx][iii]);
          SB_DMA_WRITE(P_pool4x4l2avg_O1,Ni*sizeof(VTYPE),pipedepth_bytes,Tx,&neuron_n[y+1][xx][iii]);

          SB_GARBAGE(P_pool4x4l2avg_Oa,2*pipedepth*1);
          SB_GARBAGE(P_pool4x4l2avg_Ob,2*pipedepth*1);
          SB_GARBAGE(P_pool4x4l2avg_Oc,2*pipedepth*1);
        }
      }
    }
  }
  SB_WAIT_ALL();
  return c;
}


int pooling_layer_blocked_sb_2x2_sx1_sy1(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni],
                                         VTYPE (&neuron_n)[NYSCL][NXSCL][Ni]) {
  int c=0;

  int pipedepth=16;
  int pipedepth_bytes=(pipedepth*8);

  SB_CONFIG(pool2x2l4avg_config, pool2x2l4avg_size); 

  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int iii = 0; iii < Ni; iii += pipedepth*4) {
        // — Original code — (excluding ii loop)
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += 4) { // two rows at a time
          //int xout = xx/Sx;
            //upper -- xx + Tx
            //lower - xx

          //cout << dec << "\n yy: " << yy << " xx: " << xx << " iii: " << iii << "\n";

          //First three loops produce garbage

          SB_CONST(P_pool2x2l4avg_P, 0, 4*pipedepth*1); //Initialize garbage inputs

          SB_DMA_READ(&neuron_i[y+0][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+1,P_pool2x2l4avg_R0);
          SB_DMA_READ(&neuron_i[y+1][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+1,P_pool2x2l4avg_R1);
          SB_DMA_READ(&neuron_i[y+2][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+1,P_pool2x2l4avg_R2);
          SB_DMA_READ(&neuron_i[y+3][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+1,P_pool2x2l4avg_R3);
          SB_DMA_READ(&neuron_i[y+4][xx][iii],Ni*sizeof(VTYPE),pipedepth_bytes,Tx+1,P_pool2x2l4avg_R4);

          SB_RECURRENCE(P_pool2x2l4avg_I,P_pool2x2l4avg_P,4*pipedepth*(Tx));  

          SB_GARBAGE(P_pool2x2l4avg_O0,pipedepth*1);
          SB_GARBAGE(P_pool2x2l4avg_O1,pipedepth*1);
          SB_GARBAGE(P_pool2x2l4avg_O2,pipedepth*1);
          SB_GARBAGE(P_pool2x2l4avg_O3,pipedepth*1);

          SB_DMA_WRITE(P_pool2x2l4avg_O0,Ni*sizeof(VTYPE),pipedepth_bytes,Tx,&neuron_n[y+0][xx][iii]);
          SB_DMA_WRITE(P_pool2x2l4avg_O1,Ni*sizeof(VTYPE),pipedepth_bytes,Tx,&neuron_n[y+1][xx][iii]);
          SB_DMA_WRITE(P_pool2x2l4avg_O2,Ni*sizeof(VTYPE),pipedepth_bytes,Tx,&neuron_n[y+2][xx][iii]);
          SB_DMA_WRITE(P_pool2x2l4avg_O3,Ni*sizeof(VTYPE),pipedepth_bytes,Tx,&neuron_n[y+3][xx][iii]);

          SB_GARBAGE(P_pool2x2l4avg_I,4*pipedepth*1);

        }
      }
    }
  }
  SB_WAIT_ALL();
  return c;
}

int pooling_layer_blocked_sb_2x2_sx1_sy1_full_ni(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni],
                                                 VTYPE (&neuron_n)[NYSCL][NXSCL][Ni]) {
  int c=0;

  int pipedepth=16;
  int pipedepth_bytes=(pipedepth*8);
  int DP_WIDTH=8;

  SB_CONFIG(pool2x2l4avg_config, pool2x2l4avg_size); 

  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int y = yy; y < yy + Ty; y += 4) { // two rows at a time
        //cout << dec << "\n yy: " << yy << " xx: " << xx << "\n";

        //First three loops produce garbage

        int ni_elem = Ni*sizeof(VTYPE)/DP_WIDTH;
        SB_CONST(P_pool2x2l4avg_P, 0, 4*ni_elem); 

        SB_DMA_READ(&neuron_i[y+0][xx][0],Ni*sizeof(VTYPE),Ni*sizeof(VTYPE),Tx+1,P_pool2x2l4avg_R0);
        SB_DMA_READ(&neuron_i[y+1][xx][0],Ni*sizeof(VTYPE),Ni*sizeof(VTYPE),Tx+1,P_pool2x2l4avg_R1);
        SB_DMA_READ(&neuron_i[y+2][xx][0],Ni*sizeof(VTYPE),Ni*sizeof(VTYPE),Tx+1,P_pool2x2l4avg_R2);
        SB_DMA_READ(&neuron_i[y+3][xx][0],Ni*sizeof(VTYPE),Ni*sizeof(VTYPE),Tx+1,P_pool2x2l4avg_R3);
        SB_DMA_READ(&neuron_i[y+4][xx][0],Ni*sizeof(VTYPE),Ni*sizeof(VTYPE),Tx+1,P_pool2x2l4avg_R4);

        SB_RECURRENCE(P_pool2x2l4avg_I,P_pool2x2l4avg_P,4*ni_elem*Tx);  

        SB_GARBAGE(P_pool2x2l4avg_O0,ni_elem);
        SB_GARBAGE(P_pool2x2l4avg_O1,ni_elem);
        SB_GARBAGE(P_pool2x2l4avg_O2,ni_elem);
        SB_GARBAGE(P_pool2x2l4avg_O3,ni_elem);

        SB_DMA_WRITE(P_pool2x2l4avg_O0,Ni*sizeof(VTYPE),Ni*sizeof(VTYPE),Tx,&neuron_n[y+0][xx][0]);
        SB_DMA_WRITE(P_pool2x2l4avg_O1,Ni*sizeof(VTYPE),Ni*sizeof(VTYPE),Tx,&neuron_n[y+1][xx][0]);
        SB_DMA_WRITE(P_pool2x2l4avg_O2,Ni*sizeof(VTYPE),Ni*sizeof(VTYPE),Tx,&neuron_n[y+2][xx][0]);
        SB_DMA_WRITE(P_pool2x2l4avg_O3,Ni*sizeof(VTYPE),Ni*sizeof(VTYPE),Tx,&neuron_n[y+3][xx][0]);

        SB_GARBAGE(P_pool2x2l4avg_I,4*ni_elem);
      }
    }
  }
  SB_WAIT_ALL();
  return c;
}
#endif

int test_layer(VTYPE (&neuron_i)[NYPAD][NXPAD][Ni],
               VTYPE (&neuron_n)[NYSCL][NXSCL][Ni]) {
  begin_roi();
#ifdef SB
  #if Kx == 4
    pooling_layer_blocked_sb_4x4_sx1_sy1(neuron_i,neuron_n);
  #elif Kx == 2
    #if Ni < 64
      pooling_layer_blocked_sb_2x2_sx1_sy1_full_ni(neuron_i,neuron_n);
    #else
      pooling_layer_blocked_sb_2x2_sx1_sy1(neuron_i,neuron_n);
    #endif
  #else 
    #error "Kx must be 2 or 4"
  #endif
#else
   pooling_layer_blocked(neuron_i,neuron_n);
#endif
  end_roi();
}


int main(int argc, char** argv) {
  //Arrays:
  //VTYPE  neuron_i[NYPAD][NXPAD][Ni];
  //VTYPE  neuron_n[NYSCL][NXSCL][Ni];
  //VTYPE neuron_n2[NYSCL][NXSCL][Ni];
  
  VTYPE  (*neuron_i)[NYPAD][NXPAD][Ni];
  VTYPE  (*neuron_n)[NYSCL][NXSCL][Ni];
  VTYPE (*neuron_n2)[NYSCL][NXSCL][Ni];

  //cout << "allocating memory\n";
  neuron_i  = (VTYPE (*)[NYPAD][NXPAD][Ni])aligned_malloc(64,NYPAD*NXPAD*Ni*sizeof(VTYPE)+64);
  neuron_n  = (VTYPE (*)[NYSCL][NXSCL][Ni])aligned_malloc(64,NYSCL*NXSCL*Ni*sizeof(VTYPE)+64);
  neuron_n2 = (VTYPE (*)[NYSCL][NXSCL][Ni])aligned_malloc(64,NYSCL*NXSCL*Ni*sizeof(VTYPE)+64);

  //cout << "NYSCL: " << NYSCL << "\n";
  //cout << "NXSCL: " << NXSCL << "\n";
  //cout << "Ni:    " << Ni    << "\n";

  //cout << "bound i\t"  << hex << &(*neuron_i)[0][0][0]  << " to " << &(*neuron_i)[NYPAD-1][NXPAD-1][Ni-1] << "\n";
  //cout << "bound n1\t" << hex << &(*neuron_n)[0][0][0]  << " to " << &(*neuron_n)[NYSCL-1][NXSCL-1][Ni-1] << "\n";
  //cout << "bound n2\t" << hex << &(*neuron_n2)[0][0][0] << " to " << &(*neuron_n2)[NYSCL-1][NXSCL-1][Ni-1] << "\n";

  //cout << "0,0,1\t" << hex << &(*neuron_n2)[0][0][1] << "\n";
  //cout << "0,1,0\t" << hex << &(*neuron_n2)[0][1][0] << "\n";
  //cout << "1,0,0\t" << hex << &(*neuron_n2)[1][0][0] << "\n";

  //cout << "isize: " << NYPAD*NXPAD*Ni*sizeof(VTYPE) << "\n";
  //cout << "nsize: " << NYSCL*NXSCL*Ni*sizeof(VTYPE) << "\n";


  if(argc==3) {

  //cout << "Did nothing\n";

//  } else if(argc==2 && string(argv[1])=="perf") {
  } else if(argc==2) {
    test_layer(*neuron_i,*neuron_n);
    //cout << "Perf Run Complete\n";
  } else {
    cout << "initializing arrays\n";
    fill_pooling(*neuron_i,*neuron_n,*neuron_n2);
    cout << "starting computation\n";

    int calc = 0;
    pooling_layer(*neuron_i,*neuron_n);
    test_layer(*neuron_i,*neuron_n2);
    cout << "computation complete!\n";  

    if(calc > 0) {
      cout << "calc: " << calc << "\n";
    }
    compare_short((VTYPE*)*neuron_n,(VTYPE*)*neuron_n2,NYSCL*NXSCL*Ni);
    cout << "adds: " << NYSCL*NXSCL*Ni*Ky*Kx <<  "\n";
    cout << "argc:" << argc << "\n";
//  cout << "mult-block:  " << calc.first   << " sigmoid-block: " << calc.second  << "\n";
//  cout << "mult-orig:  "  << calc2.first  << " sigmoid-orig:  " << calc2.second << "\n";
//
//  int n_outputs= Ny/Sy * Nx/Sx * Nn;
//  cout << "mult-correct: " << n_outputs*Ni*Kx*Ky
//       << " sigmoid-correct: "  << n_outputs << "\n";
  }
  sb_stats();

}

