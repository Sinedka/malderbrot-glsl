# Имя исполняемого файла
TARGET = my_program
BUILD_DIR = .build

# Компилятор и флаги
CXX = g++
CC = gcc
CXXFLAGS = -Wall -g -Iinclude
LDFLAGS = -lglfw -ldl -lGL

# Исходники
SRCS = main.cpp glad.c

# Автоматически создаём список объектных файлов в папке .build
OBJS = $(addprefix $(BUILD_DIR)/,$(SRCS:.cpp=.o))
OBJS := $(OBJS:.c=.o)  # для .c файлов

# Правило по умолчанию
all: $(BUILD_DIR) $(TARGET)

# Создаём папку .build, если её нет
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Сборка исполняемого файла из объектных файлов
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$(TARGET) $(OBJS) $(LDFLAGS)

# Сборка C++ объектных файлов
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Сборка C объектных файлов
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CXXFLAGS) -c $< -o $@

# Очистка
clean:
	rm -rf $(BUILD_DIR)

run:
	.build/my_program
