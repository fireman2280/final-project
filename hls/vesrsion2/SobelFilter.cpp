#include <cmath>
#ifndef NATIVE_SYSTEMC
#include "stratus_hls.h"
#endif

#include "SobelFilter.h"
using namespace sc_dt;
SobelFilter::SobelFilter( sc_module_name n ): sc_module( n )
{
#ifndef NATIVE_SYSTEMC
	HLS_FLATTEN_ARRAY(node);
	HLS_FLATTEN_ARRAY(n_node);
#endif
	SC_THREAD( do_filter );
	sensitive << i_clk.pos();
	dont_initialize();
	reset_signal_is(i_rst, false);
        
#ifndef NATIVE_SYSTEMC
	i_y.clk_rst(i_clk, i_rst);
  o_result.clk_rst(i_clk, i_rst);
#endif
}

SobelFilter::~SobelFilter() {}
   const bool state_o1[8][output]={{0,1},{0,1},{1,0},{1,0},{1,0},{1,0},{0,1},{0,1}};              //compute output of each state by input
    const bool state_o2[8][output]={{0,1},{1,0},{1,0},{0,1},{0,1},{1,0},{1,0},{0,1}}; 
    const sc_uint<3>input_node[8][2]={{0,4},{0,4},{1,5},{1,5},{2,6},{2,6},{3,7},{3,7}};
void SobelFilter::do_filter() {
	{
#ifndef NATIVE_SYSTEMC
		HLS_DEFINE_PROTOCOL("main_reset");
		i_y.reset();
		o_result.reset();
#endif
		wait();
	}
 

    for( sc_uint<4> i=0;i<(8);i++){             //initial every state
    node[i].p_metrics=65535;
    n_node[i].p_metrics=65535;
    } 
    node[0].p_metrics=0;
    
while_1:
	while (true) {
#if defined (II)
      HLS_PIPELINE_LOOP(SOFT_STALL, II, "Loop" );
#endif
				sc_uint<3> d;
#ifndef NATIVE_SYSTEMC
				{
					HLS_DEFINE_PROTOCOL("input");
					d = i_y.get();
					wait();
				}
#else
				rgb = i_rgb.read();
#endif
            d1=d.range(0,0);
            d2=d.range(1,1);
            en_out=d.range(2,2);
            input_node_a=0;
            input_node_b=0;
            bm_a=0;
            bm_b=0;
            input=0;
    node_loop:
        for(sc_uint<4> i=0;i<(8);i++){ 
           // HLS_CONSTRAIN_LATENCY(0, 6, "algorithm_latency01");
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
         for(sc_uint<4> i=0;i<(8);i++){   
               // HLS_CONSTRAIN_LATENCY(0, 2, "algorithm_latency04");                                                    //update node
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



			
#ifndef NATIVE_SYSTEMC
		{
			HLS_DEFINE_PROTOCOL("output");
			o_result.put(u_d);
			wait();
		}
#else
		o_result.write(total);
#endif
	}
}
