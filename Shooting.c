#include "Shooting.h"

/* real time parameters for each thread */
struct task_par {
	int arg;				/* task argument */
	long wcet;				/* in microseconds */
	int period;				/* in milliseconds */
	int deadline;				/* relative (ms) */
	int priority;				/* in [0,99] */
	int dmiss;				/* no. of misses */
	struct timespec at;			/* next activ. time */
	struct timespec dl;			/* abs deadline */
};

/************************************************************************
 * Main function				            	  	*
 * It calls all the neccessary functions to setup the game, destroys 	* 
 * bitmaps, shuts down allegro, terminate pthread			*
 * and exit				                    	  	*
 **********************************************************************/
int main(){

	graphics_Init();			/* calls functions that initialize the graphics */	
	welcome_screen(); 			/* loads the first screen with the game instructions */
	set_life();			        /* set the pigs life to 1 which means alive */
	assign_positions();			/* assign the initial postion of the pigs */
  	speed();				/* set the speed of each pig */
	
 	srand (time(NULL));			/* Seed the random number generator */

	while(!key[KEY_ESC] && start_game == 1)			/* Loop until the user presses Esc */
	{	
	 	/* setting the current time for the cuntdown thread */
  		clock_gettime(CLOCK_MONOTONIC, &t);	
  		time_add_ms(&t, DELAY);	

		buffer = create_bitmap(XMAX, YMAX);

		background_screen();		/* load the background screen */
		load_pigs_bitmaps();		/* load the pigs bitmap on the screen */
		create_threads();		/* create all the necessary threads of  the game */
		isgameover();			/* check if there is still time to play */
		update_screen();  		/* update the screen */
		reload_gun();			/* reload gun when R is pressed */
		set_new_game(); 		/* Set a new game when N is pressed */	
		reborn_pig();			/* generate new pigeons */

		/* destroying bitmaps */
 		destroy_bitmap(backgrd_bmp);
		destroy_bitmap(target_bmp);
		destroy_bitmap(buffer);

		if(is_game_over == 1) destroy_bitmap(gameover_bmp);

		int i;
  		for(i = 0; i <= NO_OF_PIGS; i++) destroy_bitmap(pig[i].pic);	
	}	
	destroy_sample(shoot);
	allegro_exit();				/* shuts down Allegro */
	pthread_exit(NULL);			/* shuts down pthread */
	exit(0);
}//end main


/* initialize all neccessary allegro libraries for the game */
void graphics_Init(){

	allegro_init();							/* allegro initialization */
	install_keyboard();						/* keyboard initialization */	
	install_mouse();						/* mouse initialization */
	set_color_depth(16);						/* VGA mode (16 bits) */	
	set_gfx_mode(GFX_SAFE, XMAX, YMAX, 0, 0);
	set_window_title("SHOOTING PEGEONS");	
	install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL);		/* sound initialization */
}


void welcome_screen(){

	/* Loads the the first screen of the game */
	welcome_bmp = load_bitmap("welcome.pcx", NULL);
	blit(welcome_bmp , screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

	if(welcome_bmp == NULL)
	{
		allegro_message("could not load the wizard image");
		allegro_exit();
	}

	/************************************************************************
 	* waits for any key to be pressed				       	*
 	* case ESC, exit the game						* 
 	* case ENTER,loads the game						*
 	* Do nothing otherwise          		          	  	*
 	**********************************************************************/	
	readkey();	

	if(key[KEY_ESC])
	{
		destroy_bitmap(welcome_bmp);
		allegro_exit();		
		exit(0);
	}

	if(key[KEY_ENTER]) start_game = 1;  	/* The game is ready to be started */	
}


void background_screen(){

	/* Loads the the background of the game */
	backgrd_bmp = load_bitmap("background.bmp", NULL);
	 
	if(backgrd_bmp == NULL)
	{
		allegro_message("could not load the wizard image");
		allegro_exit();
	}
}


/* move the target around the screen */
void *target_img_moving(){

	struct timespec t, now;
	int period = 20;  			/* period in millisecond */

	clock_gettime(CLOCK_MONOTONIC, &t);
	time_add_ms(&t, period);

	while(time_cmp(now, t) < 0)
	{
	 	/* Load target image from the current directory */
		target_bmp  = load_bitmap("target_saved.pcx",NULL);

		/* define mouse range according to coordinates (int x1, int y1, int x2, int y2) */
		set_mouse_range(X1, Y1, X2, Y2);

		if(target_bmp == NULL)
		{
			allegro_message("could not load the wizard image");
			allegro_exit();
		}

		pthread_mutex_lock(&mux);

		draw_sprite(buffer, target_bmp, mouse_x, mouse_y);

		pthread_mutex_unlock(&mux);

		clock_gettime(CLOCK_MONOTONIC, &now);
	 	time_add_ms(&now, period);	 	
	 	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &now, NULL);
	}
}


void *shootpig(){

	struct timespec t, now;
	int period = 20;  			/* period in millisecond */

	clock_gettime(CLOCK_MONOTONIC, &t);
	time_add_ms(&t, period);

	while(time_cmp(now, t) < 0)
	{

	  pthread_mutex_lock(&mux);

	  /* if the left button of the mouse is pressed and there is time to play */
	  if(mouse_b & 1 && !(displayed_min == 0))
	  {
		/* if there are enough bullets */
		if(bullets > 0)
		{
			int target_center = 60;
			int pig_center = 10;
			int error_rate = 25;

			shoot = load_sample("shoot.wav");		/* load the sound file */
			play_sample(shoot, A1, A2, A3, A4);

			int target_y = mouse_y + target_center;    	/* goes to the y center of the target bitmap */
			int target_x = mouse_x + target_center;    	/* goes to the x center of the target bitmap */

			int i, x_range, y_range;			

	 		for(i = 0; i <= NO_OF_PIGS; i++)
	  		{    
				int pig_x = pig[i].x + pig_center;	/* goes to the x center of the pig */
				int pig_y = pig[i].y + pig_center;	/* goes to the y center of the pig */

				x_range = pig_x - target_x;
				if(x_range < 0) x_range = target_x - pig_x;

				y_range = pig_y - target_y;
				if(y_range < 0) y_range = target_y - pig_y;
				
				/************************************************************************
			 	* if the pig is alive and the distance from the target center	       	*
			 	* to the pig x and y coordinates is not greater than 25 then 		* 
			 	* the pig was shoot successfully.					*
			 	************************************************************************/
		   		if (x_range <= error_rate && y_range <= error_rate && pigLife[i].state == PIG_ALIVE)    
				{
					pigLife[i].state = PID_DEAD;	/* set the pig state to dead */			    
					score += SCORE_HIT;		/* update the score */	
					no_pigs_alive--;		/* decrease the number of pigs */
				}
			}
			
			bullets --;			/* decrease the number of bullets available */

		} else {	/* empty gun */
				shoot = load_sample("emtygun.wav");	
				play_sample(shoot, A1, A2, A3, A4);
 			}
	    }

	   pthread_mutex_unlock(&mux);

	   clock_gettime(CLOCK_MONOTONIC, &now);
	   time_add_ms(&now, period);	 	
	   clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &now, NULL);				
	 }
}


/* Set the initial position of the pig based on a random number */
void assign_positions(){

 	int i;
 	for(i = 0; i <= NO_OF_PIGS; i++)
  	{   
	    	pig[i].y = (rand() % (Y_MAX - Y_MIN) + Y_MIN);  	/* y coordinates */		
   		pig[i].x = (rand() % (X_MAX - X_MIN ) + X_MIN);		/* x coordinates */
		pigLife[i].dir = 0; 					/* initial direction */
  	} 
}


/* load pigs bitmaps into the screen */
void load_pigs_bitmaps(){

	pig[0].pic = load_bitmap("pig1.bmp",NULL);
 	pig[1].pic = load_bitmap("pig2.bmp",NULL);
 	pig[2].pic = load_bitmap("pig3.bmp",NULL);
 	pig[3].pic = load_bitmap("pig3.bmp",NULL);
 	pig[4].pic = load_bitmap("pig1.bmp",NULL);
 	pig[5].pic = load_bitmap("pig2.bmp",NULL);
}


/* set all pigs state to alive */
void set_life(){

	int i;
 	for(i = 0; i <= NO_OF_PIGS; i++)  pigLife[i].state = PIG_ALIVE;	
}


void *countdwn(){

	while(time_cmp(count, t) < 0)
	{		
		/* checks if the countdown time reaches the next second */
		clock_gettime(CLOCK_MONOTONIC, &count);
	 	time_add_ms(&count, PERIOD_TO_COUNT);	 	
	 	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &count, NULL);

		pthread_mutex_lock(&mux);

	 	if(displayed_min == 0); 	
		else
		{	
			displayed_min --;				
		}	

		pthread_mutex_unlock(&mux); 	
	}
}


/* draw each pig moving on the screen */
void *movePig(){ 
	
	struct timespec t, now;
	int period = 20;  			/* period in millisecond */

	clock_gettime(CLOCK_MONOTONIC, &t);
	time_add_ms(&t, period);

	while(time_cmp(now, t) < 0)
	{
  		draw_sprite(buffer, backgrd_bmp, 0, 0);
		
		int i; 
  		for(i = 0; i <= NO_OF_PIGS; i++)  
		{
			pthread_mutex_lock(&mux);

			update_pig_position(i, pigLife[i].speed);     /* update each pig position */

			/* draw the pig on the screen if its alive */
			if(pigLife[i].state == PIG_ALIVE)				
			draw_sprite(buffer, pig[i].pic, pig[i].x, pig[i].y); 

			pthread_mutex_unlock(&mux);
		}

      		clock_gettime(CLOCK_MONOTONIC, &now);
	 	time_add_ms(&now, period);	 	
	 	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &now, NULL);
	}
}// end movePig function.


void update_screen(){

	/* display the countdown on the screen */
	textprintf_ex(buffer,  font, X_CDOWN, Y_CDOWN, makecol(0,0,0), -1, "%d ", displayed_min);

	/* display the score on the screen */
	textprintf_ex(buffer,  font, X_SCORE, Y_SCORE, makecol(0,0,0), -1, "%d ", score);	

	/* display the bullets available */
	textprintf_ex(buffer,  font, X_BULLET, Y_BULLET, makecol(0,0,0), -1, "%d ", bullets);

	/* display how many times the player can reload the gun */
	textprintf_ex(buffer,  font, X_RELOAD, Y_RELOAD, makecol(0,0,0), -1, "%d ", reload_limit);
	
	acquire_screen();

	/* display the entire screen */
  	blit(buffer,screen,0,0,0,0,XMAX,YMAX);  
	
	release_screen();	
}


/* creates all the threads */
void create_threads(){

  	pthread_t 		target_id;		/* target thread */
	pthread_t 		shoot_id;		/* shoot thread */
  	pthread_t 		tid[NO_OF_PIGS];	/* thread id */
  	pthread_attr_t 		att[NO_OF_PIGS];	/* attribute stucture */	
	pthread_t 		cntd;			/* countdown thread */

  	struct sched_param 	mypar;			/* priority structure */
	struct task_par		tp[NO_OF_PIGS];

  	int i, pig_created, target, cd, shoot;

 	/* creating pigs threads */
 	for(i = 0; i < NO_OF_PIGS; i++)
	{ 
       		pig[i].id = i;

		tp[i].arg = i;
		tp[i].period = 100;	
		tp[i].deadline = 80;
		tp[i].priority = 20;
		tp[i].dmiss = 0;

    		pthread_attr_init(&att[i]);					/* thread attribute initialization */
    		pthread_attr_setinheritsched(&att[i], PTHREAD_EXPLICIT_SCHED);	/* to use different policies */
    		pthread_attr_setschedpolicy(&att[i], SCHED_FIFO);		/* threads with same priority are managed by FIFO */
    		mypar.sched_priority = tp[i].priority;
    		pthread_attr_setschedparam(&att[i], &mypar);			/* specifing  the thread priority */

   		pig_created = pthread_create(&tid[i], &att[i], movePig, &tp[i]);/* pig thread */
  	}

	shoot = pthread_create(&shoot_id, NULL, shootpig, NULL); 		/* shoot thread */

   	target = pthread_create(&target_id, NULL, target_img_moving, NULL); 	/* target thread */
   
   	cd = pthread_create(&cntd, NULL, countdwn, NULL);			/* countdown thread */
	 
 	/* joining the threads */
	pthread_join(shoot_id, NULL);  

	pthread_join(target_id, NULL);   

 	for(i = 0; i < NO_OF_PIGS; i++) pthread_join(tid[i], NULL);	    
}


/* checks if it is the end  of the game */
void isgameover()
{
	if(displayed_min == 0) 	
	{	
		gameover_bmp  = load_bitmap("game_over.bmp",NULL);
		draw_sprite(buffer, gameover_bmp, 0, 120);
		is_game_over = 1;			
	}
	else is_game_over = 0;	
}


/* Set a new game if N is pressed */
void set_new_game()
{
		if(key[KEY_N]) 			
		{	
			
			/* reset the vars to the initial assumptions */
			displayed_min = INIT_MIN;
			score = INIT_SCORE;
			bullets = RELOAD_GUN;
			reload_limit = RELOAD_TIME;
			count_reborn = 0;
			no_pigs_alive = NO_OF_PIGS;
			
			set_life();
		}
}


/************************************************************************
* Reload gun when R is pressed       					*
* The gun can be reloaded only if there is no more bullets 		* 
* The gun can be reloaded only 5 times					*	 
************************************************************************/
void reload_gun()
{
	if(key[KEY_R] && bullets == 0 && reload_limit > 0) 	
	{	

		shoot = load_sample("reload.wav");	
		play_sample(shoot, A1, A2, A3, A4);
		bullets = RELOAD_GUN;	
		reload_limit--;	

	}		
}


/************************************************************************
* If all pigs a successful shot, wait few seconds then			*
* Reborn all the pigs for another try 					* 
* Give two more possibilities of reloading the gun			*	 
************************************************************************/
void reborn_pig()
{	
	int time_to_reborn = 20;
	int reload = 2;
	int all_pigs_dead = -1;

	if(no_pigs_alive == all_pigs_dead) 	/* check if all the pigs are dead */
	{	

		count_reborn++;	

		if (count_reborn == time_to_reborn)
		{
			count_reborn = 0;
			no_pigs_alive = NO_OF_PIGS;
			set_life();
			reload_limit +=reload;			
		}
		
	}		
}


/* fly motion rules */
void update_pig_position(int i, double speed)
{
	switch(pigLife[i].dir){

	 //Direction is northwest.
   	 case NORTHWEST: if((pig[i].x <= RAD) || (pig[i].y <= RAD + Y_MIN)){
         pigLife[i].dir = rand() % RANDOM;
         }else{
        	pig[i].x -= speed * T;
        	pig[i].y -= speed * T;
	 }break;

	 //Direction is southwest.
         case SOUTHWEST: if(((pig[i].x <= RAD) || (pig[i].y >= (Y_MAX - RAD)))){
         pigLife[i].dir = rand() % RANDOM;
         }else{
        	pig[i].x -= speed * T;
        	pig[i].y += speed * T;
        }break;

	 //Direction is northeast.
    	 case NORTHEAST: if(((pig[i].x >= (X_MAX - RAD)) || (pig[i].y <= RAD + Y_MIN))){
         pigLife[i].dir = rand() % RANDOM;
      	 }else{
       		pig[i].x += speed * T;
        	pig[i].y -= speed * T;
	}break;

	 //Direction is southeast
   	 case SOUTHEAST: if((((pig[i].x >= (X_MAX - RAD)) || (pig[i].y >= (Y_MAX - RAD))))){
         pigLife[i].dir = rand() % RANDOM;
      	 }else{
        	pig[i].x += speed * T;
		pig[i].y += speed * T;	
         }break;
    }
  }


/* define speed */
void speed()
{
	pigLife[0].speed = SPEED1;
 	pigLife[1].speed = SPEED2;
 	pigLife[2].speed = SPEED3;
 	pigLife[3].speed = SPEED3;
 	pigLife[4].speed = SPEED1;
 	pigLife[5].speed = SPEED2;	
}


/* copies a source time variable ts in a destination variable pointed by td */
void time_copy(struct timespec *td, struct timespec ts){

	td->tv_sec = ts.tv_sec;
	td->tv_nsec = ts.tv_nsec;
}
		
						                  	  					   
/* adds a value ms expressed in milliseconds to the time variable pointed by t */
void time_add_ms(struct timespec *t, int ms){

	t->tv_sec += ms/1000;
	t->tv_nsec += (ms%1000)*1000000;

	if (t->tv_nsec > 1000000000)
	{
		t->tv_nsec -= 1000000000;
		t->tv_sec += 1;
	}
}


 /* compares two time variables t1 and t2  */
int time_cmp(struct timespec t1, struct timespec t2){

	if (t1.tv_sec > t2.tv_sec) return 1;
	if (t1.tv_sec < t2.tv_sec) return -1;
	if (t1.tv_nsec > t2.tv_nsec) return 1;
	if (t1.tv_nsec < t2.tv_nsec) return -1;
	return 0;
}
