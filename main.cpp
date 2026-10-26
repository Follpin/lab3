#include <iostream>
#include <syncstream>
#include <thread>
#include <latch>
#include <chrono>
#include <format>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// Параметри В-5:
// nt = 5, a = 4, b = 5, c = 9, d = 9, e = 4, f = 4, g = 4, h = 9, i = 4, j = 6, k = 2, m = 2 (не використовується), n = 3
constexpr int NT = 5;

// Точки синхронізації:
latch latch_v4(1); // Вузол V4: очікує завершення a (T1) -> відкриває d, g
latch latch_v5(2); // Вузол V5: очікує завершення e (T2) та b (T3) -> відкриває h, i
latch latch_v6(2); // Вузол V6: очікує завершення c (T4, T5) -> відкриває f, j
latch latch_v7(5); // Вузол V7: очікує завершення d, h, j (T1..T5) -> відкриває k
latch latch_v8(3); // Вузол V8: очікує завершення g (T5), i (T3), f (T4) -> відкриває n

void f(char x, int i)
{
    this_thread::sleep_for(1s);
    osyncstream(cout) << format("З набору {} виконано дію {}.\n", x, i);
}

void run_range(char x, int from, int to)
{
    for (int i = from; i <= to; ++i)
    {
        f(x, i);
    }
}

// Потік 1 (13 дій: a[1..4], d[1..8], k[1])
void worker1()
{
    run_range('a', 1, 4);
    latch_v4.count_down();

    run_range('d', 1, 8);
    latch_v7.arrive_and_wait();

    f('k', 1);
}

// Потік 2 (13 дій: e[1..4], d[9], h[1..7], k[2])
void worker2()
{
    run_range('e', 1, 4);
    latch_v5.count_down();

    latch_v4.wait();
    f('d', 9);

    latch_v5.wait();
    run_range('h', 1, 7);
    latch_v7.arrive_and_wait();

    f('k', 2);
}

// Потік 3 (13 дій: b[1..5], i[1..4], h[8..9], n[1..2])
void worker3()
{
    run_range('b', 1, 5);
    latch_v5.arrive_and_wait();

    run_range('i', 1, 4);
    latch_v8.count_down();

    run_range('h', 8, 9);
    latch_v7.count_down();

    latch_v8.wait();
    run_range('n', 1, 2);
}

// Потік 4 (12 дій: c[1..5], f[1..4], j[1..2], n[3])
void worker4()
{
    run_range('c', 1, 5);
    latch_v6.arrive_and_wait();

    run_range('f', 1, 4);
    latch_v8.count_down();

    run_range('j', 1, 2);
    latch_v7.count_down();

    latch_v8.wait();
    f('n', 3);
}

// Потік 5 (12 дій: c[6..9], g[1..4], j[3..6])
void worker5()
{
    run_range('c', 6, 9);
    latch_v6.count_down();

    latch_v4.wait();
    run_range('g', 1, 4);
    latch_v8.count_down();

    latch_v6.wait();
    run_range('j', 3, 6);
    latch_v7.count_down();
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    osyncstream(cout) << "Обчислення розпочато.\n";

    auto time1 = chrono::steady_clock::now();

    {
        jthread t1(worker1);
        jthread t2(worker2);
        jthread t3(worker3);
        jthread t4(worker4);
        jthread t5(worker5);
    }

    auto time2 = chrono::steady_clock::now();
    auto tdiff = time2 - time1;

    osyncstream(cout) << format("Обчислення завершено. Тривалість обчислення {:%S} секунд.\n", tdiff);

    return 0;
}
