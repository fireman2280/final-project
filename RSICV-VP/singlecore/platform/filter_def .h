#ifndef FILTER_DEF_H_
#define FILTER_DEF_H_
#define CLOCK_PERIOD 10



using namespace sc_dt;
using namespace std;
#define truncate_length 32    //each survivor store 32bits 
#define output 2
#define memory_size 3



// Sobel Filter inner transport addresses
// Used between blocking_transport() & do_filter()
const int SOBEL_FILTER_R_ADDR = 0x00000000;
const int SOBEL_FILTER_RESULT_ADDR = 0x00000004;

const int SOBEL_FILTER_RS_R_ADDR   = 0x00000000;
const int SOBEL_FILTER_RS_W_WIDTH  = 0x00000004;
const int SOBEL_FILTER_RS_W_HEIGHT = 0x00000008;
const int SOBEL_FILTER_RS_W_DATA   = 0x0000000C;
const int SOBEL_FILTER_RS_RESULT_ADDR = 0x00800000;


union word {
  bool result;
  bool y[4];
};

// Sobel mask
const bool state_o1[8][2]={{0,1},{0,1},{1,0},{1,0},{1,0},{1,0},{0,1},{0,1}};              //compute output of each state by input
const bool state_o2[8][2]={{0,1},{1,0},{1,0},{0,1},{0,1},{1,0},{1,0},{0,1}}; 
const sc_uint<3>input_node[8][2]={{0,4},{0,4},{1,5},{1,5},{2,6},{2,6},{3,7},{3,7}};

                                        
#endif
