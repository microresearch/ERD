/*

MODIFICATIONS for euro:

// HW issues: jumper last cap C13 and cut trace left of R10 to X19 and
jumper wire to right of R7// GND on edge to caps for ATMEGA!

TODO:

- new generators/stacks and ports from D.I. - TURING machine somehow,
  also stack overflow and others!

- take care of speed settings for CPU and plague separately
- 

DONE:

- and others question of random and also speed *now last knob CV = PC0
and PC5 should replace rand and first is BOTH plague and CPU* DONE
- out is on PD6 now // was PD6=0C0A DONE
- adcread(3) as open/random or ???? - should now be (adcread(0)+adcread(5)) - DONE - STILL TODO????
- remove filter/filtermod and other hardware DONE
- speed setting = pinX DONE - maybe 2 different speeds lower and upper bitsDONE
// test potis first! how could work - POTI plus CV
- others = pinXX and pinXXX (once for CV, once for POTI - how to combine?) DONE

- additional instructions and plague simulations

//////
1-RV1=CV2=// PC2+PC4 (latter in all is CV)
2-RV3=CV3=// PC0+PC5
3-MID top/bottom// RV2=CV1=// PC1+PC3 =speed
- THE PLAGUE INTERPRETER

... to bring back to all of us a natural, occult equivalent of the
dogma we no longer believe. [Antonin Artaud]

Returning the body, electronics, and dystopic code to the earth,
revived and decoded years later as "yersinia pestis".

- knobs/CV:

left: select instruction set (>>x)
mid: speed of both!
right: plague process select

*/

#define F_CPU 16000000UL 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define CELLLEN 16

#define floor(x) ((int)(x))

#define MAX_SAM 255

#define BV(bit) (1<<(bit)) // Byte Value => converts bit into a byte value. One at bit location.
#define cbi(reg, bit) reg &= ~(BV(bit)) // Clears the corresponding bit in register reg
#define sbi(reg, bit) reg |= (BV(bit))              // Sets the corresponding bit in register reg
#define HEX__(n) 0x##n##UL
#define recovered 129
#define dead 255                                                                   
#define susceptible 0

signed char insdir,dir; 

unsigned char filterk, cpu, plague, step, hardk, fhk, instruction, instructionp, hardware, samp, qqq;
unsigned char IP, controls;

static unsigned char xxx[MAX_SAM+12];

void adc_init(void)
{
	cbi(ADMUX, REFS1);
	sbi(ADMUX, REFS0); // AVCC
	sbi(ADMUX, ADLAR); //8 bits
	sbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1); // change speed here! now div by 64
	//	sbi(ADCSRA, ADPS0); // change speed here!

	sbi(ADCSRA, ADEN);
	sbi(ADCSRA, ADIE);
	DDRC = 0x00;
	PORTC = 0x00;
}

unsigned char adcread(unsigned char channel){
  unsigned char result, high;
  ADMUX &= 0xF8; // clear existing channel selection                
  ADMUX |=(channel & 0x07); // set channel/pin
  ADCSRA |= (1 << ADSC);  // Start A2D Conversions 
  loop_until_bit_is_set(ADCSRA, ADIF); /* Wait for ADIF, will happen soon */
  result=ADCH;
  return result;
}

void initcell(unsigned char* cells){
  unsigned char x;
  for (x=0;x<MAX_SAM;x++){
    cells[x]=(adcread(0)+adcread(5));
  }
}

void leftsh(unsigned int cel){
  OCR1A=cel<<filterk;
}

void rightsh(unsigned int cel){
  OCR1A=cel>>filterk;
}

void mult(unsigned int cel){
OCR1A=cel*filterk;
}

void divvv(unsigned int cel){
  OCR1A=cel/(filterk+1);
}

//void (*filtermod[])(unsigned int cel) = {leftsh, rightsh, mult, divvv};  

void mutate(unsigned char* cells){
  unsigned char x,y;
  for (y=0;y<cells[0];y++){
    x=(adcread(0)+adcread(5));
  cells[x]^=(x&0x0f);
  }
}

void hodge(unsigned char* cellies){
  int sum=0, numill=0, numinf=0;
  unsigned char q,k1,k2,g;
  static unsigned char x=CELLLEN+1;
  static unsigned char flag=0;
  unsigned char *newcells, *cells;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM/2];
  }
  else {
    cells=&cellies[MAX_SAM/2]; newcells=cellies;
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
    newcells[x%128] = floor(numinf / k1) + floor(numill / k2);
  else if(cells[x] < q - 1)
    newcells[x%128] = floor(sum / (numinf + 1)) + g;
  else
    newcells[x%128] = 0;

  if(newcells[x%128] > q - 1)
    newcells[x%128] = q - 1;

  x++;
  if (x>((MAX_SAM/2)-CELLLEN-1)) {
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
  l%=CELLLEN;

  for (cell = 1; cell < CELLLEN; cell++){ 
      state = 0;
      if (cells[cell + 1+ (l*CELLLEN)]>128)
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
  unsigned char cell,x,sum=0;
  static unsigned char flag=0;
  unsigned char *newcells, *cells;
  unsigned char kk=cellies[0], p=cellies[1];

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM/2];
  }
  else {
    cells=&cellies[MAX_SAM/2]; newcells=cellies;
  }      


  for (x=CELLLEN;x<((MAX_SAM/2)-CELLLEN);x++){
    cell = cells[x];
    newcells[x]=cell;
    if (cell >= kk) newcells[x] = recovered;                                                 else if ((cell>0 && cell<kk)){
      newcells[x]++;                                                       
    }
    else if (cell == susceptible) {   
                                                 
      if ( (cells[x-CELLLEN]>0 && cells[x-CELLLEN]<kk) ||
	   (cells[x+CELLLEN]>0 && cells[x+CELLLEN]<kk) ||
	   (cells[x-1]>0 && cells[x-1]<kk) ||
	   (cells[x+1]>0 && cells[x+1]<kk))
	{
	if (rand()%10 < p) newcells[x] = 1;       
      }
    }
  }
  flag^=0x01;
}

void life(unsigned char* cellies){
  unsigned char x, sum;

  static unsigned char flag=0;
  unsigned char *newcells, *cells;

  if (flag==0) {
    cells=cellies; newcells=&cellies[MAX_SAM/2];
  }
  else {
    cells=&cellies[MAX_SAM/2]; newcells=cellies;
  }      

  for (x=CELLLEN+1;x<((MAX_SAM/2)-CELLLEN-1);x++){
    sum=cells[x]%2+cells[x-1]%2+cells[x+1]%2+cells[x-CELLLEN]%2+cells[x+CELLLEN]%2+cells[x-CELLLEN-1]%2+cells[x-CELLLEN+1]%2+cells[x+CELLLEN-1]%2+cells[x+CELLLEN+1]%2;
    sum=sum-cells[x]%2;
    if (sum==3 || (sum+(cells[x]%2)==3)) newcells[x]=255;
    else newcells[x]=0;
  }
  
  // swapping 
  flag^=0x01;
}

// instructions for plague CPUS!

unsigned char ostack[20], stack[20], omem;

/* BIOTA: two dimensional memory map */

unsigned char btdir,dcdir;

unsigned char btempty(unsigned char* cells, unsigned char IP){
  // turn around
  if (btdir==0) btdir=1;
  else if (btdir==1) btdir=0;
  else if (btdir==2) btdir=3;
  else if (btdir==3) btdir=2;
  return IP;
}

unsigned char btoutf(unsigned char* cells, unsigned char IP){
  //(*filtermod[qqq]) ((int)cells[omem]);
  return IP;
}

unsigned char btoutp(unsigned char* cells, unsigned char IP){
  OCR0A=cells[omem];
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

unsigned char clock, count;

// reddeath

//1- the plague within (12 midnight) - all the cells infect

unsigned char redplague(unsigned char* cells, unsigned char IP){
  if (clock==12){
    unsigned char tmp=IP+1;
    clock=12;
    cells[tmp]=cells[IP];
    if (IP==255) clock=13;
    return IP+1;
  }
  else return IP+insdir;
}

//2- death - one by one fall dead
unsigned char reddeath(unsigned char* cells, unsigned char IP){
  if (clock==13){
    clock=13;
    unsigned char tmp=IP+count;
    cells[tmp]=(adcread(0)+adcread(5));
    count++;
    return IP; // just keeps on going
  }
  else return IP+insdir;
}

//3- clock every hour - instruction counter or IP -some kind of TICK
unsigned char redclock(unsigned char* cells, unsigned char IP){
  clock++;
  if (clock%60==0) {
    OCR0A^=255;
    return IP; // everyone stops
  }
  else return IP+insdir;
}

//4- seven rooms: divide cellspace into 7 - 7 layers with filter each
unsigned char redrooms(unsigned char* cells, unsigned char IP){// TODO!
  /*  switch(IP%7){
  case 0:
    //blue
          sbi(DDRB,PB1);
            TCCR1B=  (1<<WGM12) |(1<<CS10); // no divider
      filterk=8;
    break;
  case 1:
    //purple
      sbi(DDRB,PB1);
      TCCR1B=  (1<<WGM12) |(1<<CS10); // no divider
    break;
  case 2:
    //green
      sbi(DDRB,PB1);
      TCCR1B=  (1<<WGM12) |(1<<CS11); // divide by 8
      filterk=8;
    break;
  case 3:
    //orange
      sbi(DDRB,PB1);
      TCCR1B=  (1<<WGM12) |(1<<CS11); // divide by 8
    break;
  case 4:
    //white
      sbi(DDRB,PB1);
      TCCR1B=  (1<<WGM12) |(1<<CS11)|(1<<CS10); // divide by 64
    break;
  case 5:
    //violet
      sbi(DDRB,PB1);
      TCCR1B=  (1<<WGM12) |(1<<CS12); //256
    break;
  case 6:
    // black
    cbi(DDRB,PB1); //filter off
    }*/
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
  dirrr=(adcread(0)+adcread(5))%4;
  if (dirrr==0) omem=omem+1;
  else if (dirrr==1) omem=omem-1;
  else if (dirrr==2) omem=omem+16;
  else if (dirrr==3) omem=omem-16;

  // output
  OCR0A=cells[omem]; 
  return IP+insdir;
}

  //7- the outside - the input!
unsigned char redoutside(unsigned char* cells, unsigned char IP){
unsigned char tmp=omem+1;
  // input sample to cell (which one neighbour to omem)
  cells[tmp]=(adcread(0)+adcread(5));

  // output to filter 
  //  (*filtermod[qqq]) ((int)cells[omem]);
  return IP+insdir;
}

// plague

unsigned char ploutf(unsigned char* cells, unsigned char IP){
  //(*filtermod[qqq]) ((int)cells[omem]);

  return IP+insdir;
}

unsigned char ploutp(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP-1;
  unsigned char tmpp=IP+1;
  OCR0A=cells[tmp]+cells[tmpp];
  return IP+insdir;
}

unsigned char plenclose(unsigned char* cells, unsigned char IP){
  unsigned char tmpp=IP+1;
  cells[IP]=255; cells[tmpp]=255;
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
  return IP+insdir;
}

unsigned char plwalk(unsigned char* cells, unsigned char IP){
  // changing direction
  if (dir<0 && (cells[IP]%0x03)==1) dir=1;
  else if (dir>1 && (cells[IP]%0x03)==0) dir=-1;
  else
    // changing pace
    insdir=(int)dir*cells[IP]>>4;
  
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

unsigned char rdoutf(unsigned char* cells, unsigned char IP){
  //  (*filtermod[qqq]) ((int)cells[IP+1]);
  IP+=3;
  return IP;
}

unsigned char rdoutp(unsigned char* cells, unsigned char IP){
  unsigned char tmpp=IP+2;
  OCR0A=cells[tmpp]; 
  IP+=3;
  return IP;
}

// SIR: inc if , die if, recover if, getinfected if 

unsigned char SIRoutf(unsigned char* cells, unsigned char IP){
  //  (*filtermod[qqq]) ((int)cells[(IP+1)]+(int)cells[IP-1]);
  return IP+insdir;
}

unsigned char SIRoutp(unsigned char* cells, unsigned char IP){
  unsigned char tmp=IP+1;
  unsigned char tmpp=IP-1;
  OCR0A=cells[tmp]+cells[tmpp];
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
    if (rand()%10 < 4) cells[IP] = dead;       
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
	if (rand()%10 < 4) cells[IP] = 1;       
	}
}
  return IP+insdir;
}

// brainfuck

unsigned char cycle;

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

unsigned char bfoutf(unsigned char* cells, unsigned char IP){
  //  (*filtermod[qqq]) ((int)cells[omem]);
  return ++IP;
}

unsigned char bfoutp(unsigned char* cells, unsigned char IP){
  OCR0A=cells[omem]; 
  return ++IP;
}

unsigned char bfin(unsigned char* cells, unsigned char IP){
  cells[omem] = (adcread(0)+adcread(5)); 
  return ++IP;
}

unsigned char bfbrac1(unsigned char* cells, unsigned char IP){
  cycle++; 
  if(cycle>=20) cycle=0; 
  ostack[cycle] = IP; 
  return ++IP;
}

unsigned char bfbrac2(unsigned char* cells, unsigned char IP){
  int i;
  if(cells[omem] != 0) i = ostack[cycle]-1; 
  cycle--; 
  if(cycle<-1) cycle=20;
  return i;
}

// first attempt

unsigned char finc(unsigned char* cells, unsigned char IP){
  omem++; 
  return IP+insdir;
}

unsigned char fin1(unsigned char* cells, unsigned char IP){
  omem=(adcread(0)+adcread(5));
  return IP+insdir;
}

unsigned char fin2(unsigned char* cells, unsigned char IP){
  omem=adcread(2);
  return IP+insdir;
}

unsigned char fin3(unsigned char* cells, unsigned char IP){
  IP=adcread(2);
  return IP+insdir;
}

unsigned char fin4(unsigned char* cells, unsigned char IP){
  cells[omem]=(adcread(0)+adcread(5));
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

unsigned char outf(unsigned char* cells, unsigned char IP){
  //(*filtermod[qqq]) ((int)cells[omem]);
  return IP+insdir;
}

unsigned char outp(unsigned char* cells, unsigned char IP){
  OCR0A=cells[omem];
  return IP+insdir;
}

unsigned char outff(unsigned char* cells, unsigned char IP){
  //(*filtermod[qqq]) ((int)cells[omem]);
  return IP+insdir;
}

unsigned char outpp(unsigned char* cells, unsigned char IP){
  OCR0A=omem;
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
  else return IP+insdir;
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
  cells[IP]=adcread(2);
  return IP+insdir;
}

unsigned char writesamp(unsigned char* cells, unsigned char IP){
  cells[IP]=(adcread(0)+adcread(5));
  return IP+insdir;
}


void main(void)
{
  unsigned int counter=0;
  
  unsigned char *cells=xxx; unsigned int speed, cpuspeed,plaguespeed;

  unsigned char (*instructionsetfirst[])(unsigned char* cells, unsigned char IP) = {outff,outpp,finc,fdec,fincm,fdecm,fin1,fin2,fin3,fin4,outf,outp,plus,minus,bitshift1,bitshift2,bitshift3,branch,jump,infect,store,writeknob,writesamp,skip,direction,die}; // 26 instructions

  unsigned char (*instructionsetplague[])(unsigned char* cells, unsigned char IP) = {writeknob,writesamp, ploutf, ploutp, plenclose, plinfect, pldie, plwalk}; // 8 

  unsigned char (*instructionsetbf[])(unsigned char* cells, unsigned char IP) = {bfinc,bfdec,bfincm,bfdecm,bfoutf,bfoutp,bfin,bfbrac1,bfbrac2}; // 9

  unsigned char (*instructionsetSIR[])(unsigned char* cells, unsigned char IP) = {SIRoutf,SIRoutp,SIRincif,SIRdieif,SIRrecif,SIRinfif}; // 6

  unsigned char (*instructionsetredcode[])(unsigned char* cells, unsigned char IP) = {rdmov,rdadd,rdsub,rdjmp,rdjmz,rdjmg,rddjz,rddat,rdcmp,rdoutf,rdoutp}; // 11

  unsigned char (*instructionsetbiota[])(unsigned char* cells, unsigned char IP) = {btempty,btoutf,btoutp,btstraight,btbackup,btturn,btunturn,btg,btclear,btdup}; // 10

  unsigned char (*instructionsetreddeath[])(unsigned char* cells, unsigned char IP) = {redplague,reddeath,redclock,redrooms,redunmask,redprospero,redoutside}; // 7

  void (*plag[])(unsigned char* cells) = {mutate,SIR,hodge,cel,hodge,SIR,life,mutate};

  adc_init();
  initcell(cells);
  dir=1;
  //  sei();
    DDRD=0x40; // 6 as out
    //    PORTB=0x40;
  //  DDRB|=0x02;  // was clock
  //    TCCR1A= (1<<COM1A0);// | (1<<WGM11) | (1<<WGM10); // KEEP AS CTC for filter
  //    TCCR1B= (1<<WGM12) |(1<<CS10); // /1024 now
  //      TCCR1A= (1<<COM1A0) | (1<<WGM11) | (1<<WGM10); // PWM
  //    TCCR1B= (1<<WGM12) | (1<<CS11);// | (1<<CS10); // /64

    //TCCR0A=(1<<COM0A0) | (1<<WGM01)| (1<<WGM00); // PWM output
  //  TCCR0A=(1<<COM0A0) | (1<<WGM01);
  //      TCCR0B|=(1<<CS00) | (1<<CS02) | (1<<WGM02);  // divide by one more - is now on /1024
  //TCCR0B|=(1<<CS00) | (1<<WGM02);  // divide by one more - is now on /1024

  // from arduino code:
        TCCR0A=(1<<COM0A1) | (1<<WGM01) | (1<<WGM00); 
        TCCR0B=(1<<CS00) | (1<<CS01);  // divide by 64 for 1KHz

      DIDR0=0x3f; // disable digital inputs on PC0 to PC5
    //    sbi(TIMSK0, TOIE0);

  //  cbi(PORTD,PD0);
  //  sbi(PORTD,PD1); 
  //  cbi(PORTD,PD2); // feedback

  instructionp=0; insdir=1; dir=1; btdir=0; dcdir=0;

      OCR0A=128;

  while(1){

        IP=adcread(2);
	//   counter++;
	//            if (counter%1000==0){
	//              IP++;
	//	  OCR0A=IP;
	//	    }
    //    qqq=controls%4; // choosing filtermod

   
	IP=(adcread(2)+adcread(4))>>2;
	//	controls=adcread(0)+adcread(5); //max 8 bits
	controls=IP&7;
	speed=(adcread(1)+adcread(3))>>1; // TESTING! - speed of both slowing!TODO!
    if (controls==0) controls=instructionp;
    counter++;
    cpu=IP>>3; // 8 CPUs // 6 bits->8 CPUs=3bits
    plague=controls%8; // figure this out 8*8=64 so wecan have 16 of each
    //    plaguespeed=controls&15; // TODO: take care of speeds!
    //    cpuspeed=IP&15;
    plaguespeed=0;cpuspeed=0;
    step=speed+1+plaguespeed; // 8 bits?

    // plague CPU!
    //    if (count%((IP%32)+1)==0){ // ??? This is where we do speed!
    
        if ((counter%(speed+1+cpuspeed))==0){ // ??? This is where we do speed!
	  //	  cpu=10;
	  switch(cpu){
	  case 0:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetfirst[instruction%26]) (cells, instructionp); // mistake before as was instruction%INSTLEN in last instance
	    //      insdir=dir*(IP%16)+1; // prev mistake as just got exponentially larger
	    insdir=dir;
	    break;
	  case 1:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetplague[instruction%8]) (cells, instructionp);
	    insdir=dir;
	    if (cells[instructionp]==255 && dir<0) dir=1;
	    else if (cells[instructionp]==255 && dir>0) dir=-1; // barrier
	    break;
	  case 2:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetbf[instruction%9]) (cells, instructionp);
	    insdir=dir;
	    break;
	  case 3:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetSIR[instruction%6]) (cells, instructionp);
	    insdir=dir;
	    break;
	  case 4:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetredcode[instruction%11]) (cells, instructionp); 
	    insdir=dir;
	    break;
	  case 5:
	    instruction=cells[instructionp];
	    OCR0A=instruction;
	    instructionp+=dir;
	    break;
	  case 6:
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetreddeath[instruction%7]) (cells, instructionp); 
	    insdir=dir;
	    break;
	  case 7:
	    //la biota
	    instruction=cells[instructionp];
	    instructionp=(*instructionsetbiota[instruction%10]) (cells, instructionp); 
	    if (btdir==0) instructionp+=1;
	    else if (btdir==1) instructionp-=1;
	    else if (btdir==2) instructionp+=16;
	    else if (btdir==3) instructionp-=16;
	    break;
	  }
	  	}
    
    if ((counter%step)==0){ //was instructionp%step
            (*plag[plague])(cells);
	    }
  }
}

