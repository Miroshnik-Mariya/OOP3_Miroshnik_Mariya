#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#define _CRT_SECURE_NO_WARNINGS

// Определили структуру студента
struct Student {
    unsigned kard; // номер зачетной книжки
    char name[21]; // фамилия студента
};

// Вспомогательная структура для хранения индексов
typedef struct IndexEntry {
    int index;           // Индекс записи в основном файле
    union {              // Полезная информация для сортировки
        unsigned kard;   // Используется для сортировки по полю kard
        size_t len;      // Используется для сортировки по длине имени
    };
} IndexEntry;

// Функция сравнения для сортировки по полю kard
static int compareByKard(const void* a, const void* b) {
    const IndexEntry* ia = (const IndexEntry*)a;
    const IndexEntry* ib = (const IndexEntry*)b;
    if (ia->kard < ib->kard) return -1;
    if (ia->kard > ib->kard) return 1;
    return 0;
}

// Функция сравнения для поиска по полю kard
static int compareSearchByKard(const void* key, const void* elem) {
    const unsigned* search_kard = (const unsigned*)key;
    const IndexEntry* index_elem = (const IndexEntry*)elem;
    if (*search_kard < index_elem->kard) return -1;
    if (*search_kard > index_elem->kard) return 1;
    return 0;
}

// Функция сравнения для сортировки по длине имени
static int compareByNameLen(const void* a, const void* b) {
    const IndexEntry* ia = (const IndexEntry*)a;
    const IndexEntry* ib = (const IndexEntry*)b;
    if (ia->len < ib->len) return -1;
    if (ia->len > ib->len) return 1;
    return 0;
}

// Функция вывода одной строки
void printStudent(struct Student* student) {
    printf("Зачётная книжка №%u, студент %s\n", student->kard, student->name);
}

// Генерация файла с записями студентов
void generateDataFile(const char* filename) {
    FILE* file;
    errno_t err = fopen_s(&file, filename, "wb");
    if (err != 0 || file == NULL) {
        fprintf(stderr, "Ошибка открытия файла.\n");
        return;
    }

    // Добавляем несколько записей вручную
    struct Student students[] = {
        {1001, "Иванов"},
        {1002, "Петрова"},
        {1003, "Смирнова"},
        {1004, "Васечкин"},
        {1005, "Петров"},
        {6101, "Мирошник"}
    };

    for (size_t i = 0; i < sizeof(students) / sizeof(students[0]); ++i) {
        fwrite(&students[i], sizeof(struct Student), 1, file);
    }

    fclose(file);
    printf("Файл данных создан успешно!\n");
}

// Создание индексных файлов
void createIndexes(const char* dataFilename, const char* idx1Filename, const char* idx2Filename) {
    FILE* dataFile;
    errno_t err = fopen_s(&dataFile, dataFilename, "rb");
    if (err != 0 || !dataFile) {
        perror("Ошибка открытия файла данных!");
        exit(EXIT_FAILURE);
    }

    // Получаем количество записей
    fseek(dataFile, 0L, SEEK_END);
    long numRecords = ftell(dataFile) / sizeof(struct Student);
    rewind(dataFile);

    // Выделяем память под массив индексов
    IndexEntry* indexes1 = (IndexEntry*)malloc(numRecords * sizeof(IndexEntry));
    IndexEntry* indexes2 = (IndexEntry*)malloc(numRecords * sizeof(IndexEntry));

    if (!indexes1 || !indexes2) {
        printf("Ошибка выделения памяти!\n");
        fclose(dataFile);
        free(indexes1);
        free(indexes2);
        return;
    }

    // Читаем каждую запись и формируем индексы
    for (long i = 0; i < numRecords; ++i) {
        struct Student s;
        size_t read = fread(&s, sizeof(struct Student), 1, dataFile);
        if (read != 1) {
            printf("Ошибка чтения записи %ld\n", i);
            break;
        }

        indexes1[i].index = i;
        indexes1[i].kard = s.kard;

        indexes2[i].index = i;
        indexes2[i].len = strlen(s.name);
    }

    // Сортируем индексы
    qsort(indexes1, numRecords, sizeof(IndexEntry), compareByKard);
    qsort(indexes2, numRecords, sizeof(IndexEntry), compareByNameLen);

    // Сохранение первого индексного файла (по полю kard)
    FILE* idx1File;
    err = fopen_s(&idx1File, idx1Filename, "wb");
    if (err != 0 || !idx1File) {
        perror("Ошибка открытия индексного файла 1!");
        free(indexes1);
        free(indexes2);
        fclose(dataFile);
        exit(EXIT_FAILURE);
    }
    fwrite(&numRecords, sizeof(long), 1, idx1File);
    fwrite(indexes1, sizeof(IndexEntry), numRecords, idx1File);
    fclose(idx1File);

    // Сохранение второго индексного файла (по длине полей name)
    FILE* idx2File;
    err = fopen_s(&idx2File, idx2Filename, "wb");
    if (err != 0 || !idx2File) {
        perror("Ошибка открытия индексного файла 2!");
        free(indexes1);
        free(indexes2);
        fclose(dataFile);
        exit(EXIT_FAILURE);
    }
    fwrite(&numRecords, sizeof(long), 1, idx2File);
    fwrite(indexes2, sizeof(IndexEntry), numRecords, idx2File);
    fclose(idx2File);

    free(indexes1);
    free(indexes2);
    fclose(dataFile);
    printf("Индексные файлы созданы успешно!\n");
}

// Нахождение записи по номеру зачётной книжки с помощью бинарного поиска
void findByCardNumber(const char* dataFilename, const char* idxFilename, unsigned cardNum) {
    FILE* dataFile;
    errno_t err = fopen_s(&dataFile, dataFilename, "rb");
    if (err != 0 || !dataFile) {
        perror("Ошибка открытия файла данных!");
        return;
    }

    FILE* idxFile;
    err = fopen_s(&idxFile, idxFilename, "rb");
    if (err != 0 || !idxFile) {
        perror("Ошибка открытия индексного файла!");
        fclose(dataFile);
        return;
    }

    // Получаем количество записей
    long numRecords;
    fread(&numRecords, sizeof(long), 1, idxFile);

    // Загружаем индексы в память
    IndexEntry* indexes = (IndexEntry*)malloc(numRecords * sizeof(IndexEntry));
    if (!indexes) {
        printf("Ошибка выделения памяти!\n");
        fclose(dataFile);
        fclose(idxFile);
        return;
    }

    fread(indexes, sizeof(IndexEntry), numRecords, idxFile);

    // Бинарный поиск
    IndexEntry* found = (IndexEntry*)bsearch(&cardNum, indexes, numRecords, sizeof(IndexEntry), compareSearchByKard);

    if (found != NULL) {
        // Переходим к нужной записи в файле данных
        fseek(dataFile, found->index * sizeof(struct Student), SEEK_SET);
        struct Student s;
        fread(&s, sizeof(struct Student), 1, dataFile);
        printStudent(&s);
    }
    else {
        printf("Запись с номером зачетки %u не найдена!\n", cardNum);
    }

    free(indexes);
    fclose(dataFile);
    fclose(idxFile);
}

// Вывод имен студентов в отсортированном по длине списке
void listNamesSortedByLength(const char* dataFilename, const char* idxFilename) {
    FILE* dataFile;
    errno_t err = fopen_s(&dataFile, dataFilename, "rb");
    if (err != 0 || !dataFile) {
        perror("Ошибка открытия файла данных!");
        return;
    }

    FILE* idxFile;
    err = fopen_s(&idxFile, idxFilename, "rb");
    if (err != 0 || !idxFile) {
        perror("Ошибка открытия индексного файла!");
        fclose(dataFile);
        return;
    }

    // Получаем количество записей
    long numRecords;
    fread(&numRecords, sizeof(long), 1, idxFile);

    // Загружаем индексы в память
    IndexEntry* indexes = (IndexEntry*)malloc(numRecords * sizeof(IndexEntry));
    if (!indexes) {
        printf("Ошибка выделения памяти!\n");
        fclose(dataFile);
        fclose(idxFile);
        return;
    }

    fread(indexes, sizeof(IndexEntry), numRecords, idxFile);

    printf("\nСписок студентов по длине фамилии:\n");

    // Проход по каждому индексу и получение записей
    for (long i = 0; i < numRecords; ++i) {
        fseek(dataFile, indexes[i].index * sizeof(struct Student), SEEK_SET);
        struct Student s;
        fread(&s, sizeof(struct Student), 1, dataFile);
        printf("Длина: %2zu | Фамилия: %-20s | Зачетка: %u\n", indexes[i].len, s.name, s.kard);
    }

    free(indexes);
    fclose(dataFile);
    fclose(idxFile);
}

// Главная программа
int main() {
    setlocale(LC_ALL, "");
    setlocale(LC_ALL, ".1251");
    printf("Лабораторная работа №3\nВыполнила студентка группы 6301-020302D\nМирошник Мария Леонидовна\nВариант №10\n\n");
    const char* dataFilename = "students.dat";
    const char* idx1Filename = "card_idx.idx";  // Индексация по номеру зачётной книжки
    const char* idx2Filename = "name_len_idx.idx";  // Индексация по длине имени

    // Генерируем тестовые данные
    generateDataFile(dataFilename);

    //Формируем два индексных файла
    createIndexes(dataFilename, idx1Filename, idx2Filename);

    //Найти запись по номеру зачётной книжки
    printf("\nПоиск студента по номеру зачетки:\n");
    unsigned searchCardNum = 1003;
    findByCardNumber(dataFilename, idx1Filename, searchCardNum);

    //Вывести список студентов, отсортированных по длине фамилии
    listNamesSortedByLength(dataFilename, idx2Filename);

    return 0;
}