#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

//Структура студента
struct Student {
    unsigned kard; // номер зачетной книжки
    char name[21]; // фамилия студента

    // Конструктор по умолчанию
    Student() {
        kard = 0;
        for (int i = 0; i < 21; i++) {
            name[i] = '\0';
        }
    }

    // Конструктор с параметрами
    Student(unsigned k, const char* n) {
        kard = k;
        int i;
        for (i = 0; i < 20 && n[i] != '\0'; i++) {
            name[i] = n[i];
        }
        name[i] = '\0'; // Завершающий ноль
    }

    // Функция вывода Студента
    void printStudent() {
        printf("Зачётная книжка №%u, студент %s\n", kard, name);
    }
};

//Структура для хранения индексов
struct IndexEntry {
    int index;           // Индекс записи в файле
    union {           //Поля объекта Студент
        unsigned kard;
        size_t len;      // Длина фамилии
    };

    //Сравнение сортировки по полю kard 
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

    // Наполнение файла с записями студентов через конструкторы
    static void generateDataFile(const char* filename) {
        FILE* file;
        fopen_s(&file, filename, "wb");
        if (file == NULL) {
            printf("Ошибка открытия файла %s\n", filename);
            return;
        }

        // Создание студентов через конструкторы
        Student students[] = {
            Student(1001, "Иванов"),
            Student(1002, "Петрова"),
            Student(1003, "Смирнова"),
            Student(1004, "Васечкин"),
            Student(1005, "Петров"),
            Student(6101, "Мирошник")
        };

        for (size_t i = 0; i < sizeof(students) / sizeof(students[0]); ++i) {
            fwrite(&students[i], sizeof(Student), 1, file);
        }

        fclose(file);
        printf("Файл данных создан успешно!\n");
    }

    // Создание индексных файлов
    static int createIndexes(const char* dataFilename, const char* idx1Filename, const char* idx2Filename) {
        FILE* dataFile;
        fopen_s(&dataFile, dataFilename, "rb");
        if (dataFile == NULL) {
            printf("Ошибка открытия файла данных %s\n", dataFilename);
            return 0;
        }

        // Получаем количество записей с помощью fseek и ftell
        if (fseek(dataFile, 0L, SEEK_END) != 0) {
            printf("Ошибка позиционирования в файле!\n");
            fclose(dataFile);
            return 0;
        }

        long fileSize = ftell(dataFile);
        if (fileSize == -1L) {
            printf("Ошибка определения размера файла!\n");
            fclose(dataFile);
            return 0;
        }

        long numRecords = fileSize / sizeof(Student);

        if (fseek(dataFile, 0L, SEEK_SET) != 0) {
            printf("Ошибка перемотки файла!\n");
            fclose(dataFile);
            return 0;
        }

        printf("Размер файла: %ld байт, количество записей: %ld\n", fileSize, numRecords);

        // Выделяем память под массив индексов
        IndexEntry* indexes1 = (IndexEntry*)malloc(numRecords * sizeof(IndexEntry));
        IndexEntry* indexes2 = (IndexEntry*)malloc(numRecords * sizeof(IndexEntry));

        if (indexes1 == NULL || indexes2 == NULL) {
            printf("Ошибка выделения памяти!\n");
            fclose(dataFile);
            if (indexes1 != NULL) free(indexes1);
            if (indexes2 != NULL) free(indexes2);
            return 0;
        }

        // Читаем каждую запись и формируем индексы
        for (long i = 0; i < numRecords; ++i) {
            Student s;
            size_t read = fread(&s, sizeof(Student), 1, dataFile);
            if (read != 1) {
                printf("Ошибка чтения записи %ld\n", i);
                break;
            }
            indexes1[i].index = i;
            indexes1[i].kard = s.kard;

            indexes2[i].index = i;
            // Ручной подсчет длины строки
            int length = 0;
            while (length < 20 && s.name[length] != '\0') {
                length++;
            }
            indexes2[i].len = length;
        }

        // Сортируем индексы
        qsort(indexes1, numRecords, sizeof(IndexEntry), compareByKard);
        qsort(indexes2, numRecords, sizeof(IndexEntry), compareByNameLen);

        // Сохранение первого индексного файла (по полю kard)
        FILE* idx1File;
        fopen_s(&idx1File, idx1Filename, "wb");
        if (idx1File == NULL) {
            printf("Ошибка открытия индексного файла 1 %s\n", idx1Filename);
            free(indexes1);
            free(indexes2);
            fclose(dataFile);
            return 0;
        }

        if (fwrite(&numRecords, sizeof(long), 1, idx1File) != 1) {
            printf("Ошибка записи в индексный файл 1\n");
        }
        if (fwrite(indexes1, sizeof(IndexEntry), numRecords, idx1File) != numRecords) {
            printf("Ошибка записи индексов в файл 1\n");
        }
        fclose(idx1File);

        // Сохранение второго индексного файла (по длине полей name)
        FILE* idx2File;
        fopen_s(&idx2File, idx2Filename, "wb");
        if (idx2File == NULL) {
            printf("Ошибка открытия индексного файла 2 %s\n", idx2Filename);
            free(indexes1);
            free(indexes2);
            fclose(dataFile);
            return 0;
        }

        if (fwrite(&numRecords, sizeof(long), 1, idx2File) != 1) {
            printf("Ошибка записи в индексный файл 2\n");
        }
        if (fwrite(indexes2, sizeof(IndexEntry), numRecords, idx2File) != numRecords) {
            printf("Ошибка записи индексов в файл 2\n");
        }
        fclose(idx2File);

        free(indexes1);
        free(indexes2);
        fclose(dataFile);
        printf("Индексные файлы созданы успешно!\n");
        return 1;
    }

    // Нахождение записи по номеру зачётной книжки с помощью бинарного поиска
    static void findByCardNumber(const char* dataFilename, const char* idxFilename, unsigned cardNum) {
        FILE* dataFile;
        fopen_s(&dataFile, dataFilename, "rb");
        if (dataFile == NULL) {
            printf("Ошибка открытия файла данных %s\n", dataFilename);
            return;
        }

        FILE* idxFile;
        fopen_s(&idxFile, idxFilename, "rb");
        if (idxFile == NULL) {
            printf("Ошибка открытия индексного файла %s\n", idxFilename);
            fclose(dataFile);
            return;
        }

        // Получаем количество записей
        long numRecords;
        if (fread(&numRecords, sizeof(long), 1, idxFile) != 1) {
            printf("Ошибка чтения количества записей!\n");
            fclose(dataFile);
            fclose(idxFile);
            return;
        }

        // Загружаем индексы в память
        IndexEntry* indexes = (IndexEntry*)malloc(numRecords * sizeof(IndexEntry));
        if (indexes == NULL) {
            printf("Ошибка выделения памяти!\n");
            fclose(dataFile);
            fclose(idxFile);
            return;
        }

        if (fread(indexes, sizeof(IndexEntry), numRecords, idxFile) != numRecords) {
            printf("Ошибка чтения индексов!\n");
            free(indexes);
            fclose(dataFile);
            fclose(idxFile);
            return;
        }

        // Бинарный поиск
        IndexEntry* found = (IndexEntry*)bsearch(&cardNum, indexes, numRecords, sizeof(IndexEntry), compareSearchByKard);

        if (found != NULL) {
            // Переходим к нужной записи в файле данных
            if (fseek(dataFile, found->index * sizeof(Student), SEEK_SET) != 0) {
                printf("Ошибка позиционирования в файле данных!\n");
            }
            else {
                Student s;
                if (fread(&s, sizeof(Student), 1, dataFile) == 1) {
                    s.printStudent();
                }
                else {
                    printf("Ошибка чтения записи студента!\n");
                }
            }
        }
        else {
            printf("Запись с номером зачетки %u не найдена!\n", cardNum);
        }

        free(indexes);
        fclose(dataFile);
        fclose(idxFile);
    }

    // Вывод имен студентов в отсортированном по длине списке
    static void listNamesSortedByLength(const char* dataFilename, const char* idxFilename) {
        FILE* dataFile;
        fopen_s(&dataFile, dataFilename, "rb");
        if (dataFile == NULL) {
            printf("Ошибка открытия файла данных %s\n", dataFilename);
            return;
        }

        FILE* idxFile;
        fopen_s(&idxFile, idxFilename, "rb");
        if (idxFile == NULL) {
            printf("Ошибка открытия индексного файла %s\n", idxFilename);
            fclose(dataFile);
            return;
        }

        // Получаем количество записей
        long numRecords;
        if (fread(&numRecords, sizeof(long), 1, idxFile) != 1) {
            printf("Ошибка чтения количества записей!\n");
            fclose(dataFile);
            fclose(idxFile);
            return;
        }

        // Загружаем индексы в память
        IndexEntry* indexes = (IndexEntry*)malloc(numRecords * sizeof(IndexEntry));
        if (indexes == NULL) {
            printf("Ошибка выделения памяти!\n");
            fclose(dataFile);
            fclose(idxFile);
            return;
        }

        if (fread(indexes, sizeof(IndexEntry), numRecords, idxFile) != numRecords) {
            printf("Ошибка чтения индексов!\n");
            free(indexes);
            fclose(dataFile);
            fclose(idxFile);
            return;
        }

        printf("\nСписок студентов по длине фамилии:\n");
        printf("Длина | Фамилия            | Зачетка\n");

        // Проход по каждому индексу и получение записей
        for (long i = 0; i < numRecords; ++i) {
            if (fseek(dataFile, indexes[i].index * sizeof(Student), SEEK_SET) != 0) {
                printf("Ошибка позиционирования для записи %ld!\n", i);
                continue;
            }
            Student s;
            if (fread(&s, sizeof(Student), 1, dataFile) == 1) {
                printf("%5zu | %-18s | %u\n", indexes[i].len, s.name, s.kard);
            }
            else {
                printf("Ошибка чтения записи %ld!\n", i);
            }
        }

        free(indexes);
        fclose(dataFile);
        fclose(idxFile);
    }
};

// Главная программа
int main() {
    setlocale(LC_ALL, "");
    setlocale(LC_ALL, ".1251");
    printf("Лабораторная работа №3\nВыполнила студентка группы 6301-020302D\nМирошник Мария Леонидовна\nВариант №10\n\n");
    const char* dataFilename = "students.dat";
    const char* idx1Filename = "card_idx.idx";
    const char* idx2Filename = "name_len_idx.idx";

    // Наполняю файл данными через конструкторы
    IndexEntry::generateDataFile(dataFilename);

    //Формируем два индексных файла
    IndexEntry::createIndexes(dataFilename, idx1Filename, idx2Filename);

    //Найти запись по номеру зачётной книжки
    printf("\nПоиск студента по номеру зачетки:\n");
    unsigned searchCardNum = 1003;
    IndexEntry::findByCardNumber(dataFilename, idx1Filename, searchCardNum);

    //Вывести список студентов, отсортированных по длине фамилии
    IndexEntry::listNamesSortedByLength(dataFilename, idx2Filename);

    return 0;
}