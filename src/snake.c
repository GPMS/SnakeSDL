#include "snake.h"

#include <stdlib.h>

#include "draw.h"
#include "game.h"
#include "highscore.h"
#include "input.h"
#include "window.h"

static unsigned int RangedRandom(unsigned int min, unsigned int max)
{
    const double scaled = (double)rand() / RAND_MAX;
    return (max - min + 1) * scaled + min;
}

static void SetRandomApplePos(Game* game)
{
    Snake* snake = &game->snake;
    Vec2*  apple = &game->applePos;

    const int maxRow = game->rows - 2;
    const int minRow = 3;
    const int maxCol = game->cols - 3;
    const int minCol = 2;

    while (1) {
        // Change apple location
        *apple = Vector2(RangedRandom(minCol, maxCol),
                         RangedRandom(minRow, maxRow));

        // Check if location doesn't overlap with the snake
        bool isOverlapping = false;

        for (unsigned int i = 0; i < snake->partsDrawn; i++) {
            if ((apple->x == snake->body[i].pos.x) && (apple->y == snake->body[i].pos.y)) {
                isOverlapping = true;
                break;
            }
        }
        if (!isOverlapping)
            return;
    }
}

void Snake_Reset(Game* game)
{
    Snake* snake = &game->snake;

    free(snake->body);
    snake->direction  = EAST;
    snake->parts      = game->initialSize;
    snake->partsDrawn = 1;

    snake->bodyLength  = game->initialSize;
    snake->body        = malloc(snake->bodyLength * sizeof(Body));
    snake->body[0].pos = Vector2(5, 10);

    SetRandomApplePos(game);
}

static void AppleCollision(Game* game)
{
    Snake* snake = &game->snake;
    Vec2   apple = game->applePos;

    if (Vector2_Compare(snake->body[0].pos, apple) == 0) {
        const int gamePlayArea     = game->rows * game->cols;
        const int scoreGain        = 10;
        const int maxPossibleScore = (gamePlayArea - game->initialSize) * scoreGain;

        if (game->score < maxPossibleScore)
            game->score += scoreGain;

        SetRandomApplePos(game);

        // Add body parts
        snake->parts++;
    }
}

static void BodyCollision(Game* game)
{
    Snake* snake = &game->snake;

    for (unsigned int i = 1; i < snake->partsDrawn; i++) {
        if (Vector2_Compare(snake->body[0].pos, snake->body[i].pos) == 0) {
            if (game->score > Highscore_GetLowestScore()) {
                Highscore_New(game);
            } else {
                game->state = GAMEOVER;
                break;
            }
        }
    }
}

static void WallCollision(Game* game)
{
    Body* head = &game->snake.body[0];

    const int maxRow = game->rows - 2;
    const int minRow = 3;
    const int maxCol = game->cols - 3;
    const int minCol = 2;

    // up
    if (head->pos.y < minRow) {
        head->pastPos.y = head->pos.y;
        head->pos.y     = maxRow;
    }
    // down
    else if (head->pos.y > maxRow) {
        head->pastPos.y = head->pos.y;
        head->pos.y     = minRow;
    }
    // right
    if (head->pos.x > maxCol) {
        head->pastPos.x = head->pos.x;
        head->pos.x     = minCol;
    }
    // left
    else if (head->pos.x < minCol) {
        head->pastPos.x = head->pos.x;
        head->pos.x     = maxCol;
    }
}

static void CollisionCheck(Game* game)
{
    AppleCollision(game);
    BodyCollision(game);
    WallCollision(game);
}

static void Move(Game* game)
{
    Snake* snake = &game->snake;

    // MOVE HEAD
    Body* head = &snake->body[0];

    head->pastPos = head->pos;
    switch (snake->direction) {
        case NORTH:
            head->pos.y--;
            break;
        case SOUTH:
            head->pos.y++;
            break;
        case EAST:
            head->pos.x++;
            break;
        case WEST:
            head->pos.x--;
            break;
    }

    // Loop through the body of the snake, update the past
    // position of the current part and move it to the
    // past position of the previous part
    for (unsigned int i = 1; i < snake->partsDrawn; i++) {
        snake->body[i].pastPos = snake->body[i].pos;
        snake->body[i].pos     = snake->body[i - 1].pastPos;
    }
}

void Snake_Update(Game* game)
{
    Snake* snake = &game->snake;

    if (IsKeyDown(SDL_SCANCODE_UP)) {
        if (snake->parts == 1 || snake->direction != SOUTH)
            snake->direction = NORTH;
    } else if (IsKeyDown(SDL_SCANCODE_DOWN)) {
        if (snake->parts == 1 || snake->direction != NORTH)
            snake->direction = SOUTH;
    } else if (IsKeyDown(SDL_SCANCODE_LEFT)) {
        if (snake->parts == 1 || snake->direction != EAST)
            snake->direction = WEST;
    } else if (IsKeyDown(SDL_SCANCODE_RIGHT)) {
        if (snake->parts == 1 || snake->direction != WEST)
            snake->direction = EAST;
    }

    static int time = 0;
    time += game->dt;
    const int moveDelay = 80;  // in miliseconds
    if (time >= moveDelay) {
        Move(game);
        time = 0;
    }

    CollisionCheck(game);

    while (snake->partsDrawn < snake->parts) {
        if (snake->partsDrawn > snake->bodyLength - 1) {
            snake->bodyLength *= 2;
            snake->body = realloc(snake->body, snake->bodyLength * sizeof(Body));
        }

        snake->body[snake->partsDrawn].pos     = snake->body[snake->partsDrawn - 1].pastPos;
        snake->body[snake->partsDrawn].pastPos = Vector2(0, 0);

        snake->partsDrawn++;
    }
}

void Snake_Draw(const Game* game)
{
    SDL_Renderer* renderer = game->window->SDLRenderer;
    const Snake*  snake    = &game->snake;

    const int size = BLOCK_SIZE - 5;

    for (unsigned int i = 0; i < snake->partsDrawn; i++) {
        // Don't draw blocks without defined position
        if (snake->body[i].pos.x == 0)
            break;

        DrawFillSquare(renderer, &green, Vector2_Mul(snake->body[i].pos, BLOCK_SIZE), size);
    }
}
