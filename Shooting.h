#include<allegro.h>
#include<sched.h>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<semaphore.h>


#define NO_OF_PIGS 	5		/* number of pigeons */

/*  coordenates of the window - virtual world */
#define XMAX 		693
#define YMAX 		520

/* coordenates of the screen where the game is played - graphical world */
#define RAD  		9		/* orientation angle*/
#define X_MIN  		2
#define X_MAX 		470
#define Y_MIN 		135
#define Y_MAX 		360

/* coordenates of the countdown on the screen */
#define X_CDOWN  	620	
#define Y_CDOWN 	85

/* coordenates of the score on the screen */
#define X_SCORE  	620	
#define Y_SCORE 	195	

/* coordenates of the bullets on the screen */
#define X_BULLET  	620	
#define Y_BULLET 	300	

/* coordenates of the reload on the screen */
#define X_RELOAD  	620	
#define Y_RELOAD 	395		

#define PERIOD_TO_COUNT 1000
#define DELAY 		500

#define RELOAD_GUN 	10		/* the amount of bullets that will be reloaded */
#define RELOAD_TIME 	5		    /* max number of times that a gun can be reloaded */
#define SCORE_HIT	10		        /* the score when a pig is hitted */

#define PIG_ALIVE 	1		/* holds 1 if the pig is alive */
#define PID_DEAD	0		/* holds 0 if the pig is dead */

#define INIT_SCORE 	0		/* set the score when the game is restarted */
#define INIT_MIN	25		    /* set the game time when its restarted */

/* constants for the fly motion rules */
#define NORTHWEST	0
#define SOUTHWEST	1
#define NORTHEAST	2
#define SOUTHEAST	3
#define RANDOM		4		/* to generate random direction between 0 and 3  */

#define T		30		                /* motion constant */
#define SPEED1		0.2		/* speed of the first type of pigeons */
#define SPEED2		0.3		/* speed of the second type of pigeons */
#define SPEED3		0.4		/* speed of the third type of pigeons */

/* mouse range */
#define X1		0		
#define Y1		120		
#define X2		442		
#define Y2		312

/* play sample */		
#define A1		255		
#define A2		128		
#define A3		1000		
#define A4		0

struct timespec	count;			/* represents the count down timer time */
struct timespec t;			        /* represents the starting time */
struct timespec	now;			/* represents the current time */

/* bitmaps used of the program */
BITMAP *backgrd_bmp, *welcome_bmp, *target_bmp, *buffer, *gameover_bmp; 

SAMPLE *shoot;				/* play the sound for shoting and empty gun*/

int displayed_min = 25;			/* the time for a game to be finished */
int start_game = 0;			        /* holds 0 if game didnt start and 1 if it does start */
int is_game_over = 0;			/* holds 1 if the game is over, 0 otherwise */

int score = 0;				            /* holds the score of the game */  
int bullets = 10;			            /* holds the bullets remaining */
int reload_limit = 5;			        /* counts how many times a gun is reloaded */

int no_pigs_alive = 5;			    /* counts the number of pigs alive */
int count_reborn = 0;			    /* holds the time to reborn the pigs */


pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;	/* definition and direct initialization of a mutex semaphore */


typedef struct _pig_
{
  int x,y;				            /* x and y position of the pig */
  int id;				                /* pig id */
  BITMAP *pic;				/* pig bitmap*/
} pig_;

pig_ pig[NO_OF_PIGS];


typedef struct _pigLife_
{
  int state;				        /* state of the pig, 1 alive and 0 dead */
  int dir;				            /* direction of the pig */
  double speed;				/* speed of each pig */
} pigLife_;

pigLife_ pigLife[NO_OF_PIGS];


/* function prototype definitions */
void graphics_Init();
void background_screen();
void welcome_screen();

void assign_positions();
void load_pigs_bitmaps();
void create_threads();
void update_screen();
void update_pig_position(int i, double speed);

void speed();
void set_life();
void reload_gun();
void set_new_game();
void isgameover();
void reborn_pig();

void *target_img_moving();
void *countdwn();
void *movePig();
void *shootpig();

void time_copy(struct timespec *td, struct timespec ts);
void time_add_ms(struct timespec *t, int ms);
int  time_cmp(struct timespec t1, struct timespec t2);
  
 
