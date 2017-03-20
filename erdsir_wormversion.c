/*

- THE PLAGUE INTERPRETER

... to bring back to all of us a natural, occult equivalent of the
dogma we no longer believe. [Antonin Artaud]

Returning the body, electronics, and dystopic code to the earth,
revived and decoded years later as "yersinia pestis".

///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// SIR 2016 re-issue with some tweaks and new Turing Machine implementation.
////////////////////////////////////////////////////////////////////////////

LAYOUT:

   0 // PLAGUE/CPU ADC:X+2
0 // INSTR ADC:X+1
   0 // SPEED ADC: X+0

0 0 0 // CPU/SPEED/INST

*/

#define F_CPU 16000000UL 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#ifdef AVR_IS
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#define randi() (adcread(1)+adcread(5))  // INSTRUCTION
#else
#include <time.h>
#define randi() (rand()%255)
#endif

typedef int u16;
typedef unsigned char u8;

#define trand ((rand()%255)+randi()+(adcread(7)&15))
//#define randi() (incincom())
#define CELLLEN 16
#define floor(x) ((int)(x))
#define MAX_SAM 255
#define MAX_SAM2 127

#define BV(bit) (1<<(bit)) // Byte Value => converts bit into a byte value. One at bit location.
#define cbi(reg, bit) reg &= ~(BV(bit)) // Clears the corresponding bit in register reg
#define sbi(reg, bit) reg |= (BV(bit))              // Sets the corresponding bit in register reg
#define HEX__(n) 0x##n##UL
#define recovered 129
#define dead 255                                                                   
#define susceptible 0

signed char insdir,dir; 
unsigned char modifier,flaggy; 
unsigned char xxx[MAX_SAM+12]; // for tm this is our tape
unsigned char desc[64]; // read into desc from randi and check for bounds
//unsigned char incom[255];
signed char m_stack_pos;
u8 m_stack[16];
u8 m_reg8bit1,m_reg8bit2;

#define NOP 0
#define ORG 1
#define EQU 2
#define JMP 3 
#define JMPZ 4
#define PSHL 5
#define PSH 6 
#define PSHI 7
#define POP 8
#define POPI 9
#define ADD 10
#define SUB 11
#define INC 12
#define DEC 13
#define AND 14
#define OR 15
#define XOR 16
#define NOT 17
#define ROR 18
#define ROL 19
#define PIP 20
#define PDP 21
#define DUP 22
#define SAY 23
#define INP 24

//unsigned char countt=0;

#ifdef AVR_IS
unsigned char adcread(unsigned char channel){
  unsigned char result, high;
  ADMUX &= 0xF8; // clear existing channel selection                
  ADMUX |=(channel & 0x07); // set channel/pin
  ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
  loop_until_bit_is_set(ADCSRA, ADIF); /* Wait for ADIF, will happen soon */
  result=ADCH;
  return result;
}
#else
unsigned char adcread(unsigned char channel){
  unsigned char result, high;
  if (channel==0 ||channel==1 || channel==7) result=rand()%255;
  else result=10;
  return result;
}

#endif

unsigned char oldone;

/*ISR(TIMER2_COMPA_vect){//timer2 interrupt 200 Hz
  //    incom[countt]=(adcread(0)+adcread(5));
  //  countt++;
  // divider
  static unsigned char speedy=0;
  if (++speedy>=speed) { //once per sec test was passed
    speedy=0;
    oldone++;
    if (flaggy==1) {
	    flaggy=0;
	  }
    else modifier=xxx[oldone];
	  /// invert or?
    OCR0A=255-modifier;
    //      OCR0A=255-(xxx[oldone]);
  }
  }*/


/*inline u8 incincom(){
    static unsigned char counttt=0;
    unsigned char returnee=incom[counttt];
    counttt++;
    }*/

u8 thread_stack_count(u8 c) { 
    return (c-1)<=m_stack_pos; 
}

void thread_push(u8 data) {
  if (m_stack_pos<14 && m_stack_pos>-1) // STACK_SIZEE-2
	{
	  m_stack[++m_stack_pos]=data;
	}
}

u8 thread_pop() {
 	if (m_stack_pos>=0 && m_stack_pos<16)
	{
	  u8 ret=m_stack[m_stack_pos];
	  m_stack_pos--;
	  return ret;
		//		return 0;
	}
	//    printf("errorr\n");
	return 0;   
}

u8 thread_top() {
	if (m_stack_pos>=0 && m_stack_pos<16)
	{
	  return m_stack[m_stack_pos];
	}
	return 0;
}

inline u8 machine_p88k(u8 addr) {
  //	return m_heap[addr%HEAP_SIZE];
  return xxx[addr];
}


inline void machine_poke(u8 addr, u8 data) {
  //	m_heap[addr%HEAP_SIZE]=data;
  xxx[addr]=data;
  //      printf("%d\n",&m_memory[addr]);
}


u8 antrulee(u8 dir,u8 inst, u8 rule){
  u8 index;
  // process state from rule 
  // rule in binary
  // inst is index into binary array - so inst must be lower than 0->7 - it is
  index=(rule>>inst)&1;
  if (index==0) return dir-1; 
  else return dir+1; 
  }

// above ported from cpuintrev

#ifdef AVR_IS
void adc_init(void)
{
	cbi(ADMUX, REFS1);
	sbi(ADMUX, REFS0); // AVCC
	sbi(ADMUX, ADLAR); //8 bits
	sbi(ADCSRA, ADPS2);
	//	sbi(ADCSRA, ADPS1); // change speed here! now div by 64
	//	sbi(ADCSRA, ADPS0); // change speed here!

	sbi(ADCSRA, ADEN);
	//	sbi(ADCSRA, ADIE); // not needed
	DDRC = 0x00;
	PORTC = 0x00;
}
#endif

void initcell(unsigned char* cells){
  unsigned char x;
  for (x=0;x<MAX_SAM;x++){
    //    cells[x]=randi();
    cells[x]=((rand()%255)+randi()+(adcread(7)&15)); // added bracket for AND!
    //    printf("%d %d\n", cells[x],xxx[x]);
  }
}

void mutate(unsigned char* cells){
  unsigned char x,y;
  for (y=0;y<cells[0];y++){// try this?
    x=randi();
    //    printf("plague %d %d    ", x,cells[0]);

        cells[x]^=(trand&0x0f);
    //      cells[x]^=(trand);
  //    printf("%d xxx:%d\n",y, cells[x]);
  }
}

void inc(unsigned char* cells){
  unsigned char x,y;
  x=randi();
  for (y=0;y<cells[0];y++){
  cells[x++]++;
  }
}

void shift(unsigned char* cells){
  unsigned char x,y;
  x=randi();
  for (y=0;y<cells[0];y++){
  cells[x++]=cells[x++];
  }
}

void chunk(unsigned char* cells){
  unsigned char x,y,tmp;
  x=randi();
  for (y=0;y<cells[0];y++){
    tmp=cells[1] + ++x;
  cells[x]=cells[tmp];
  }
}

u8 headcount(u8 *datagenbuffer,u8 place){
  u8 counter=0;
  u8 n=randi();
  place-=n-1;
  if (datagenbuffer[place]==1) counter++;
  place+=1;
  if (datagenbuffer[place]==1) counter++;
  place+=1;
  if (datagenbuffer[place]==1) counter++;
  place+=n-2;
  if (datagenbuffer[place]==1) counter++;
  place+=2;
  if (datagenbuffer[place]==1) counter++;
  place+=n-2; // was -1
  if (datagenbuffer[place]==1) counter++;
  place+=1;
  if (datagenbuffer[place]==1) counter++;
  place+=1;
  if (datagenbuffer[place]==1) counter++;
  if (counter<3 && counter!=0) return 1;
  else return 0;
}

void runwire(unsigned char* cellies){

  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char place,x,n;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
  }      

  n=randi();
  if (n==0) n=1;

  for (x=0;x<MAX_SAM2; x++){
    if (cells[x]==0) newcells[x]=0; //blank
    else if (cells[x]==1) newcells[x]=255;  // head to tail
    else if (cells[x]==255) newcells[x]=129;  // tail to copper
    else if (cells[x]>128 && headcount(cells,x)==1) newcells[x]=1;
    else newcells[x]=cells[x];

    //  printf("%c", cells[x]);
  }

  // swapping 
  flag^=0x01;
}

void runhodgenet(unsigned char* cellies){

  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char place,x,n;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
  }      

  n=randi();
  u8 sum, numill, numinf,y;
  u8 k1=cellies[1];
  u8 k2=cellies[2];
  if (k1==0) k1=1;
  if (k2==0) k2=1;

  for (x=1;x<MAX_SAM2; x++){
    sum=0; numill=0; numinf=0;
    place=x-cellies[4]-1;
    place=place&127;
  if (cells[place]==cellies[0]) numill++; if (cells[place]>0) numinf++;  
  sum+=cells[place];

  place+=1;
    place=place&127;
  if (cells[place]==cellies[0]) numill++; if (cells[place]>0) numinf++;  
  sum+=cells[place];

  place+=1;
    place=place&127;
  if (cells[place]==cellies[0]) numill++; if (cells[place]>0) numinf++;  
  sum+=cells[place];

  place+=cellies[4]-2;
    place=place&127;
  if (cells[place]==cellies[0]) numill++; if (cells[place]>0) numinf++;  
  sum+=cells[place];

  place+=1;
    place=place&127;
  if (cells[place]==cellies[0]) numill++; if (cells[place]>0) numinf++;  
  sum+=cells[place];

  place+=1;
    place=place&127;
  if (cells[place]==cellies[0]) numill++; if (cells[place]>0) numinf++;  
  sum+=cells[place];

  place+=cellies[4]-2;
    place=place&127;
  if (cells[place]==cellies[0]) numill++; if (cells[place]>0) numinf++;  
  sum+=cells[place];

  place+=1;
    place=place&127;
  if (cells[place]==cellies[0]) numill++; if (cells[place]>0) numinf++;  
  sum+=cells[place];

  place+=1;
    place=place&127;
  if (cells[place]==cellies[0]) numill++; if (cells[place]>0) numinf++;  
  sum+=cells[place];

  //    y=x+32768; UPDATE!

  if(cells[x] == 0)
    newcells[x] = floor(numinf / k1) + floor(numill / k2);
  else if(cells[x] < cellies[0])
    {    if ((numinf+1)!=0)    newcells[x] = floor(sum / (numinf + 1)) + cellies[3];}
  else
    newcells[x] = 0;

  if(newcells[x] > cellies[0])    newcells[x] = cellies[0];


  //    printf("numinf%d", numinf);
  }
  // swapping 
  flag^=0x01;
  }




void runhodge(unsigned char* cellies){

  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char place,x,n;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
  }      

  n=randi();

  u8 sum, numill, numinf,y;
  u8 k1=cellies[1];
  u8 k2=cellies[2];
  if (k1==0) k1=1;
  if (k2==0) k2=1;

  for (x=0;x<MAX_SAM2; x++){
    sum=0; numill=0; numinf=0;
    y=x-n-1;
    sum+=cellies[y];
    if (cellies[y]==(cellies[0]-1)) numill++; else if (cellies[y]>0) numinf++;

    y+=1;
    sum+=cellies[y];
    if (cellies[y]==(cellies[0]-1)) numill++; else if (cellies[y]>0) numinf++;

    y+=1;
    sum+=cellies[y];
    if (cellies[y]==(cellies[0]-1)) numill++; else if (cellies[y]>0) numinf++;

    y+=cellies[4]-2;
    sum+=cellies[y];
    if (cellies[y]==(cellies[0]-1)) numill++; else if (cellies[y]>0) numinf++;

    y+=1;
    sum+=cellies[y];
    if (cellies[y]==(cellies[0]-1)) numill++; else if (cellies[y]>0) numinf++;

    y+=1;
    sum+=cellies[y];
    if (cellies[y]==(cellies[0]-1)) numill++; else if (cellies[y]>0) numinf++;

    y+=cellies[4]-2;
    sum+=cellies[y];
    if (cellies[y]==(cellies[0]-1)) numill++; else if (cellies[y]>0) numinf++;

    y+=1;
    sum+=cellies[y];
    if (cellies[y]==(cellies[0]-1)) numill++; else if (cellies[y]>0) numinf++;

    y+=1;
    sum+=cellies[y];
    if (cellies[y]==(cellies[0]-1)) numill++; else if (cellies[y]>0) numinf++;


    //    y=x+wrap; UPDATE!
  if(cells[x] == 0)
    newcells[x] = floor(numinf / k1) + floor(numill / k2);
  else if(cells[x] < cellies[0] - 1)
    newcells[x] = floor(sum / (numinf + 1)) + cellies[3];
  else
    newcells[x] = 0;

  if(newcells[x] > cellies[0] - 1)
    newcells[x] = cellies[0] - 1;


  //  printf("%c", cells[x]);
  }

  // swapping 
  flag^=0x01;
}




void runfire(unsigned char* cellies){

  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char place,x,n;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
  }      

  u8 sum;
  u8 y;
  u8 probB=cellies[0]/32;
  u8 probI=cellies[1]/10;

  for (x=0;x<MAX_SAM2; x++){
    n=randi();

    y=x-n-1;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=n-2;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=n-2;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    //    sum=(datagenbuffer[x-1]&1)+(datagenbuffer[x+1]&1)+(datagenbuffer[x-n]&1)+(datagenbuffer[x+n]&1)+(datagenbuffer[x-n-1]&1)+(datagenbuffer[x-n+1]&1)+(datagenbuffer[x+n-1]&1)+(datagenbuffer[x+n+1]&1);

    //    y=x+32768; // now update

    if (cells[x]==0 || cells[x]==254) newcells[x]=cells[x]; //empty or burnt
    // now deal with vegetation(bit1 empty) and burning(&1)
    else if ((cells[x]&1)==0 && randi()<=(sum*probI)) newcells[x]=cells[x]|1;  //veg->burning
    else if ((cells[x]&1)==1 && randi()<=probB) newcells[x]=254; // burning->burnt
    else newcells[x]=cells[x];
    //  printf("%c", cells[x]);
  }

  // swapping 
  flag^=0x01;
}

void runkrum(unsigned char* cellies){

  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char place,x,n;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
  }      

  n=randi()&127;
  if (n==0) n=1;

  for (x=0;x<MAX_SAM2; x++){

  place=x-cellies[0];
  if (cells[place]==(cells[x]+1)%n) newcells[x]=cells[place];

  place+=cellies[0]-1;
  if (cells[place]==(cells[x]+1)%n) newcells[x]=cells[place];

  place+=2;
  if (cells[place]==(cells[x]+1)%n) newcells[x]=cells[place];

  place+=cellies[0]-1;
  if (cells[place]==(cells[x]+1)%n) newcells[x]=cells[place];
  //    printf("%c", cells[x]);
  }

  // swapping 
  flag^=0x01;

}

void runlife(unsigned char* cellies){

  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char place,x,n,sum,y;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
  }      

  n=randi();

  for (x=0;x<MAX_SAM2; x++){
    sum=0;
    y=x-n-1;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=n-2;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=n-2;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    y+=1;
    sum+=cellies[y]&1;

    /*    if (sum==3 || (sum+(datagenbuffer[x]&0x01)==3)) newdatagenbuffer[x]=255; /// 
	  else newdatagenbuffer[x]=0;*/
    //  y=x+32768; update

  if ((cells[x]&1)==1 && sum<2) newcells[x]=0;
  else if ((cells[x]&1)==1 && sum>3) newcells[x]=0;
  else if ((cells[x]&1)==0 && sum==3) newcells[x]=255;
  else newcells[x]=cells[x];

  //  printf("%c", cells[x]);
  }

  // swapping 
  flag^=0x01;
}

void runcel(unsigned char* cellies){

  //  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char place,x,n,sum,state,y;

  /*  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
    } */     

  n=randi();

  for (x=0;x<MAX_SAM; x++){

    state = 0;

  if (cellies[(x +1+ (n))&255]>128)
      state |= 0x4;
  if (cellies[(x+n)&255]>128)
      state |= 0x2;
  y=x-1;
  if (cellies[(y + n)&255]>128)
      state |= 0x1;

  y=(x+(n*2));///&255;// next row but one!

      if ((cellies[0] >> state) & 1){
	cellies[y] = 0;
      }
      else{
	cellies[y] = 255;
      } 

      //  printf("%c", cellies[x]);
  }

  // swapping 
  //  flag^=0x01;
}

//u8 table[21];

/*void inittable(u8 r, u8 k, int rule){
  u8 max; int z; u8 summ;

  //    free(table);
  max = (k-1)*((r*2)+1);
  //  //  printf("ttt %d\n",max);

  //  table= malloc(max+1);
  for (z=max;z>=0;z--){
    summ=0;
    while ((rule-pow(k,z))>=0) {
      summ++;
      rule=rule-pow(k,z);
    }
    if (summ>=1) {
      table[z]=summ;
    }
    else table[z]=0;
    //        printf("%d\n",table[z]);
  }
  }*/

/*void runcel1d(unsigned char* cellies){

  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char place,x,n,sum,y,z,zz;

  n=randi();

  for (x=0;x<MAX_SAM; x++){

    sum=0;
    
    // sum of datagenbuffer in radius - not looping!
    for (z=-3;z<3;z++){ // hardcode sans radius...
      zz=x+z;
      if (zz>=n) zz=zz-n;
      //      if (zz<0) zz=n+zz;
      sum+=(cellies[zz])&3; // crash here? and it did/// not now !
    }

    y=x+n;
    cellies[y]=table[sum]<<5;  

    //  printf("%c", cellies[x]);
  }

  }*/

void hodge(unsigned char* cellies){
  int sum=0, numill=0, numinf=0;
  unsigned char q,k1,k2,g;
  static unsigned char x=CELLLEN+1;
  static unsigned char flag=0;
  unsigned char *newcells, *cells;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
  }      

  q=cells[0];k1=cells[1];k2=cells[2];g=cells[3];
  if (k1==0) k1=1;
  if (k2==0) k2=1;

  sum=cells[x]+cells[x-1]+cells[x+1]+cells[x-CELLLEN]+cells[x+CELLLEN]+cells[x-CELLLEN-1]+cells[x-CELLLEN+1]+cells[x+CELLLEN-1]+cells[x+CELLLEN+1];

  if (cells[x-1]==(q-1)) numill++; else if (cells[x-1]>0) numinf++;
  if (cells[x+1]==(q-1)) numill++; else if (cells[x+1]>0) numinf++;
  if (cells[x-CELLLEN]==(q-1)) numill++; else if (cells[x-CELLLEN]>0) numinf++;
  if (cells[x+CELLLEN]==(q-1)) numill++; else if (cells[x+CELLLEN]>0) numinf++;
  if (cells[x-CELLLEN-1]==q) numill++; else if (cells[x-CELLLEN-1]>0) numinf++;
  if (cells[x-CELLLEN+1]==q) numill++; else if (cells[x-CELLLEN+1]>0) numinf++;
  if (cells[x+CELLLEN-1]==q) numill++; else if (cells[x+CELLLEN-1]>0) numinf++;
  if (cells[x+CELLLEN+1]==q) numill++; else if (cells[x+CELLLEN+1]>0) numinf++;

  if(cells[x] == 0)
    newcells[x&127] = floor(numinf / k1) + floor(numill / k2);
  else if(cells[x] < q - 1)
    newcells[x&127] = floor(sum / (numinf + 1)) + g;
  else
    newcells[x&127] = 0;

  if(newcells[x&127] > q - 1)
    newcells[x&127] = q - 1;

  x++;
  if (x>((MAX_SAM2)-CELLLEN-1)) {
    x=CELLLEN+1;
    flag^=0x01;
  }
}

void cel(unsigned char* cells){

  static unsigned char l=0; unsigned char cell, state, res;
  unsigned char rule=cells[0];
  unsigned char tmp;
  res=0;
  l++;
  l&=15;

  for (cell = 1; cell < CELLLEN; cell++){ 
      state = 0;
      tmp=cell + 1+ (l*CELLLEN); // changed
      if (cells[tmp]>128) // max 256
	state |= 0x4;
      if (cells[cell+(CELLLEN*l)]>128)
	state |= 0x2;
      if (cells[cell - 1 +(CELLLEN*l)]>128)
	state |= 0x1;
                     
      if ((rule >> state) & 1){
	res += 1;
	tmp=cell+(((l+1)%CELLLEN)*CELLLEN);
	cells[tmp] = 255;
      }
      else{
	tmp=cell+(((l+1)%CELLLEN)*CELLLEN);
	cells[tmp] = 0;
      } 
  }
}

void SIR(unsigned char* cellies){
  unsigned char cell,x,sum=0,tmp,tmpp,tmppp,tmpppp;
  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char kk=cellies[0], p=cellies[1];

    if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
    //    printf("SWOP %d %d\n", cells[11], newcells[11]);
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
    //    printf("UN_SWOP %d %d\n", cells[11], newcells[11]);
    } 

  for (x=0;x<127;x++){
    cell=cells[x];
    newcells[x]=cell; // swop
        if (cell >= kk) newcells[x] += recovered;
	else if ((cell>0 && cell<kk)){
	  newcells[x]++;                                                       
	}
	else if (cell == susceptible) {   
	  tmp=(x-16)&127;
	  tmpp=(x+16)&127;
	  tmppp=(x-1)&127;
	  tmpppp=(x+1)&127;

	  if ( (cells[tmp]>0 && cells[tmp]<kk) ||
	       (cells[tmpp]>0 && cells[tmpp]<kk) ||
	       (cells[tmppp]>0 && cells[tmppp]<kk) ||
	       (cells[tmpppp]>0 && cells[tmpppp]<kk))
	    {
	      if (randi()%10 < p) newcells[x] = 1;       
	    }
	}
	}

  /* for (x=CELLLEN;x<((MAX_SAM2)-CELLLEN);x++){
    cell = cells[x];
    newcells[x]=cell;
    //    newcells[x]=rand()%255;
        if (cell >= kk) newcells[x] = recovered;
	else if ((cell>0 && cell<kk)){
	  newcells[x]++;                                                       
	}
	else if (cell == susceptible) {   
                                                 
	  if ( (cells[x-CELLLEN]>0 && cells[x-CELLLEN]<kk) ||
	       (cells[x+CELLLEN]>0 && cells[x+CELLLEN]<kk) ||
	       (cells[x-1]>0 && cells[x-1]<kk) ||
	       (cells[x+1]>0 && cells[x+1]<kk))
	    {
	      if (randi()%10 < p) newcells[x] = 1;       
	    }
	}
	}*/
  flag^=0x01;
}

void life(unsigned char* cellies){
  unsigned char x, sum;

  static unsigned char flag=0;
  unsigned char *newcells, *cells;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM2];
  }
  else {
    cells=&cellies[MAX_SAM2]; newcells=cellies;
  }      

  for (x=CELLLEN+1;x<((MAX_SAM2)-CELLLEN-1);x++){
    sum=cells[x]%2+cells[x-1]%2+cells[x+1]%2+cells[x-CELLLEN]%2+cells[x+CELLLEN]%2+cells[x-CELLLEN-1]%2+cells[x-CELLLEN+1]%2+cells[x+CELLLEN-1]%2+cells[x+CELLLEN+1]%2;
    sum=sum-cells[x]%2;
    if (sum==3 || (sum+(cells[x]%2)==3)) newcells[x]=255;
    else newcells[x]=0;
  }
  
  // swapping 
  flag^=0x01;
}

// instructions for plague CPUS!

unsigned char omem;

/* BIOTA: two dimensional memory map - add/replace with wormings */

unsigned char btdir,dcdir;

unsigned char btempty(unsigned char* cells, unsigned char IP){
  // turn around
  if (btdir==0) btdir=1;
  else if (btdir==1) btdir=0;
  else if (btdir==2) btdir=3;
  else if (btdir==3) btdir=2;
  return IP;
}

unsigned char btoutp(unsigned char* cells, unsigned char IP){
    modifier=cells[omem];
    flaggy=1;
  return IP;
}

unsigned char btstraight(unsigned char* cells, unsigned char IP){
  if (dcdir==0) omem+=1;
  else if (dcdir==1) omem-=1;
  else if (dcdir==2) omem+=16;
  else if (dcdir==3) omem-=16;

  if (cells[omem]==0) 
    { // change dir
  if (btdir==0) btdir=1;
  else if (btdir==1) btdir=0;
  else if (btdir==2) btdir=3;
  else if (btdir==3) btdir=2;
    }
  return IP;
}


unsigned char btbackup(unsigned char* cells, unsigned char IP){
  if (dcdir==0) omem-=1;
  else if (dcdir==1) omem+=1;
  else if (dcdir==2) omem-=16;
  else if (dcdir==3) omem+=16;
  if (cells[omem]==0) 
    {
  if (btdir==0) btdir=1;
  else if (btdir==1) btdir=0;
  else if (btdir==2) btdir=3;
  else if (btdir==3) btdir=2;
    }
  return IP;
}

unsigned char btturn(unsigned char* cells, unsigned char IP){
  if (dcdir==0) omem+=16;
  else if (dcdir==1) omem-=16;
  else if (dcdir==2) omem+=1;
  else if (dcdir==3) omem-=1;
  return IP;
}

unsigned char btunturn(unsigned char* cells, unsigned char IP){
  if (dcdir==0) omem-=16;
  else if (dcdir==1) omem+=16;
  else if (dcdir==2) omem-=1;
  else if (dcdir==3) omem+=1;
  return IP;
}

unsigned char btg(unsigned char* cells, unsigned char IP){
  unsigned char x=0;
  while (x<20 && cells[omem]!=0){
    if (dcdir==0) omem+=1;
    else if (dcdir==1) omem-=1;
    else if (dcdir==2) omem+=16;
    else if (dcdir==3) omem-=16;
    x++;
  }
  return IP;
}

unsigned char btclear(unsigned char* cells, unsigned char IP){
  if (cells[omem]==0){
  if (btdir==0) btdir=1;
  else if (btdir==1) btdir=0;
  else if (btdir==2) btdir=3;
  else if (btdir==3) btdir=2;
  }
  else cells[omem]=0;
  return IP;
}

unsigned char btdup(unsigned char* cells, unsigned char IP){
  unsigned char tmp=omem-1;
  if (cells[omem]==0 || cells[tmp]!=0){
  if (btdir==0) btdir=1;
  else if (btdir==1) btdir=0;
  else if (btdir==2) btdir=3;
  else if (btdir==3) btdir=2;
  }
  else cells[tmp]=cells[omem];
  return IP;
}

unsigned char clocky, count;

// reddeath

//1- the plague within (12 midnight) - all the cells infect

unsigned char redplague(unsigned char* cells, unsigned char IP){
  if (clocky==12){
    unsigned char tmp=IP+1;
    clocky=12;
    cells[tmp]=cells[IP];
    if (IP==255) clocky=13;
    return IP+1;
  }
  else return IP+insdir;
}

//2- death - one by one fall dead
unsigned char reddeath(unsigned char* cells, unsigned char IP){
  if (clocky==13){
    //    clocky=13; // >>>>????
    unsigned char tmp=IP+count;
    cells[tmp]=randi();
    count++;
    return IP; // just keeps on going
  }
  else return IP+insdir;
}

//3- clocky every hour - instruction counter or IP -some kind of TICK
unsigned char redclocky(unsigned char* cells, unsigned char IP){
  clocky++;
  if (clocky%60==0) {
    modifier^=255;
    flaggy=1;
    return IP; // everyone stops
  }
  else return IP+insdir;
}

//4- seven rooms: divide cellspace into 7 - 7 layers with filter each
unsigned char redrooms(unsigned char* cells, unsigned char IP){// TODO!
    switch(IP%7){
  case 0:
    //blue
    modifier=modifier<<1;
    flaggy=1;
    break;
  case 1:
    //purple
    modifier=modifier<<2;
    flaggy=1;
    break;
  case 2:
    //green
    modifier=modifier<<3;
    flaggy=1;
    break;
  case 3:
    //orange
    modifier=modifier<<4;
    flaggy=1;
    break;
  case 4:
    //white
    modifier=modifier<<5;
    flaggy=1;
    break;
  case 5:
    //violet
    modifier=modifier<<6;
    flaggy=1;
    break;
  case 6:
    modifier=modifier<<7;
    flaggy=1;
    // black
    }
  return IP+insdir;
}

  //5- unmasking (change neighbouring cells)

unsigned char redunmask(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP-1;
  unsigned char tmpp=IP+1;
  cells[tmp]^=255;
  cells[tmpp]^=255;
return IP+insdir;
}
  //6- the prince (omem) - the output! walking through 7 rooms 

unsigned char redprospero(unsigned char* cells, unsigned char IP){

  unsigned char dirrr;
  // prince/omem moves at random through rooms
  dirrr=randi()&3;
  if (dirrr==0) omem=omem+1;
  else if (dirrr==1) omem=omem-1;
  else if (dirrr==2) omem=omem+16;
  else if (dirrr==3) omem=omem-16;

  // output
  modifier=cells[omem]; 
    flaggy=1;
  return IP+insdir;
}

  //7- the outside - the input!
unsigned char redoutside(unsigned char* cells, unsigned char IP){
unsigned char tmp=omem+1;
  // input sample to cell (which one neighbour to omem)
  cells[tmp]=randi();
  modifier=cells[omem];
  flaggy=1;
  // output to filter 
  //  (*filtermod[qqq]) ((int)cells[omem]);
  return IP+insdir;
}

// plague

unsigned char ploutp(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP-1;
  unsigned char tmpp=IP+1;
  modifier=cells[tmp]+cells[tmpp];
    flaggy=1;
  return IP+insdir;
}

unsigned char plenclose(unsigned char* cells, unsigned char IP){
  unsigned char tmpp=IP+cells[IP];
  unsigned char tmp=IP-cells[IP];
  cells[tmp]=255; cells[tmpp]=255;
  return IP+2;
}

unsigned char plinfect(unsigned char* cells, unsigned char IP){
  if (cells[IP]<128) {
  unsigned char tmp=IP-1;
  unsigned char tmpp=IP+1;
    cells[tmp]= cells[IP];
    cells[tmpp]= cells[IP];
  }
  return IP+insdir;
}

unsigned char pldie(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP-1;
  unsigned char tmpp=IP+1;
  cells[tmpp]=0; cells[tmp]=0;
  return IP+(insdir*2);
}

unsigned char plwalk(unsigned char* cells, unsigned char IP){
  // changing direction
  if (dir<0 && (cells[IP]%0x03)==1) dir=1;
  else if (dir>1 && (cells[IP]%0x03)==0) dir=-1;
  else
    // changing pace
    insdir=dir*cells[IP]>>4;
  
  return IP+insdir;
}

// redcode

unsigned char rdmov(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP+2;
  cells[(IP+cells[tmpp])&255]=cells[(IP+cells[tmpp])&255];
  return IP+=3;
}

unsigned char rdadd(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP+2;
  cells[(IP+cells[tmpp])&255]=cells[(IP+cells[tmpp])&255]+cells[(IP+cells[tmp])&255];
  return IP+=3;
}

unsigned char rdsub(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP+2;
  cells[(IP+cells[tmpp])&255]=cells[(IP+cells[tmpp])&255]-cells[(IP+cells[tmp])&255];
  return IP+=3;
}

unsigned char rdjmp(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  IP=IP+cells[tmp];
  return IP;
}

unsigned char rdjmz(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP+2;
  if (cells[(IP+cells[tmpp])&255]==0) IP=cells[tmp];
  else IP+=3;
  return IP;
}

unsigned char rdjmg(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP+2;
  if (cells[(IP+cells[tmpp])&255]>=0) IP=cells[tmp];
  else IP+=3;
  return IP;
}

unsigned char rddjz(unsigned char* cells, unsigned char IP){
  unsigned char x;
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP+2;
  x=(IP+cells[tmpp]);
  cells[x]=cells[x]-1;
  if (cells[x]==0) IP=cells[tmp];
  else IP+=3;
  return IP;
}

unsigned char rddat(unsigned char* cells, unsigned char IP){
  IP+=3;
  return IP;
}

unsigned char rdcmp(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP+2;
  if (cells[(IP+cells[tmpp])&255]!=cells[(IP+cells[tmp])&255]) IP+=6;
  else IP+=3;
  return IP;
}


unsigned char rdoutp(unsigned char* cells, unsigned char IP){
  unsigned char tmpp=IP+2;
  modifier=cells[tmpp]; 
    flaggy=1;
  IP+=3;
  return IP;
}

// SIR: inc if , die if, recover if, getinfected if 

unsigned char SIRoutp(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP-1;
  modifier=cells[tmp]+cells[tmpp];
    flaggy=1;
  return IP+insdir;
}

unsigned char SIRincif(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  if ((cells[tmp]>0 && cells[tmp]<128)) cells[IP]++;
  return IP+insdir;
}

unsigned char SIRdieif(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  if ((cells[tmp]>0 && cells[tmp]<128)) {
    if (randi()%10 < 4) cells[IP] = dead;       
  }
  return IP+insdir;
}

unsigned char SIRrecif(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  if (cells[tmp] >= 128) cells[IP] = recovered;                                             
  return IP+insdir;
}

unsigned char SIRinfif(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP-1;

  if (cells[tmp] == 0) {   
                                                 
    if ((cells[tmpp]>0 && cells[tmpp]<128) ||
	(cells[tmp]>0 && cells[tmp]<128))
	{
	if (randi()%10 < 4) cells[IP] = 1;       
	}
}
  return IP+insdir;
}

// brainfuck

//signed char cycle; // was unsigned

unsigned char bfinc(unsigned char* cells, unsigned char IP){
  omem++; 
  return ++IP;
}

unsigned char bfdec(unsigned char* cells, unsigned char IP){
  omem--; 
  return ++IP;
}

unsigned char bfincm(unsigned char* cells, unsigned char IP){
  cells[omem]++; 
  return ++IP;
}

unsigned char bfdecm(unsigned char* cells, unsigned char IP){
  cells[omem]--; 
  return ++IP;
}


unsigned char bfoutp(unsigned char* cells, unsigned char IP){
  modifier=cells[omem]; 
  flaggy=1;
  return ++IP;
}

unsigned char bfin(unsigned char* cells, unsigned char IP){
  cells[omem] = randi(); 
  return ++IP;
}

//unsigned char ostack[20];

unsigned char bfbrac1(unsigned char* cells, unsigned char IP){ // redo these

  //  [ if the byte at the data pointer is zero, then instead of moving the instruction pointer forward to the next command, jump it forward to the command after the matching ] command.

  signed char bal = 1, count=0;
  if (cells[omem] == 0) {
    do {
      IP++;
      if      (cells[IP%255]%9 == 6) bal++; // [
      else if (cells[IP%255]%9 == 7) bal--; // ]
      count++;
    } while ( bal != 0 && count<127 );
  }
  IP++;
  return IP;
}

unsigned char bfbrac2(unsigned char* cells, unsigned char IP){

  signed char bal = 0, count=0;
  if (cells[omem] =! 0) {
  do {
    if      (cells[IP%255]%9 == 6) bal++; // [
    else if (cells[IP%255]%9 == 7) bal--; // ]
    IP--;
    count++;
  } while ( bal != 0 && count<127);
  }
  IP++;
    return IP;
}

// first attempt

unsigned char finc(unsigned char* cells, unsigned char IP){
  omem++; 
  return IP+insdir;
}

unsigned char fin1(unsigned char* cells, unsigned char IP){
  omem=randi();
  return IP+insdir;
}

unsigned char fin2(unsigned char* cells, unsigned char IP){
  omem=adcread(1);
  return IP+insdir;
}

unsigned char fin3(unsigned char* cells, unsigned char IP){
  IP=adcread(1);
  return IP+insdir;
}

unsigned char fin4(unsigned char* cells, unsigned char IP){
  cells[omem]=randi();
  return IP+insdir;
}

unsigned char fdec(unsigned char* cells, unsigned char IP){
  omem--; 
  return IP+insdir;
}

unsigned char fincm(unsigned char* cells, unsigned char IP){
  cells[omem]++; 
  return IP+insdir;
}

unsigned char fdecm(unsigned char* cells, unsigned char IP){
  cells[omem]--; 
  return IP+insdir;
}


unsigned char outp(unsigned char* cells, unsigned char IP){
  modifier=cells[omem];
    flaggy=1;
  return IP+insdir;
}


unsigned char outpp(unsigned char* cells, unsigned char IP){
  modifier=omem;
    flaggy=1;
  return IP+insdir;
}

unsigned char plus(unsigned char* cells, unsigned char IP){
  cells[IP]+=1;
  return IP+insdir;
}

unsigned char minus(unsigned char* cells, unsigned char IP){
  cells[IP]-=1;
  return IP+insdir;
}

unsigned char bitshift1(unsigned char* cells, unsigned char IP){
  cells[IP]=cells[IP]<<1;
  return IP+insdir;
}

unsigned char bitshift2(unsigned char* cells, unsigned char IP){
  cells[IP]=cells[IP]<<2;
  return IP+insdir;
}

unsigned char bitshift3(unsigned char* cells, unsigned char IP){
  cells[IP]=cells[IP]<<3;
  return IP+insdir;
}

unsigned char branch(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  if (cells[tmp]==0) IP=cells[omem];
  return IP+insdir;
}

unsigned char jump(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  if (cells[tmp]<128) return IP+cells[tmp];
  return IP+insdir; // removed else
}

unsigned char infect(unsigned char* cells, unsigned char IP){
  int x=IP-1;
  unsigned char tmp=IP+1;
  if (x<0) x=MAX_SAM;
  if (cells[x]<128) cells[tmp]= cells[IP];
  return IP+insdir;
}

unsigned char store(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  cells[IP]=cells[cells[tmp]&255];
  return IP+insdir;
}

unsigned char skip(unsigned char* cells, unsigned char IP){
  return IP+insdir;
}

unsigned char direction(unsigned char* cells, unsigned char IP){
  if (dir<0) dir=1;
  else dir=-1;
  return IP+insdir;
}

unsigned char die(unsigned char* cells, unsigned char IP){
  return IP+insdir;
}

unsigned char writeknob(unsigned char* cells, unsigned char IP){
  cells[IP]=adcread(1);
  return IP+insdir;
}

unsigned char writesamp(unsigned char* cells, unsigned char IP){
  cells[IP]=randi();
  return IP+insdir;
}

unsigned char dirout(unsigned char* cells, unsigned char IP){
#ifdef AVR_IS
  OCR0A=cells[IP];
#endif
  return IP+insdir;
}

#ifdef AVR_IS
void main(void)
{
   unsigned char cpu, plague;
   adc_init();
   srand(randi());

#else
void main(int argc, char *argv[])
{
   unsigned char plague =atoi(argv[1]);
   unsigned char cpu=atoi(argv[2]);
   srand(time(NULL));
   
#endif
   unsigned char instruction, instructionp;
  unsigned int counter=0,counterr=0,othercounter=0;
  unsigned int speed,plaguespeed;
  unsigned char *cells=xxx; 
  unsigned char cpuspeed;//,plaguespeed;
  unsigned char tmp;
  unsigned char wormdir,instr;
  u8 biotadir[8]={240,241,1,15,16,17,255,239};
  u8 deltastate[16] = {1, 4, 2, 7, 3, 13, 4, 7, 8, 9, 3, 12, 6, 11, 5, 13};

  // from TM
  unsigned char ch, reader, state;
  unsigned char count=0, pc=0, towrite, togo;
  signed int tc=0;

  //  inittable(3,4,randi());

  unsigned char (*instructionsetfirst[])(unsigned char* cells, unsigned char IP) = {outpp,finc,fdec,fincm,fdecm,fin1,fin2,fin3,fin4,outp,plus,minus,bitshift1,bitshift2,bitshift3,branch,jump,infect,store,writeknob,writesamp,skip,direction,die,dirout}; // 24 instructions +1=25

  unsigned char (*instructionsetplague[])(unsigned char* cells, unsigned char IP) = {writesamp, ploutp, plenclose, plinfect, pldie, plwalk,dirout}; // 6 +1=7

  unsigned char (*instructionsetbf[])(unsigned char* cells, unsigned char IP) = {bfinc,bfdec,bfincm,bfdecm,bfoutp,bfin,bfbrac1,bfbrac2,dirout}; // 8 +1=9

  unsigned char (*instructionsetSIR[])(unsigned char* cells, unsigned char IP) = {writesamp,SIRoutp,SIRincif,SIRdieif,SIRrecif,SIRinfif,dirout}; // 5+1+1=7

  unsigned char (*instructionsetredcode[])(unsigned char* cells, unsigned char IP) = {writesamp,rdmov,rdadd,rdsub,rdjmp,rdjmz,rdjmg,rddjz,rddat,rdcmp,rdoutp,dirout}; // 10+1+1=12

  unsigned char (*instructionsetbiota[])(unsigned char* cells, unsigned char IP) = {btempty,btoutp,btstraight,btbackup,btturn,btunturn,btg,btclear,btdup,writesamp,dirout}; // 9+1+1=11

  unsigned char (*instructionsetreddeath[])(unsigned char* cells, unsigned char IP) = {redplague,reddeath,redclocky,redrooms,redunmask,redprospero,redoutside,writesamp,dirout}; // 7+1+1=9

  void (*plag[])(unsigned char* cells) = {mutate,SIR,hodge,cel,life,inc,shift,chunk,runwire,runhodge,runfire,runkrum,runhodgenet,runlife,runcel,mutate};// 16

  omem=1;

  dir=1;
  initcell(cells);
  for (tmp=0;tmp<64;tmp++){
    desc[tmp]=randi();
  }
  
#ifdef AVR_IS
   DDRD=0x40; // 6 as out
   cli();//stop interrupts
   TCCR0A=(1<<COM0A1) | (1<<WGM01) | (1<<WGM00); // fast PWM
   TCCR0B=(1<<CS00) | (1<<CS01);  // divide by 64 for 1KHz

   DIDR0=0x3f; // disable digital inputs on PC0 to PC5
#endif
  instructionp=0; insdir=1; dir=1; btdir=0; dcdir=0;

  while(1){
    speed=(adcread(0)+adcread(3))<<4; // TESTING! - speed of both slowing!TODO! was <<2 .. ??SPEED
    plaguespeed=speed>>2;//>>4; // 4 bits=16;
    cpuspeed=speed>>5;//&15; // should be logarithmic and longer
    tmp=(adcread(2)+adcread(4)); // 8 bits // CPU and PLAGUE
#ifdef AVR_IS
    cpu=tmp>>4; // 8 CPUs // 6 bits->8 CPUs=3bits - now 4 =16 CPUS TODO!
    plague=tmp&15;
    if (plague==0) plague=cells[instructionp]>>4; /// ????
#else
    cpuspeed=1;plaguespeed=1;speed=1;
#endif
    // TESTING:
    //	cpu=0;
    
    // plague CPU!
    //    if (count%((IP%32)+1)==0){ // ??? This is where we do speed!
       		if (++counter>=cpuspeed){
	  counter=0;
	//        if ((counter%(1+cpuspeed))==0){ // ??? This is where we do speed!
	  //	oldone=instructionp;// TESTY!!!!
	  switch(cpu){
	  case 14: // TM is now zero
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetfirst[instruction%25]) (cells, instructionp); // mistake before as was instruction%INSTLEN in last instance
	    //	    insdir=dir*(IP%16)+1; // prev mistake as just got exponentially larger
	    insdir=dir;
	    break;
	  case 1:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetplague[instruction%7]) (cells, instructionp);
	    if (cells[instructionp]==255 && dir<0) dir=1;
	    else if (cells[instructionp]==255 && dir>0) dir=-1; // barrier
	    insdir=dir;
	    break;
	  case 2:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetbf[instruction%9]) (cells, instructionp);
	    insdir=dir;//NOt needed
	    break;
	  case 3:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetSIR[instruction%7]) (cells, instructionp);
	    insdir=dir;
	    break;
	  case 4:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetredcode[instruction%12]) (cells, instructionp); 
	    insdir=dir; //NOt needed
	    break;
	  case 5:
	    cells[instructionp]=randi();
	    instructionp+=dir;
	    break;
	  case 6:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetreddeath[instruction%9]) (cells, instructionp); 
	    insdir=dir;
	    break;
	  case 7:	    //la biota
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetbiota[instruction%11]) (cells, instructionp); 
	    if (btdir==0) instructionp+=1;
	    else if (btdir==1) instructionp-=1;
	    else if (btdir==2) instructionp+=16;
	    else if (btdir==3) instructionp-=16;
	    break;
	    ////////////////////////////CPUINTREV
	    	  case 8: // leakystack
	    instruction=cells[instructionp];
      switch(instruction%25)
	{
	case NOP: instructionp++; break;
	case ORG: instructionp=1; break;
	case EQU: if (thread_stack_count(2)) {
	    thread_push(thread_pop()==thread_pop()); instructionp++;} break;
	case JMP: instructionp=machine_p88k(instructionp++); break;
	case JMPZ: if (thread_stack_count(1) && thread_pop()==0) instructionp=machine_p88k(instructionp); else instructionp+=1; break;
    case PSHL: thread_push(machine_p88k(instructionp++)); break;
	case PSH: thread_push(machine_p88k(machine_p88k(instructionp++))); break;
	case PSHI: thread_push(machine_p88k(machine_p88k(machine_p88k(instructionp++)))); break;
	case POP: if (thread_stack_count(1)) machine_poke(machine_p88k(instructionp++),thread_pop()); 
	  else instructionp++;
	  break;
	case POPI: if (thread_stack_count(1)) machine_poke(machine_p88k(machine_p88k(instructionp++)),thread_pop()); 
	  	  else instructionp++;
	  break;
	case ADD: if (thread_stack_count(2)) thread_push(thread_pop()+thread_pop()); 
	  instructionp++;
	  break;
	case SUB: if (thread_stack_count(2)) thread_push(thread_pop()-thread_pop());instructionp++; break;
	case INC: if (thread_stack_count(1)) thread_push(thread_pop()+1);instructionp++; break;
	case DEC: if (thread_stack_count(1)) thread_push(thread_pop()-1);instructionp++; break;
	case AND: if (thread_stack_count(2)) thread_push(thread_pop()&thread_pop());instructionp++; break;
	case OR: if (thread_stack_count(2)) thread_push(thread_pop()|thread_pop());instructionp++; break;
	case XOR: if (thread_stack_count(2)) thread_push(thread_pop()^thread_pop());instructionp++; break;
	case NOT: if (thread_stack_count(1)) thread_push(~thread_pop());instructionp++; break;
	case ROR: if (thread_stack_count(2)) thread_push(thread_pop()>>(machine_p88k(instructionp++)&7)); 
 else instructionp++; break;
    case ROL: if (thread_stack_count(2)) thread_push(thread_pop()<<(machine_p88k(instructionp++)&7)); else instructionp++;break;
    case PIP: 
    {
        u8 d=machine_p88k(instructionp++); 
        machine_poke(d,machine_p88k(d)+1); 
    } break;
    case PDP: 
    {
        u8 d=machine_p88k(instructionp++); 
        machine_poke(d,machine_p88k(d)-1); 
    } break;
    case DUP: if (thread_stack_count(1)) thread_push(thread_top()); instructionp++; break;
    case SAY: 
      instructionp++;
      modifier=cells[instructionp];
      flaggy=1;
      //      printf("%c",thread_pop());
      //      machine_poke(machine_p88k(instructionp++),randi()%255);      
        break;
	case INP:
	  machine_poke(machine_p88k(instructionp++),randi());      
	  //	  instructionp++;
	  break;
	}
      //      instructionp++;
      break;
	    ////////////////////////////CPUINTREV
	  case 9: // worm
	    wormdir=randi()&7;
	    m_reg8bit1=biotadir[wormdir];
	    instructionp+=m_reg8bit1;
	    instr=cells[instructionp];
	    switch(instr%12){ //13 NOW 12
      case 0:
	machine_poke(machine_p88k(instructionp),randi());      
	break;
      case 1:
	machine_poke(instructionp,machine_p88k(instructionp)+1);
	break;
      case 2:
	machine_poke(instructionp,machine_p88k(instructionp)-1);
	break;
      case 3:
	instructionp+=machine_p88k(instructionp+m_reg8bit1);
	break;
      case 4:
	machine_poke(instructionp,machine_p88k(instructionp)+machine_p88k(instructionp+m_reg8bit1));
	break;
      case 5:
	machine_poke(instructionp,machine_p88k(instructionp)-machine_p88k(instructionp+m_reg8bit1));
	break;
      case 6:
	machine_poke(instructionp,machine_p88k(instructionp)<<1);
	break;
      case 7:
	machine_poke(instructionp,machine_p88k(instructionp)>>1);
	break;
      case 8:
	if (machine_p88k(instructionp+m_reg8bit2)==0) instructionp+=machine_p88k(instructionp+m_reg8bit1);
	break;
      case 9:
	thread_push(machine_p88k(instructionp+m_reg8bit1));
	break;
      case 10:
	machine_poke(instructionp+m_reg8bit1,thread_pop());
	break;
      case 11:
	machine_poke(instructionp+m_reg8bit1,randi());      
	break;
      }
      break;
	    ////////////////////////////CPUINTREV
      	  case 10: // befunge
	    instr=xxx[instructionp];
      switch(instr%33){
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
	thread_push(instr%30);
	break;
      case 10:
	thread_push(thread_pop()-thread_pop());
	break;
      case 11:
	tmp=thread_pop();
	thread_push(thread_pop()-tmp);
	break;
      case 12:
	thread_push(thread_pop()*thread_pop());
	break;
      case 13:
	tmp=thread_pop();
	if (tmp!=0) thread_push(thread_pop()/tmp);
	break;
        case 14:
	tmp=thread_pop();
	if (tmp!=0) thread_push(thread_pop()%tmp);
	break;
      case 15:
	tmp=thread_pop();
	if (tmp==0) thread_push(1);
	else thread_push(0);
	break;
      case 16:
	tmp=thread_pop();
	if (thread_pop()>tmp) thread_push(1);
	else thread_push(0);
	break;
      case 17: // right
	  m_reg8bit2=2;
  	break;
      case 18: // left
	m_reg8bit2=6;
	break;
      case 19: // up
	m_reg8bit2=0;
	break;
      case 20: // down
	m_reg8bit2=4;
	break;
      case 21:
	m_reg8bit2=(randi()&3)*2;
	break;
      case 22:
	if (thread_pop()==0)	m_reg8bit2=2;
	else 	m_reg8bit2=6;
	break;
      case 23:
	if (thread_pop()==0)	m_reg8bit2=4;
	else 	m_reg8bit2=0;
	break;
      case 24:
	tmp=thread_pop();
	thread_push(tmp);
	thread_push(tmp);
	break;
      case 25: // swap two values
	m_reg8bit1=thread_pop();
	tmp=thread_pop();
	thread_push(m_reg8bit1);
	thread_push(tmp);
	break;
      case 26:
	thread_pop();
	break;
	// pop values and output - ADDED!
      case 27:
	modifier=thread_pop();
	flaggy=1;
	break;
      case 28:
	modifier=thread_pop();
	flaggy=1;
	break;
      case 29: // skip
	instructionp+=biotadir[m_reg8bit2&7];
	break;
      case 30: //put
	machine_poke((thread_pop())*(thread_pop()),thread_pop());
	break;
      case 31:///get
	thread_push(machine_p88k(thread_pop()*thread_pop()));
	break;
      case 32: // push value
	machine_poke((thread_pop())*(thread_pop()),randi());      
	break;
      }
      wormdir=m_reg8bit2&7;
      instructionp+=biotadir[wormdir];
      break;
	    ////////////////////////////CPUINTREV
	  case 11: //LANG
    instr=xxx[instructionp];
      switch(instr%14)
	{
	case 0:
	  machine_poke(machine_p88k(instructionp),randi());      
	  break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	  thread_push(instr);
	  break;
	case 12:
	  m_reg8bit2+=2;
	  machine_poke(instructionp,instr^thread_pop());
	  break;
	case 13:
	  m_reg8bit2-=2;
	  machine_poke(instructionp,instr^thread_pop());
	  break;
	}
      wormdir=m_reg8bit2&7;
      instructionp+=biotadir[wormdir];
      break;
	    ////////////////////////////CPUINTREV
	  case 12: // TURMITE?
	    instr=xxx[instructionp];
      machine_poke(instructionp,instr+m_reg8bit1);
      //delta = dmove[(instr - reg8bit1) & 0xf];
      tmp=instr - m_reg8bit1;
      //tm->dir = (tm->dir + delta) & 3;
      m_reg8bit2=(m_reg8bit2+tmp)&7;
      //do move and wrap
      wormdir=(m_reg8bit2);// removed ^7 as was above
      instructionp+=biotadir[wormdir];
      // finally
      m_reg8bit1 += deltastate[instr&15];
      break;
	    ////////////////////////////CPUINTREV
      	  case 13: // ANT ???
	    instr=xxx[instructionp];
	    machine_poke(instructionp,instr+biotadir[m_reg8bit1&7]);
	    m_reg8bit1=antrulee(m_reg8bit1,instr&7,machine_p88k(0));//last is rule
	    instructionp+=biotadir[m_reg8bit1&7];
      break;
	    ////////////////////////////TURING
      /*      
	    	    instr=xxx[instructionp];
      if (thread_stack_count(16)) machine_poke(instructionp,thread_pop());
      else thread_push(machine_p88k(instructionp));
      instructionp++;
      */
	  case 0:
      	    desc[count++]=randi();
	    if (count==64) count=0;

	    reader=(xxx[tc>>3]>>(tc&7)) &1;
	    if (reader==0) state=desc[pc];
	    else state=desc[(pc+1)%64];

	    // do state
	    pc=(state>>2)&63; // new state - last 6 bits
	    towrite=(state&1); // 1st bit
	    togo=(state>>1)&1; // 2nd bit

	    // write tape bit 
	    if (towrite==0)    xxx[tc>>3]&=~(1 << (tc&7)); // clear bit
	    else xxx[tc>>3]|=(1 << (tc&7)); // set bit			 
	    modifier=xxx[tc>>3]; // TESTING?
	    flaggy=1;
	    if (togo==0) tc-=1;
	    else tc+=1;
	    if (tc<0) tc=2048; // wrap on >>3
	    if (tc>2048) tc=0;
      break;
	    ////////////////////////////CPUINTREV
	  case 15: //Corewars again
	    instr=xxx[instructionp];
      switch(instr%29){
      case 0:
	// MOV # to direct.
	machine_poke(instructionp+machine_p88k(instructionp+2),machine_p88k(instructionp+1));
	instructionp+=3;
	break;
      case 1:	  
	// MOV indirect to direct.
	machine_poke(instructionp+machine_p88k(instructionp+2),machine_p88k(machine_p88k(instructionp+1)));
	instructionp+=3;
	break;
      case 2:
	// MOV # to indirect.
	machine_poke(machine_p88k(machine_p88k(instructionp+2)),machine_p88k(instructionp+1));
	instructionp+=3;
	break;
      case 3:
	//MOV indirect to indirect
	machine_poke(machine_p88k(machine_p88k(instructionp+2)),machine_p88k(machine_p88k(instructionp+1)));
	instructionp+=3;
	break;
      case 4:
	//ADD # to direct
	machine_poke(instructionp+machine_p88k(instructionp+2),machine_p88k(instructionp+1)+machine_p88k(instructionp+machine_p88k(instructionp+2)));
	instructionp+=3;
	break;
	//HERE
      case 5:
	// ADD indirect to direct.
	machine_poke(instructionp+machine_p88k(instructionp+2),machine_p88k(machine_p88k(instructionp+1))+(machine_p88k(instructionp+(machine_p88k(instructionp+2)>>8))));
	instructionp+=3;
	break;
      case 6:
	// ADD # to indirect.
	machine_poke(machine_p88k(machine_p88k(instructionp+2)),machine_p88k(instructionp+1)+machine_p88k(machine_p88k(instructionp+2)));
	instructionp+=3;
	break;
      case 7:
	//ADD indirect to indirect
	machine_poke(machine_p88k(machine_p88k(instructionp+2)),machine_p88k(machine_p88k(instructionp+1))+machine_p88k(machine_p88k(instructionp+2)));
	instructionp+=3;
	break;
      case 8:
	//SUB # to direct
	machine_poke(instructionp+machine_p88k(instructionp+2),machine_p88k(instructionp+1)-machine_p88k(instructionp+machine_p88k(instructionp+2)));
	instructionp+=3;
	break;
      case 9:
	// indirect to direct.
	machine_poke(instructionp+machine_p88k(instructionp+2),machine_p88k(machine_p88k(instructionp+1)-machine_p88k(instructionp+machine_p88k(instructionp+2))));
	instructionp+=3;
	break;
      case 10:
	// # to indirect.
	machine_poke(machine_p88k(machine_p88k(instructionp+2)),machine_p88k(instructionp+1)-machine_p88k(machine_p88k(instructionp+2)));
	instructionp+=3;
	break;
      case 11:
	// indirect to indirect
	machine_poke(machine_p88k(machine_p88k(instructionp+2)),machine_p88k(machine_p88k(instructionp+1))-machine_p88k(machine_p88k(instructionp+2)));
	instructionp+=3;
	break;
      case 12:
	// jmp to direct
	instructionp+=machine_p88k(instructionp+1);
	break;
      case 13:
	// jmp to indirect
	instructionp=machine_p88k(machine_p88k(instructionp+1));
	break;
      case 14:
	// JMZdirect to direct
	if (machine_p88k(instructionp+machine_p88k(instructionp+2))==0) 	instructionp+=machine_p88k(instructionp+1);
	else 	instructionp+=3;
	break;
      case 15:
	// JMZdirect to indirect
	if (machine_p88k(instructionp+machine_p88k(instructionp+2))==0) 	instructionp=machine_p88k(machine_p88k(instructionp+1));
	else 	instructionp+=3;
	break;
      case 16:	
	// JMZindirect to direct
	if (machine_p88k(machine_p88k(machine_p88k(instructionp+2)))==0) 	instructionp+=machine_p88k(instructionp+1);
	else 	instructionp+=3;
	break;
      case 17:
	// JMZindirect to indirect
	if (machine_p88k(machine_p88k(machine_p88k(instructionp+2)))==0) 	instructionp=machine_p88k(machine_p88k(instructionp+1));
	else 	instructionp+=3;
	break;
      case 18:
	// JMGdirect to direct
	if (machine_p88k(instructionp+machine_p88k(instructionp+2))>0) 	instructionp+=machine_p88k(instructionp+1);
	else 	instructionp+=3;
	break;
      case 19:
	// JMGdirect to indirect
	if (machine_p88k(instructionp+machine_p88k(instructionp+2))>0) 	instructionp=machine_p88k(machine_p88k(instructionp+1));
	else 	instructionp+=3;
	break;
      case 20:	
	// JMGindirect to direct
	if (machine_p88k(machine_p88k(machine_p88k(instructionp+2)))>0) 	instructionp+=machine_p88k(instructionp+1);
	else 	instructionp+=3;
	break;
      case 21:
	// JMGindirect to indirect
	if (machine_p88k(machine_p88k(machine_p88k(instructionp+2)))>0) 	instructionp=machine_p88k(machine_p88k(instructionp+1));
	else 	instructionp+=3;
	break;
      case 22:
	// Dec, jump if zero... sub 1 from b. jump to a if b is zero
	// DJZ dir to dir
	machine_poke(instructionp+machine_p88k(instructionp+2), machine_p88k(instructionp+machine_p88k(instructionp+2))-1);
	if (machine_p88k(instructionp+machine_p88k(instructionp+2))==0)	  instructionp+=machine_p88k(instructionp+1);
	else instructionp+=3;
	break;
      case 23:
	// DJZ dir to indir
	machine_poke(instructionp+machine_p88k(instructionp+2), machine_p88k(instructionp+machine_p88k(instructionp+2))-1);
	if (machine_p88k(instructionp+machine_p88k(instructionp+2))==0)	  instructionp=machine_p88k(machine_p88k(instructionp+1));
	else instructionp+=3;
	break;
      case 24:
	// DJZ indir to dir
	machine_poke(machine_p88k(machine_p88k(instructionp+2)), machine_p88k(machine_p88k(instructionp+2)-1));
	if (machine_p88k(machine_p88k(machine_p88k(instructionp+2)))==0) instructionp+=machine_p88k(instructionp+1);
	else instructionp+=3;
	break;
      case 25:
	// DJZ indir to indir
	machine_poke(machine_p88k(machine_p88k(instructionp+2)), machine_p88k(machine_p88k(instructionp+2)-1));
	if (machine_p88k(machine_p88k(machine_p88k(instructionp+2)))==0) instructionp=machine_p88k(machine_p88k(machine_p88k(instructionp+1)));
	else instructionp+=3;
	break;
	/* case 26: */ // removed
      /* 	// SPL */
      /* 	//- add new thread at address x TESTY TODO! */
      /* 	//	cpustackpush(machine_p88k(instructionp+1),machine_p88k(instructionp+2),6,m_del); */
      /* 	//	printf("adde\n"); */
      /* 	instructionp+=3; */
      /* 	break; */
      case 26:
	instructionp+=3;
	break;
      case 27:
	// input to direct.
	machine_poke(instructionp+machine_p88k(instructionp+2),randi());
	instructionp+=3;
	break;
      case 28:
	// to indirect.
	machine_poke(machine_p88k(machine_p88k(instructionp+2)),randi());
	instructionp+=3;
	break;
      }

	    break;
      /////_>>       
	  } // switch CPU
	}
	

  if (++counterr>=speed) { //once per sec test was passed
    counterr=0;
    oldone++; 
    if (flaggy==1) {
	    flaggy=0;
	  }
    else modifier=xxx[oldone];
	  /// invert or?
#ifdef AVR_IS
    OCR0A=255-modifier;
#else
    //        printf("x: %d mod: %d\n",oldone, modifier);
       printf("%c",modifier);
#endif
  }
  if (++othercounter>=plaguespeed){ //was instructionp%step
    othercounter=0;
           (*plag[plague])(cells);
  }
	    //  }
  }
}

