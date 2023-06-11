#ifndef SOBEL_FILTER_H_
#define SOBEL_FILTER_H_
#include <systemc>
using namespace sc_core;

#ifndef NATIVE_SYSTEMC
#include <cynw_p2p.h>
#endif

#include "filter_def.h"
struct node_type {
    bool survivor[32]={0};
    #if decode_method == hard
    int  p_metrics;
    #else
    double p_metrics;
    #endif

};

class SobelFilter: public sc_module
{
public:
	sc_in_clk i_clk;
	sc_in < bool >  i_rst;
#ifndef NATIVE_SYSTEMC
	cynw_p2p< sc_dt::sc_uint<3>  >::in i_y;
	cynw_p2p< sc_dt::sc_uint<1> >::out o_result;
#else
	sc_fifo_in< sc_dt::sc_uint<24> > i_rgb;
	sc_fifo_out< sc_dt::sc_uint<32> > o_result;
#endif

	SC_HAS_PROCESS( SobelFilter );
	SobelFilter( sc_module_name n );
	~SobelFilter();
private:
	void do_filter();
	node_type node[8],n_node[8];
	int   input_node_a=0;
    int   input_node_b=0;
    int  bm_a=0;
    int  bm_b=0;
    bool input=0;
	bool u_d;
	bool d1;
	bool d2;
	bool en_out;
};
#endif
