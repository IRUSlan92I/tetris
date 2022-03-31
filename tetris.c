#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>


#define MAX_KEY_COUNT 10

#define FIELD_WIDTH 12
#define FIELD_HEIGHT 22


#define COLOR_PAIR_I        1
#define COLOR_PAIR_O        2
#define COLOR_PAIR_T        3
#define COLOR_PAIR_J        4
#define COLOR_PAIR_L        5
#define COLOR_PAIR_S        6
#define COLOR_PAIR_Z        7
#define COLOR_PAIR_SHADOW   8
#define COLOR_PAIR_SPEED    9

#define FIGURE_CELL_COUNT   4

#define SPEEDS_COUNT        6

#define CBUTTON_DROP        KEY_UP
#define CBUTTON_RIGHT       KEY_RIGHT
#define CBUTTON_DOWN        KEY_DOWN
#define CBUTTON_LEFT        KEY_LEFT
#define CBUTTON_ROTCW       'x'
#define CBUTTON_ROTCCW      'z'
#define CBUTTON_NEWGAME     'g'
#define CBUTTON_STORAGE     ' '
#define CBUTTON_EXIT        KEY_F(10)
#define CBUTTON_PAUSE       'p'


typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    int height;
    int width;
} Size;

typedef enum {
    TetrominoInit = -1,
    TetrominoNone = 0,
    TetrominoI = 1,
    TetrominoO = 2,
    TetrominoT = 3,
    TetrominoJ = 4,
    TetrominoL = 5,
    TetrominoS = 6,
    TetrominoZ = 7,
} Tetromino;

typedef enum {
    Clockwise,
    Counterclockwise,
} Rotation;


void init(void);
void work(void);
void draw(void);
void kbin(void);

void drawField(void);
void drawScore(void);
void drawSpeed(void);
void drawShadow(void);
void drawFigure(void);
void drawNextFigure(void);
void drawStoredFigure(void);

int keyWasPressed(int key);

void setNewRotationClockwise(void);
void setNewRotationCounterclockwise(void);
Point rotatePoint(Point point, Point origin, Rotation direction);
void rotateClockwise(void);
void rotateCounterclockwise(void);
int canBeRotatedClockwise(void);
int canBeRotatedCounterclockwise(void);


int moveUp(void);
int moveRight(void);
int moveDown(void);
int moveLeft(void);
void dropDown(void);
int canBeMovedUp(void);
int canBeMovedRight(void);
int canBeMovedDown(void);
int canBeMovedLeft(void);

void deployFigure(void);
void newFigure(void);
Tetromino randomTetromino(void);

void checkForFilledLines(void);
void updateSpeed(void);

int isCellFilled(int x, int y);
void setCellFilling(int x, int y, int filling);

void newGame(void);
void exitGame(void);
void storageFigure(void);
int moveFigureToDefaultPosition(void);
void updateShadowPosition(void);

void pauseGame(void);

int *keys;

WINDOW *wField;
WINDOW *wScore;
WINDOW *wSpeed;
WINDOW *wNextFigure;
WINDOW *wStoredFigure;

Size mainWindowSize = {0, 0};
Size keysWindowSize = {0, 0};
Size fieldWindowSize = {0, 0};
Size scoreWindowSize = {0, 0};
Size speedWindowSize = {0, 0};
Size nextFigureWindowSize = {0, 0};
Size storedFigureWindowSize = {0, 0};

int filledCells[FIELD_WIDTH][FIELD_HEIGHT];

Tetromino figure;
Tetromino nextFigure;
Tetromino storedFigure;
Point figureCellsPos[FIGURE_CELL_COUNT];
Point shadowCellsPos[FIGURE_CELL_COUNT];

int isGameOver;
int isPaused;

int speed;
int score;

int fieldRedrawNeeded;

int chanceI;
int chanceO;
int chanceT;
int chanceJ;
int chanceL;
int chanceS;
int chanceZ;

int isMoving;

int storageUsed;

int hasColors;

unsigned long workCount = 0;

int *chances[7] = {&chanceI, &chanceO, &chanceT,
                   &chanceJ, &chanceL, &chanceS, &chanceZ};

int speedList[SPEEDS_COUNT] = {25, 20,  15,  10,   5,    1};
int scoreList[SPEEDS_COUNT] = { 0, 10, 100, 250, 500, 1000};


int main(void) {
    init();
    newGame();

    while (1) {
        kbin();
        work();
        draw();

        usleep(50000);
    }

    return 0;
}

void init(void)
{
    srand((unsigned int)time(NULL));
    initscr();
    nodelay(stdscr, TRUE);
    cbreak();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    noecho();

    keys = malloc(sizeof(*keys)*MAX_KEY_COUNT);

    getmaxyx(stdscr, mainWindowSize.height, mainWindowSize.width);

    Size realFieldSize;
    realFieldSize.height = 22;
    realFieldSize.width = 12;
    wField = newwin(realFieldSize.height, realFieldSize.width,
                    1, mainWindowSize.width/2 - realFieldSize.width/2);
    getmaxyx(wField, fieldWindowSize.height, fieldWindowSize.width);

    Size realScoreSize;
    realScoreSize.height = 3;
    realScoreSize.width = 8;
    wScore = newwin(realScoreSize.height, realScoreSize.width,
            1, mainWindowSize.width/2 -
            realScoreSize.width/2 +fieldWindowSize.width);
    getmaxyx(wScore, scoreWindowSize.height, scoreWindowSize.width);

    Size realSpeedSize;
    realSpeedSize.height = 3;
    realSpeedSize.width = 8;
    wSpeed = newwin(realSpeedSize.height, realSpeedSize.width,
            1, mainWindowSize.width/2 - realSpeedSize.width/2 -
            fieldWindowSize.width);
    getmaxyx(wSpeed, speedWindowSize.height, speedWindowSize.width);

    Size realNextFigureSize;
    realNextFigureSize.height = 6;
    realNextFigureSize.width = 8;
    wNextFigure = newwin(realNextFigureSize.height, realNextFigureSize.width,
            scoreWindowSize.height+1, mainWindowSize.width/2 -
            realNextFigureSize.width/2 + fieldWindowSize.width);
    getmaxyx(wNextFigure, nextFigureWindowSize.height,
             nextFigureWindowSize.width);

    Size realStoredFigureSize;
    realStoredFigureSize.height = 6;
    realStoredFigureSize.width = 8;
    wStoredFigure = newwin(realStoredFigureSize.height, realStoredFigureSize.width,
            speedWindowSize.height+1, mainWindowSize.width/2 -
            realStoredFigureSize.width/2 - fieldWindowSize.width);
    getmaxyx(wStoredFigure, storedFigureWindowSize.height,
            storedFigureWindowSize.width);

    hasColors = has_colors() == TRUE;

    if (hasColors) {
        start_color();
        init_pair(COLOR_PAIR_I, COLOR_CYAN, COLOR_CYAN);
        init_pair(COLOR_PAIR_O, COLOR_YELLOW, COLOR_YELLOW);
        init_pair(COLOR_PAIR_T, COLOR_MAGENTA, COLOR_MAGENTA);
        init_pair(COLOR_PAIR_J, COLOR_BLUE, COLOR_BLUE);
        init_pair(COLOR_PAIR_L, COLOR_WHITE, COLOR_WHITE);
        init_pair(COLOR_PAIR_S, COLOR_GREEN, COLOR_GREEN);
        init_pair(COLOR_PAIR_Z, COLOR_RED, COLOR_RED);
        init_pair(COLOR_PAIR_SHADOW, COLOR_BLACK, COLOR_BLACK);
        init_pair(COLOR_PAIR_SPEED, COLOR_RED, COLOR_RED);
    }
}

void kbin(void)
{
    memset(keys, 0, sizeof(*keys)*MAX_KEY_COUNT);

    int key;
    int keyPointer = 0;
    while ((key = getch()) != ERR) {
        if (keyPointer < MAX_KEY_COUNT && !keyWasPressed(key)) {
            switch (key) {
                case CBUTTON_DROP:
                case CBUTTON_RIGHT:
                case CBUTTON_DOWN:
                case CBUTTON_LEFT:
                case CBUTTON_ROTCW:
                case CBUTTON_ROTCCW:
                case CBUTTON_NEWGAME:
                case CBUTTON_STORAGE:
                case CBUTTON_EXIT:
                case CBUTTON_PAUSE:
                    keys[keyPointer++] = key;
                    break;
            }
        }
    }
}


void draw(void)
{
    drawField();
    drawScore();
    drawSpeed();
    drawNextFigure();
    drawStoredFigure();
}

void drawField(void)
{
    if (fieldRedrawNeeded) {
        wclear(wField);
        box(wField, ACS_VLINE, ACS_HLINE);

        int x;
        int y;
        for (x = 1; x < FIELD_WIDTH-1; x++) {
            for (y = 0; y < FIELD_HEIGHT-1; y++) {
                if (isCellFilled(x, y) > 0) {
                    if (hasColors) {
                        wattron(wField, COLOR_PAIR(isCellFilled(x, y)));
                    }
                    mvwaddch(wField, y, x, ACS_BLOCK);
                    if (hasColors) {
                        wattroff(wField, COLOR_PAIR(isCellFilled(x, y)));
                    }
                }
                else if (isCellFilled(x, y) < 0) {
                    mvwaddch(wField, y, x, ACS_BLOCK);
                }
                else {
                    mvwaddch(wField, y, x, '.');
                }
            }
        }
        drawShadow();
        drawFigure();

        if (isGameOver) {
            mvwprintw(wField, 0, 2, "GAME OVER");
        }
        else if (isPaused) {
            mvwprintw(wField, 0, 3, "PAUSED");
        }

        touchwin(wField);
        wrefresh(wField);

        fieldRedrawNeeded = 0;
    }
}

void drawScore(void)
{
    static int oldScore = -1;
    if (oldScore != score) {
        oldScore = score;

        wclear(wScore);
        box(wScore, ACS_VLINE, ACS_HLINE);

        mvwprintw(wScore, 1, 1, "%6d", score);

        mvwprintw(wScore, 0, 2, "SCORE");

        touchwin(wScore);
        wrefresh(wScore);
    }
}

void drawSpeed(void)
{
    static int oldSpeed = -1;
    if (oldSpeed != speed) {
        oldSpeed = speed;

        wclear(wSpeed);
        box(wSpeed, ACS_VLINE, ACS_HLINE);

        if (hasColors) {
            wattron(wSpeed, COLOR_PAIR(COLOR_PAIR_SPEED));
        }

        int i;
        for (i = 0; i < SPEEDS_COUNT; i++) {
            if (speed <= speedList[i]) {
                mvwaddch(wSpeed, 1, 1+i, ACS_BLOCK);
            }
            else {
                break;
            }
        }

        if (hasColors) {
            wattroff(wSpeed, COLOR_PAIR(COLOR_PAIR_SPEED));
        }

        mvwprintw(wSpeed, 0, 2, "SPEED");

        touchwin(wSpeed);
        wrefresh(wSpeed);
    }
}

void drawShadow(void)
{
    if (hasColors) {
        wattron(wField, COLOR_PAIR(COLOR_PAIR_SHADOW));
    }

    int i;
    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        if (shadowCellsPos[i].x >= 0 && shadowCellsPos[i].x >= 0) {
            mvwaddch(wField, shadowCellsPos[i].y,
                    shadowCellsPos[i].x, ACS_BLOCK);
        }
    }

    if (hasColors) {
        wattroff(wField, COLOR_PAIR(COLOR_PAIR_SHADOW));
    }
}

void drawFigure()
{
    int colorPair;

    switch (figure) {
        case TetrominoI:
            colorPair = COLOR_PAIR_I;
            break;
        case TetrominoO:
            colorPair = COLOR_PAIR_O;
            break;
        case TetrominoT:
            colorPair = COLOR_PAIR_T;
            break;
        case TetrominoJ:
            colorPair = COLOR_PAIR_J;
            break;
        case TetrominoL:
            colorPair = COLOR_PAIR_L;
            break;
        case TetrominoS:
            colorPair = COLOR_PAIR_S;
            break;
        case TetrominoZ:
            colorPair = COLOR_PAIR_Z;
            break;
        case TetrominoNone:
        case TetrominoInit:
            return;
    }

    if (hasColors) {
        wattron(wField, COLOR_PAIR(colorPair));
    }

    int i;
    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        if (figureCellsPos[i].x >= 0 && figureCellsPos[i].x >= 0) {
            mvwaddch(wField, figureCellsPos[i].y,
                     figureCellsPos[i].x, ACS_BLOCK);
        }
    }

    if (hasColors) {
        wattroff(wField, COLOR_PAIR(colorPair));
    }
}

void drawNextFigure(void)
{
    static Tetromino oldNextFigure = TetrominoNone;
    if (nextFigure != oldNextFigure) {
        oldNextFigure = nextFigure;

        wclear(wNextFigure);
        box(wNextFigure, ACS_VLINE, ACS_HLINE);

        int colorPair = -1;
        Point pos[FIGURE_CELL_COUNT];

        switch (nextFigure) {
            case TetrominoI:
                colorPair = COLOR_PAIR_I;
                pos[0].x = 2;
                pos[0].y = 3;
                pos[1].x = 3;
                pos[1].y = 3;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoO:
                colorPair = COLOR_PAIR_O;
                pos[0].x = 3;
                pos[0].y = 2;
                pos[1].x = 4;
                pos[1].y = 2;
                pos[2].x = 3;
                pos[2].y = 3;
                pos[3].x = 4;
                pos[3].y = 3;
                break;
            case TetrominoT:
                colorPair = COLOR_PAIR_T;
                pos[0].x = 4;
                pos[0].y = 2;
                pos[1].x = 3;
                pos[1].y = 3;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoJ:
                colorPair = COLOR_PAIR_J;
                pos[0].x = 3;
                pos[0].y = 2;
                pos[1].x = 3;
                pos[1].y = 3;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoL:
                colorPair = COLOR_PAIR_L;
                pos[0].x = 5;
                pos[0].y = 2;
                pos[1].x = 3;
                pos[1].y = 3;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoS:
                colorPair = COLOR_PAIR_S;
                pos[0].x = 4;
                pos[0].y = 2;
                pos[1].x = 5;
                pos[1].y = 2;
                pos[2].x = 3;
                pos[2].y = 3;
                pos[3].x = 4;
                pos[3].y = 3;
                break;
            case TetrominoZ:
                colorPair = COLOR_PAIR_Z;
                pos[0].x = 3;
                pos[0].y = 2;
                pos[1].x = 4;
                pos[1].y = 2;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoNone:
            case TetrominoInit:
                break;
        }

        if (colorPair >= 0) {
            if (hasColors) {
                wattron(wNextFigure, COLOR_PAIR(colorPair));
            }

            int i;
            for (i = 0; i < FIGURE_CELL_COUNT; i++) {
                mvwaddch(wNextFigure, pos[i].y, pos[i].x, ACS_BLOCK);
            }

            if (hasColors) {
                wattroff(wNextFigure, COLOR_PAIR(colorPair));
            }
        }

        mvwprintw(wNextFigure, 0, 2, "NEXT");

        touchwin(wNextFigure);
        wrefresh(wNextFigure);
    }
}

void drawStoredFigure(void)
{
    static Tetromino oldStoredFigure = TetrominoNone;
    if (storedFigure != oldStoredFigure) {
        oldStoredFigure = storedFigure;

        wclear(wStoredFigure);
        box(wStoredFigure, ACS_VLINE, ACS_HLINE);

        int colorPair = -1;
        Point pos[FIGURE_CELL_COUNT];

        switch (storedFigure) {
            case TetrominoI:
                colorPair = COLOR_PAIR_I;
                pos[0].x = 2;
                pos[0].y = 3;
                pos[1].x = 3;
                pos[1].y = 3;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoO:
                colorPair = COLOR_PAIR_O;
                pos[0].x = 3;
                pos[0].y = 2;
                pos[1].x = 4;
                pos[1].y = 2;
                pos[2].x = 3;
                pos[2].y = 3;
                pos[3].x = 4;
                pos[3].y = 3;
                break;
            case TetrominoT:
                colorPair = COLOR_PAIR_T;
                pos[0].x = 4;
                pos[0].y = 2;
                pos[1].x = 3;
                pos[1].y = 3;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoJ:
                colorPair = COLOR_PAIR_J;
                pos[0].x = 3;
                pos[0].y = 2;
                pos[1].x = 3;
                pos[1].y = 3;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoL:
                colorPair = COLOR_PAIR_L;
                pos[0].x = 5;
                pos[0].y = 2;
                pos[1].x = 3;
                pos[1].y = 3;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoS:
                colorPair = COLOR_PAIR_S;
                pos[0].x = 4;
                pos[0].y = 2;
                pos[1].x = 5;
                pos[1].y = 2;
                pos[2].x = 3;
                pos[2].y = 3;
                pos[3].x = 4;
                pos[3].y = 3;
                break;
            case TetrominoZ:
                colorPair = COLOR_PAIR_Z;
                pos[0].x = 3;
                pos[0].y = 2;
                pos[1].x = 4;
                pos[1].y = 2;
                pos[2].x = 4;
                pos[2].y = 3;
                pos[3].x = 5;
                pos[3].y = 3;
                break;
            case TetrominoNone:
            case TetrominoInit:
                break;
        }

        if (colorPair >= 0) {
            if (hasColors) {
                wattron(wStoredFigure, COLOR_PAIR(colorPair));
            }

            int i;
            for (i = 0; i < FIGURE_CELL_COUNT; i++) {
                mvwaddch(wStoredFigure, pos[i].y, pos[i].x, ACS_BLOCK);
            }

            if (hasColors) {
                wattroff(wStoredFigure, COLOR_PAIR(colorPair));
            }
        }

        mvwprintw(wStoredFigure, 0, 1, "STORED");

        touchwin(wStoredFigure);
        wrefresh(wStoredFigure);
    }
}

void work(void)
{
    int i = 0;
    while (i < MAX_KEY_COUNT && keys[i] != 0) {
        switch (keys[i]) {
            case CBUTTON_EXIT:
                exitGame();
                break;
            case CBUTTON_ROTCCW:
                rotateCounterclockwise();
                break;
            case CBUTTON_ROTCW:
                rotateClockwise();
                break;
            case CBUTTON_DROP:
                dropDown();
                break;
            case CBUTTON_LEFT:
                moveLeft();
                break;
            case CBUTTON_DOWN:
                moveDown();
                break;
            case CBUTTON_RIGHT:
                moveRight();
                break;
            case CBUTTON_PAUSE:
                pauseGame();
                break;
            case CBUTTON_NEWGAME:
                newGame();
                break;
            case CBUTTON_STORAGE:
                storageFigure();
                break;
        }
        i++;
    }

    if (!isGameOver && !isPaused && !(workCount%speed)) {
        if (!moveDown() && !isMoving) {
            deployFigure();
        }
    }

    if (isMoving > 0) {
        isMoving--;
    }

    workCount++;
}

void setNewRotationClockwise(void)
{
    int i;
    for (i = 1; i < FIGURE_CELL_COUNT; i++) {
        figureCellsPos[i] = rotatePoint(figureCellsPos[i], figureCellsPos[0],
                                        Clockwise);
    }

    updateShadowPosition();

    isMoving = 15;
    fieldRedrawNeeded = 1;
}

void setNewRotationCounterclockwise(void)
{
    int i;
    for (i = 1; i < FIGURE_CELL_COUNT; i++) {
        figureCellsPos[i] = rotatePoint(figureCellsPos[i], figureCellsPos[0],
                                        Counterclockwise);
    }

    updateShadowPosition();

    isMoving = 15;
    fieldRedrawNeeded = 1;
}

Point rotatePoint(Point point, Point origin, Rotation direction)
{
    Point retVal;

    retVal.x = origin.y - point.y;
    retVal.y = origin.x - point.x;

    switch (direction) {
        case Clockwise:
            retVal.y = -retVal.y;
            break;
        case Counterclockwise:
            retVal.x = -retVal.x;
            break;
    }

    retVal.x += origin.x;
    retVal.y += origin.y;

    return retVal;
}

void rotateClockwise(void)
{
    if (isGameOver || isPaused) {
        return;
    }

    if (figure == TetrominoO) {
        return;
    }

    if (canBeRotatedClockwise()) {
        setNewRotationClockwise();
        return;
    }

    moveLeft();
    if (canBeRotatedClockwise()) {
        setNewRotationClockwise();
        return;
    }
    moveRight();

    moveRight();
    if (canBeRotatedClockwise()) {
        setNewRotationClockwise();
        return;
    }
    moveLeft();

    moveUp();
    if (canBeRotatedClockwise()) {
        setNewRotationClockwise();
        return;
    }
    moveDown();
}

void rotateCounterclockwise(void)
{
    if (isGameOver || isPaused) {
        return;
    }

    if (figure == TetrominoO) {
        return;
    }

    if (canBeRotatedCounterclockwise()) {
        setNewRotationCounterclockwise();
        return;
    }

    moveLeft();
    if (canBeRotatedCounterclockwise()) {
        setNewRotationCounterclockwise();
        return;
    }
    moveRight();

    moveRight();
    if (canBeRotatedCounterclockwise()) {
        setNewRotationCounterclockwise();
        return;
    }
    moveLeft();

    moveUp();
    if (canBeRotatedCounterclockwise()) {
        setNewRotationCounterclockwise();
        return;
    }
    moveDown();
}

int canBeRotatedClockwise(void)
{
    int i;
    for (i = 1; i < FIGURE_CELL_COUNT; i++) {
        Point p = rotatePoint(figureCellsPos[i], figureCellsPos[0],
                              Clockwise);
        if (isCellFilled(p.x, p.y)) {
            return 0;
        }
    }

    return 1;
}

int canBeRotatedCounterclockwise(void)
{
    int i;
    for (i = 1; i < FIGURE_CELL_COUNT; i++) {
        Point p = rotatePoint(figureCellsPos[i], figureCellsPos[0],
                              Counterclockwise);
        if (isCellFilled(p.x, p.y)) {
            return 0;
        }
    }

    return 1;
}

int moveUp(void)
{
    if (isGameOver || isPaused) {
        return 0;
    }

    if (canBeMovedUp()) {
        int i;
        for (i = 0; i < FIGURE_CELL_COUNT; i++) {
            figureCellsPos[i].y--;
        }

        isMoving = 15;
        fieldRedrawNeeded = 1;
        return 1;
    }

    return 0;
}

int moveRight(void)
{
    if (isGameOver || isPaused) {
        return 0;
    }

    if (canBeMovedRight()) {
        int i;
        for (i = 0; i < FIGURE_CELL_COUNT; i++) {
            figureCellsPos[i].x++;
        }

        updateShadowPosition();

        isMoving = 15;
        fieldRedrawNeeded = 1;
        return 1;
    }

    return 0;
}

int moveDown(void)
{
    if (isGameOver || isPaused) {
        return 0;
    }

    if (canBeMovedDown()) {
        int i;
        for (i = 0; i < FIGURE_CELL_COUNT; i++) {
            figureCellsPos[i].y++;
        }

        isMoving = 15;
        fieldRedrawNeeded = 1;
        return 1;
    }

    return 0;
}

int moveLeft(void)
{
    if (isGameOver || isPaused) {
        return 0;
    }

    if (canBeMovedLeft()) {
        int i;
        for (i = 0; i < FIGURE_CELL_COUNT; i++) {
            figureCellsPos[i].x--;
        }

        updateShadowPosition();

        isMoving = 15;
        fieldRedrawNeeded = 1;
        return 1;
    }

    return 0;
}

void dropDown(void)
{
    if (isGameOver || isPaused) {
        return;
    }

    if (isGameOver || isPaused) {
        return;
    }

    while (canBeMovedDown()) {
        int i;
        for (i = 0; i < FIGURE_CELL_COUNT; i++) {
            figureCellsPos[i].y++;
        }

        fieldRedrawNeeded = 1;
    }

    deployFigure();
}

int canBeMovedUp(void)
{
    int i;
    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        if (figureCellsPos[i].y-1 < 0) {
            return 0;
        }

        if (isCellFilled(figureCellsPos[i].x, figureCellsPos[i].y-1)) {
            return 0;
        }
    }
    return 1;
}

int canBeMovedRight(void)
{
    int i;
    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        if (figureCellsPos[i].x+1 >= FIELD_WIDTH) {
            return 0;
        }
        if (isCellFilled(figureCellsPos[i].x+1, figureCellsPos[i].y)) {
            return 0;
        }
    }
    return 1;
}

int canBeMovedDown(void)
{
    int i;
    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        if (figureCellsPos[i].y-1 >= FIELD_HEIGHT) {
            return 0;
        }
        if (isCellFilled(figureCellsPos[i].x, figureCellsPos[i].y+1)) {
            return 0;
        }
    }
    return 1;
}

int canBeMovedLeft(void)
{
    int i;
    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        if (figureCellsPos[i].x-1 < 0) {
            return 0;
        }
        if (isCellFilled(figureCellsPos[i].x-1, figureCellsPos[i].y)) {
            return 0;
        }
    }
    return 1;
}


int keyWasPressed(int key)
{
    int i = 0;
    while (i < MAX_KEY_COUNT && keys[i] != 0) {
        if (keys[i] == key) {
            return 1;
        }
        i++;
    }

    return 0;
}

void deployFigure(void)
{
    int i;
    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        setCellFilling(figureCellsPos[i].x, figureCellsPos[i].y, figure);
    }
    storageUsed = 0;
    workCount = 0;
    checkForFilledLines();
    newFigure();
}

void newFigure(void)
{
    if (nextFigure == TetrominoInit || nextFigure == TetrominoNone) {
        nextFigure = randomTetromino();
    }

    figure = nextFigure;
    nextFigure = randomTetromino();

    moveFigureToDefaultPosition();
    updateShadowPosition();
    fieldRedrawNeeded = 1;

    if (!canBeMovedDown()) {
        isGameOver = 1;
    }
}

Tetromino randomTetromino(void)
{
    Tetromino tetramino = TetrominoNone;

    int total = 0;

    do {
        total = chanceI + chanceO + chanceT +
                chanceJ + chanceL + chanceS + chanceZ;
        if (total == 0) {
            chanceI = 15;
            chanceO = 15;
            chanceT = 15;
            chanceJ = 15;
            chanceL = 15;
            chanceS = 15;
            chanceZ = 15;

        }
    } while (total == 0);

    int value = rand()%total;

    chanceI += 2;
    chanceO += 2;
    chanceT += 2;
    chanceJ += 2;
    chanceL += 2;
    chanceS += 2;
    chanceZ += 2;

    int dChance = 0;
    int *pChance = NULL;

    if (value < chanceI) {
        tetramino = TetrominoI;
        pChance = &chanceI;
    }
    else if (value < (chanceI + chanceO)) {
        tetramino = TetrominoO;
        pChance = &chanceO;
    }
    else if (value < (chanceI + chanceO + chanceT)) {
        tetramino = TetrominoT;
        pChance = &chanceT;
    }
    else if (value < (chanceI + chanceO + chanceT + chanceJ)) {
        tetramino = TetrominoJ;
        pChance = &chanceJ;
    }
    else if (value < (chanceI + chanceO + chanceT + chanceJ + chanceL)) {
        tetramino = TetrominoL;
        pChance = &chanceL;
    }
    else if (value < (chanceI + chanceO + chanceT + chanceJ + chanceL +
                      chanceS)) {
        tetramino = TetrominoS;
        pChance = &chanceS;
    }
    else if (value < (chanceI + chanceO + chanceT + chanceJ + chanceL +
                      chanceS + chanceZ)) {
        tetramino = TetrominoZ;
        pChance = &chanceZ;
    }

    if (pChance != NULL) {
        *pChance -= 14;
        if (*pChance < 0)  {
            dChance = -*pChance;
            *pChance = 0;
        }
    }

    while (dChance) {
        int i = rand()%7;

        if (*chances[i] > 0) {
            (*chances[i])--;
            dChance--;
        }
    }

    return tetramino;
}

void checkForFilledLines(void)
{
    int filledCount = 0;
    int x;
    int y;
    for (y = FIELD_HEIGHT-2; y > 0; y--) {
        int filled = 1;
        for (x = 1; x < FIELD_WIDTH-1; x++) {
            if (!isCellFilled(x, y)) {
                filled = 0;
                break;
            }
        }
        if (filled) {
            filledCount++;
            int xx;
            int yy;
            for (yy = y; yy > 0; yy--) {
                for (xx = 1; xx < FIELD_WIDTH-1; xx++) {
                    setCellFilling(xx, yy, isCellFilled(xx, yy-1));
                }
            }
            y++;
        }
    }
    switch (filledCount) {
        case 1:
            score += 1;
            updateSpeed();
            break;
        case 2:
            score += 3;
            updateSpeed();
            break;
        case 3:
            score += 7;
            updateSpeed();
            break;
        case 4:
            score += 15;
            updateSpeed();
            break;
        default:
            break;
    }

    fieldRedrawNeeded = 1;
}

void updateSpeed(void)
{
    int i;
    for (i = 0; i < SPEEDS_COUNT; i++) {
        if (score > scoreList[i]) {
            speed = speedList[i];
        }
        else {
            break;
        }
    }
}

int isCellFilled(int x, int y)
{
    if (x < 0 || y < 0 || x >= FIELD_WIDTH || y >= FIELD_HEIGHT) {
        return 0;
    }

    return filledCells[x][y];
}

void setCellFilling(int x, int y, int filling)
{
    if (x < 0 || y < 0 || x >= FIELD_WIDTH || y >= FIELD_HEIGHT) {
        return;
    }

    filledCells[x][y] = filling;
}

void newGame(void)
{
    memset(filledCells, 0, sizeof(**filledCells)*(FIELD_WIDTH*FIELD_HEIGHT));

    int x;
    int y;
    for (x = 0; x < FIELD_WIDTH; x++) {
        setCellFilling(x, FIELD_HEIGHT-1, -1);
    }
    for (y = 0; y < FIELD_HEIGHT; y++) {
        setCellFilling(0, y, -1);
        setCellFilling(FIELD_WIDTH-1, y, -1);
    }

    figure = TetrominoInit;
    nextFigure = TetrominoInit;
    storedFigure = TetrominoInit;

    isGameOver = 0;
    isPaused = 0;

    speed = 25;
    score = 0;

    fieldRedrawNeeded = 1;

    isMoving = 0;

    workCount = 0;

    chanceI = 0;
    chanceO = 0;
    chanceT = 0;
    chanceJ = 0;
    chanceL = 0;
    chanceS = 0;
    chanceZ = 0;

    storageUsed = 0;

    newFigure();
}

void storageFigure(void)
{
    if (!storageUsed) {
        Tetromino tmp = figure;
        figure = storedFigure;
        storedFigure = tmp;

        if (figure == TetrominoInit || figure == TetrominoNone) {
            figure = randomTetromino();
        }

        moveFigureToDefaultPosition();
        updateShadowPosition();
        fieldRedrawNeeded = 1;

        storageUsed = 1;
        workCount = 0;
    }
}

int moveFigureToDefaultPosition(void)
{
    switch (figure) {
        case TetrominoI:
            figureCellsPos[0].x = 5;
            figureCellsPos[0].y = 0;
            figureCellsPos[1].x = 4;
            figureCellsPos[1].y = 0;
            figureCellsPos[2].x = 6;
            figureCellsPos[2].y = 0;
            figureCellsPos[3].x = 7;
            figureCellsPos[3].y = 0;
            break;
        case TetrominoO:
            figureCellsPos[0].x = 5;
            figureCellsPos[0].y = 0;
            figureCellsPos[1].x = 6;
            figureCellsPos[1].y = 0;
            figureCellsPos[2].x = 5;
            figureCellsPos[2].y = -1;
            figureCellsPos[3].x = 6;
            figureCellsPos[3].y = -1;
            break;
        case TetrominoT:
            figureCellsPos[0].x = 6;
            figureCellsPos[0].y = 0;
            figureCellsPos[1].x = 5;
            figureCellsPos[1].y = 0;
            figureCellsPos[2].x = 7;
            figureCellsPos[2].y = 0;
            figureCellsPos[3].x = 6;
            figureCellsPos[3].y = -1;
            break;
        case TetrominoJ:
            figureCellsPos[0].x = 6;
            figureCellsPos[0].y = 0;
            figureCellsPos[1].x = 5;
            figureCellsPos[1].y = 0;
            figureCellsPos[2].x = 7;
            figureCellsPos[2].y = 0;
            figureCellsPos[3].x = 5;
            figureCellsPos[3].y = -1;
            break;
        case TetrominoL:
            figureCellsPos[0].x = 6;
            figureCellsPos[0].y = 0;
            figureCellsPos[1].x = 5;
            figureCellsPos[1].y = 0;
            figureCellsPos[2].x = 7;
            figureCellsPos[2].y = 0;
            figureCellsPos[3].x = 7;
            figureCellsPos[3].y = -1;
            break;
        case TetrominoS:
            figureCellsPos[0].x = 6;
            figureCellsPos[0].y = 0;
            figureCellsPos[1].x = 5;
            figureCellsPos[1].y = 0;
            figureCellsPos[2].x = 6;
            figureCellsPos[2].y = -1;
            figureCellsPos[3].x = 7;
            figureCellsPos[3].y = -1;
            break;
        case TetrominoZ:
            figureCellsPos[0].x = 6;
            figureCellsPos[0].y = 0;
            figureCellsPos[1].x = 7;
            figureCellsPos[1].y = 0;
            figureCellsPos[2].x = 6;
            figureCellsPos[2].y = -1;
            figureCellsPos[3].x = 5;
            figureCellsPos[3].y = -1;
            break;
        case TetrominoNone:
        case TetrominoInit:
            break;
    }

    int i;
    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        if (isCellFilled(figureCellsPos[i].x, figureCellsPos[i].y)) {
            return 0;
        }
    }

    return 1;
}

void updateShadowPosition(void)
{
    int i;

    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        shadowCellsPos[i] = figureCellsPos[i];
    }

    while (shadowCellsPos[0].y < FIELD_HEIGHT) {
        for (i = 0; i < FIGURE_CELL_COUNT; i++) {
            shadowCellsPos[i].y++;
        }
        int isFilled = 0;
        for (i = 0; i < FIGURE_CELL_COUNT; i++) {
            if (isCellFilled(shadowCellsPos[i].x, shadowCellsPos[i].y)) {
                isFilled = 1;
            }
        }
        if (isFilled) {
            break;
        }
    }

    for (i = 0; i < FIGURE_CELL_COUNT; i++) {
        shadowCellsPos[i].y--;
    }
}

void pauseGame(void)
{
    isPaused = !isPaused;
    fieldRedrawNeeded = 1;
}

void exitGame(void)
{
    wclear(wField);
    wrefresh(wField);

    wclear(wScore);
    wrefresh(wScore);

    wclear(wSpeed);
    wrefresh(wSpeed);

    wclear(wNextFigure);
    wrefresh(wNextFigure);

    wclear(wStoredFigure);
    wrefresh(wStoredFigure);

    endwin();
    exit(0);
}
