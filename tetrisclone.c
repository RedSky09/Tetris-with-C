#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 550
#define SCREEN_HEIGHT 480
#define BLOCK_SIZE 20
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define FONT_SIZE 20
#define BOARD_OFFSET_X 170
#define BOARD_OFFSET_Y 60
#define TEXT_COLOR (SDL_Color){255, 255, 255, 255}

int board[BOARD_WIDTH][BOARD_HEIGHT] = {0};
int score = 0;
int completedLines = 0;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position position[4];
    SDL_Color color;
} Block;

int isPaused = 0;
int isMusicPlaying = 1;
int nextBlockIndex;

Block blocks[] = {
    { {{0, 0}, {1, 0}, {0, 1}, {1, 1}}, {255, 255, 0, 1} },       // Block: Square (KUNING)
    { {{0, 0}, {1, 0}, {2, 0}, {3, 0}}, {0, 255, 255, 1} },       // Block: Line (CYAN)
    { {{0, 0}, {0, 1}, {0, 2}, {1, 2}}, {255, 165, 0, 1} },       // Block: L (ORANGE)
    { {{1, 0}, {1, 1}, {1, 2}, {0, 2}}, {0, 0, 255, 1} },         // Block: Reverse L (BIRU)
    { {{0, 0}, {0, 1}, {0, 2}, {1, 1}}, {128, 0, 128, 1} },       // Block: T (UNGU)
    { {{0, 0}, {1, 0}, {1, 1}, {2, 1}}, {255, 0, 0, 1} },         // Block: Z (Red)
    { {{0, 1}, {1, 1}, {1, 0}, {2, 0}}, {0, 128, 0, 1} },         // Block: Reverse Z (Green)
};

int currentBlockIndex;
Block currentBlock;
Position currentBlockPosition;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
Mix_Music* music = NULL;

void drawBlock(SDL_Renderer* renderer, int x, int y, SDL_Color color) {
    SDL_Rect blockRect = {x, y, BLOCK_SIZE, BLOCK_SIZE};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &blockRect);
}

void drawBoard(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw outer border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect outerBorderRect = {BOARD_OFFSET_X - 2, BOARD_OFFSET_Y - 2, BLOCK_SIZE * BOARD_WIDTH + 4, BLOCK_SIZE * BOARD_HEIGHT + 4};
    SDL_RenderDrawRect(renderer, &outerBorderRect);

    // Draw inner grid
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    for (int x = 0; x < BOARD_WIDTH; ++x) {
        for (int y = 0; y < BOARD_HEIGHT; ++y) {
            SDL_Rect gridRect = {BOARD_OFFSET_X + x * BLOCK_SIZE, BOARD_OFFSET_Y + y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE};
            SDL_RenderDrawRect(renderer, &gridRect);
        }
    }

    // Draw placed blocks
    for (int x = 0; x < BOARD_WIDTH; ++x) {
        for (int y = 0; y < BOARD_HEIGHT; ++y) {
            if (board[x][y] == 1) {
                drawBlock(renderer, BOARD_OFFSET_X + x * BLOCK_SIZE, BOARD_OFFSET_Y + y * BLOCK_SIZE, TEXT_COLOR);
            }
        }
    }

    // Draw current block
    for (int i = 0; i < 4; ++i) {
        int x = currentBlockPosition.x + currentBlock.position[i].x;
        int y = currentBlockPosition.y + currentBlock.position[i].y;
        drawBlock(renderer, BOARD_OFFSET_X + x * BLOCK_SIZE, BOARD_OFFSET_Y + y * BLOCK_SIZE, currentBlock.color);
    }

    // Draw score
    char scoreText[16];
    sprintf(scoreText, "SCORE: %d", score);
    SDL_Surface* surface = TTF_RenderText_Solid(font, scoreText, TEXT_COLOR);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect scoreRect = {390, 100, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &scoreRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // Draw "Tetris" text
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, " T E T R I S ", TEXT_COLOR);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {181, 5, textSurface->w, textSurface->h };
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    lines(renderer);

    SDL_RenderPresent(renderer);
}

void lines(SDL_Renderer* renderer) {
    char linesText[16];
    sprintf(linesText, "LINES: %d", completedLines);
    SDL_Surface* surface = TTF_RenderText_Solid(font, linesText, TEXT_COLOR);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect linesRect = {25, 100, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &linesRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int checkCollision(int x, int y) {
    for (int i = 0; i < 4; ++i) {
        int blockX = x + currentBlock.position[i].x;
        int blockY = y + currentBlock.position[i].y;

        if (blockX < 0 || blockX >= BOARD_WIDTH || blockY < 0 || blockY >= BOARD_HEIGHT || board[blockX][blockY] == 1) {
            return 1;  // Collision detected
        }
    }

    return 0;  // No collision
}

void rotateBlock() {
    Block tempBlock = currentBlock;

    // Perform rotation (90 degrees clockwise)
    for (int i = 0; i < 4; ++i) {
        int tempX = tempBlock.position[i].x;
        tempBlock.position[i].x = -tempBlock.position[i].y;
        tempBlock.position[i].y = tempX;
    }

    // Check collision after rotation
    if (!checkCollision(currentBlockPosition.x, currentBlockPosition.y)) {
        currentBlock = tempBlock;
    }
}

void moveBlock(int deltaX, int deltaY) {
    int newX = currentBlockPosition.x + deltaX;
    int newY = currentBlockPosition.y + deltaY;

    if (!checkCollision(newX, newY)) {
        currentBlockPosition.x = newX;
        currentBlockPosition.y = newY;
    } else if (deltaY > 0) {  // If collision is detected while moving down, place the block and generate a new one
        placeBlock();
        generateNewBlock();
        checkLines();

        if (checkCollision(currentBlockPosition.x, currentBlockPosition.y)) { //Game Over
            Mix_HaltMusic();
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Game Over", "Game over!", window);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_CloseFont(font);
            TTF_Quit();
            SDL_Quit();
            Mix_CloseAudio();
            exit(0);
        }
    }
}

void placeBlock() {
    for (int i = 0; i < 4; ++i) {
        int x = currentBlockPosition.x + currentBlock.position[i].x;
        int y = currentBlockPosition.y + currentBlock.position[i].y;
        board[x][y] = 1;
        if (y <= 0) {
            // Game over condition: If any block is placed at or above the top row, exit the game
            Mix_HaltMusic();
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Game Over", "Game over!", window);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_CloseFont(font);
            TTF_Quit();
            SDL_Quit();
            Mix_CloseAudio();
            exit(0);
        }
    }
}

void generateNewBlock() {
    srand(time(NULL));
    currentBlockIndex = rand() % 7;
    currentBlock = blocks[currentBlockIndex];
    currentBlockPosition.x = BOARD_WIDTH / 2 - 1;
    currentBlockPosition.y = 0;

}

void checkLines() {
    completedLines = 0; // Update the global variable

    for (int y = 0; y < BOARD_HEIGHT; ++y) {
        int lineCompleted = 1;

        for (int x = 0; x < BOARD_WIDTH; ++x) {
            if (board[x][y] == 0) {
                lineCompleted = 0;
                break;
            }
        }

        if (lineCompleted) {
            // Move all the lines above the completed line one step down
            for (int i = y; i > 0; --i) {
                for (int x = 0; x < BOARD_WIDTH; ++x) {
                    board[x][i] = board[x][i - 1];
                }
            }

            // Clear the top line
            for (int x = 0; x < BOARD_WIDTH; ++x) {
                board[x][0] = 0;
            }

            ++completedLines;
        }
    }

    // Update score based on completed lines
    score = score + completedLines * completedLines * 5;

    completedLines = score / 5;
}

void toggleMusic() {
    if (isMusicPlaying) {
        Mix_PauseMusic();
        isMusicPlaying = 0;
    } else {
        Mix_ResumeMusic();
        isMusicPlaying = 1;
    }
}


int main(int argc, char* args[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    if (TTF_Init() == -1) {
        printf("TTF_Init: %s\n", TTF_GetError());
        return 0;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return 0;
    }

    window = SDL_CreateWindow("Tetris Clone", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    font = TTF_OpenFont("pixel.ttf", FONT_SIZE);
    if (font == NULL) {
        printf("Failed to load font! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    // Load music
    music = Mix_LoadMUS("tetris.wav");
    if (music == NULL) {
        printf("Failed to load music! SDL_mixer Error: %s\n", Mix_GetError());
        return 0;
    }

    Mix_PlayMusic(music, -1);  // Play music in loop

    SDL_Event e;
    int quit = 0;

    generateNewBlock();
    completedLines = 0;

    while (!quit) {

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = 1;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    moveBlock(-1, 0);
                    break;

                case SDLK_RIGHT:
                    moveBlock(1, 0);
                    break;

                case SDLK_DOWN:
                    moveBlock(0, 1);
                    break;

                case SDLK_UP:
                    rotateBlock();
                    break;

                case SDLK_SPACE:
                    isPaused = !isPaused;
                    toggleMusic();
                    break;
            }
        }
    }

    if (!isPaused) {
        moveBlock(0, 1);
    }

    drawBoard(renderer);
    SDL_Delay(300);  // Delay to control the game speed
}

    Mix_HaltMusic();
    Mix_FreeMusic(music);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    Mix_CloseAudio();

    return 0;
}
