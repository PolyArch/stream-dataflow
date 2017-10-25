#include <iostream>
#include <string>
#include "dnn.hpp"

#if SB
#include "red32to1sig.h"
#include "red16to1sig.h"
#include "red8to1sig.h"
#endif

#include "sim_timing.h"


using namespace std;

#ifndef SHARED
#define SHARED 1
#endif

//Lets just make sure all problem specific stuff gets defined before we get here!
//#ifndef Ny
//  //Problem Size
//  #define Ny 32
//  #define Ny 32
//  
//  #define Kx 4
//  #define Ky 4
//  //#define Ni 108
//  //#define Nn 200
//  
//  #define Ni 112
//  #define Nn 224
//#endif
//

//slide increment
#ifndef Sy
  #define Sy 1
  #define Sx 1
#endif

#ifndef Tnn
  //Tiling Sizes
  #define Tnn 32
  //#define Tn  25
  //#define Ti  16
  #define Tn  16
  #define Ti  16
  
  #define Ty  8
  #define Tx  8
#endif

#define NYPAD (Ny+Ky)
#define NXPAD (Nx+Kx)

#define NYSCL (Ny/Sy)
#define NXSCL (Nx/Sx)


//Arrays:
#if SHARED == 1
#define SYNAPSE_SIZE (1L*Ky*Kx*Nn*Ni)
#else
#define SYNAPSE_SIZE (1L*NYSCL*NXSCL*Ky*Kx*Nn*Ni)
#endif

#if SHARED == 1
VTYPE (*synapse)[Ky][Kx][Nn][Ni];
#else
VTYPE (*synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni];
#endif

//VTYPE neuron_i[NYPAD][NXPAD][Ni];
//VTYPE neuron_n[NYSCL][NXSCL][Nn]={0},    neuron_n2[NYSCL][NXSCL][Nn]={0};

VTYPE  (*neuron_i)[NYPAD][NXPAD][Ni];
VTYPE  (*neuron_n)[NYSCL][NXSCL][Nn];
VTYPE (*neuron_n2)[NYSCL][NXSCL][Nn];


void fill_convolution_shared_simple(VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
                                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni]) {
  for(int yy = 0; yy < Ky; ++yy) {
    for(int xx = 0; xx < Kx; ++xx) {
      for(int nn = 0; nn < Nn; ++nn) {
        for(int ni = 0; ni < Ni; ++ni) {
          synapse[yy][xx][nn][ni] = rand()%(1+nn/4+ni/2);;
  } } } }
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_i[yy][xx][ni] = rand()%16;
  }  }  }
}

void fill_convolution_private(VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
                                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni]) {
  for(int yout = 0; yout < NYSCL; ++yout) {
    for(int xout = 0; xout < NXSCL; ++xout) {
      for(int yy = 0; yy < Ky; ++yy) {
        for(int xx = 0; xx < Kx; ++xx) {
          for(int nn = 0; nn < Nn; ++nn) {
            for(int ni = 0; ni < Ni; ++ni) {
              synapse[xout][yout][yy][xx][nn][ni] = rand()%(1+nn/4+ni/2);;

  } } } } } }
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_i[yy][xx][ni] =  rand()%16;
  }  }  }
}


void fill_convolution_shared(VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
                             VTYPE (&neuron_i)[NYPAD][NXPAD][Ni]) {
  int total1=0,total2=0;
  for(int yy = 0; yy < Ky; ++yy) {
    for(int xx = 0; xx < Kx; ++xx) {
      for(int nn = 0; nn < Nn; ++nn) {
        for(int ni = 0; ni < Ni; ++ni) {
          synapse[yy][xx][nn][ni] = total1;
          total1+=1;
        }
      }
    }
  }
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_i[yy][xx][ni] = total2;
        total2+=2;
      }
    }
  }
}



std::pair<int,int> convolution_layer_blocked(
#if SHARED == 1
                              VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
#else
                              VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                              VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                              VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
  int c1=0,c2=0;
  VTYPE sum[Nn]={0};

  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int nnn = 0; nnn < Nn; nnn += Tnn) {
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += Sy) { // tiling for y;
          int xout = xx/Sx;

          for (int x = xx; x < xx + Tx; x += Sx) { // tiling for x;

            for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {
              for (int n = nn; n < nn + Tn; n++) {
                sum[n] = 0;
              }

              for (int ky = 0; ky < Ky; ky++) {  // sliding window;
                for (int kx = 0; kx < Kx; kx++) {

                  int ii = 0;
                  VTYPE sum_sc;

                  for (; ii < Ni -Ti+1; ii += Ti) {
                    for (int n = nn; n < nn + Tn; n++) {
                      sum_sc=0;
                      for (int i = ii; i < ii + Ti; i++) {
                        #if SHARED == 1 // version with shared kernels
                        VTYPE sv = synapse[ky][kx][n][i];
                        VTYPE nv = neuron_i[ky + y][kx + x][i];
                        #else // version with private kernels
                        VTYPE sv = synapse[yout][xout][ky][kx][n][i];
                        VTYPE nv = neuron_i[ky + y][kx + x][i];
                        #endif
                        sum_sc+=sv*nv;
                      }
                      sum[n]+=sum_sc;
                    }
                  }
                }
              }

              //sigmoid
              for (int n = nn; n < nn + Tn; n++) {
                neuron_n[yout][xout][n] = sigmoid(sum[n]);
                //c2++;
              }
            }
            xout++; 
          }
          yout++;
        }
      }
    }
  }
  return make_pair(c1,c2);
}

#if SB

int convolution_layer_sb_inner_doublebuf(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni],  
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
  int c1=0,c2=0;
  VTYPE sum[Nn]={0};

  //int tn_elem = Tn * sizeof(VTYPE) / DATA_WIDTH;

  // Stream in CGRA config (do this somewhere else?) 
  SB_CONFIG(red32to1sig_config, red32to1sig_size);

  unsigned even_addr=0;
  unsigned odd_addr=2048; //(Ni * sizeof(VTYPE) * Kx);
  bool even_phase=true; 


  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int nnn = 0; nnn < Nn; nnn += Tnn) {
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += Sy) { // tiling for y;
          int xout = xx/Sx;

          for (int x = xx; x < xx + Tx; x += Sx) { // tiling for x;
            for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {

              SB_CONST(P_red32to1sig_acc, 0, Tn); 

              int t=0;
              int l=Ky*Kx*Ni;

              for (int ky = 0; ky < Ky; ky++) {  // sliding window;

                //Do kx=0 load
                if(even_phase) {
                  SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky][x+0][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), 1, even_addr);
                } else {
                  SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky][x+0][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), 1, odd_addr);
                }
                SB_WAIT_SCR_WR();
      

                for (int kx = 0; kx < Kx; kx++) {
                  SB_WAIT_SCR_RD();

                  for (int ii=0; ii < Ni -Ti+1; ii += Ti) {

                    t+=Ti;
                    if(t!=l) {
                      SB_CONST(P_red32to1sig_pred, 0, Tn); 
                    } else {
                      SB_CONST(P_red32to1sig_pred, 1, Tn);
                    }
                    
                    if(even_phase) {
                      SB_SCR_PORT_STREAM(even_addr+(ii)*sizeof(VTYPE), 0, sizeof(VTYPE)*Ti, Tn, P_red32to1sig_N);
                    } else {
                      SB_SCR_PORT_STREAM(odd_addr +(ii)*sizeof(VTYPE), 0, sizeof(VTYPE)*Ti, Tn, P_red32to1sig_N);
                    }

                    #if SHARED == 1
                    SB_DMA_READ(&synapse[ky][kx][nn][ii],             sizeof(VTYPE)*Ni, sizeof(VTYPE)*Ti, Tn, P_red32to1sig_S); 
                    #else 
                    SB_DMA_READ(&synapse[yout][xout][ky][kx][nn][ii], sizeof(VTYPE)*Ni, sizeof(VTYPE)*Ti, Tn, P_red32to1sig_S); 
                    #endif

                    //SB_WAIT(2);

                    if(t!=l) {
                      SB_RECURRENCE(P_red32to1sig_out, P_red32to1sig_acc, Tn);
                    }
                  }

                  // Load next version of kx
                  if(kx+1!=Kx) {
                    if(even_phase) { //load to odd_addr if 
                      SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky][x+kx+1][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), 1, odd_addr);
                    } else {
                      SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky][x+kx+1][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), 1, even_addr);
                    }
                  }

                  even_phase=!even_phase;
                  SB_WAIT_SCR_WR();

                }

               }

              // write completed outputs out to memory
              SB_DMA_WRITE_SHF16(P_red32to1sig_out, 4*sizeof(VTYPE), 4*sizeof(VTYPE), Tn/4, &neuron_n[yout][xout][nn]);

            }
            xout++; 
          }
          yout++;
        }
      }
    }
  }
  SB_WAIT(0);

  return 0;
}

int convolution_layer_sb_doublebuf_ni8(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni],  
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
  int c1=0,c2=0;
  VTYPE sum[Nn]={0};

  //int tn_elem = Tn * sizeof(VTYPE) / DATA_WIDTH;

  // Stream in CGRA config (do this somewhere else?) 
  SB_CONFIG(red8to1sig_config, red8to1sig_size);

  unsigned even_addr=0;
  unsigned odd_addr=2048; (Ni * sizeof(VTYPE) * Kx);
  bool even_phase=true; 


  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int nnn = 0; nnn < Nn; nnn += Tnn) {
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += Sy) { // tiling for y;
          int xout = xx/Sx;

          for (int x = xx; x < xx + Tx; x += Sx) { // tiling for x;
            for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {

              SB_CONST(P_red8to1sig_acc, 0, Tn); 

              int t=0;
              int l=Ky*Kx*Ni;

              //Do ky=0 load
              if(even_phase) {
                SB_DMA_SCRATCH_LOAD(&neuron_i[y+0][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, even_addr);
              } else {
                SB_DMA_SCRATCH_LOAD(&neuron_i[y+0][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, odd_addr);
              }
              SB_WAIT_SCR_WR();

              for (int ky = 0; ky < Ky; ky++) {  // sliding window;

                SB_WAIT_SCR_RD();
      
                for (int kx = 0; kx < Kx; kx++) {
                  for (int ii=0; ii < Ni -Ti+1; ii += Ti) {

                    t+=Ti;
                    if(t!=l) {
                      SB_CONST(P_red8to1sig_pred, 0, Tn/2); 
                    } else {
                      SB_CONST(P_red8to1sig_pred, 1, Tn/2);
                    }

                    if(even_phase) {
                      SB_SCR_PORT_STREAM(even_addr+(kx*Ni+ii)*sizeof(VTYPE), 0, sizeof(VTYPE)*Ti, Tn/2, P_red8to1sig_N);
                    } else {
                      SB_SCR_PORT_STREAM(odd_addr +(kx*Ni+ii)*sizeof(VTYPE), 0, sizeof(VTYPE)*Ti, Tn/2, P_red8to1sig_N);
                    }

                    #if SHARED == 1
                    SB_DMA_READ(&synapse[ky][kx][nn][ii],             sizeof(VTYPE)*Ni, sizeof(VTYPE)*Ti, Tn, P_red8to1sig_S); 
                    #else 
                    SB_DMA_READ(&synapse[yout][xout][ky][kx][nn][ii], sizeof(VTYPE)*Ni, sizeof(VTYPE)*Ti, Tn, P_red8to1sig_S); 
                    #endif

                    //if(kx==0 && ii==0) { //first time in loop, do scratch read
                    //  SB_WAIT_SCR_RD_QUEUED();
      
                    //  // Load next version of ky
                    //  if(ky+1!=Ky) {
                    //    if(even_phase) { //load to odd_addr if 
                    //      SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, odd_addr);
                    //    } else {
                    //      SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, even_addr);
                    //    }
                    //  }
                    //} 

                    //SB_WAIT(2);

                    if(t!=l) {
                      SB_RECURRENCE(P_red8to1sig_out, P_red8to1sig_acc, Tn);
                    }


                  }
                }

                // Load next version of ky
                if(ky+1!=Ky) {
                  if(even_phase) { //load to odd_addr if 
                    SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, odd_addr);
                  } else {
                    SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, even_addr);
                  }
                }

                even_phase=!even_phase;
                SB_WAIT_SCR_WR();
              }

              // write completed outputs out to memory
              SB_DMA_WRITE_SHF16(P_red8to1sig_out, 4*sizeof(VTYPE), 4*sizeof(VTYPE), Tn/4, &neuron_n[yout][xout][nn]);
            }
            xout++; 
          }
          yout++;
        }
      }
    }
  }
  SB_WAIT(0);

  return 0;
}


int convolution_layer_sb_doublebuf_ni16(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni],  
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
  int c1=0,c2=0;
  VTYPE sum[Nn]={0};

  //int tn_elem = Tn * sizeof(VTYPE) / DATA_WIDTH;

  // Stream in CGRA config (do this somewhere else?) 
  SB_CONFIG(red16to1sig_config, red16to1sig_size);

  unsigned even_addr=0;
  unsigned odd_addr=2048; (Ni * sizeof(VTYPE) * Kx);
  bool even_phase=true; 


  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int nnn = 0; nnn < Nn; nnn += Tnn) {
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += Sy) { // tiling for y;
          int xout = xx/Sx;

          for (int x = xx; x < xx + Tx; x += Sx) { // tiling for x;
            for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {

              SB_CONST(P_red16to1sig_acc, 0, Tn); 

              int t=0;
              int l=Ky*Kx*Ni;

              //Do ky=0 load
              if(even_phase) {
                SB_DMA_SCRATCH_LOAD(&neuron_i[y+0][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, even_addr);
              } else {
                SB_DMA_SCRATCH_LOAD(&neuron_i[y+0][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, odd_addr);
              }
              SB_WAIT_SCR_WR();

              for (int ky = 0; ky < Ky; ky++) {  // sliding window;

                SB_WAIT_SCR_RD();
      
                for (int kx = 0; kx < Kx; kx++) {
                  for (int ii=0; ii < Ni -Ti+1; ii += Ti) {

                    t+=Ti;
                    if(t!=l) {
                      SB_CONST(P_red16to1sig_pred, 0, Tn/2); 
                    } else {
                      SB_CONST(P_red16to1sig_pred, 1, Tn/2);
                    }

                    if(even_phase) {
                      SB_SCR_PORT_STREAM(even_addr+(kx*Ni+ii)*sizeof(VTYPE), 0, sizeof(VTYPE)*Ti, Tn/2, P_red16to1sig_N);
                    } else {
                      SB_SCR_PORT_STREAM(odd_addr +(kx*Ni+ii)*sizeof(VTYPE), 0, sizeof(VTYPE)*Ti, Tn/2, P_red16to1sig_N);
                    }

                    #if SHARED == 1
                    SB_DMA_READ(&synapse[ky][kx][nn][ii],             sizeof(VTYPE)*Ni, sizeof(VTYPE)*Ti, Tn, P_red16to1sig_S); 
                    #else 
                    SB_DMA_READ(&synapse[yout][xout][ky][kx][nn][ii], sizeof(VTYPE)*Ni, sizeof(VTYPE)*Ti, Tn, P_red16to1sig_S); 
                    #endif

                    //if(kx==0 && ii==0) { //first time in loop, do scratch read
                    //  SB_WAIT_SCR_RD_QUEUED();
      
                    //  // Load next version of ky
                    //  if(ky+1!=Ky) {
                    //    if(even_phase) { //load to odd_addr if 
                    //      SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, odd_addr);
                    //    } else {
                    //      SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, even_addr);
                    //    }
                    //  }
                    //} 

                    //SB_WAIT(2);

                    if(t!=l) {
                      SB_RECURRENCE(P_red16to1sig_out, P_red16to1sig_acc, Tn);
                    }


                  }
                }

                // Load next version of ky
                if(ky+1!=Ky) {
                  if(even_phase) { //load to odd_addr if 
                    SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, odd_addr);
                  } else {
                    SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, even_addr);
                  }
                }

                even_phase=!even_phase;
                SB_WAIT_SCR_WR();
              }

              // write completed outputs out to memory
              SB_DMA_WRITE_SHF16(P_red16to1sig_out, 4*sizeof(VTYPE), 4*sizeof(VTYPE), Tn/4, &neuron_n[yout][xout][nn]);
            }
            xout++; 
          }
          yout++;
        }
      }
    }
  }
  SB_WAIT(0);

  return 0;
}


int convolution_layer_sb_doublebuf(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni],  
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
  int c1=0,c2=0;
  VTYPE sum[Nn]={0};

  //int tn_elem = Tn * sizeof(VTYPE) / DATA_WIDTH;

  // Stream in CGRA config (do this somewhere else?) 
  SB_CONFIG(red32to1sig_config, red32to1sig_size);

  unsigned even_addr=0;
  unsigned odd_addr=2048; (Ni * sizeof(VTYPE) * Kx);
  bool even_phase=true; 


  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int nnn = 0; nnn < Nn; nnn += Tnn) {
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += Sy) { // tiling for y;
          int xout = xx/Sx;

          for (int x = xx; x < xx + Tx; x += Sx) { // tiling for x;
            for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {

              SB_CONST(P_red32to1sig_acc, 0, Tn); 

              int t=0;
              int l=Ky*Kx*Ni;


              //Do ky=0 load
              if(even_phase) {
                SB_DMA_SCRATCH_LOAD(&neuron_i[y+0][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, even_addr);
              } else {
                SB_DMA_SCRATCH_LOAD(&neuron_i[y+0][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, odd_addr);
              }
              SB_WAIT_SCR_WR();

              for (int ky = 0; ky < Ky; ky++) {  // sliding window;

                SB_WAIT_SCR_RD();
      
                for (int kx = 0; kx < Kx; kx++) {
                  for (int ii=0; ii < Ni -Ti+1; ii += Ti) {

                    t+=Ti;
                    if(t!=l) {
                      SB_CONST(P_red32to1sig_pred, 0, Tn); 
                    } else {
                      SB_CONST(P_red32to1sig_pred, 1, Tn);
                    }

                    if(even_phase) {
                      SB_SCR_PORT_STREAM(even_addr+(kx*Ni+ii)*sizeof(VTYPE), 0, sizeof(VTYPE)*Ti, Tn, P_red32to1sig_N);
                    } else {
                      SB_SCR_PORT_STREAM(odd_addr +(kx*Ni+ii)*sizeof(VTYPE), 0, sizeof(VTYPE)*Ti, Tn, P_red32to1sig_N);
                    }

                    SB_DMA_READ(&synapse[ky][kx][nn][ii],  sizeof(VTYPE)*Ni, sizeof(VTYPE)*Ti, Tn, P_red32to1sig_S); 

                    //if(kx==0 && ii==0) { //first time in loop, do scratch read
                    //  SB_WAIT_SCR_RD_QUEUED();
      
                    //  // Load next version of ky
                    //  if(ky+1!=Ky) {
                    //    if(even_phase) { //load to odd_addr if 
                    //      SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, odd_addr);
                    //    } else {
                    //      SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, even_addr);
                    //    }
                    //  }
                    //} 

                    //SB_WAIT(2);
                    //

                    //sb_verify();

                    if(t!=l) {
                      SB_RECURRENCE(P_red32to1sig_out, P_red32to1sig_acc, Tn);
                    }


                  }
                }

                // Load next version of ky
                if(ky+1!=Ky) {
                  if(even_phase) { //load to odd_addr if 
                    SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, odd_addr);
                  } else {
                    SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky+1][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, even_addr);
                  }
                }

                even_phase=!even_phase;
                SB_WAIT_SCR_WR();
              }

              // write completed outputs out to memory
              SB_DMA_WRITE_SHF16(P_red32to1sig_out, 4*sizeof(VTYPE), 4*sizeof(VTYPE), Tn/4, &neuron_n[yout][xout][nn]);
              //SB_WAIT_ALL();
            }
            xout++; 
          }
          yout++;
        }
      }
    }
  }
  SB_WAIT(0);

  return 0;
}


int convolution_layer_sb_small_kernel(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni],  
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
  int c1=0,c2=0;
  VTYPE sum[Nn]={0};

  //int tn_elem = Tn * sizeof(VTYPE) / DATA_WIDTH;

  // Stream in CGRA config (do this somewhere else?) 
  SB_CONFIG(red32to1sig_config, red32to1sig_size);

  for (int yy = 0; yy < Ny; yy += Ty) {
    for (int xx = 0; xx < Nx; xx += Tx) {
      for (int nnn = 0; nnn < Nn; nnn += Tnn) {
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += Sy) { // tiling for y;
          int xout = xx/Sx;

          for (int x = xx; x < xx + Tx; x += Sx) { // tiling for x;
            for(int ky = 0; ky < Ky; ky++) {
              SB_DMA_SCRATCH_LOAD(&neuron_i[y+ky][x][0], sizeof(VTYPE) * Ni, Ni * sizeof(VTYPE), Kx, (Ni * sizeof(VTYPE) * Kx) * ky);
            }
            SB_WAIT(1);

            for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {

              SB_CONST(P_red32to1sig_acc, 0, Tn); 

              int t=0;
              int l=Ky*Kx*Ni;

              for (int ky = 0; ky < Ky; ky++) {  // sliding window;
                for (int kx = 0; kx < Kx; kx++) {
                  for (int ii=0; ii < Ni -Ti+1; ii += Ti) {
                    t+=Ti;
                    if(t!=l) {
                      SB_CONST(P_red32to1sig_pred, 0, Tn); 
                      SB_RECURRENCE(P_red32to1sig_out, P_red32to1sig_acc, Tn);
                    } else {
                      SB_CONST(P_red32to1sig_pred, 1, Tn);
                    }

                    //SB_CONST(P_red32to1sig_S, 1, Tn*Ti/4);
                    //SB_CONST(P_red32to1sig_N, 1, Tn*Ti/4);

                    //SB_DMA_READ(&neuron_i[y+ky][x+kx][ii],       0,                sizeof(VTYPE)*Ti, Tn, P_red32to1sig_N); 
                    SB_SCR_PORT_STREAM(((ky*Kx+kx)*Ni+ii)*sizeof(VTYPE), 0, sizeof(VTYPE)*Ti, Tn, P_red32to1sig_N);
                    SB_DMA_READ(&synapse[ky][kx][nn][ii], sizeof(VTYPE)*Ni, sizeof(VTYPE)*Ti, Tn, P_red32to1sig_S); 

                    //SB_WAIT(2);

                  }
                }
              }

              // write completed outputs out to memory
              SB_DMA_WRITE_SHF16(P_red32to1sig_out, 4*sizeof(VTYPE), 4*sizeof(VTYPE), Tn/4, &neuron_n[yout][xout][nn]);

              SB_WAIT(0);
            }
            xout++; 
          }
          yout++;
        }
      }
    }
  }
  SB_WAIT(0);

  return 0;

}
#endif

std::pair<int,int>  convolution_layer(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
  int c1=0,c2=0;
  VTYPE sum[Nn]={0};

  // — Original code — (excluding nn, ii loops)
  int yout = 0;
  for (int y = 0; y < Ny; y += Sy) { // tiling for y;
    int xout = 0;
    for (int x = 0; x < Ny; x += Sx) { // tiling for x;
      for (int nn = 0; nn < Nn; nn += Tn) {
        for (int n = nn; n < nn + Tn; n++) {
          sum[n]=0;
        }

        // sliding window;
        for (int ky = 0; ky < Ky; ky++)
          for (int kx = 0; kx < Kx; kx++)
            for (int n = nn; n < nn + Tn; n++)
              for (int i = 0; i < Ni; i++) {
                #if SHARED == 1 // version with shared kernels
                VTYPE sv = synapse[ky][kx][n][i];
                VTYPE nv = neuron_i[ky + y][kx + x][i];
                #else // version with private kernels
                VTYPE sv = synapse[yout][xout][ky][kx][n][i];
                VTYPE nv = neuron_i[ky + y][kx + x][i];
                #endif
                sum[n]+=sv*nv;
              }
        //sigmoid
        for (int n = nn; n < nn + Tn; n++) {
          neuron_n[yout][xout][n] = sigmoid(sum[n]);
          c2++;
        }
      }
      xout++; 
    }
    yout++;
  }
  return make_pair(c1,c2);
}


std::pair<int,int>  test_layer(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {

  begin_roi();
#ifdef SB
  // SB SMALL KERNEL IS TOO SLOW!
//    #if Ni * Kx * Ky * 2 < 4096
//    convolution_layer_sb_small_kernel(synapse,neuron_i,neuron_n);
//    #endif
    #if Ni == 8
    #warning USING DUBBLEBUF NI8
    convolution_layer_sb_doublebuf_ni8(synapse,neuron_i,neuron_n);

    #elif Ni == 16
    #warning USING DUBBLEBUF NI16
    convolution_layer_sb_doublebuf_ni16(synapse,neuron_i,neuron_n);

    #elif Ni * Kx * 2 > 4096
    #warning USING INNER DUBBLEBUF
    convolution_layer_sb_inner_doublebuf(synapse,neuron_i,neuron_n);
    #else 
    #warning USING REGULAR DOUBLEBUF
    convolution_layer_sb_doublebuf(synapse,neuron_i,neuron_n);
    #endif
#else
    #warning USING BLOCKED CPU VERSION
    convolution_layer_blocked(synapse,neuron_i,neuron_n);
#endif
  end_roi();
  sb_stats();

}



int main(const int argc, const char** argv) {
  cout << "allocating memory\n";

  #if SHARED == 1
  synapse = (VTYPE (*)[Ky][Kx][Nn][Ni]) aligned_malloc(64,SYNAPSE_SIZE*sizeof(VTYPE));
  #else
  synapse = (VTYPE (*)[NYSCL][NXSCL][Ky][Kx][Nn][Ni]) aligned_malloc(64,SYNAPSE_SIZE*sizeof(VTYPE));
  #endif

  neuron_i  = (VTYPE (*)[NYPAD][NXPAD][Ni])aligned_malloc(64,NYPAD*NXPAD*Ni*sizeof(VTYPE));
  neuron_n  = (VTYPE (*)[NYSCL][NXSCL][Nn])aligned_malloc(64,NYSCL*NXSCL*Nn*sizeof(VTYPE));
  neuron_n2 = (VTYPE (*)[NYSCL][NXSCL][Nn])aligned_malloc(64,NYSCL*NXSCL*Nn*sizeof(VTYPE));

  cout << "initializing arrays\n";

  #if SHARED == 1
  fill_convolution_shared_simple(*synapse,*neuron_i);
  #else
  fill_convolution_private(*synapse,*neuron_i);
  #endif

  cout << "starting computation\n";

  if(argc==3) {

//  } else if(argc==2 && string(argv[1])=="perf") {
  } else if(argc==2) {
    test_layer(*synapse,*neuron_i,*neuron_n2);
    //cout << "Perf Run Complete\n";
  } else {
    convolution_layer(*synapse,*neuron_i,*neuron_n);
    test_layer(*synapse,*neuron_i,*neuron_n2);
    cout << "computation complete!\n";  

    compare((VTYPE*)*neuron_n,(VTYPE*)*neuron_n2,NYSCL*NXSCL*Nn);

    int n_outputs= Ny/Sy * Ny/Sx * Nn;
    cout << "mults: " << n_outputs*Ni*Kx*Ky << " sigmoids: "  << n_outputs << "\n";
    cout << "argc: " << argc << "\n";
  }

  cout << "done\n";

  //cout << "mult-block:  " << calc.first   << " sigmoid-block: " << calc.second  << "\n";
  //cout << "mult-orig:  "  << calc2.first  << " sigmoid-orig:  " << calc2.second << "\n";
}



