//Ammar Bin Yasir and Saad Amir
//20I-0501 and 20I-0650
//Section G

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <GL/glut.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#define BOARD_WIDTH 20
#define BOARD_HEIGHT 20
#define NUM_GHOSTS 4
#define DOT_RADIUS 0.1f
#define GHOST_SPEED 0.05f 
#define POWER_PELLET_PERCENTAGE 5
#define VULNERABLE_DURATION 5 
#define GHOST_CONTROLLER_SLEEP_DURATION 200000
#define FAST_GHOST_CONTROLLER_SLEEP_DURATION 100000
#define GHOST_HOUSE_X (BOARD_WIDTH - 2) / 2
#define GHOST_HOUSE_Y (BOARD_HEIGHT - 1) /2
#define NUM_KEYS 4
#define NUM_EXIT_PERMITS 4


	typedef enum {
	    GHOST_STATE_NORMAL,
	    GHOST_STATE_VULNERABLE,
	    GHOST_STATE_INSIDE_HOUSE
	} GhostState;

	typedef struct {
	    int x;
	    int y;
	    bool isVulnerable;
	    GhostState state;
	    float color[3];
	    float originalColor[3];
	    bool hasSpeedBoost;
	} Ghost;

	typedef struct {
	    int x;
	    int y;
	    int lives;
	} Pacman;

	typedef struct {
	    int x;
	    int y;
	    bool isPowerPellet;
	    bool eaten;
	} Pellet;


	pthread_mutex_t vulnerable_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t ghosts_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t board_mutex = PTHREAD_MUTEX_INITIALIZER;
	int argc;
	char** argv;
	pthread_t engine_thread, ui_thread, ghost_threads[NUM_GHOSTS]; 
	bool vulnerable_timer_expired = false;
	time_t ghost_eaten_time[NUM_GHOSTS];
	bool gameStarted = false;
	int pelletCount = 0;
	Pellet pellets[BOARD_HEIGHT * BOARD_WIDTH];
	int score = 0;
	int lives = 3;
	int keys = NUM_KEYS;
	int exitPermits = NUM_EXIT_PERMITS;
	float squareX = BOARD_WIDTH / 2.0f;
	float squareY = BOARD_HEIGHT / 2.0f;
	Ghost ghosts[NUM_GHOSTS];
	bool gamePaused = false;


	char board[BOARD_HEIGHT][BOARD_WIDTH] = {
	    "####################",//bottom
	    "#..................#",
	    "#.####.####.#.####.#",
	    "#.#  #.#  #.#.#  #.#",
	    "#.####.####.######.#",
	    "#..................#",
	    "#.####.##.######.#.#",
	    "#.........######.#.#",
	    "#.................##",
	    "#..................#",
	    "#............#..#..#",
	    "#.##########.#####.#",
	    "#.##########.#####.#",
	    "#..#...............#",
	    "#.####.####.####.#.#",
	    "#.####.####.####.#.#",
	    "#..#........#......#",
	    "#..###.....###.....#",
	    "#..................#",
	    "####################"//top
	};

	void* ghost_controller(void* arg);
	void* ui_thread_function(void* arg);
	void* game_engine(void* arg);
	void findShortestPath(int srcX, int srcY, int destX, int destY, int* pathX, int* pathY);
	void handlePowerPelletEffects();
	float colorSimilarity(float color1[3], float color2[3]);
	void drawLives();
	bool checkCollision(Pacman* pacman, Ghost* ghosts);
	void generateRandomPosition(int *x, int *y);
	void moveGhosts();
	void updateGhosts(int value);
	void checkCollisionAndUpdateScore(Pacman* pacman, Pellet* pellets, int pelletCount, int* score);
	void specialKeyboard(int key, int xx, int yy);
	void initOpenGL();
	void display();
	void menu(int value);
	void startGame();

	int main(int argc, char** argv);

	//=========================================================================================================
	//=========================================================================================================


		
		void drawLives() {
		    glColor3f(1.0f, 0.0f, 0.0f);

		    
		    float diamondSize = 0.5f;
		    float startX = BOARD_WIDTH - 2.5f + 1.0f;
		    float startY = BOARD_HEIGHT - 0.5f;
		    float gap = 2.0f; 

		    
		    for (int i = 0; i < lives; i++) {
			float diamondX = startX - i * gap;
			float diamondY = startY;

			glBegin(GL_TRIANGLES);
			glVertex2f(diamondX, diamondY + diamondSize); 
			glVertex2f(diamondX + diamondSize / 2, diamondY); 
			glVertex2f(diamondX + diamondSize, diamondY + diamondSize); 
			glEnd();

			glBegin(GL_TRIANGLES);
			glVertex2f(diamondX, diamondY + diamondSize); 
			glVertex2f(diamondX - diamondSize / 2, diamondY); 
			glVertex2f(diamondX + diamondSize / 2, diamondY); 
			glEnd();
		    }
		}


		bool checkCollision(Pacman* pacman, Ghost* ghosts);
		void handlePowerPelletEffects();

		void findShortestPath(int srcX, int srcY, int destX, int destY, int* pathX, int* pathY) 
		{
		    int queueX[BOARD_WIDTH * BOARD_HEIGHT];
		    int queueY[BOARD_WIDTH * BOARD_HEIGHT];
		    int front = 0, rear = 0;

		    
		    bool visited[BOARD_HEIGHT][BOARD_WIDTH];
		    memset(visited, false, sizeof(visited));

		    
		    int parentX[BOARD_HEIGHT][BOARD_WIDTH];
		    int parentY[BOARD_HEIGHT][BOARD_WIDTH];

		    
		    int dx[] = {-1, 0, 1, 0};
		    int dy[] = {0, 1, 0, -1};

		    
		    queueX[rear] = srcX;
		    queueY[rear++] = srcY;
		    visited[srcY][srcX] = true;

		    
		    while (front != rear) 
		    {
			int cx = queueX[front];
			int cy = queueY[front++];
			if (cx == destX && cy == destY) 
			{
			    int x = destX;
			    int y = destY;
			    int idx = 0;
			    while (x != srcX || y != srcY) 
			    {
				pathX[idx] = x;
				pathY[idx++] = y;
				int px = parentX[y][x];
				int py = parentY[y][x];
				x = px;
				y = py;
			    }

			    pathX[idx] = srcX;
			    pathY[idx] = srcY;
			    return;
			}

			for (int i = 0; i < 4; i++) 
			{
			    int nx = cx + dx[i];
			    int ny = cy + dy[i];
			    if (nx >= 0 && nx < BOARD_WIDTH && ny >= 0 && ny < BOARD_HEIGHT &&
				!visited[ny][nx] && board[ny][nx] != '#') 
				{
				queueX[rear] = nx;
				queueY[rear++] = ny;
				visited[ny][nx] = true;
				parentX[ny][nx] = cx;
				parentY[ny][nx] = cy;
			    }
			}
		    }
		}



pthread_cond_t thread_complete_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t thread_complete_mutex = PTHREAD_MUTEX_INITIALIZER;
bool threads_complete = false;


void signal_threads_complete() 
{
    pthread_mutex_lock(&thread_complete_mutex);
    threads_complete = true;
    pthread_cond_signal(&thread_complete_cond);
    pthread_mutex_unlock(&thread_complete_mutex);
}




void* ui_thread_function(void* arg) 
{
    pthread_mutex_lock(&thread_complete_mutex);
    while (!threads_complete) 
    {
        pthread_cond_wait(&thread_complete_cond, &thread_complete_mutex);
    }

    pthread_mutex_unlock(&thread_complete_mutex);

    return NULL;
}


void* game_engine(void* arg) 
{
    signal_threads_complete();

    return NULL;
}
		
bool leave_ghost_house(Ghost* ghost, int *ghostIndex, int* keys, int* exitPermits)  // will update keys and permits counts, manage ghost house
{
		    if (ghost->state == GHOST_STATE_INSIDE_HOUSE && *keys > 0 && *exitPermits > 0) 
		    {
			
			(*keys)--;
			(*exitPermits)--;

			ghost->x = (BOARD_WIDTH - 2) / 2;
			ghost->y = (BOARD_HEIGHT - 1) / 2 ;
			ghost->state = GHOST_STATE_NORMAL;
			printf("Ghost with index: %d have key/permit and is ready to leave.\n\n", *ghostIndex);
			return true;
		    }

		    else
		    {
		   
		     return false;
		    }
		    
		    
		}

		void* ghost_controller(void* arg)  // overall manages ghost movements, attacks, vulnerability
		{
			int ghost_index = *((int*)arg); 
			
			bool fast = false; 
			bool permit = false;
		    
		    Ghost* ghost = &ghosts[ghost_index]; 
		    srand(time(NULL)); 
		    int sleep_duration = GHOST_CONTROLLER_SLEEP_DURATION;
		    time_t startTime = 0; 

		    while (true) 
		    {
			    pthread_mutex_lock(&ghosts_mutex);
			
			if (!gamePaused) 
			{
			
				
			    if (ghost->x == -1 && time(NULL) - ghost_eaten_time[ghost_index] >= 3)  // if ghost is eaten and needed to restore
			    {
				ghost->x = (BOARD_WIDTH - 2) / 2; 
				ghost->y = (BOARD_HEIGHT - 1) / 2 ; 
			    }

			    if (ghost->state == GHOST_STATE_INSIDE_HOUSE) 
			    {
				    permit = leave_ghost_house(ghost, &ghost_index , &keys, &exitPermits);
				}
			    
			    
			
			    if (ghosts->isVulnerable) // when pacman eats powerpellets and ghosts become vulnerable
			    {  
					// first finding shortest path towards pacman and moving in same opposite direction
					    int pathX[BOARD_WIDTH * BOARD_HEIGHT];
					    int pathY[BOARD_WIDTH * BOARD_HEIGHT];
					    findShortestPath(ghost->x, ghost->y, squareX, squareY, pathX, pathY);

					    
					    int lastX = pathX[1]; 
					    int lastY = pathY[1];

					  
					    int dx = ghost->x - lastX;
					    int dy = ghost->y - lastY;

					    int nextX = ghost->x;
					    int nextY = ghost->y;

					    
					    if (abs(dx) > abs(dy)) 
					    {
						nextX = ghost->x + (dx > 0 ? 1 : -1);
					    } 

					    else 
					    {
						nextY = ghost->y + (dy > 0 ? 1 : -1);
					    }

					    
					    if (nextX >= 0 && nextX < BOARD_WIDTH && nextY >= 0 && nextY < BOARD_HEIGHT && board[nextY][nextX] != '#') 
					    {
						ghost->x = nextX;
						ghost->y = nextY;
					    }
					
					
					
					if (startTime == 0) 
					{
						startTime = time(NULL); 
					} 

					else 
					{
						time_t currentTime = time(NULL);
						if ((currentTime - startTime) >= VULNERABLE_DURATION) 
						{
						    ghost->isVulnerable = false;
						    memcpy(ghost->color, ghost->originalColor, sizeof(float) * 3);
						    startTime = 0;
						}
					}
				
			    } 

			    else   // when ghosts are active and eager to attack pacman
			    {
				    int pathX[BOARD_WIDTH * BOARD_HEIGHT];
				    int pathY[BOARD_WIDTH * BOARD_HEIGHT];
				    findShortestPath(ghost->x, ghost->y, squareX, squareY, pathX, pathY);

				   
				    int randomDirection = rand() % 4; 
				    int nextX = ghost->x;
				    int nextY = ghost->y;

				    switch (randomDirection) 
				    {
					case 0: 
					    nextX = ghost->x - 1;
					    break;
					case 1: 
					    nextX = ghost->x + 1;
					    break;
					case 2: 
					    nextY = ghost->y + 1;
					    break;
					case 3: 
					    nextY = ghost->y - 1;
					    break;
				    }
				    
				    if (ghost->hasSpeedBoost) 
				    {

					    fast= true;

					} 

					else 
					{

					}

				    if (nextX >= 0 && nextX < BOARD_WIDTH && nextY >= 0 && nextY < BOARD_HEIGHT && board[nextY][nextX] != '#') 
				    {
					ghost->x = nextX;
					ghost->y = nextY;
				    }
				    
				    if(fast == true)  // managin speed based on the selected boosted ghosts
				    {
					usleep(FAST_GHOST_CONTROLLER_SLEEP_DURATION);
					}

					else 
					{
					usleep(GHOST_CONTROLLER_SLEEP_DURATION);
					}
			    }
				
					    
			}

			pthread_mutex_unlock(&ghosts_mutex);
			usleep(200000); 
		    }

		    return NULL;
		  }

		void display() 
		{
		    glClear(GL_COLOR_BUFFER_BIT);
		    glLoadIdentity();

		    glViewport(0, 0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));

		    glMatrixMode(GL_PROJECTION);
		    glLoadIdentity();
		    gluOrtho2D(0, BOARD_WIDTH, 0, BOARD_HEIGHT);

		    glMatrixMode(GL_MODELVIEW);
		    glLoadIdentity();

		    
		    for (int i = 0; i < BOARD_HEIGHT; i++)   // displaying board
		    {
			for (int j = 0; j < BOARD_WIDTH; j++) 
			{
			    if (board[i][j] == '#') 
			    {
				glColor3f(0.0f, 0.0f, 0.5f);  
			    } 

			    else 
			    {
				glColor3f(1.0f, 1.0f, 1.0f); 

				glBegin(GL_POINTS);
				for (float x = j + 0.5f; x < j + 1.0f; x += 0.1f) 
				{
				    for (float y = i + 0.5f; y < i + 1.0f; y += 0.1f) 
				    {
				        if (x >= 0 && x < BOARD_WIDTH && y >= 0 && y < BOARD_HEIGHT && board[(int)y][(int)x] != '#') 
				        {
				            glColor3f(0.0f, 0.0f, 0.0f); 
				            glVertex2f(x, y);
				        }
				    }
				}
				glEnd();
			    }
			    glBegin(GL_QUADS);
			    glVertex2f(j, i);
			    glVertex2f(j + 1, i);
			    glVertex2f(j + 1, i + 1);
			    glVertex2f(j, i + 1);
			    glEnd();
			}
		    }
		    
			for (int i = 0; i < pelletCount; i++)  // displaying pellets on the board
			{
			    if (!pellets[i].eaten) 
			    {
				if (pellets[i].isPowerPellet) 
				{
				    glPointSize(5.0f); 
				    glColor3f(1.0f, 1.0f, 1.0f); 
				} 

				else 
				{
				    glPointSize(1.0f); 
				    glColor3f(1.0f, 1.0f, 1.0f); 
				}
				
				glBegin(GL_POINTS);
				glVertex2f(pellets[i].x + 0.5f, pellets[i].y + 0.5f); // Center of the pellet
				glEnd();
			    }
			}


		    
		    glColor3f(1.0f, 1.0f, 0.0f); 
		    glBegin(GL_TRIANGLE_FAN);
		    float centerX = squareX + 0.5f;
		    float centerY = squareY + 0.5f;
		    glVertex2f(centerX, centerY); 

		    for (int k = 0; k <= 360; k += 10) 
		    {
			float angle = k * M_PI / 180.0f;
			float x = centerX + 0.3f * cos(angle); 
			float y = centerY + 0.3f * sin(angle);
			glVertex2f(x, y);
		    }

		    glEnd();

		    
		    int boxWidth = 2; 
		    int boxHeight = 1; 

		    int boxStartX = (BOARD_WIDTH - boxWidth) / 2;
		    int boxEndX = boxStartX + boxWidth;
		    int boxStartY = (BOARD_HEIGHT - boxHeight) / 2;
		    int boxEndY = boxStartY + boxHeight;

		    glColor3f(1.0f, 1.0f, 1.0f); 

		    
		    glBegin(GL_LINES);
		    glVertex2f(boxStartX, boxStartY);
		    glVertex2f(boxEndX, boxStartY);
		    glEnd();

		    
		    glBegin(GL_LINES);
		    glVertex2f(boxStartX, boxStartY);
		    glVertex2f(boxStartX, boxEndY);
		    glEnd();

		    
		    glBegin(GL_LINES);
		    glVertex2f(boxEndX, boxStartY);
		    glVertex2f(boxEndX, boxEndY);
		    glEnd();

		    
		    for (int i = 0; i < NUM_GHOSTS; i++) 
		    {
			glColor3fv(ghosts[i].color); 
			glBegin(GL_TRIANGLE_FAN);
			float radius = 0.25f; 
			float centerX = ghosts[i].x + 0.5f;
			float centerY = ghosts[i].y + 0.5f;
			glVertex2f(centerX, centerY); 

			for (int angle = 0; angle <= 360; angle += 10) 
			{
			    float x = centerX + radius * cos(angle * M_PI / 180);
			    float y = centerY + radius * sin(angle * M_PI / 180);
			    glVertex2f(x, y);
			}
			glEnd();
		    }
		  
		    drawLives();
		    
		
		    glColor3f(1.0f, 1.0f, 1.0f); 
		    
		    
		    float scoreX = 1.0f; 
		    float scoreY = BOARD_HEIGHT - 0.8f; 

		    glRasterPos2f(scoreX, scoreY); 
		    
		    char scoreText[20];
		    sprintf(scoreText, "Score: %d", score); 
		    int length = strlen(scoreText);
		    for (int i = 0; i < length; i++) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, scoreText[i]); 
		    }
		    glutSwapBuffers();

		}

		bool checkCollision(Pacman* pacman, Ghost* ghosts)  // check collision btw ghosts and pacman
		{
		    for (int i = 0; i < NUM_GHOSTS; i++) 
		    {
			if (pacman->x == ghosts[i].x && pacman->y == ghosts[i].y)  // if both are on same location
			{
			    return true; 
			}
		    }
		    return false; 
		}

		void moveGhosts() 
		{
		    
		    for (int i = 0; i < NUM_GHOSTS; i++) 
		    {
			int dx = squareX - ghosts[i].x;
			int dy = squareY - ghosts[i].y;
			int adx = abs(dx);
			int ady = abs(dy);

			
			int nextX = ghosts[i].x;
			int nextY = ghosts[i].y;
			if (adx > ady) 
			{
			    int dir = (dx > 0) ? 1 : -1;
			    nextX += dir;
			} 

			else 
			{
			    int dir = (dy > 0) ? 1 : -1;
			    nextY += dir;
			}

			if (nextX >= 0 && nextX < BOARD_WIDTH && nextY >= 0 && nextY < BOARD_HEIGHT && board[nextY][nextX] != '#') 
			{
			    ghosts[i].x = nextX;
			    ghosts[i].y = nextY;
			}

		    }
		}

		
		void updateGhosts(int value) 
		{
		    moveGhosts();
		    glutPostRedisplay();
		    glutTimerFunc(1000, updateGhosts, 0); 
		}

		void checkCollisionAndUpdateScore(Pacman* pacman, Pellet* pellets, int pelletCount, int* score);


		void specialKeyboard(int key, int xx, int yy) 
		{
		    int newX = squareX;
		    int newY = squareY;

		    
		    Pacman pacman;
		    pacman.x = squareX;
		    pacman.y = squareY;

		    switch (key)  // move pacman based on user input
		    {
			case GLUT_KEY_RIGHT: 
			    newX = squareX + 1.0f;
			    break;
			case GLUT_KEY_LEFT: 
			    newX = squareX - 1.0f;
			    break;
			case GLUT_KEY_UP: 
			    newY = squareY + 1.0f;
			    break;
			case GLUT_KEY_DOWN: 
			    newY = squareY - 1.0f;
			    break;
		    }

		   
		    if (newX >= 0 && newX < BOARD_WIDTH && newY >= 0 && newY < BOARD_HEIGHT && board[newY][newX] != '#') 
		    {
			squareX = newX;
			squareY = newY;
			
			checkCollisionAndUpdateScore(&pacman, pellets, pelletCount, &score);
		    }

		    glutPostRedisplay();

		   
		    if (checkCollision(&pacman, ghosts))  // check if new location has ghosts, if yes its collision and live decreased
		    {
		    	lives--;
		    	if (lives <= 0) 
		    	{
			    printf("Collision detected!\n\n * Game Over! You ran out of lives. *\n");
			    sleep(3);
	    
			    printf("\n Do you want to restart the game?(Y/N) : ");
			    char opt;
			    scanf("%c", &opt);
			    if(opt == 'y' || opt == 'Y')
			    {
			    	startGame();
			    }
			    else 
			    {
			    	printf("\n\n Thank you for your time! \n"); 
			    	exit(0);
			    }
			    
			}
			
			printf("Collision detected! Game paused for 5 seconds.\n");
			gamePaused = true; 
			glutPostRedisplay(); 
			sleep(5);
			printf("Game resumed.\n");
			gamePaused = false; 
		    }
		}



		void initOpenGL() 
		{
		    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		    glMatrixMode(GL_PROJECTION);
		    glLoadIdentity();
		    gluOrtho2D(0, BOARD_WIDTH, 0, BOARD_HEIGHT);
		    glMatrixMode(GL_MODELVIEW);
		}

		
		void generateRandomPosition(int *x, int *y) 
		{
		    do 
		    {
			*x = rand() % BOARD_WIDTH;
			*y = rand() % BOARD_HEIGHT;
		    } while (board[*y][*x] == '#'); 
		}

		void checkCollisionAndUpdateScore(Pacman* pacman, Pellet* pellets, int pelletCount, int* score)  
		{
		    for (int i = 0; i < pelletCount; i++) 
		    {
			if (!pellets[i].eaten && pacman->x == pellets[i].x && pacman->y == pellets[i].y) 
			{
			    pellets[i].eaten = true;
			    *score += 10;   // 10 score for normal pellet and 200 for powerpellet
			    if (pellets[i].isPowerPellet) 
			    {
				handlePowerPelletEffects(); 
			    }
			}
		    }
		    
		    
		    pthread_mutex_lock(&vulnerable_mutex);

		    if (!vulnerable_timer_expired) 
		    {
			for (int i = 0; i < NUM_GHOSTS; i++) 
			{
			    if (pacman->x == ghosts[i].x && pacman->y == ghosts[i].y) 
			    {
				if (ghosts[i].isVulnerable) 
				{
				    *score += 200;  // 200 score for each power pellet
				   
				    ghosts[i].x = (BOARD_WIDTH - 2) / 2; 
				    ghosts[i].y = (BOARD_HEIGHT - 1) / 2 + i; 
				    
				    ghost_eaten_time[i] = time(NULL);
				    
				    ghosts[i].isVulnerable = false;
				    
				    memcpy(ghosts[i].color, ghosts[i].originalColor, sizeof(float) * 3); // storing original color to  be resetted after vulnerable time interval
				} 

				else 
				{
				   
				}

			    }
			}
		    }

		    pthread_mutex_unlock(&vulnerable_mutex);
		}


		
		void menu(int value) 
		{
		    switch (value) 
		    {
			case 1:
			    gameStarted = true;
			    break;
			case 2:
			    printf("Instructions:\n");
			    printf("Use arrow keys to move Pac-Man.\n");
			    printf("Avoid ghosts and eat all the pellets.\n");
			    printf("Select 'Exit' to quit.\n");
			    break;
			case 3:
			    exit(0);
			    break;
		    }
		}

		
		void handlePowerPelletEffects() 
		{
		    
		    for (int i = 0; i < NUM_GHOSTS; i++) 
		    {
			ghosts[i].isVulnerable = true;
			ghosts[i].color[0] = 0.0f; 
			ghosts[i].color[1] = 0.0f;
			ghosts[i].color[2] = 1.0f;
		    }
		    
		    
		    pthread_mutex_lock(&vulnerable_mutex);  
		    vulnerable_timer_expired = false;
		    pthread_mutex_unlock(&vulnerable_mutex);
		}



		float colorSimilarity(float color1[3], float color2[3]) 
		{
		    float distance = sqrt(pow(color1[0] - color2[0], 2) +
				         pow(color1[1] - color2[1], 2) +
				         pow(color1[2] - color2[2], 2));
		    return distance;
		}
		
		
		//----------------------------------------------------- GAME INITIALIZATION ---------------------------------------------
		//-----------------------------------------------------------------------------------------------------------------------
		
		void resetGame()  // reset the game after all lives are lost and needed to be restarts
		{
		   
		    score = 0;
		    lives = 3;
		    pelletCount = 0;

		   
		    for (int i = 0; i < NUM_GHOSTS; i++) 
		    {
			ghosts[i].x = (BOARD_WIDTH - 2) / 2; 
			ghosts[i].y = (BOARD_HEIGHT - 1) / 2 + i; 
			memcpy(ghosts[i].color, ghosts[i].originalColor, sizeof(float) * 3);
			ghosts[i].isVulnerable = false;
			ghosts[i].hasSpeedBoost = false; 
		    }

		    
		    for (int i = 0; i < BOARD_HEIGHT * BOARD_WIDTH; i++) 
		    {
			pellets[i].x = -1;
			pellets[i].y = -1;
			pellets[i].eaten = true;
			pellets[i].isPowerPellet = false;
		    }

		}



		
void startGame() 
{
    resetGame();

    
    while (!gameStarted) 
    {
        glutMainLoopEvent();
    }

    int ghostsWithSpeedBoost[2] = {-1, -1};

    for (int i = 0; i < 2; i++) 
    {
        int randomGhostIndex;
        do 
        {
            randomGhostIndex = rand() % NUM_GHOSTS;
        } 
        while (ghostsWithSpeedBoost[0] == randomGhostIndex || ghostsWithSpeedBoost[1] == randomGhostIndex);
        ghostsWithSpeedBoost[i] = randomGhostIndex;
        ghosts[ghostsWithSpeedBoost[i]].hasSpeedBoost = true;
    }

    
    int score = 0;

    pthread_t engine_thread, ui_thread, ghost_threads[NUM_GHOSTS];

    
    if (pthread_create(&engine_thread, NULL, game_engine, NULL) != 0) 
    {
        fprintf(stderr, "Error creating game engine thread\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&ui_thread, NULL, ui_thread_function, NULL) != 0) 
    {
        fprintf(stderr, "Error creating UI thread\n");
        exit(EXIT_FAILURE);
    }

    
    float pacmanColor[3] = {1.0f, 1.0f, 0.0f}; 

   
    for (int i = 0; i < NUM_GHOSTS; i++) 
    {
        ghosts[i].x = (BOARD_WIDTH - 2) / 2.0f;
        ghosts[i].y = (BOARD_HEIGHT - 1) / 2.0f;
        ghosts[i].state = GHOST_STATE_INSIDE_HOUSE;

        
        float ghostColor[3];

        do 
        {
            ghostColor[0] = (float)(rand() % 256) / 255.0f; 
            ghostColor[1] = (float)(rand() % 256) / 255.0f; 
            ghostColor[2] = (float)(rand() % 256) / 255.0f; 
        } while (colorSimilarity(pacmanColor, ghostColor) < 0.3f); 

        if (ghosts[i].hasSpeedBoost == true) 
        {
            ghostColor[0] = 1.0f;
            ghostColor[1] = 0.5f;
            ghostColor[2] = 0.0f;
        }

        glColor3fv(ghostColor);

        memcpy(ghosts[i].color, ghostColor, sizeof(float) * 3);
        memcpy(ghosts[i].originalColor, ghostColor, sizeof(float) * 3);

       
        int* ghost_index = malloc(sizeof(int)); 
        *ghost_index = i; 

        if (pthread_create(&ghost_threads[i], NULL, ghost_controller, (void*)ghost_index) != 0) 
        {
            fprintf(stderr, "Error creating ghost thread\n");
            exit(EXIT_FAILURE);
        }
    }

  
    for (int i = 0; i < BOARD_HEIGHT; i++) 
    {
        for (int j = 0; j < BOARD_WIDTH; j++) 
        {
            if (board[i][j] == '.') 
            {
                pellets[pelletCount].x = j;
                pellets[pelletCount].y = i;
                pellets[pelletCount].eaten = false;

                
                if (rand() % 100 < POWER_PELLET_PERCENTAGE) 
                {
                    pellets[pelletCount].isPowerPellet = true; 
                } 

                else 
                {
                    pellets[pelletCount].isPowerPellet = false; 
                }

                pelletCount++;
            }
        }
    }

    
    int pacX, pacY;
    generateRandomPosition(&pacX, &pacY);

    
    while (true) 
    {
        bool farEnough = true;
        for (int i = 0; i < NUM_GHOSTS; i++) 
        {
            if (abs(pacX - ghosts[i].x) <= 2 && abs(pacY - ghosts[i].y) <= 2) 
            {
                farEnough = false;
                break;
            }
        }
        if (farEnough) 
        {
            break;
        } 

        else 
        {
            generateRandomPosition(&pacX, &pacY);
        }
    }

   
    squareX = pacX;
    squareY = pacY;

    
    glutTimerFunc(0, updateGhosts, 0);
    glutMainLoop();
}
		
		
int main(int argc, char** argv) 
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 1000);
    glutCreateWindow("Pac-Man");

    glutCreateMenu(menu);
    glutAddMenuEntry("Start Game", 1);
    glutAddMenuEntry("Instructions", 2);
    glutAddMenuEntry("Quit", 3);
    glutAttachMenu(GLUT_RIGHT_BUTTON); 


    glutDisplayFunc(display);
    glutSpecialFunc(specialKeyboard); 

    
    initOpenGL();

    
    glutPostRedisplay();

   
    pthread_t engine_thread, ui_thread;
    pthread_create(&engine_thread, NULL, game_engine, NULL);
    pthread_create(&ui_thread, NULL, ui_thread_function, NULL);

    startGame(); 

    return 0;
}




		



