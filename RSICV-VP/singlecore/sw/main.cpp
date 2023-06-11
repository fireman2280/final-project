#include "string"
#include "string.h"
#include "cassert"
#include <cstdio>
#include <cstdlib>
#include "math.h"
#include <iostream>

using namespace std;
#define truncate_length 32    //each survivor store 32bits 
#define N 1000
#define output 2
#define memory_size 3
union word {
  bool result;
  bool y[4];
};


bool _is_using_dma = false;
unsigned long long SEED;
unsigned long long RANV;
int RANI;

// Sobel Filter ACC
static bool* const SOBELFILTER_START_ADDR = reinterpret_cast<bool* const>(0x73000000);
static bool* const SOBELFILTER_READ_ADDR  = reinterpret_cast<bool* const>(0x73000004);

// DMA 
static volatile uint32_t * const DMA_SRC_ADDR  = (uint32_t * const)0x70000000;
static volatile uint32_t * const DMA_DST_ADDR  = (uint32_t * const)0x70000004;
static volatile uint32_t * const DMA_LEN_ADDR  = (uint32_t * const)0x70000008;
static volatile uint32_t * const DMA_OP_ADDR   = (uint32_t * const)0x7000000C;
static volatile uint32_t * const DMA_STAT_ADDR = (uint32_t * const)0x70000010;
static const uint32_t DMA_OP_MEMCPY = 1;





void write_data_to_ACC(bool* ADDR, bool* buffer, int len){
  if(_is_using_dma){  
    // Using DMA 
    *DMA_SRC_ADDR = (uint32_t)(buffer);
    *DMA_DST_ADDR = (uint32_t)(ADDR);
    *DMA_LEN_ADDR = len;
    *DMA_OP_ADDR  = DMA_OP_MEMCPY;
  }else{
    // Directly Send
 
    memcpy(ADDR, buffer, sizeof( bool)*len);
  }
}
void read_data_from_ACC(bool* ADDR, bool* buffer, int len){
  if(_is_using_dma){
    // Using DMA 
    *DMA_SRC_ADDR = (uint32_t)(ADDR);
    *DMA_DST_ADDR = (uint32_t)(buffer);
    *DMA_LEN_ADDR = len;
    *DMA_OP_ADDR  = DMA_OP_MEMCPY;
  }else{
    // Directly Read
    memcpy(buffer, ADDR, sizeof(bool)*len);
  
}
}
void  encode(bool input,bool *mem,bool* x1,bool* x2){ // encode generate two output x1 x2
bool mask[2][memory_size+1]={{1,0,1,1},{1,1,1,0}};
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


double  Ranq1(){

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

void  normal(double &n1,double &n2,double STD){
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

void  add_noise(double*x1 ,double*x2,double*y1,double*y2,double SD){     //add noise to each bits
    double n1=0,n2=0;
        normal(n1,n2,SD);
        *y1=*x1+n1;
        *y2=*x2+n2;
    
}



void  channel(bool*x1 ,bool*x2,bool*y1,bool*y2,double*y1_f,double*y2_f,double SD){     //through channel
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

  add_noise(&channel_x1,&channel_x2,&channel_y1,&channel_y2,SD);
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







int main(int argc, char *argv[]) {

bool  buffer[4] = {0};
word data;
int c=0;
for(int k=6;k<8;k++){
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
        buffer[0]=y1;
        buffer[1]=y2;
        buffer[2]=en_out;
        write_data_to_ACC(SOBELFILTER_START_ADDR, buffer, 4);
       
        
        read_data_from_ACC(SOBELFILTER_READ_ADDR, buffer, 4);
        memcpy(data.y, buffer, 4);
        bool u_d=data.result;
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
    cout<<"\n";
    cout<<"error after decode:\t"<<error_decoded<<"\n BER:\t";
    //ofs_hard<<setprecision(6)<<BER<<"\t";
   }
  

}
