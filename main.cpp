#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <vector>
#include <set>
#include <map>
#include <string.h>
#include <iostream>
#include <cassert>

struct COUNT_LINES {
    int x;
    int y;
};

struct CORDINATES{
    float x;
    float y;
};

struct COLOR{
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
};

struct MESSAGE_DATA{
    const char* title;
    const char* message;
    std::vector<const char*> list_but;
};

static SDL_Window* window;
static SDL_Renderer* render;
static int* but_ptr;

static int SCREEN_WIDTH; 
static int SCREEN_HEIGHT; 
static bool paus = true;
const int SIZE_CELL = 10;
static int fps = 0;
static COUNT_LINES SCREEN_NET;
const COLOR COLOR_SCREEN{80, 25 ,0, 255};
const COLOR COLOR_NET{0, 0 ,0, 255};
const COLOR COLOR_CELL{255, 0 ,0, 0};
std::map<int, std::vector<bool>> parent_cells;
std::map<int, std::vector<bool>> child_cells;
std::set<std::string> history_pos_cells;

MESSAGE_DATA message_start{"Game start", "The game starts with a pause. Place the colonies by pressing 'p'", {"Yes"}};
MESSAGE_DATA message_end{"Game end", "90", {"Yes"}};


void ShowMessageData(MESSAGE_DATA& message){
    SDL_MessageBoxData* message_data;
    const SDL_MessageBoxButtonData  buttons{
        SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, message.list_but[0]    
    };

    const SDL_MessageBoxColorScheme colorScheme{};

    const SDL_MessageBoxData box_data{
        SDL_MESSAGEBOX_INFORMATION,  
        window,  
        message.title,  
        message.message,  
        message.list_but.size(),  
        &buttons,  
        &colorScheme 
    };
    SDL_ShowMessageBox(&box_data, but_ptr);
}

int SDL_SetRenderDrawColor(SDL_Renderer* renderer, const COLOR& color){
    return SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void AddCell(int height, int width, std::map<int, std::vector<bool>>& cells){
   cells[height][width] = true;
}

void ClearCells(std::map<int, std::vector<bool>>& cells, int height, int width){
    for (int y = 0; y < height; y++){
        cells[y].clear();
        cells[y].resize(width);
    } 
}

void DrowScrinNet(){
    SDL_SetRenderDrawColor(render, COLOR_NET); 
    for (int x = 0; x < SCREEN_NET.x; x++){
        SDL_FPoint lines_x[2];
        lines_x[0] = SDL_FPoint{static_cast<float>(x * SIZE_CELL), 0};
        lines_x[1] = SDL_FPoint{static_cast<float>(x * SIZE_CELL), static_cast<float>(SCREEN_HEIGHT)};
        SDL_RenderLines(render, lines_x, 2);
    }
    for (int y = 0; y < SCREEN_NET.y; y++){
        SDL_FPoint lines_y[2];
        lines_y[0] = SDL_FPoint{0, static_cast<float>(y * SIZE_CELL)};
        lines_y[1] = SDL_FPoint{static_cast<float>(SCREEN_WIDTH), static_cast<float>(y * SIZE_CELL)};
        SDL_RenderLines(render, lines_y, 2);
    }
}

void DrowCell(CORDINATES& pos){
    SDL_FPoint lines_r[2];
    lines_r[0] = SDL_FPoint{pos.x * SIZE_CELL, pos.y * SIZE_CELL};
    lines_r[1] = SDL_FPoint{pos.x * SIZE_CELL + SIZE_CELL, pos.y * SIZE_CELL+ SIZE_CELL};
    SDL_RenderLines(render, lines_r, 2);
    SDL_FPoint lines_l[2];
    lines_l[0] = SDL_FPoint{pos.x * SIZE_CELL + SIZE_CELL, pos.y * SIZE_CELL};
    lines_l[1] = SDL_FPoint{pos.x * SIZE_CELL, pos.y * SIZE_CELL + SIZE_CELL};
    SDL_RenderLines(render, lines_l, 2);
}

void DrowCells(){
    SDL_SetRenderDrawColor(render, COLOR_CELL);
    for (int y = 0; y < SCREEN_NET.y; y++){
        for(int x = 0; x < SCREEN_NET.x; x++){
            if (parent_cells[y][x]){
                CORDINATES pos{x,y};
                DrowCell(pos);
            }
        }  
    } 
}

void SetPaus(){
    if (paus){
    paus = false;
    std::cout<< "not paus "<<std::endl;
    } else {
    paus = true;
     std::cout<< "paus "<<std::endl;
    }
}

int MatchWithCell(const int pos){
    return pos - pos % SIZE_CELL;
}

unsigned int CountNeighbors(const CORDINATES& pos_sell, const std::map<int, std::vector<bool>>& cells, COUNT_LINES& min, COUNT_LINES& max){
    //создадим лямбды для вычесление кординат левее и правее, с учетом что можем выйти за граници поля
    //тогда надо перейти на обратную сторону 
    auto right_cordinate { [](int cordinate, int max, int min) -> int {return cordinate + 1  >= max ? min: cordinate + 1;;} };
    auto left_cordinate { [](int cordinate, int max, int min) -> int {return cordinate - 1  <= min ? max: cordinate - 1;;} };
    //для избежание ошибки прохода циклом, создадим линию из готовых кординат по x и по y
    std::set<int> line_x;
    std::set<int> line_y;
    unsigned int count_heighbors = 0;
    line_x.insert(left_cordinate(pos_sell.x, max.x - 1, min.x));
    line_x.insert(pos_sell.x);
    line_x.insert(right_cordinate(pos_sell.x, max.x - 1, min.x));

    line_y.insert(left_cordinate(pos_sell.y, max.y - 1, min.y));
    line_y.insert(pos_sell.y);
    line_y.insert(right_cordinate(pos_sell.y, max.y - 1, min.y));

    for (int y: line_y){
        for (int x: line_x){//проверим что этот сосед и что сосед жев
            if ((y != pos_sell.y || x != pos_sell.x) && cells.at(y).at(x)){
                count_heighbors++;

            }
        }
    }
    return count_heighbors;
} 

std::string CountChild(){
    //считаем новое поколение клеток на основе текущего
    //если клетка мертва то при наличии болие или равном трех соседей она оживает
    //если жива, то продолжает жить при наличии двух или трех живых клеток рядом
    COUNT_LINES min{0, 0};
    COUNT_LINES max{SCREEN_NET.x, SCREEN_NET.y};
    std::string now_pos_sells;
    for (int  y = 0; y < SCREEN_NET.y; y++){
        for (int x = 0; x < SCREEN_NET.x; x++){
            CORDINATES pos{x,y};
            unsigned int count_heighbors = CountNeighbors(pos, parent_cells, min, max);
            if (parent_cells.at(y).at(x) && count_heighbors >=2 && parent_cells.at(y).at(x) && count_heighbors <= 3){
                //если клетка имеет от 2 до 3 она продолжит жить
                AddCell(y, x, child_cells);
                now_pos_sells+= std::to_string(y);
                now_pos_sells+= std::to_string(x);
            } else if (!parent_cells.at(y).at(x) && count_heighbors >= 3){ 
                //но если она не жива и ее соседей больше или равно  3 она оживает
                AddCell(y, x, child_cells);
                now_pos_sells+= std::to_string(y);
                now_pos_sells+= std::to_string(x);
            }
        }
    }
    return now_pos_sells;
}

SDL_AppResult UpdateGeneration(){
    std::string now_pos_sells = CountChild();
    if (history_pos_cells.find(now_pos_sells) != history_pos_cells.end()){
        //если ситуация повторилась
        return SDL_AppResult::SDL_APP_FAILURE;
    } else if (!child_cells.size()){
        //или нет жизни
        return SDL_AppResult::SDL_APP_FAILURE;
    }
    history_pos_cells.insert(now_pos_sells);
    ClearCells(parent_cells, SCREEN_NET.y, SCREEN_NET.x);
    parent_cells = child_cells;
    ClearCells(child_cells, SCREEN_NET.y, SCREEN_NET.x);
    return SDL_AppResult::SDL_APP_CONTINUE;
}

int GetFPS(int fps){
    return 1000 / fps;
}

void Test_plane(){
    std::cout << "Test plane empty" << std::endl;
    std::map<int, std::vector<bool>> cells;
    ClearCells(cells, 0, 0);
   
    assert(cells.size() == 0);
    std::cout << "Done" << std::endl;

    std::cout << "Test plane one cell" << std::endl;
    std::map<int, std::vector<bool>> cells_0;
    ClearCells(cells_0, 1, 1);
   
    assert(cells_0.size() == 1);
    assert(cells_0.at(0).size() == 1);
    std::cout << "Done" << std::endl;

    std::cout << "Test plane one line" << std::endl;
    std::map<int, std::vector<bool>> cells_1;
    ClearCells(cells_1, 1, 3);
   
    assert(cells_1.size() == 1);
    assert(cells_1.at(0).size() == 3);
    std::cout << "Done" << std::endl;
}

void Test_CountChild_1(){
    std::cout << "Test line one heighbor" << std::endl;
    COUNT_LINES min{0, 0};
    COUNT_LINES max{3, 1};
    std::map<int, std::vector<bool>> cells;
    ClearCells(cells, max.y, max.x);
    AddCell(0, 1, cells);
    assert(CountNeighbors({0, 0,}, cells, min, max) == 1);//правый 
    assert(CountNeighbors({1, 0,}, cells, min, max) == 0);//средний 
    assert(CountNeighbors({2, 0,}, cells, min, max) == 1);//левый
    std::cout << "Done" << std::endl;
}

SDL_AppResult SDL_AppInit(void** appatate, int args, char *argv[]){
    Test_plane();
    Test_CountChild_1();
    if (args != 4){
        std::cout<< "The application launch parameters were not specified or their number does not correspond to the required ones";
        return SDL_APP_FAILURE;
    }
    SCREEN_WIDTH =  MatchWithCell(atoi(argv[1]));
    SCREEN_HEIGHT = MatchWithCell(atoi(argv[2]));
    SCREEN_NET.x = SCREEN_WIDTH / 10;
    SCREEN_NET.y = SCREEN_HEIGHT / 10;
    fps = atoi(argv[3]);
    ClearCells(parent_cells, SCREEN_NET.y, SCREEN_NET.x);
    ClearCells(child_cells, SCREEN_NET.y, SCREEN_NET.x);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer("Game of life", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &render);
    ShowMessageData(message_start);
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appatate){
    SDL_AppResult result = SDL_APP_CONTINUE;;
    SDL_SetRenderDrawColor(render, COLOR_SCREEN);//задаем цвет поля
    SDL_RenderClear(render);

    DrowScrinNet(); //рисуем сетку
    if (!paus){
        result = UpdateGeneration();
        std::string text = "Game over total generations = " + std::to_string(history_pos_cells.size());
        switch (result){
        case SDL_AppResult::SDL_APP_FAILURE:
            message_end.message = text.c_str();
            ShowMessageData(message_end);
            break;   
        default:
            break;
        }      
    }
    
    DrowCells(); //русуем жизнь
    SDL_RenderPresent(render); 
    SDL_Delay(GetFPS(fps));
    
    return result;
    
}

SDL_AppResult SDL_AppEvent(void* appatate, SDL_Event* event){
    switch (event->type){
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
        break;
    case SDL_EVENT_KEY_DOWN:
        switch (event->key.scancode){
        case SDL_SCANCODE_P:
            SetPaus();
            break;
        default:
            break;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        switch (event->button.button){
        case 1:
            if (paus){
                AddCell(MatchWithCell(event->button.y) / SIZE_CELL, MatchWithCell(event->button.x) / SIZE_CELL, parent_cells);  
            }  
            break;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appatate, SDL_AppResult resoult){
    switch (resoult){
    case SDL_AppResult::SDL_APP_FAILURE:
        std::cout<<"your game end "<<std::endl;
        break;
    default:
        break;
    }
}

