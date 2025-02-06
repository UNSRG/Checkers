#pragma once
#include <iostream>
#include <fstream>
#include <vector>

#include "../Models/Move.h"
#include "../Models/Project_path.h"

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#endif

using namespace std;

class Board
{
public:
    Board() = default; // Конструктор по умолчанию
    // Конструктор с заданными шириной и высотой окна
    Board(const unsigned int W, const unsigned int H) : W(W), H(H)
    {
    }

    // Метод для отрисовки начальной доски
    int start_draw()
    {
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) // Инициализация SDL2 библиотеки
        {
            print_exception("SDL_Init can't init SDL2 lib");
            return 1;
        }
        if (W == 0 || H == 0) // Если ширина или высота не заданы
        {
            SDL_DisplayMode dm;
            if (SDL_GetDesktopDisplayMode(0, &dm)) // Получаем размер экрана
            {
                print_exception("SDL_GetDesktopDisplayMode can't get desctop display mode");
                return 1;
            }
            W = min(dm.w, dm.h); // Устанавливаем минимальное значение из ширины и высоты экрана
            W -= W / 15; // Корректируем размер окна
            H = W; // Устанавливаем высоту равной ширине
        }
        win = SDL_CreateWindow("Checkers", 0, H / 30, W, H, SDL_WINDOW_RESIZABLE); // Создаем окно
        if (win == nullptr)
        {
            print_exception("SDL_CreateWindow can't create window");
            return 1;
        }
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); // Создаем рендерер
        if (ren == nullptr)
        {
            print_exception("SDL_CreateRenderer can't create renderer");
            return 1;
        }
        board = IMG_LoadTexture(ren, board_path.c_str()); // Загружаем текстуры игровой доски
        w_piece = IMG_LoadTexture(ren, piece_white_path.c_str()); // Загружаем текстуры белых фигур
        b_piece = IMG_LoadTexture(ren, piece_black_path.c_str()); // Загружаем текстуры черных фигур
        w_queen = IMG_LoadTexture(ren, queen_white_path.c_str()); // Загружаем текстуры белых дамок
        b_queen = IMG_LoadTexture(ren, queen_black_path.c_str()); // Загружаем текстуры черных дамок
        back = IMG_LoadTexture(ren, back_path.c_str()); // Загружаем текстуру кнопки "Назад"
        replay = IMG_LoadTexture(ren, replay_path.c_str()); // Загружаем текстуру кнопки "Повторить игру"
        if (!board || !w_piece || !b_piece || !w_queen || !b_queen || !back || !replay) // Проверка загрузки текстур
        {
            print_exception("IMG_LoadTexture can't load main textures from " + textures_path);
            return 1;
        }
        SDL_GetRendererOutputSize(ren, &W, &H); // Получаем размеры окна рендера
        make_start_mtx(); // Создаем начальную матрицу доски
        rerender(); // Перерисовываем доску
        return 0;
    }
    
    // Метод для перерисовки доски
    void redraw()
    {
        game_results = -1; // Сбрасываем результат игры
        history_mtx.clear(); // Очищаем историю ходов
        history_beat_series.clear(); // Очищаем серию взятий
        make_start_mtx(); // Создаем начальную матрицу доски
        clear_active(); // Сбрасываем активную клетку
        clear_highlight(); // Сбрасываем выделенные клетки
    }

    // Метод для перемещения фигуры на доске
    void move_piece(move_pos turn, const int beat_series = 0)
    {
        if (turn.xb != -1) // Если есть взятие фигуры
        {
            mtx[turn.xb][turn.yb] = 0; // Удаляем взятую фигуру
        }
        move_piece(turn.x, turn.y, turn.x2, turn.y2, beat_series); // Выполняем ход
    }

    // Метод для перемещения фигуры на доске по координатам
    void move_piece(const POS_T i, const POS_T j, const POS_T i2, const POS_T j2, const int beat_series = 0)
    {
        if (mtx[i2][j2]) // Если конечная позиция занята
        {
            throw runtime_error("final position is not empty, can't move"); // Бросаем исключение
        }
        if (!mtx[i][j]) // Если начальная позиция пуста
        {
            throw runtime_error("begin position is empty, can't move"); // Бросаем исключение
        }
        if ((mtx[i][j] == 1 && i2 == 0) || (mtx[i][j] == 2 && i2 == 7)) // Преобразование в дамку при достижении противоположного края доски
            mtx[i][j] += 2;
        mtx[i2][j2] = mtx[i][j]; // Перемещаем фигуру
        drop_piece(i, j); // Удаляем фигуру с начальной позиции
        add_history(beat_series); // Добавляем ход в историю
    }

    // Метод для удаления фигуры с доски
    void drop_piece(const POS_T i, const POS_T j)
    {
        mtx[i][j] = 0; // Устанавливаем позицию как пустую
        rerender(); // Перерисовываем доску
    }

    // Метод для превращения фигуры в дамку
    void turn_into_queen(const POS_T i, const POS_T j)
    {
        if (mtx[i][j] == 0 || mtx[i][j] > 2) // Проверка возможности превращения в дамку
        {
            throw runtime_error("can't turn into queen in this position"); // Бросаем исключение
        }
        mtx[i][j] += 2; // Превращаем фигуру в дамку
        rerender(); // Перерисовываем доску
    }
    // Метод для получения текущей матрицы доски
    vector<vector<POS_T>> get_board() const
    {
        return mtx;
    }

    // Метод для выделения клеток на доске
    void highlight_cells(vector<pair<POS_T, POS_T>> cells)
    {
        for (auto pos : cells)
        {
            POS_T x = pos.first, y = pos.second;
            is_highlighted_[x][y] = 1; // Отмечаем клетку как выделенную
        }
        rerender(); // Перерисовываем доску
    }

    // Метод для сброса выделенных клеток
    void clear_highlight()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            is_highlighted_[i].assign(8, 0); // Сбрасываем все клетки как невыделенные
        }
        rerender(); // Перерисовываем доску
    }

    // Метод для установки активной клетки
    void set_active(const POS_T x, const POS_T y)
    {
        active_x = x;
        active_y = y;
        rerender(); // Перерисовываем доску
    }

    // Метод для сброса активной клетки
    void clear_active()
    {
        active_x = -1;
        active_y = -1;
        rerender(); // Перерисовываем доску
    }

    // Метод для проверки, выделена ли клетка
    bool is_highlighted(const POS_T x, const POS_T y)
    {
        return is_highlighted_[x][y];
    }

    // Метод для отката хода
    void rollback()
    {
        auto beat_series = max(1, *(history_beat_series.rbegin())); // Получаем последнюю серию взятий
        while (beat_series-- && history_mtx.size() > 1) // Откатываем ходы до начала серии взятий
        {
            history_mtx.pop_back();
            history_beat_series.pop_back();
        }
        mtx = *(history_mtx.rbegin()); // Восстанавливаем предыдущее состояние доски
        clear_highlight(); // Сбрасываем выделенные клетки
        clear_active(); // Сбрасываем активную клетку
    }

    // Метод для отображения результата игры
    void show_final(const int res)
    {
        game_results = res; // Устанавливаем результат игры
        rerender(); // Перерисовываем доску
    }

    // Метод для обновления размера окна
    void reset_window_size()
    {
        SDL_GetRendererOutputSize(ren, &W, &H); // Получаем новые размеры окна рендера
        rerender(); // Перерисовываем доску
    }

    // Метод для завершения работы SDL2
    void quit()
    {
        SDL_DestroyTexture(board); // Уничтожаем текстуры
        SDL_DestroyTexture(w_piece);
        SDL_DestroyTexture(b_piece);
        SDL_DestroyTexture(w_queen);
        SDL_DestroyTexture(b_queen);
        SDL_DestroyTexture(back);
        SDL_DestroyTexture(replay);
        SDL_DestroyRenderer(ren); // Уничтожаем рендерер
        SDL_DestroyWindow(win); // Уничтожаем окно
        SDL_Quit(); // Завершаем работу SDL2
    }
    
    // Деструктор
    ~Board()
    {
        if (win)
            quit(); // Завершаем работу SDL2 при уничтожении объекта
    }

private:
    // Метод для добавления хода в историю
    void add_history(const int beat_series = 0)
    {
        history_mtx.push_back(mtx); // Добавляем текущее состояние доски в историю
        history_beat_series.push_back(beat_series); // Добавляем серию взятий в историю
    }
    // Метод для создания начальной матрицы доски
    void make_start_mtx()
    {
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                mtx[i][j] = 0; // Сбрасываем все клетки как пустые
                if (i < 3 && (i + j) % 2 == 1) // Размещаем черные фигуры в нижней части доски
                    mtx[i][j] = 2;
                if (i > 4 && (i + j) % 2 == 1) // Размещаем белые фигуры в верхней части доски
                    mtx[i][j] = 1;
            }
        }
        add_history(); // Добавляем начальное состояние доски в историю
    }

    // Метод для перерисовки всех текстур на доске
    void rerender()
    {
        // draw board
        SDL_RenderClear(ren); // Очищаем рендерер
        SDL_RenderCopy(ren, board, NULL, NULL); // Рисуем доску

        // draw pieces
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!mtx[i][j])
                    continue;  // Пропускаем пустые клетки
                int wpos = W * (j + 1) / 10 + W / 120; // Вычисляем координаты для рисования фигуры
                int hpos = H * (i + 1) / 10 + H / 120;
                SDL_Rect rect{ wpos, hpos, W / 12, H / 12 };

                SDL_Texture* piece_texture;
                if (mtx[i][j] == 1) // Выбираем текстуру для рисования фигуры
                    piece_texture = w_piece;
                else if (mtx[i][j] == 2)
                    piece_texture = b_piece;
                else if (mtx[i][j] == 3)
                    piece_texture = w_queen;
                else
                    piece_texture = b_queen;

                SDL_RenderCopy(ren, piece_texture, NULL, &rect); // Рисуем фигуру
            }
        }

        // draw hilight
        SDL_SetRenderDrawColor(ren, 0, 255, 0, 0); // Устанавливаем цвет для выделения клеток
        const double scale = 2.5;
        SDL_RenderSetScale(ren, scale, scale); // Устанавливаем масштаб для выделения клеток
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (!is_highlighted_[i][j])
                    continue; // Пропускаем невыделенные клетки
                SDL_Rect cell{ int(W * (j + 1) / 10 / scale), int(H * (i + 1) / 10 / scale), int(W / 10 / scale),
                              int(H / 10 / scale) };
                SDL_RenderDrawRect(ren, &cell); // Рисуем выделенную клетку
            }
        }

        // draw active
        if (active_x != -1) // Рисуем активную клетку
        {
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 0);
            SDL_Rect active_cell{ int(W * (active_y + 1) / 10 / scale), int(H * (active_x + 1) / 10 / scale),
                                 int(W / 10 / scale), int(H / 10 / scale) };
            SDL_RenderDrawRect(ren, &active_cell);
        }
        SDL_RenderSetScale(ren, 1, 1);

        // draw arrows
        SDL_Rect rect_left{ W / 40, H / 40, W / 15, H / 15 }; // Рисуем кнопку "Назад"
        SDL_RenderCopy(ren, back, NULL, &rect_left);
        SDL_Rect replay_rect{ W * 109 / 120, H / 40, W / 15, H / 15 }; // Рисуем кнопку "Повторить игру"
        SDL_RenderCopy(ren, replay, NULL, &replay_rect);

        // draw result
        if (game_results != -1) // Рисуем результат игры
        {
            string result_path = draw_path;
            if (game_results == 1)
                result_path = white_path;
            else if (game_results == 2)
                result_path = black_path;
            SDL_Texture* result_texture = IMG_LoadTexture(ren, result_path.c_str());
            if (result_texture == nullptr)
            {
                print_exception("IMG_LoadTexture can't load game result picture from " + result_path);
                return;
            }
            SDL_Rect res_rect{ W / 5, H * 3 / 10, W * 3 / 5, H * 2 / 5 };
            SDL_RenderCopy(ren, result_texture, NULL, &res_rect);
            SDL_DestroyTexture(result_texture);
        }

        SDL_RenderPresent(ren); // Обновляем содержимое окна
        // next rows for mac os
        SDL_Delay(10); // Задержка для корректной работы на macOS
        SDL_Event windowEvent;
        SDL_PollEvent(&windowEvent);
    }

    // Метод для записи ошибок в лог-файл
    void print_exception(const string& text) {
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Error: " << text << ". " << SDL_GetError() << endl;
        fout.close();
    }

public:
    int W = 0; // Ширина окна
    int H = 0; // Высота окна
    // history of boards
    // История состояний доски
    vector<vector<vector<POS_T>>> history_mtx;

private:
    SDL_Window* win = nullptr; // Указатель на окно SDL2
    SDL_Renderer* ren = nullptr; // Указатель на рендерер SDL2
    // textures
    // Текстуры игровых элементов
    SDL_Texture* board = nullptr;
    SDL_Texture* w_piece = nullptr;
    SDL_Texture* b_piece = nullptr;
    SDL_Texture* w_queen = nullptr;
    SDL_Texture* b_queen = nullptr;
    SDL_Texture* back = nullptr;
    SDL_Texture* replay = nullptr;
    // texture files names
    // Пути к файлам текстур
    const string textures_path = project_path + "Textures/";
    const string board_path = textures_path + "board.png";
    const string piece_white_path = textures_path + "piece_white.png";
    const string piece_black_path = textures_path + "piece_black.png";
    const string queen_white_path = textures_path + "queen_white.png";
    const string queen_black_path = textures_path + "queen_black.png";
    const string white_path = textures_path + "white_wins.png";
    const string black_path = textures_path + "black_wins.png";
    const string draw_path = textures_path + "draw.png";
    const string back_path = textures_path + "back.png";
    const string replay_path = textures_path + "replay.png";
    // coordinates of chosen cell
    // Координаты выбранной клетки
    int active_x = -1, active_y = -1;
    // game result if exist
    // Результат игры
    int game_results = -1;
    // matrix of possible moves
    // Матрица выделенных клеток
    vector<vector<bool>> is_highlighted_ = vector<vector<bool>>(8, vector<bool>(8, 0));
    // matrix of possible moves
    // 1 - white, 2 - black, 3 - white queen, 4 - black queen
    // Матрица игрового поля
    // 1 - белая фигура, 2 - черная фигура, 3 - белая дамка, 4 - черная дамка
    vector<vector<POS_T>> mtx = vector<vector<POS_T>>(8, vector<POS_T>(8, 0));
    // series of beats for each move
    // Серии взятий для каждого хода
    vector<int> history_beat_series;
};