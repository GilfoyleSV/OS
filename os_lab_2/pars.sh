#!/bin/bash

# Название исполняемого файла (после компиляции)
EXECUTABLE="./main"
# Файл для сохранения данных
DATA_FILE="timing_data2.csv"

# Заголовки для CSV
echo "Threads,Time_sec" > $DATA_FILE

# Диапазон количества потоков для тестирования
# Проверяем 1, 2, 4, 8, 16, 32, 64 потока
THREADS=(1 2 4 8 12 16 18 20)

for t in "${THREADS[@]}"; do
    if [ $t -gt 64 ]; then
        # Ограничение MAX_THREADS в вашем коде
        echo "Пропускаем $t: превышает MAX_THREADS (64)"
        continue 
    fi
    echo "Тестирование с $t потоками..."
    # Запуск программы и извлечение времени
    # 'grep' и 'awk' используются для поиска строки с "Время выполнения" и извлечения числа
    OUTPUT=$($EXECUTABLE $t)
    TIME=$(echo "$OUTPUT" | grep "Время выполнения" | awk '{print $3}')
    
    # Запись данных в CSV
    echo "$t,$TIME" >> $DATA_FILE
done

echo "Сбор данных завершен. Результаты сохранены в $DATA_FILE"