#ifndef SOBEL_FILTER_H_
#define SOBEL_FILTER_H_
#include <systemc>
#include <cmath>
#include <iomanip>
using namespace sc_core;
using namespace sc_dt;
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

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

struct node_type {
    bool survivor[32]={0};
    #if decode_method == hard
    sc_uint<16> p_metrics;
    #else
    double p_metrics;
    #endif

};

struct decoder : public sc_module {
  tlm_utils::simple_target_socket<decoder> tsock;
	sc_fifo< bool > i_y1;
  sc_fifo< bool > i_y2;
  sc_fifo< bool > i_en;
  
	sc_fifo< unsigned char > o_result;

  SC_HAS_PROCESS(decoder);

  decoder(sc_module_name n): 
    sc_module(n), 
    tsock("t_skt"), 
    base_offset(0) 
  {
    tsock.register_b_transport(this, &decoder::blocking_transport);
    SC_THREAD(do_filter);
  }

  ~decoder() {
	}


  unsigned int base_offset;
  node_type node[8],n_node[8];
  void do_filter(){
    { wait(CLOCK_PERIOD, SC_NS); }

    for( sc_uint<4> i=0;i<(8);i++){             //initial every state
      node[i].p_metrics=65535;
      n_node[i].p_metrics=65535;
    } 
    node[0].p_metrics=0;


    while (true) {
        bool d1=i_y1.read();
        bool d2=i_y2.read();
        bool en_out=i_en.read();
        sc_uint<3>  input_node_a=0;
        sc_uint<3>  input_node_b=0;
        sc_uint<16> bm_a=0;
        sc_uint<16> bm_b=0;
        bool input=0;
		    bool u_d;
 node_loop:
        for(sc_uint<4> i=0;i<(8);i++){ 
            input=i.range(0,0);                                    
            input_node_a=input_node[i][0];
            input_node_b=input_node[i][1];
            //cout<<input_node_a<<"\t"<<input_node_b<<"\t"<<input<<"\n";
            bm_a=0;
            bm_b=0;
            if(d1!=state_o1[input_node_a][input])                                                //compute up branch metric
                bm_a++;
            if(d2!=state_o2[input_node_a][input])
                bm_a++;
            if(d1!=state_o1[input_node_b][input])                                                //compute down branch metric
                bm_b++;
            if(d2!=state_o2[input_node_b][input])
                bm_b++; 

            //cout<<d1<<d2<<"\t"<<bm_a<<"\t"<<bm_b<<"\t"<<input<<"\n";
    
            if((node[input_node_a].p_metrics+bm_a)<=(node[input_node_b].p_metrics+bm_b)){               //equal up metric win
                n_node[i].p_metrics=(node[input_node_a].p_metrics+bm_a);   
                if(input==1){
                    n_node[i].survivor[0]=1;
                }else{
                    n_node[i].survivor[0]=0;
                } 

                for(sc_uint<6> j=0;j<31;j++){
                    n_node[i].survivor[31-j]=node[input_node_a].survivor[31-j-1];                            //buffer shift
                }   
                                               
            }else{
                n_node[i].p_metrics=(node[input_node_b].p_metrics+bm_b);
                if(input==1){
                    n_node[i].survivor[0]=1;
                }else{
                    n_node[i].survivor[0]=0;
                }
                for(sc_uint<6> j=0;j<31;j++){
                    n_node[i].survivor[31-j]=node[input_node_b].survivor[31-j-1];                            //buffer shift
                }  
            }
          
        }

    shift_loop:
         for(sc_uint<4> i=0;i<(8);i++){                                                       //update node
            for(sc_uint<6> j=0;j<32;j++){
                node[i].survivor[j]=n_node[i].survivor[j];                           
                }
            node[i].p_metrics=n_node[i].p_metrics;
        }


		if(en_out==1){                                                                   //get decoded result from best metric survivor's bit 31
                sc_uint<16> min=65535;
                for(sc_uint<4> i=0;i<(8);i++){ 
                if(min>node[i].p_metrics){
                    min=node[i].p_metrics;
                    u_d=node[i].survivor[31];
                }
                
            }
    
        } 
        o_result.write(u_d);

    }
  }

  void blocking_transport(tlm::tlm_generic_payload &payload, sc_core::sc_time &delay){
    wait(delay);
    // unsigned char *mask_ptr = payload.get_byte_enable_ptr();
    // auto len = payload.get_data_length();
    tlm::tlm_command cmd = payload.get_command();
    sc_dt::uint64 addr = payload.get_address();
    unsigned char *data_ptr = payload.get_data_ptr();

    addr -= base_offset;


    // cout << (int)data_ptr[0] << endl;
    // cout << (int)data_ptr[1] << endl;
    // cout << (int)data_ptr[2] << endl;
    word buffer;

    switch (cmd) {
      case tlm::TLM_READ_COMMAND:
        // cout << "READ" << endl;
        switch (addr) {
          case SOBEL_FILTER_RESULT_ADDR:
            buffer.result = o_result.read();
            break;
          default:
            std::cerr << "READ Error! SobelFilter::blocking_transport: address 0x"
                      << std::setfill('0') << std::setw(8) << std::hex << addr
                      << std::dec << " is not valid" << std::endl;
          }
        data_ptr[0] = buffer.y[0];
        data_ptr[1] = buffer.y[1];
        data_ptr[2] = buffer.y[2];
        data_ptr[3] = buffer.y[3];
        break;
      case tlm::TLM_WRITE_COMMAND:
        // cout << "WRITE" << endl;
        switch (addr) {
          case SOBEL_FILTER_R_ADDR:
            i_y1.write(data_ptr[0]);
            i_y2.write(data_ptr[1]);
            i_en.write(data_ptr[2]);
            break;
          default:
            std::cerr << "WRITE Error! SobelFilter::blocking_transport: address 0x"
                      << std::setfill('0') << std::setw(8) << std::hex << addr
                      << std::dec << " is not valid" << std::endl;
        }
        break;
      case tlm::TLM_IGNORE_COMMAND:
        payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
        return;
      default:
        payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
        return;
      }
      payload.set_response_status(tlm::TLM_OK_RESPONSE); // Always OK
  }
};
#endif
