#ifndef TESTBENCH_H_
#define TESTBENCH_H_

#include <string>
using namespace std;

#include <systemc>
using namespace sc_core;

#ifndef NATIVE_SYSTEMC
#include <cynw_p2p.h>
#endif

#include "filter_def.h"

#define WHITE 255
#define BLACK 0
#define THRESHOLD 90

class Testbench : public sc_module {
public:
	sc_in_clk i_clk;
	sc_out < bool >  o_rst;
#ifndef NATIVE_SYSTEMC
	cynw_p2p< sc_dt::sc_uint<3> >::base_out o_y;
	cynw_p2p< sc_dt::sc_uint<1> >::base_in i_result;
#else
	sc_fifo_out< sc_dt::sc_uint<24> > o_rgb;
	sc_fifo_in< sc_dt::sc_uint<32> > i_result;
#endif

  SC_HAS_PROCESS(Testbench);

  Testbench(sc_module_name n);
  ~Testbench();

  int write_bmp(string outfile_name);
private:
	unsigned int n_txn;
	sc_time max_txn_time;
	sc_time min_txn_time;
	sc_time total_txn_time;
	sc_time total_start_time;
	sc_time total_run_time;
  unsigned long long SEED;
  unsigned long long RANV;
  int RANI;

  void encode(bool input,bool *mem,bool* x1,bool* x2);
  void get_input(bool*u);
  void channel(bool*x1 ,bool*x2,bool*y1,bool*y2,double*y1_f,double*y2_f,double SD);
  void add_noise(double*x1 ,double*x2,double*y1,double*y2,double SD);
  void normal(double &n1,double &n2,double s_d);
  double Ranq1();
  void do_test();
};
#endif
