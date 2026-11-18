/* TANK BATTLE 2D - Turbo C++ 3.0 / Borland BGI
   Single-file arcade game. No STL, OpenGL, or external headers.
   CG: DDA, Bresenham, midpoint circle, translation, rotation,
       scaling, composite transformations, clipping, bar3d, shearing.
*/
#include <graphics.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define SW 640
#define SH 480
#define TOP 45
#define BOTTOM 424
#define MAX_ENEMY 6
#define MAX_SHOTS 24
#define PI 3.14159265
#define PLAYER 0
#define ENEMY 1

typedef struct { int x,y,alive,hp,dir,wait; } Tank;
typedef struct { int x,y,dx,dy,active,owner; } Shot;
Tank hero, foes[MAX_ENEMY];
Shot shots[MAX_SHOTS];
int state=0, level=1, score=0, lives=3, kills=0, goal=0;
int tick=0, paused=0, fireDelay=0, invincible=0;
int wallX[5]={145,290,425,205,480};
int wallY[5]={125,220,130,330,320};
int wallW[5]={70,65,70,65,55};
int wallH[5]={24,24,24,24,24};

void dda(int x1,int y1,int x2,int y2,int color)
{
 float x=x1,y=y1,dx=x2-x1,dy=y2-y1,steps,xi,yi;
 int i;
 steps=(float)(abs(x2-x1)>abs(y2-y1)?abs(x2-x1):abs(y2-y1));
 if(steps==0){putpixel(x1,y1,color);return;}
 xi=dx/steps; yi=dy/steps;
 for(i=0;i<=(int)steps;i++){putpixel((int)(x+0.5),(int)(y+0.5),color);x+=xi;y+=yi;}
}
void bres(int x1,int y1,int x2,int y2,int color)
{
 int dx=abs(x2-x1),dy=abs(y2-y1),sx=x1<x2?1:-1;
 int sy=y1<y2?1:-1,err=dx-dy,e2;
 while(1){putpixel(x1,y1,color);if(x1==x2&&y1==y2)break;
 e2=2*err;if(e2>-dy){err-=dy;x1+=sx;}if(e2<dx){err+=dx;y1+=sy;}}
}
void octants(int cx,int cy,int x,int y,int color)
{
 putpixel(cx+x,cy+y,color);putpixel(cx-x,cy+y,color);
 putpixel(cx+x,cy-y,color);putpixel(cx-x,cy-y,color);
 putpixel(cx+y,cy+x,color);putpixel(cx-y,cy+x,color);
 putpixel(cx+y,cy-x,color);putpixel(cx-y,cy-x,color);
}
void midcircle(int cx,int cy,int r,int color)
{
 int x=0,y=r,p=1-r;
 while(x<=y){octants(cx,cy,x,y,color);x++;
 if(p<0)p+=2*x+1;else{y--;p+=2*(x-y)+1;}}
}
int inside(int x,int y){return x>=14&&x<=625&&y>=TOP+14&&y<=BOTTOM-14;}
int hitwall(int x,int y,int radius)
{
 int i;
 for(i=0;i<5;i++)
 if(x+radius>wallX[i]&&x-radius<wallX[i]+wallW[i]&&
    y+radius>wallY[i]&&y-radius<wallY[i]+wallH[i])return 1;
 return 0;
}
int canmove(int x,int y){return inside(x,y)&&!hitwall(x,y,16);}
void textxy(int x,int y,char *s,int color){setcolor(color);outtextxy(x,y,s);}
void centered(int y,char *s,int color)
{
 int x=(SW-textwidth(s))/2;textxy(x,y,s,color);
}
void button(int x1,int y1,int x2,int y2,char *label,int color)
{
 int tx;
 setcolor(WHITE);setfillstyle(SOLID_FILL,color);bar3d(x1,y1,x2,y2,5,1);
 tx=x1+(x2-x1-textwidth(label))/2;
 textxy(tx,y1+11,label,WHITE);
}
void resetshots(){int i;for(i=0;i<MAX_SHOTS;i++)shots[i].active=0;}
void setupLevel()
{
 int i,n;
 hero.x=65;hero.y=370;hero.alive=1;hero.hp=3;hero.dir=1;hero.wait=0;
 n=level+1;if(n>MAX_ENEMY)n=MAX_ENEMY;
 goal=n;kills=0;tick=0;fireDelay=0;invincible=35;paused=0;
 resetshots();
 for(i=0;i<MAX_ENEMY;i++){
 foes[i].alive=(i<n);foes[i].hp=(level==3?3:2);
 foes[i].x=365+(i%3)*100;
 foes[i].y=90+(i/3)*190;
 foes[i].dir=3;foes[i].wait=10+i*13;
 }
}
void newGame(){level=1;score=0;lives=3;setupLevel();state=1;}
void spawnShot(int x,int y,int dir,int owner)
{
 int i,dx=0,dy=0;
 for(i=0;i<MAX_SHOTS;i++)if(!shots[i].active){
 if(dir==0)dy=-7;if(dir==1)dx=7;if(dir==2)dy=7;if(dir==3)dx=-7;
 shots[i].active=1;shots[i].owner=owner;
 shots[i].x=x+dx*3;shots[i].y=y+dy*3;
 shots[i].dx=dx;shots[i].dy=dy;break;
 }
}
void drawTank(int x,int y,int dir,int color,int hp)
{
 int i,cx,cy,tx,ty;
 float a,px,py;
 setcolor(DARKGRAY);setfillstyle(SOLID_FILL,DARKGRAY);
 bar(x-16,y-15,x+16,y+15);
 for(i=-1;i<=1;i++){
 setcolor(LIGHTGRAY);
 rectangle(x-19,y+i*9-3,x-13,y+i*9+3);
 rectangle(x+13,y+i*9-3,x+19,y+i*9+3);
 }
 setcolor(color);setfillstyle(SOLID_FILL,color);
 bar(x-12,y-12,x+12,y+12);
 midcircle(x,y,9,WHITE);
 setfillstyle(SOLID_FILL,LIGHTGRAY);fillellipse(x,y,6,6);
 /* Composite: rotate cannon vector then translate to tank center */
 a=dir*PI/2.0;
 px=0*cos(a)-(-23)*sin(a);
 py=0*sin(a)+(-23)*cos(a);
 tx=x+(int)px;ty=y+(int)py;
 setcolor(YELLOW);setlinestyle(SOLID_LINE,0,THICK_WIDTH);
 bres(x,y,tx,ty,YELLOW);
 setlinestyle(SOLID_LINE,0,NORM_WIDTH);
 setcolor(WHITE);rectangle(x-15,y-24,x+15,y-20);
 setfillstyle(SOLID_FILL,hp>=2?LIGHTGREEN:LIGHTRED);
 bar(x-14,y-23,x-14+hp*9,y-21);
}
void drawArena()
{
 int i,x,y;
 setfillstyle(SOLID_FILL,BLACK);bar(0,0,639,479);
 setfillstyle(SOLID_FILL,DARKGRAY);bar(0,TOP,639,BOTTOM);
 for(x=12;x<640;x+=25)for(y=TOP+12;y<BOTTOM;y+=25)
 putpixel(x,y,((x+y+tick/3)%3)?LIGHTGRAY:DARKGRAY);
 setcolor(LIGHTCYAN);rectangle(5,TOP,634,BOTTOM);
 for(i=0;i<5;i++){
 setcolor(WHITE);setfillstyle(SOLID_FILL,BROWN);
 bar3d(wallX[i],wallY[i],wallX[i]+wallW[i],wallY[i]+wallH[i],5,1);
 bres(wallX[i]+4,wallY[i]+4,wallX[i]+wallW[i]-4,wallY[i]+wallH[i]-4,YELLOW);
 }
}
void drawHUD()
{
 char buf[80];
 setfillstyle(SOLID_FILL,BLUE);bar(0,0,639,TOP-1);
 sprintf(buf,"TANK BATTLE 2D   LEVEL: %d   SCORE: %d   LIVES: %d",level,score,lives);
 textxy(10,10,buf,WHITE);
 sprintf(buf,"ENEMIES: %d / %d",goal-kills,goal);
 textxy(460,28,buf,YELLOW);
 setfillstyle(SOLID_FILL,BLUE);bar(0,BOTTOM+1,639,479);
 textxy(8,436,"ARROWS / WASD: MOVE & AIM   SPACE: FIRE   P: PAUSE",WHITE);
 textxy(8,455,"R: RESTART   ESC: MENU   Destroy all enemy tanks!",LIGHTCYAN);
}
void drawShots()
{
 int i;
 for(i=0;i<MAX_SHOTS;i++)if(shots[i].active){
 setcolor(shots[i].owner==PLAYER?YELLOW:LIGHTRED);
 setfillstyle(SOLID_FILL,shots[i].owner==PLAYER?YELLOW:LIGHTRED);
 fillellipse(shots[i].x,shots[i].y,4,4);
 }
}
void drawGame()
{
 int i;
 drawArena();
 if(invincible==0||(tick/3)%2==0)drawTank(hero.x,hero.y,hero.dir,LIGHTGREEN,hero.hp);
 for(i=0;i<MAX_ENEMY;i++)if(foes[i].alive)
 drawTank(foes[i].x,foes[i].y,foes[i].dir,LIGHTRED,foes[i].hp);
 drawShots();drawHUD();
 if(paused){setfillstyle(SOLID_FILL,BLACK);bar(190,175,455,280);
 setcolor(WHITE);rectangle(190,175,455,280);
 centered(195,"GAME PAUSED",YELLOW);
 centered(235,"PRESS P TO RESUME",WHITE);}
}
void drawMenu()
{
 int i,x,y;
 setfillstyle(SOLID_FILL,BLACK);bar(0,0,639,479);
 for(i=0;i<80;i++){x=(i*79+19)%640;y=(i*43+11)%480;putpixel(x,y,i%3?LIGHTGRAY:WHITE);}
 settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
 centered(75,"TANK BATTLE 2D",YELLOW);
 settextstyle(DEFAULT_FONT,HORIZ_DIR,1);
 centered(125,"TURBO C++ 3.0 COMPUTER GRAPHICS GAME",LIGHTCYAN);
 button(205,185,430,225,"ENTER - PLAY",GREEN);
 button(205,245,430,285,"H - HOW TO PLAY",BLUE);
 button(205,305,430,345,"ESC - EXIT",RED);
 centered(405,"SINGLE-FILE BGI PROJECT",LIGHTGRAY);
}
void drawHelp()
{
 cleardevice();
 centered(55,"HOW TO PLAY",YELLOW);
 textxy(90,115,"MOVE / AIM : ARROW KEYS OR W A S D",WHITE);
 textxy(90,150,"SHOOT      : SPACEBAR",WHITE);
 textxy(90,185,"PAUSE      : P",WHITE);
 textxy(90,220,"RESTART    : R",WHITE);
 textxy(90,255,"MENU       : ESC",WHITE);
 textxy(90,305,"Destroy every enemy tank to unlock the next level.",LIGHTGREEN);
 textxy(90,335,"Avoid bullets and walls. You have three lives.",LIGHTGREEN);
 centered(400,"PRESS ENTER OR ESC TO RETURN",YELLOW);
}
void drawEnd()
{
 char buf[80];
 cleardevice();settextstyle(DEFAULT_FONT,HORIZ_DIR,2);
 if(state==3)centered(100,"GAME OVER",LIGHTRED);
 else centered(100,"VICTORY!",LIGHTGREEN);
 settextstyle(DEFAULT_FONT,HORIZ_DIR,1);
 sprintf(buf,"FINAL SCORE: %d",score);centered(185,buf,WHITE);
 if(state==4)centered(230,"YOU DEFENDED THE BASE!",YELLOW);
 centered(315,"ENTER - PLAY AGAIN",LIGHTCYAN);
 centered(350,"ESC - MAIN MENU",LIGHTCYAN);
}
void damageHero()
{
 if(invincible>0)return;
 hero.hp--;invincible=45;
 if(hero.hp<=0){lives--;if(lives<=0){state=3;return;}
 hero.x=65;hero.y=370;hero.hp=3;hero.dir=1;invincible=65;}
}
void updateShots()
{
 int i,j,x,y;
 for(i=0;i<MAX_SHOTS;i++)if(shots[i].active){
 shots[i].x+=shots[i].dx;shots[i].y+=shots[i].dy;
 x=shots[i].x;y=shots[i].y;
 if(!inside(x,y)||hitwall(x,y,3)){shots[i].active=0;continue;}
 if(shots[i].owner==PLAYER){
 for(j=0;j<MAX_ENEMY;j++)if(foes[j].alive&&
 abs(x-foes[j].x)<20&&abs(y-foes[j].y)<20){
 shots[i].active=0;foes[j].hp--;score+=10;
 if(foes[j].hp<=0){foes[j].alive=0;kills++;score+=100;}
 break;
 }}else if(abs(x-hero.x)<18&&abs(y-hero.y)<18){
 shots[i].active=0;damageHero();}
 }
}
void updateEnemies()
{
 int i,nx,ny,dx,dy;
 for(i=0;i<MAX_ENEMY;i++)if(foes[i].alive){
 foes[i].wait--;
 if(foes[i].wait<=0){
 dx=hero.x-foes[i].x;dy=hero.y-foes[i].y;
 if(abs(dx)>abs(dy))foes[i].dir=dx>0?1:3;
 else foes[i].dir=dy>0?2:0;
 foes[i].wait=18+rand()%35;
 if(rand()%3!=0)spawnShot(foes[i].x,foes[i].y,foes[i].dir,ENEMY);
 }
 if(tick%(8-level*2)==0){
 nx=foes[i].x;ny=foes[i].y;
 if(foes[i].dir==0)ny-=3;
 if(foes[i].dir==1)nx+=3;
 if(foes[i].dir==2)ny+=3;
 if(foes[i].dir==3)nx-=3;
 if(canmove(nx,ny)&&!(abs(nx-hero.x)<38&&abs(ny-hero.y)<38)){
 foes[i].x=nx;foes[i].y=ny;
 }else foes[i].wait=0;
 }
 }
}
void handleKey(int key)
{
 int nx=hero.x,ny=hero.y;
 if(key==27){state=0;return;}
 if(key=='p'||key=='P'){paused=!paused;return;}
 if(key=='r'||key=='R'){newGame();return;}
 if(paused)return;
 if(key==72||key=='w'||key=='W'){ny-=8;hero.dir=0;}
 if(key==77||key=='d'||key=='D'){nx+=8;hero.dir=1;}
 if(key==80||key=='s'||key=='S'){ny+=8;hero.dir=2;}
 if(key==75||key=='a'||key=='A'){nx-=8;hero.dir=3;}
 if(canmove(nx,ny)){hero.x=nx;hero.y=ny;}
 if(key==' '&&fireDelay==0){spawnShot(hero.x,hero.y,hero.dir,PLAYER);fireDelay=7;}
}
int readKey()
{
 int key=getch();
 if(key==0||key==224)key=getch();
 return key;
}
int main()
{
 int gd=DETECT,gm,key,err;
 /* Change this path if your Turbo C++ BGI folder is elsewhere. */
 initgraph(&gd,&gm,"C:\\TURBOC3\\BGI");
 err=graphresult();
 if(err!=grOk){printf("BGI error: %s\n",grapherrormsg(err));return 1;}
 srand(13);state=0;
 while(1){
 if(state==0){drawMenu();key=readKey();
 if(key==27)break;
 if(key==13)newGame();
 else if(key=='h'||key=='H'){drawHelp();do{key=readKey();}while(key!=13&&key!=27);}
 }else if(state==1){
 if(kbhit()){key=readKey();handleKey(key);}
 if(state!=1)continue;
 if(!paused){tick++;if(fireDelay>0)fireDelay--;
 if(invincible>0)invincible--;
 updateShots();if(state!=1)continue;
 updateEnemies();if(kills>=goal){
 score+=200*level;
 if(level>=3)state=4;
 else{level++;setupLevel();}
 }}
 drawGame();delay(45);
 }else{drawEnd();key=readKey();
 if(key==13)newGame();else if(key==27)state=0;
 }
 }
 closegraph();return 0;
}
