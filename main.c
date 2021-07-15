/* Super_Mario.c */ 

#define D4 eS_PORTD4
#define D5 eS_PORTD5
#define D6 eS_PORTD6
#define D7 eS_PORTD7
#define RS eS_PORTD0
#define EN eS_PORTD1

#include <avr/io.h>     
#define F_CPU 1000000 
#include <util/delay.h>  
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <avr/interrupt.h>
#include <string.h>
#include "lcd.h"

#define MAN_HEIGHT 2
#define INF 1e9
const int totalBar=3;

char str1[10],str2[10],str3[10];
int matrix1R[8][8]={0},matrix3R[8][8]={0},matrix1L[8][8]={0},matrix2L[8][8]={0},matrix3L[8][8]={0};
int genRow[8]={1,2,4,8,16,32,64,128};

struct Bars{
	int row,col,length;
}bars[3];

struct Obstacles{
	int row,col;
}obs[3];
	
struct Player{
	int row,col,lifeLeft,score;
}man;	

void setBar(){
	for(int i=0;i<totalBar;i++){
		if(i==0){
			//bars[i].row=(rand()%(7-6+1))+6;// 6th or 7th row
			bars[i].row=7;
		    bars[i].col=5;
		}
		else if(i==1){
			//bars[i].row=(rand()%(5-4+1))+4;// 4th or 5th row
			bars[i].row=5;
			bars[i].col=-2;
		}
		else if(i==2){
			//bars[i].row=(rand()%(3-2+1))+2;// 2nd or 3rd row
			bars[i].row=3;
			bars[i].col=-8;
		}
		bars[i].length=5;
	}
}

int barInsideFrame(int id){
	int begin=bars[id].col;
	int end=begin+bars[id].length-1;

	int leftFrame=0;
	int rightFrame=15;

	if(begin>=leftFrame && end<=rightFrame) return 1;
	if(begin<=leftFrame && end>=leftFrame) return 1;
	if(begin<=rightFrame && end>=rightFrame) return 1;

	return 0;
}

void setMatrixForBar(int id) {
	
	int barRow=bars[id].row;
	int barColBegin=bars[id].col;
	int barColEnd=barColBegin+bars[id].length-1;
	
	int printBegin=0;
	
	if(barColBegin>=printBegin && barColBegin<=7){
		printBegin=barColBegin;
        int printEnd=7;
	    if(barColEnd<=printEnd){
		   printEnd=barColEnd;
	       for(int i=printBegin;i<=printEnd;i++){
			   if(i>=0) matrix1R[barRow][i]=1;
	       }
	    }
		else{
		   int flag=0;
		   for(int i=printBegin;i<=7;i++){
			   matrix1R[barRow][i]=1;
			   flag++;
		   }	
		   printEnd=bars[id].length-flag-1;
		   for(int i=0;i<=printEnd;i++){
			   matrix1L[barRow][i]=1;
		   }
		}
	}
	else if(barColBegin>=printBegin && barColBegin<=15){
		printBegin=barColBegin;
		int printEnd=15;
		if(barColEnd<=printEnd)	printEnd=barColEnd;
		printBegin-=8;
		printEnd-=8;
		for(int i=printBegin;i<=printEnd;i++){
				matrix1L[barRow][i]=1;
		}
	}
	
}

void updateBar(){
	memset(matrix1R,0,sizeof matrix1R);
	memset(matrix1L,0,sizeof matrix1L);
	for(int i=0;i<totalBar;i++){
		if(barInsideFrame(i)){
			setMatrixForBar(i);
		}
	}
}

void printBar(){
	for(int r=0;r<8;r++){
		int sum1=0,sum2=0;
		for(int c=0;c<8;c++){
			if(matrix1R[r][c]==1){
				sum1=sum1|(1<<c);
			}
			else if(matrix1L[r][c]==1){
				sum2=sum2|(1<<c);
			}
		}
		PORTB=~genRow[r];
		PORTA=sum1;
		PORTC=sum2;
		_delay_ms(1);
	}
}

void moveBar(){
	for(int i=0;i<totalBar;i++){
		if(bars[i].col==15){
			if(i==0) bars[i].col=(rand()%(-1))+(-1);
			else if(i==1) bars[i].col=(rand()%(-1))+(-1);
			else if(i==2) bars[i].col=(rand()%(-3))+(-3);
		}
		else bars[i].col++;
	}
}

void setPlayer(){
	man.row=2;//man's leg
	man.col=5;
	man.lifeLeft=7;
	man.score=0;
	memset(matrix2L,0,sizeof matrix2L);
	for(int i=0;i<MAN_HEIGHT;i++){
		matrix2L[man.row-i][man.col]=1;
	}
}

void recoverPlayer(){
	man.row=2;
	man.col=5;
	memset(matrix2L,0,sizeof matrix2L);
	for(int i=0;i<MAN_HEIGHT;i++){
	   matrix2L[man.row-i][man.col]=1;
	}
}

void jumpPlayer(){
	memset(matrix2L,0,sizeof matrix2L);
	man.row=man.row-MAN_HEIGHT;
	if(man.row==0){
		//collusion with upper screen
		man.lifeLeft--;
		recoverPlayer();
	}
	int flag=0;
	//check collusion with upper bar
	for(int i=0;i<MAN_HEIGHT;i++){
		if(matrix1L[man.row-i][man.col]==1) flag=1;
	}
	if(flag==0){
		man.score++;
		for(int i=0;i<MAN_HEIGHT;i++){
			matrix2L[man.row-i][man.col]=1;
		}
	}
}


void updatePlayer(){
	memset(matrix2L,0,sizeof matrix2L);
	
	if(matrix1L[man.row+1][man.col]==0){
		//falling down
		man.row++;
		for(int i=0;i<MAN_HEIGHT;i++){
			matrix2L[man.row-i][man.col]=1;
		}	
		if(man.row==7){
			//collusion with lower screen
			man.lifeLeft--;
			recoverPlayer();
		}
	}
	else if(matrix1L[man.row+1][man.col]==1){
		//no change
		for(int i=0;i<MAN_HEIGHT;i++){
			matrix2L[man.row-i][man.col]=1;
		}
	}
	
	//collusion with obstacle
	if(matrix3L[man.row][man.col]==1){
		man.lifeLeft--;
		recoverPlayer();
	}
}

void printPlayer(){
	for(int r=0;r<8;r++){
		int sum=0;
		for(int c=0;c<8;c++){
			if(matrix2L[r][c]==1){
				sum=sum|(1<<c);
			}
		}
		PORTB=~genRow[r];
		PORTA=0;
		PORTC=sum;
		_delay_ms(1);
	}
}

void setObstacle(){
	for(int i=0;i<totalBar;i++){
		obs[i].row=bars[i].row-1;
		obs[i].col=bars[i].col+1;
	}
	memset(matrix3R,0,sizeof matrix3R);
	memset(matrix3L,0,sizeof matrix3L);
	for(int i=0;i<totalBar;i++) {
		if(obs[i].col<=7){
		   matrix3R[obs[i].row][obs[i].col]=1;
		   matrix3R[obs[i].row][obs[i].col+1]=1;
		}
	}
}

void moveObstacles(){
	memset(matrix3R,0,sizeof matrix3R);
	memset(matrix3L,0,sizeof matrix3L);
	for(int i=0;i<totalBar;i++){
		if(bars[i].col>=0 && bars[i].col<7){
			obs[i].col=bars[i].col+1;
		    matrix3R[obs[i].row][obs[i].col]=1;
			if(obs[i].col!=7) matrix3R[obs[i].row][obs[i].col+1]=1;
			if(bars[i].col==6) matrix3L[obs[i].row][0]=1;
		}
		else if(bars[i].col>=7 && bars[i].col<15){
			obs[i].col=bars[i].col+1-8;
			matrix3L[obs[i].row][obs[i].col]=1;
			if(obs[i].col!=7) matrix3L[obs[i].row][obs[i].col+1]=1;
		}
	}
}

void printObstacles(){
	for(int r=0;r<8;r++){
		int sum1=0,sum2=0;
		for(int c=0;c<8;c++){
			if(matrix3R[r][c]==1){
				sum1=sum1|(1<<c);
			}
			else if(matrix3L[r][c]==1){
				sum2=sum2|(1<<c);
			}
		}
		PORTB=~genRow[r];
		PORTA=sum1;
		PORTC=sum2;
		_delay_ms(1);
	}
}

void gameDelay(int n){
	while(n--){
		_delay_ms(1);
	}
}

int main(void)
{
	DDRA=0xFF; //For LED matrix
	DDRB=0xFF; 
	DDRC=0xFF;
	
	DDRD=0b11111011;//For LCD display //button is in PIND2
	Lcd4_Init();
	Lcd4_Set_Cursor(1,1);
	Lcd4_Write_String("Score:");
	Lcd4_Set_Cursor(1,10);
	Lcd4_Write_String("Life:");
	Lcd4_Set_Cursor(2,1);
	Lcd4_Write_String("Level:");
	
	setBar();
	setObstacle();
	setPlayer();
	
	int game_delay=250;
	int level_count=1;
	int p=1;
	
	while(1)
	{	
		gameDelay(game_delay);
			
		updateBar();
		printBar();
		moveBar();
		
		printObstacles();
		moveObstacles();
		
		sprintf(str1,"%d",man.score);
		Lcd4_Set_Cursor(1,7);
		Lcd4_Write_String(str1);
		
		sprintf(str2,"%d",man.lifeLeft);
		Lcd4_Set_Cursor(1,15);
		Lcd4_Write_String(str2);
		
		sprintf(str3,"%d",level_count);
		Lcd4_Set_Cursor(2,7);
		Lcd4_Write_String(str3);
		
		printPlayer();
		updatePlayer();
		
		unsigned char in=PIND;
		if(in & 0b00000100){
			jumpPlayer();
		}
		
		if(man.lifeLeft<=0){
			sprintf(str2,"%d",0);
			Lcd4_Set_Cursor(1,15);
			Lcd4_Write_String(str2);
			sprintf(str3,"%d",level_count);
			Lcd4_Set_Cursor(2,7);
			Lcd4_Write_String(str3);
			Lcd4_Set_Cursor(2,10);
			Lcd4_Write_String("Over!");
			break;
		}
		
		if(man.score==3*p && game_delay-50>=0) {
			level_count++;
			p++;
			game_delay-=50;
		}
	}
} 
