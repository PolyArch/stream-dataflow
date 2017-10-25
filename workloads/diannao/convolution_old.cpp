#include <iostream>
#include <string>
#include "dnn.hpp"

using namespace std;

#ifndef SHARED
#define SHARED 1
#endif

#ifndef Ny
  //Problem Size
  #define Ny 32
  #define Ny 32
  
  #define Kx 4
  #define Ky 4
  //#define Ni 108
  //#define Nn 200
  
  #define Ni 112
  #define Nn 224
#endif

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
#define NXPAD (Ny+Kx)

#define NYSCL (Ny/Sy)
#define NXSCL (Ny/Sx)


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
          synapse[yy][xx][nn][ni] = 2;
  } } } }
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_i[yy][xx][ni] = 1;
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
              synapse[xout][yout][yy][xx][nn][ni] = 2;
  } } } } } }
  for(int yy = 0; yy < NYPAD; ++yy) {
    for(int xx = 0; xx < NXPAD; ++xx) {      
      for(int ni = 0; ni < Ni; ++ni) {
        neuron_i[yy][xx][ni] = 1;
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
    for (int xx = 0; xx < Ny; xx += Tx) {
      for (int nnn = 0; nnn < Nn; nnn += Tnn) {
        int yout = yy/Sy;
        for (int y = yy; y < yy + Ty; y += Sy) { // tiling for y;
          int xout = xx/Sx;

          for (int x = xx; x < xx + Tx; x += Sx) { // tiling for x;

            //LOAD_SCRATCH -- read cube from larger 3D cube to compact 3D cube
            //start_addr: neuron[y][x][i]
            //stride_size: Ni*Kx
            //stride: Ni*Nx
            //num_strides: ky

            for (int nn = nnn; nn < nnn + Tnn; nn += Tn) {
              for (int n = nn; n < nn + Tn; n++) {
                sum[n] = 0;
              }

              for (int ky = 0; ky < Ky; ky++) {  // sliding window;
                for (int kx = 0; kx < Kx; kx++) {

                  int ii = 0;
                  VTYPE sum_sc;

                  for (; ii < Ni -Ti+1; ii += Ti) {
                    //SCRATCH -> PORT
                    // addr: neuron_i[ky + y][kx + x][ii]
                    // stride_len: Tn*2
                    // num_strides: 8
                    
                    //DMA -> PORT
                    // addr: synapse[ky][kx][n][ii]
                    // stride_len: Tn*2
                    // stride_dist: 
                    // num_strides:8

                    //*****
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
                        sum_sc+=(sv*nv)>>1;
                      }
                      sum[n]+=sum_sc;
                    }
                    //****
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

/*
* MM convolution layer implemented for softbrain
* Sb config constants (SCRATCHSIZE, NUMPIPES, etc) in softbrain.hh
* Code assumes NUMPIPES = 2
* Safe because inputs % PIPEDEPTH = 0 (for PIPEDEPTH = 32)
* This version does not tile inputs. If the inputs don't fit in the 
* scratchpad, it invokes convolution_layer_sb_tiled
* NOTE THE CHANGED ARRAY DIMS FOR SYNAPSE
*/
int convolution_layer_sb(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni],   // MM CHANGED ORDERING OF ALL INPUTS
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NXPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {

  // Stream in CGRA config 
  int *cgra_config;
  int cgra_cofig_sz;
  DMA_SB_CONFIG(cgra_config, cgra_config_sz);
 
  if(Kx * Ky * Ni > SCRATCHSIZE){       // input neurons don't fit in scratch
    convolution_layer_sb_tiled(synapse, neuron_i, neuron_n); // call tiled version
  }

  VTYPE neuron_i_scratch[Ky][Kx][Ni];
  &neuron_i_scratch[0][0][0] = SCRATCHSTART;
  int yout = 0;
  for (int y = 0; y < Ny; y += Sy) {
    int xout = 0;
    for (int x = 0; x < Nx; x += Sx) {
      IC_DMA_SCRATCH_LOAD(&neuron_i[y][x][0], sizeof(VTYPE) * Ni * Nx, sizeof(VTYPE) * Ni * Kx, Ky, &neuron_i_scratch[0][0][0]);
      for(int n = 0; n < Nn; n += 2*PIPEDEPTH){ // each pipe does PIPEDEPTH output layers
        IC_CONST(INPUTPRED0, 0, Ni*Kx*Ky - 1);
        IC_CONST(INPUTPRED1, 0, Ni*Kx*Ky - 1); 
        for(int ky = 0; ky < Ky; ++ky){ // Spin through windows...
          for(int kx = 0; kx < Kx; ++kx){ 
            for(int i = 0; i < Ni; i+=PIPEWIDTH){   // ...and input layers
              for(int nn = 0; nn < PIPEDEPTH; ++nn){ // Both pipes get PIPEDEPTH copies of same neurons
                IC_SCRATCH_READ(&neuron_i_scratch[ky][kx][i], PIPEWIDTH, INPUTNEURON0);  
                IC_SCRATCH_READ(&neuron_i_scratch[ky][kx][i], PIPEWIDTH, INPUTNEURON1);  
              } 
          
              // Each entry gets weights for different output layers
              #if SHARED == 1
                IC_DMA_READ((&synapse)[ky][kx][n][i], sizeof(VTYPE)*Ni, sizeof(VTYPE)*PIPEWIDTH, PIPEDEPTH, INPUTWEIGHT0); 
                IC_DMA_READ((&synapse)[ky][kx][n+PIPEDEPTH][i], sizeof(VTYPE)*Ni, sizeof(VTYPE)*PIPEWIDTH, PIPEDEPTH, INPUTWEIGHT1); 
              #else
                IC_DMA_READ((&synapse)[yout][xout][ky][kx][n][i], sizeof(VTYPE)*Ni, sizeof(VTYPE)*PIPEWIDTH, PIPEDEPTH, INPUTWEIGHT0); 
                IC_DMA_READ((&synapse)[yout][xout][ky][kx][n+PIPEWIDTH][i], sizeof(VTYPE)*Ni, sizeof(VTYPE)*PIPEWIDTH, PIPEDEPTH, INPUTWEIGHT1); 
              #endif

              if(ky + 1 < Ky || kx + 1 < Kx || i + PIPEWIDTH < Ni){ // don't recurse on last pass
                OC_RECURRENCE(OUTPUT0, INPUTACC0, PIPEDEPTH); // until input stack complete
                OC_RECURRENCE(OUTPUT1, INPUTACC1, PIPEDEPTH);
              }
              if((kx + 1 == Kx) && (ky + 1 == Ky) && (i + PIPEWIDTH >= Ni - 1)){    // sigmoid -- before last step of last tile
                IC_CONST(INPUTPRED0, 1, PIPEDEPTH); 
                IC_CONST(INPUTPRED1, 1, PIPEDEPTH); 
              }
            }
          }
        }
        // Write completed input stacks out to mem
        OC_DMA_WRITE(OUTPUT0, sizeof(VTPYE), sizeof(VTYPE), PIPEDEPTH, neuron_n[yout][xout][n]);
        OC_DMA_WRITE(OUTPUT1, sizeof(VTPYE), sizeof(VTYPE), PIPEDEPTH, neuron_n[yout][xout][n+PIPEDEPTH]);
      }
      xout++; 
    }
    yout++;
  }
  
  return 0;
}

// The full input stack won't fit in the scratchpad at once,
// so we must tile it into chunks that do.
std::pair<int,int>  convolution_layer_sb_tiled(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NYPAD][Ni], 
                    VTYPE (&neuron_n)[NYSCL][NXSCL][Nn]) {
 
  // Most places in the code, Ni is replaced with ti-excess (number of input layers in current tile) 
  int ti = SCRATCHSIZE / (Kx*Ky); // tile size: # of input layers that fit in scratch, round down
  if(ti % PIPEWIDTH != 0){      // Make sure ti%pipewidth = 0 for easy chunking later
    ti = ti - (ti % PIPEWIDTH);
  }
  int excess;

  VTYPE neuron_i_scratch[Ky][Kx][ti];
  neuron_i_scratch = SCRATCHSTART;
 
  // Outer tiled loop 
  for(int ii = 0; ii < Ni - 1 + ti; ii += ti){
    // deal with overflow (Ni%ti != 0)
    if(ii >= Ni){
      excess = ii - (Ni - 1);
    } else {
      excess = 0;
    }
    int yout = 0;
    for (int y = 0; y < Ny; y += Sy) {
      int xout = 0;
      for (int x = 0; x < Nx; x += Sx) {
        IC_DMA_SCRATCH_LOAD(&neuron_i[y][x][ii], sizeof(VTYPE)*Ni*Nx, sizeof(VTYPE)*(ti-excess)*Kx, Ky, neuron_i_scratch);
        for(int n = 0; n < Nn; n += 2*PIPEDEPTH){ // each pipe does PIPEDEPTH output layers
          IC_CONST(INPUTPRED0, 0, (ti-excess)*Kx*Ky - 1);
          IC_CONST(INPUTPRED1, 0, (ti-excess)*Kx*Ky - 1); 
         
          // If not first ii itr, load output acc. from memory
          if(ii != 0){
            IC_DMA_READ(neruon_n[yout][xout][n], sizeof(VTYPE) * PIPEDEPTH, sizeof(VTYPE)*PIPEDEPTH, 1, INPUTACC0); 
            IC_DMA_READ(neruon_n[yout][xout][n+PIPEDEPTH], sizeof(VTYPE) * PIPEDEPTH, sizeof(VTYPE)*PIPEDEPTH, 1, INPUTACC1); 
          }
 
          for(int ky = 0; ky < Ky; ++ky){ // Spin through windows...
            for(int kx = 0; kx < Kx; ++kx){ 
              for(int i = ii; i < ii+(ti-excess); i+=PIPEWIDTH){   // ...and input layers
                for(int nn = 0; nn < PIPEDEPTH; ++nn){ // Both pipes get PIPEDEPTH copies of same neurons
                  IC_SCRATCH_READ(&neuron_i_scratch[ky][kx][i], PIPEWIDTH, INPUTNEURON0);  
                  IC_SCRATCH_READ(&neuron_i_scratch[ky][kx][i], PIPEWIDTH, INPUTNEURON1);  
                } 
          
                // Each entry gets weights for different output layers
                #if SHARED == 1
                  IC_DMA_READ((&synapse)[ky][kx][n][i], sizeof(VTYPE)*Ni, sizeof(VTYPE)*PIPEWIDTH, PIPEDEPTH, INPUTWEIGHT0); 
                  IC_DMA_READ((&synapse)[ky][kx][n+PIPEDEPTH][i], sizeof(VTYPE)*Ni, sizeof(VTYPE)*PIPEWIDTH, PIPEDEPTH, INPUTWEIGHT1); 
                #else
                  IC_DMA_READ((&synapse)[yout][xout][ky][kx][n][i], sizeof(VTYPE)*Ni, sizeof(VTYPE)*PIPEWIDTH, PIPEDEPTH, INPUTWEIGHT0); 
                  IC_DMA_READ((&synapse)[yout][xout][ky][kx][n+PIPEWIDTH][i], sizeof(VTYPE)*Ni, 
                              sizeof(VTYPE)*PIPEWIDTH, PIPEDEPTH, INPUTWEIGHT1); 
                #endif

                if(ky + 1 < Ky || kx + 1 < Kx || i + PIPEWIDTH < (ti-excess)){ // don't recurse on last pass
                  OC_RECURRENCE(OUTPUT0, INPUTACC0, PIPEDEPTH); // until input stack complete
                  OC_RECURRENCE(OUTPUT1, INPUTACC1, PIPEDEPTH);
                }
                if((kx + 1 == Kx) && (ky + 1 == Ky) && (i + PIPEWIDTH >= (ti-excess) - 1)){    // sigmoid -- before last step of last tile
                  IC_CONST(INPUTPRED0, 1, PIPEDEPTH); 
                  IC_CONST(INPUTPRED1, 1, PIPEDEPTH); 
                }
              }
            }
          }
          // Write partial output stacks out to mem
          OC_DMA_WRITE(OUTPUT0, sizeof(VTPYE), sizeof(VTYPE), PIPEDEPTH, neuron_n[yout][xout][n]);
          OC_DMA_WRITE(OUTPUT1, sizeof(VTPYE), sizeof(VTYPE), PIPEDEPTH, neuron_n[yout][xout][n+PIPEDEPTH]);
        }
        xout++;
      } 
      yout++;
    }
  }
 
  return 0;
}

std::pair<int,int>  convolution_layer(
#if SHARED == 1
                    VTYPE (&synapse)[Ky][Kx][Nn][Ni], 
#else
                    VTYPE (&synapse)[NYSCL][NXSCL][Ky][Kx][Nn][Ni], 
#endif
                    VTYPE (&neuron_i)[NYPAD][NYPAD][Ni], 
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
                sum[n]+=(sv*nv)>>1;
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

int main(const int argc, const char** argv) {

  #if SHARED == 1
  synapse = (VTYPE (*)[Ky][Kx][Nn][Ni]) malloc(SYNAPSE_SIZE*sizeof(VTYPE));
  #else
  synapse = (VTYPE (*)[NYSCL][NXSCL][Ky][Kx][Nn][Ni]) malloc(SYNAPSE_SIZE*sizeof(VTYPE));
  #endif

  neuron_i  = (VTYPE (*)[NYPAD][NXPAD][Ni])malloc(NYPAD*NXPAD*Ni*sizeof(VTYPE));
  neuron_n  = (VTYPE (*)[NYSCL][NXSCL][Nn])malloc(NYSCL*NXSCL*Nn*sizeof(VTYPE));
  neuron_n2 = (VTYPE (*)[NYSCL][NXSCL][Nn])malloc(NYSCL*NXSCL*Nn*sizeof(VTYPE));

  #if SHARED == 1
  fill_convolution_shared_simple(*synapse,*neuron_i);
  #else
  fill_convolution_private(*synapse,*neuron_i);
  #endif

  begin_roi();
  if(argc==3) {

//  } else if(argc==2 && string(argv[1])=="perf") {
  } else if(argc==2) {
    auto calc  = convolution_layer_blocked(*synapse,*neuron_i,*neuron_n);
    //cout << "Perf Run Complete\n";
  } else {
  cout << "argc: " << argc << "\n";

    auto calc  = convolution_layer_blocked(*synapse,*neuron_i,*neuron_n);
    auto calc2 = convolution_layer(*synapse,*neuron_i,*neuron_n2);
    if(calc.first!=0) {
      cout << "blocks=" << calc.first << "\n";
    }
    compare((VTYPE*)*neuron_n,(VTYPE*)*neuron_n2,NYSCL*NXSCL*Nn);
    int n_outputs= Ny/Sy * Ny/Sx * Nn;
    cout << "mults: " << n_outputs*Ni*Kx*Ky << " sigmoids: "  << n_outputs << "\n";
    cout << "argc: " << argc << "\n";
  }
  end_roi();
 
  //cout << "mult-block:  " << calc.first   << " sigmoid-block: " << calc.second  << "\n";
  //cout << "mult-orig:  "  << calc2.first  << " sigmoid-orig:  " << calc2.second << "\n";
}

