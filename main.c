#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>


#define WIDTH       	26	// BLOCK_SIZEs
#define HEIGHT      	26	// BLOCK_SIZEs
#define NAME        	"Snake"
#define BLOCK_SIZE  	25
#define INITIAL_SIZE	3	// How many parts the snake starts with


int done = 0;

enum dir { NORTH, EAST, WEST, SOUTH };
enum state { GAME, PAUSE, GAMEOVER };

typedef struct Body{
    int pastXGrid, pastYGrid;
    int xGrid, yGrid;
    struct Body *next;
} Body;

typedef struct{
    int xGrid, yGrid;
} Apple;

typedef struct{
    SDL_bool running;
    int state;
    
    /* Player */
    int direction;
    int parts;
    int partsDrawn;
    int score;
    
    SDL_Texture *label;
    TTF_Font *font;

} GameState;


int processEvents(SDL_Window *window, GameState *game);
void doRender(SDL_Renderer *renderer, Body *head, Apple apple, GameState *game);
void moveSnake(GameState game, Body *head);
unsigned int randr(unsigned int min, unsigned int max);
void collisionCheck(GameState *game, Body *head, Apple *apple);
Body *newBody(Body *tail);
void deleteSnake(Body *head);


int main(int argc, char **argv)
{
    GameState game;
    game.state = GAME;
    
    SDL_Window *window;
    SDL_Renderer *renderer;
    
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    
    window = SDL_CreateWindow("Game Window",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WIDTH * BLOCK_SIZE, HEIGHT * BLOCK_SIZE,
                              0
                             );
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    game.running = SDL_TRUE;
    
    Body *head = NULL;
    Body *tail = NULL;
    
    head = malloc(sizeof(Body));
    tail = head;
    
    srandom((int)time(NULL));
    
    game.direction = EAST;
    game.parts = INITIAL_SIZE;
    game.partsDrawn = 1;
    game.score = 0;
    
    game.font = NULL;
    game.font = TTF_OpenFont("emulogic.ttf", 24);
    if (game.font == NULL) {
    	printf("COULD NOT FIND FONT FILE!\n");
    	SDL_Quit();
    	return 1;
    }
    
    head->xGrid = 5;
    head->yGrid = 10;
    head->next = NULL;
    
    Apple apple;	
    apple.xGrid = 2;
    apple.yGrid = 10;

    /* Event loop */
    while (game.running) {
        processEvents(window, &game);
        
        if (game.state == GAME) {
			while (game.partsDrawn < game.parts) {
				tail = newBody(tail);
				game.partsDrawn++;
			}
			moveSnake(game, head);
			collisionCheck(&game, head, &apple);
		}
		
		doRender(renderer, head, apple, &game);
		
		SDL_Delay(100);

		done = 0;
    }
    
    deleteSnake(head);
    
    SDL_DestroyTexture(game.label);
    TTF_CloseFont(game.font);
    
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    
    SDL_Quit();
    
    return 0;
}



int processEvents(SDL_Window *window, GameState *game)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_WINDOWEVENT_CLOSE:
                if (window) {
                    SDL_DestroyWindow(window);
                    window = NULL;
                    game->running = SDL_FALSE;
                }
                break;
        
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        game->running = SDL_FALSE;
                        break;
                    case SDLK_p:
                    	if (game->state == GAME)
                    		game->state = PAUSE;
                    	else if (game->state == PAUSE)
                    		game->state = GAME;
                    	
                    	break;
                    case SDLK_UP:
                        if ((game->parts == 1 || game->direction != SOUTH) && done == 0) {
                            game->direction = NORTH;
                            done = 1;
                        }
                        break;
                    case SDLK_DOWN:
                        if ((game->parts == 1 || game->direction != NORTH) && done == 0) {
                            game->direction = SOUTH;
                            done = 1;
                        }
                        break;
                    case SDLK_RIGHT:
                        if ((game->parts == 1 || game->direction != WEST) && done ==  0){
                            game->direction = EAST;
                            done = 1;
                        }
                        break;
                    case SDLK_LEFT:
                        if ((game->parts == 1 || game->direction != EAST) && done == 0) {
                            game->direction = WEST;
                            done = 1;
                        }
                        break;
                }
                break;
        
            case SDL_QUIT:
                game->running = SDL_FALSE;
                break;
        }
    }
    
    return done;
}


SDL_Renderer *drawGame(SDL_Renderer *renderer, Body *head, Apple apple, GameState *game)
{
    /* Draw black background */
    SDL_SetRenderDrawColor(renderer,
                           0, 0, 0,
                           255
                          );
    SDL_RenderClear(renderer);
    
    /* Draw score */
    SDL_Color white = {255, 255, 255, 255};
    
    char str[128] = "";
    sprintf(str, "score:%d", game->score);
    
    SDL_Surface *tmp = TTF_RenderText_Blended(game->font, str, white);
    game->label = SDL_CreateTextureFromSurface(renderer, tmp);
    SDL_Rect textRect = {BLOCK_SIZE, BLOCK_SIZE, tmp->w, tmp->h};
    SDL_RenderCopy(renderer, game->label, NULL, &textRect);
    SDL_FreeSurface(tmp);
    
    /* Draw main screen */
    SDL_SetRenderDrawColor(renderer,
                           50, 50, 50,
                           255
                          );
    SDL_Rect mainScreen = { 2*BLOCK_SIZE, 3*BLOCK_SIZE, 22*BLOCK_SIZE, 22*BLOCK_SIZE };
    SDL_RenderFillRect(renderer, &mainScreen);
    
    /* Draw apple */
    SDL_SetRenderDrawColor(renderer,
                       255, 0, 0,
                       255
                      );
    SDL_Rect appleRect = { apple.xGrid*BLOCK_SIZE, apple.yGrid*BLOCK_SIZE, BLOCK_SIZE-5, BLOCK_SIZE-5 };
    SDL_RenderFillRect(renderer, &appleRect);

    /* Draw snake */
    // Set Colour
    SDL_SetRenderDrawColor(renderer,
                       0, 255, 0,
                       255
                      );
    
    Body *current = head;
    
    // Draw snake
    while (current != NULL){
        SDL_Rect snakeRect = { current->xGrid*BLOCK_SIZE, current->yGrid*BLOCK_SIZE, BLOCK_SIZE-5, BLOCK_SIZE-5 };
        SDL_RenderFillRect(renderer, &snakeRect);
        
        current = current->next;
    }
    
    return renderer;

}


SDL_Renderer *drawPause(SDL_Renderer *renderer, GameState *game)
{
    SDL_Color white = {255, 255, 255, 255};
    
    /* Write 'Pause' */
    SDL_Surface *tmp1 = TTF_RenderText_Blended(game->font, "PAUSED", white);
    game->label = SDL_CreateTextureFromSurface(renderer, tmp1);
    SDL_Rect pauseRect = {10 * BLOCK_SIZE, 5 * BLOCK_SIZE, tmp1->w, tmp1->h};
    SDL_RenderCopy(renderer, game->label, NULL, &pauseRect);
    SDL_FreeSurface(tmp1);
    /* Write instructions */
    SDL_Surface *tmp2 = TTF_RenderText_Blended(game->font, "Press p to continue", white);
    game->label = SDL_CreateTextureFromSurface(renderer, tmp2);
    SDL_Rect textRect = {4 * BLOCK_SIZE, 10 * BLOCK_SIZE, tmp2->w, tmp2->h};
    SDL_RenderCopy(renderer, game->label, NULL, &textRect);
    SDL_FreeSurface(tmp2);
    
    return renderer;
}


SDL_Renderer *drawGameOver(SDL_Renderer *renderer, GameState *game)
{
	/* Draw black background */
    SDL_SetRenderDrawColor(renderer,
                           0, 0, 0,
                           255
                          );
    SDL_RenderClear(renderer);
    
    /* Write gameover */
    SDL_Color white = {255, 255, 255, 255};
    
    SDL_Surface *tmp = TTF_RenderText_Blended(game->font, "GameOver", white);
    game->label = SDL_CreateTextureFromSurface(renderer, tmp);
    SDL_Rect textRect = {9 * BLOCK_SIZE, 5 * BLOCK_SIZE, tmp->w, tmp->h};
    SDL_RenderCopy(renderer, game->label, NULL, &textRect);
    SDL_FreeSurface(tmp);
    
    return renderer;
}


void doRender(SDL_Renderer *renderer, Body *head, Apple apple, GameState *game)
{
	switch(game->state) {
		case GAME:
    		renderer = drawGame(renderer, head, apple, game);
    		break;
    	case PAUSE:
    		renderer = drawPause(renderer, game);
    		break;
    	case GAMEOVER:
    		renderer = drawGameOver(renderer, game);
    		break;
    }
    SDL_RenderPresent(renderer);
}


void moveSnake(GameState game, Body *head)
{
    /* Move Head */
    switch(game.direction) {
        case NORTH:
            head->pastYGrid = head->yGrid;
            head->pastXGrid = head->xGrid;
            head->yGrid--;
            break;
        case SOUTH:
            head->pastYGrid = head->yGrid;
            head->pastXGrid = head->xGrid;
            head->yGrid++;
            break;
        case EAST:
            head->pastYGrid = head->yGrid;
            head->pastXGrid = head->xGrid;
            head->xGrid++;
            break;
        case WEST:
            head->pastYGrid = head->yGrid;
            head->pastXGrid = head->xGrid;
            head->xGrid--;
            break;
    }
    
    /* Move Body */
    Body *current = head->next;
    Body *previous = head;
    
    while (current != NULL) {
        current->pastXGrid = current->xGrid;
        current->xGrid = previous->pastXGrid;
        current->pastYGrid = current->yGrid;
        current->yGrid = previous->pastYGrid;
        
        previous = current;
        current = current->next;
    }
}

unsigned int randr(unsigned int min, unsigned int max)
{
       double scaled = (double)rand()/RAND_MAX;

       return (max - min +1)*scaled + min;
}

void collisionCheck(GameState *game, Body *head, Apple *apple)
{   
    /* Apple collision */
    if ( (head->xGrid == apple->xGrid) && (head->yGrid == apple->yGrid) ) {
        
        if (game->score < WIDTH*HEIGHT*10-INITIAL_SIZE+1*10)
        	game->score += 10;
        
        while (1) {
            // Change apple location
            apple->xGrid = randr(2, WIDTH-3);
            apple->yGrid = randr(3, HEIGHT-2);
            
            // Check if location doesn't overlap with the snake
            int ok = 1;
            if ( (head->xGrid == apple->xGrid) && (head->yGrid == apple->yGrid) )
                ok = 0;
            
            Body *current = head;
            Body *previous = head;
        
            while(current != NULL) {
                if ( (apple->xGrid == current->xGrid) && (apple->yGrid == current->yGrid) ) {
                    ok = 0;
                    break;
                }
                
                previous = current;
                current = current->next;
            }
            
            if (ok) break;
        }
        
        game->parts++;  // Add body parts
    }
    
    /* Body collision */
    Body *current = head->next;
    Body *previous = head->next;
    
    while (current != NULL) {
        if ( (head->xGrid == current->xGrid) && (head->yGrid == current->yGrid) ) {
            game->state = GAMEOVER;
            break;
        }
        
        previous = current;
        current = current->next;
    }
    
    /* Outside boundary */
    
    // up
    if (head->yGrid < 3) {
        head->pastYGrid = head->yGrid;
        head->yGrid = HEIGHT-2;
    // down
    } else if (head->yGrid > HEIGHT-2) {
        head->pastYGrid = head->yGrid;
        head->yGrid = 3;
    }
    // right
    if (head->xGrid > WIDTH-3) {
        head->pastXGrid = head->xGrid;
        head->xGrid = 2;
    // left
    } else if (head->xGrid < 2) {
        head->pastXGrid = head->xGrid;
        head->xGrid = WIDTH-3;
    }
}


Body *newBody(Body *tail)
{
    Body *newBody = malloc(sizeof(Body));
    
    tail->next = newBody;
    
    newBody->next = NULL;
    newBody->xGrid = tail-> pastXGrid;
    newBody->yGrid = tail-> pastYGrid;
    
    return newBody;
}


void deleteSnake(Body *head)
{
    Body *freeMe = head;
    Body *holdMe = NULL;
    
    while(freeMe != NULL) {
        holdMe = freeMe->next;
        free(freeMe);
        freeMe = holdMe;
    }
}
