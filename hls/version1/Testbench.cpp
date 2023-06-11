#include <cstdio>
#include <cstdlib>
using namespace std;

#include "Testbench.h"


Testbench::Testbench(sc_module_name n) : sc_module(n) {
  SC_THREAD(do_test);
  sensitive << i_clk.pos();
  dont_initialize();
}

Testbench::~Testbench() {
	//cout<< "Max txn time = " << max_txn_time << endl;
	//cout<< "Min txn time = " << min_txn_time << endl;
	//cout<< "Avg txn time = " << total_txn_time/n_txn << endl;
	cout << "Total run time = " << total_run_time << endl;
}


int Testbench::write_bmp(string outfile_name) {
  FILE *fp_t = NULL;      // target file handler
  unsigned int file_size; // file size

  fp_t = fopen(outfile_name.c_str(), "wb");
  if (fp_t == NULL) {
    printf("fopen %s error\n", outfile_name.c_str());
    return -1;
  }


  fclose(fp_t);
  return 0;
}

void Testbench::do_test() {
for(int k=6;k<7;k++){
    bool u[6]={0,0,0,0,0,1};
    bool input_buffer[truncate_length]={0};
    bool mem[memory_size]={0,0,0};
    bool out=0;
    int error_c=0;
    int error_decoded=0;
    double BER=0;
    double SNR=0;
    double SD=0;
    SEED=2023,RANI=0;
    SNR=k*0.5;                //dB
    SD=sqrt(1/(pow(10,SNR/10)));

  #ifndef NATIVE_SYSTEMC
	o_y.reset();
  i_result.reset();
  #endif
	o_rst.write(false);
	wait(5);
	o_rst.write(true);
	wait(1);
	total_start_time = sc_time_stamp();



    for(int i=0;i<(N+truncate_length-1);i++){                   //generate the imformation bits period 63  
        bool u6=u[4]^u[5];
        bool x1,x2;
        encode(u[5],mem,&x1,&x2);
        bool y1,y2;
        double y1_f,y2_f;
        channel(&x1,&x2,&y1,&y2,&y1_f,&y2_f,SD);    //through channel
        if(x1!=y1)
        error_c++;
        else if(x2!=y2){
        error_c++;
        }
        bool en_out=0;
        if(i>=31){
          en_out=1;
        } 
        sc_dt::sc_uint<3> y;
        y.range(0, 0)=y1;
        y.range(1, 1)=y2;
        y.range(2, 2)=en_out;
        o_y.put(y);
       
        sc_dt::sc_uint<1> u_d;
        u_d=i_result.get();
        if(i>=31){
        //cout<<u_d;
        if(u_d!=input_buffer[31])
        error_decoded++;
        }

        input_buffer[0]=u[5];
        for(int j=0;j<truncate_length-1;j++){                   //shift u input buffer
            input_buffer[truncate_length-j-1]=input_buffer[truncate_length-j-2];
        }
        for(int j=0;j<memory_size-1;j++){                  //shift memory 
            mem[memory_size-j-1]=mem[memory_size-j-2];
        }
        
        mem[0]=u[5];
        for(int j=0;j<5;j++){                   //shift u input buffer
         u[5-j]=u[5-j-1];
        }
        u[0]=u6;
       }
    cout<<"\n"<<"channel error: \t"<<error_c;
    BER=(double)error_decoded/(double)(N);
    cout<<"\n";
    cout<<"error after decode:\t"<<error_decoded<<"\n BER:\t";
    printf("%.6lf",BER);
    //ofs_hard<<setprecision(6)<<BER<<"\t";
   }
  total_run_time = sc_time_stamp() - total_start_time;
  sc_stop();
}





void Testbench::encode(bool input,bool *mem,bool* x1,bool* x2){ // encode generate two output x1 x2
bool mask[output][memory_size+1]={{1,0,1,1},{1,1,1,0}};
bool temp1,temp2;
    temp1=0,temp2=0;
    temp1=(input)*mask[0][0];
    temp2=(input)*mask[1][0];
    for(int j=0;j<memory_size;j++){
    temp1^=(mem[j]*mask[0][j+1]);
    temp2^=(mem[j]*mask[1][j+1]);
    }
    *x1=temp1;
    *x2=temp2;  
}


double Testbench::Ranq1(){

    if(RANI ==0){
        RANV=SEED^4101842887655102017LL;
        RANV^= (RANV>>21);
        RANV^= (RANV<<35);
        RANV^= (RANV>>4);
        RANV =RANV*2685821657736338717LL;
        RANI++;
    }
        RANV^= (RANV>>21);
        RANV^= (RANV<<35);
        RANV^= (RANV>>4);
        return RANV*(2685821657736338717LL)*5.42101086242752217E-20;
}

void Testbench::normal(double &n1,double &n2,double STD){
    double s=0,x1,x2;
    do{
        x1=Ranq1();
        x2=Ranq1();
        x1=2*x1-1;
        x2=2*x2-1;
        s=pow(x1,2)+pow(x2,2);
    }while(s>=1.0);

    n1=STD*x1*sqrt(-2*log(s)/s);
    n2=STD*x2*sqrt(-2*log(s)/s);
}

void Testbench::add_noise(double*x1 ,double*x2,double*y1,double*y2,double SD){     //add noise to each bits
    double n1=0,n2=0;
        normal(n1,n2,SD);
        *y1=*x1+n1;
        *y2=*x2+n2;
    
}



void Testbench::channel(bool*x1 ,bool*x2,bool*y1,bool*y2,double*y1_f,double*y2_f,double SD){     //through channel
double channel_x1;
double channel_x2;
double channel_y1;
double channel_y2;                  
    if(*x1){                  //map 0 to -1   1 to 1
        channel_x1=1;
    }else{
        channel_x1=-1;
    }
    if(*x2){
        channel_x2=1;
    }else{
        channel_x2=-1;
    }

Testbench::add_noise(&channel_x1,&channel_x2,&channel_y1,&channel_y2,SD);
    *y1_f=channel_y1;              //map 0 to -1   1 to 1
    *y2_f=channel_y2;
    if(channel_y1>=0){
        *y1=1;
    }else{
        *y1=0;
    }
    if(channel_y2>=0){
        *y2=1;
    }else{
        *y2=0;
    }
}
