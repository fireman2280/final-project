#include "string"
#include "string.h"
#include "math.h"

using namespace std;
#define truncate_length 32    //each survivor store 32bits 
#define N 1000
#define output 2
#define memory_size 3
struct word {
  bool result;
  bool y[4];
};

#define PROCESSORS 2

unsigned long long SEED;
unsigned long long RANV;
int RANI;

// Sobel Filter ACC
static bool* const FILTER_START_ADDR[PROCESSORS] = {reinterpret_cast<bool* const>(0x73000000),reinterpret_cast<bool* const>(0x74000000)};
static bool* const FILTER_READ_ADDR[PROCESSORS] = {reinterpret_cast<bool* const>(0x73000004),reinterpret_cast<bool* const>(0x74000004)};
// DMA 
static volatile uint32_t * const DMA_SRC_ADDR  = (uint32_t * const)0x70000000;
static volatile uint32_t * const DMA_DST_ADDR  = (uint32_t * const)0x70000004;
static volatile uint32_t * const DMA_LEN_ADDR  = (uint32_t * const)0x70000008;
static volatile uint32_t * const DMA_OP_ADDR   = (uint32_t * const)0x7000000C;
static volatile uint32_t * const DMA_STAT_ADDR = (uint32_t * const)0x70000010;
static const uint32_t DMA_OP_MEMCPY = 1;
static const uint32_t DMA_OP_NOP = 0;
bool _is_using_dma = true;
//the barrier synchronization objects
uint32_t barrier_counter=0; 
uint32_t barrier_lock; 
uint32_t barrier_sem; 
uint32_t pass0=0; 
uint32_t pass1=0; 
//the mutex object to control global summation
uint32_t lock; 
uint32_t lock_0; 
uint32_t lock_p0;
uint32_t lock_p1;
int dma_in=0;
int dma_out=0;
//print synchronication semaphore (print in core order)
uint32_t print_sem[PROCESSORS]; 
int sem_init (uint32_t *__sem, uint32_t count) __THROW{
  *__sem=count;
  return 0;
}
int sem_wait (uint32_t *__sem) __THROW{
  uint32_t value, success; //RV32A
  __asm__ __volatile__("\
L%=:\n\t\
     lr.w %[value],(%[__sem])            # load reserved\n\t\
     beqz %[value],L%=                   # if zero, try again\n\t\
     addi %[value],%[value],-1           # value --\n\t\
     sc.w %[success],%[value],(%[__sem]) # store conditionally\n\t\
     bnez %[success], L%=                # if the store failed, try again\n\t\
"
    : [value] "=r"(value), [success]"=r"(success)
    : [__sem] "r"(__sem)
    : "memory");
  return 0;
}

int sem_post (uint32_t *__sem) __THROW{
  uint32_t value, success; //RV32A
  __asm__ __volatile__("\
L%=:\n\t\
     lr.w %[value],(%[__sem])            # load reserved\n\t\
     addi %[value],%[value], 1           # value ++\n\t\
     sc.w %[success],%[value],(%[__sem]) # store conditionally\n\t\
     bnez %[success], L%=                # if the store failed, try again\n\t\
"
    : [value] "=r"(value), [success]"=r"(success)
    : [__sem] "r"(__sem)
    : "memory");
  return 0;
}

int barrier(uint32_t *__sem, uint32_t *__lock, uint32_t *counter, uint32_t thread_count) {
	sem_wait(__lock);
	if (*counter == thread_count - 1) { //all finished
		*counter = 0;
		sem_post(__lock);
		for (int j = 0; j < thread_count - 1; ++j) sem_post(__sem);
	} else {
		(*counter)++;
		sem_post(__lock);
		sem_wait(__sem);
	}
	return 0;
}

void write_data_to_ACC(bool* ADDR, bool* buffer, int len){
  if(_is_using_dma){  
    // Using DMA 
    *DMA_SRC_ADDR = (uint32_t)(buffer);
    *DMA_DST_ADDR = (uint32_t)(ADDR);
    *DMA_LEN_ADDR = len;
    *DMA_OP_ADDR  = DMA_OP_MEMCPY;
    dma_in++;
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
    dma_out++;
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



int main(unsigned hart_id) {

  
	if (hart_id == 0) {
		// create a barrier object with a count of PROCESSORS
		sem_init(&barrier_lock, 1);
		sem_init(&barrier_sem, 0); //lock all cores initially
		for(int i=0; i< 2; ++i){
			sem_init(&print_sem[i], 0); //lock printing initially
		}
		// Create mutex lock
		sem_init(&lock, 1);
	}

	
bool  buffer[4] = {0};
word data;

 
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
    if(hart_id==0){
    SNR=3;   
    }else{
    SNR=3.5;
    }               //dB
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
        write_data_to_ACC(FILTER_START_ADDR[hart_id], buffer, 4);
       
        
        read_data_from_ACC(FILTER_READ_ADDR[hart_id], buffer, 4);
        
        bool u_d=buffer[0];
        if(i>=31){
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
      
      sem_wait(&lock);
      printf("\n%d\tchannel error: \t %d ",hart_id,error_c);
       sem_post(&lock);
        sem_wait(&lock);
      printf("\n%d\t error after decode: \t %d ",hart_id,error_decoded);
       sem_post(&lock);
    // barrier(&barrier_sem, &barrier_lock, &barrier_counter, 2);

    // barrier(&barrier_sem, &barrier_lock, &barrier_counter, 2);

    printf("\ndma_in%d \tdma_out%d",dma_in,dma_out);
  return 0;

}
