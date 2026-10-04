# Эмулятор «Мафия»

Проект для задания 1. Требуется компилятор с поддержкой C++20 и CMake 3.20+.

## Сборка

```sh
cmake -S . -B build
cmake --build build
```

План выполнения и выбранный минимально затратный маршрут к баллам находятся в [PLAN.txt](PLAN.txt).

## Параметры запуска (по заданию)

```sh
./build/mafia --players 10 --interactive --open-announcements --full-log
```
