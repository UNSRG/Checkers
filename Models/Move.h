#pragma once
#include <stdlib.h>

// “ип дл€ представлени€ позиции на доске (8 бит целое число)
typedef int8_t POS_T;

// —труктура дл€ представлени€ хода в игре
struct move_pos
{
    POS_T x, y;             //  оординаты начальной клетки (откуда)
    POS_T x2, y2;           //  оординаты конечной клетки (куда)
    POS_T xb = -1, yb = -1; //  оординаты вз€той фигуры (если есть)

    //  онструктор дл€ создани€ хода без вз€ти€ фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    //  онструктор дл€ создани€ хода с вз€тием фигуры
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }
    // ќператор сравнени€ на равенство двух ходов
    bool operator==(const move_pos &other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    // ќператор сравнени€ на неравенство двух ходов
    bool operator!=(const move_pos &other) const
    {
        return !(*this == other); // »спользуем оператор == дл€ определени€ неравенства
    }
};
