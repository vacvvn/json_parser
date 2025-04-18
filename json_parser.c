#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>



// #define ETHERNET_MODE 	0
// #define MA159_MODE 	0
// #define GREK_FCRT 	1

// Коды возвратов
#define SUCCESS					0		// Успешное завершение
#define DEF_ERROR				-1	    // Ошибка по умолчанию


// Флаг включения ВК
#define VC_OFF      0
#define VC_ON       1

// Тип сообщения ВК. L - низкоприоритетное, H - Высокоприоритетное
#define VC_TYPE_LOW     'L'
#define VC_TYPE_HIGH    'H'

#ifndef ETHERNET_MODE
// Тип дублирования. 0 - канал А, 1 - канал В, 2 - оба канала
#define VC_DUPLICATION_A        0
#define VC_DUPLICATION_B        1
#define VC_DUPLICATION_AB       2

// Тип канала. 0 - ВСРВ, 1 - ASM. Если ASM, то дублирование не может быть 2
#define VC_FCRT     0
#define VC_ASM      1
#endif

typedef void (*vector_foreach_t)(void *);

#ifdef ETHERNET_MODE
// Структура описания ВК регулярного сообщения
typedef struct
{
    char type;
    char client_ip[16];
    uint32_t client_rcv_port;
    uint32_t server_rcv_port;
    uint32_t server_snd_port;
    uint32_t priority;
    uint8_t enabled;
} vc_regular_data_t;

// Структура описания ВК периодического сообщения
typedef struct
{
    char type;
    char server_ip[16];
    char client_ip[16];
    uint32_t client_rcv_port;
    uint32_t server_snd_port;
    uint32_t period;
    uint32_t max_size;
    uint8_t enabled;
} vc_periodical_data_t;

// Структура данных из таблицы конфигурации
typedef struct
{
    uint32_t regular_overall_count;
    uint32_t regular_enabled_count;
    uint8_t periodical_state;
    vc_regular_data_t *vc_regular_array;
    vc_periodical_data_t vc_periodical_array;
} fc_settings_t;
#else
// Структура описания ВК регулярного сообщения
typedef struct
{
    char comment[128];
    char type;
    uint32_t dst_id;
#ifdef GREK_FCRT
    uint32_t period;
#else
    uint32_t src_id;
    uint32_t input_port;
    uint32_t output_port;
#endif
    uint32_t priority;
    uint32_t input_asm_id;
    uint32_t output_asm_id;
    uint32_t max_size;
    uint32_t input_queue;
    uint32_t output_queue;
    uint8_t duplication;
    uint8_t channel_type;
#ifndef GREK_FCRT
    uint32_t timeout_AB;
#endif
    uint8_t enabled;
} vc_regular_data_t;

// Структура описания ВК периодического сообщения
typedef struct
{
    char comment[128];
    char type;
#ifndef GREK_FCRT
    uint32_t dst_id;
    uint32_t src_id;
#endif
    uint32_t output_port;
    uint32_t period;
    uint32_t output_asm_id;
    uint32_t max_size;
    uint8_t duplication;
#ifdef GREK_FCRT
    uint8_t channel_type;
#endif
#ifndef GREK_FCRT
    uint32_t priority;
#endif
} vc_periodical_data_t;

// Структура данных из таблицы конфигурации
typedef struct
{
#ifndef GREK_FCRT
    uint32_t reset_pause;
    uint32_t deep_filter;
#endif
    uint32_t regular_overall_count;
    uint32_t regular_enabled_count;
    uint8_t periodical_state;
    vc_regular_data_t *vc_regular_array;
    vc_periodical_data_t vc_periodical_array;
} fc_settings_t;
#endif


typedef struct {
    size_t capacity;
    size_t data_size;
    size_t size;
    char* data;
} vector;


enum json_value_type {
    TYPE_NULL,
    TYPE_BOOL,
    TYPE_NUMBER,
    TYPE_OBJECT, // Is a vector with pairwise entries, key, value
    TYPE_ARRAY, // Is a vector, all entries are plain
    TYPE_STRING,
    TYPE_KEY
};

typedef struct {
    int type;
    union {
        int boolean;
        double number;
        char* string;
        char* key;
        vector array;
        vector object;
    } value;
} json_value;


int json_parse_value (const char **cursor, json_value *parent);
void json_free_value (json_value *val);

/*
 * Функция получения получения размера файла по его имени
 *
 * Входные данные:
 *  file_path - полный путь к файлу
 *
 * Возвращаемое значение:
 *  размер файла или 0 (с кодом в errno), если произошла ошибка
 */
long long get_file_size (const char *file_path)
{
    struct stat statbuf;
    int fstat_res = stat(file_path, &statbuf);

    if (fstat_res < 0)
    {
        return 0;
    }

    return (long long)statbuf.st_size;
}




/*
 * Функция выделения памяти для структуры данных вектора
 *
 * Входные данные:
 *  v         - указатель на вектор
 *  data_size - размер данных
 */
void vector_init (vector *v, size_t data_size)
{
    if (v == NULL)
        return;

    v->data = malloc(data_size);

    if (v->data != NULL)
    {
        v->capacity = 1;
        v->data_size = data_size;
        v->size = 0;
    }
}


/*
 * Функция освобождения памяти для структуры данных вектора
 *
 * Входные данные:
 *  v - указатель на вектор
 */
void vector_free (vector *v)
{
    if (v)
    {
        free(v->data);
        v->data = NULL;
    }
}


/*
 * Функция получения элемента вектора по его индексу
 *
 * Входные данные:
 *  v     - указатель на вектор
 *  index - индекс
 *
 * Возвращаемое значение:
 *  указатель на элемент вектора. Не проверяется на NULL
 */
void *vector_get (const vector *v, size_t index)
{
    return &(v->data[index * v->data_size]);
}


/*
 * Функция получения элемента вектора по его индексу с проверкой на границы вектора
 *
 * Входные данные:
 *  v     - указатель на вектор
 *  index - индекс
 *
 * Возвращаемое значение:
 *  указатель на элемент вектора либо NULL
 */
void *vector_get_checked (const vector *v, size_t index)
{
    return (index < v->size) ? &(v->data[index * v->data_size]) : NULL;
}


/*
 * Функция резервирования вместимости вектора
 *
 * Входные данные:
 *  v            - указатель на вектор
 *  new_capacity - значение вместимости
 */
void vector_reserve (vector *v, size_t new_capacity)
{
    if (new_capacity <= v->capacity)
        return;

    void *new_data = realloc(v->data, new_capacity*v->data_size);

    if (new_data)
    {
        v->capacity = new_capacity;
        v->data = new_data;
    }
    else
    {
        return;
    }
}


/*
 * Функция размещения элемента data[size * data_size]. Если размер size больше capacity, резервируется больше памяти
 *
 * Входные данные:
 *  v    - указатель на вектор
 *  data - указатель на элемент
 */
static void vector_push_back (vector *v, void *data)
{
    if (v->size >= v->capacity)
    {
        size_t new_capacity = (v->capacity > 0) ? (size_t)(v->capacity * 2) : 1;
        vector_reserve(v, new_capacity);
    }

    memcpy(vector_get(v,v->size), data, v->data_size);
    ++v->size;
}


/*
 * Функция выполнения для каждого элемента вектора функции fp
 *
 * Входные данные:
 *  v  - указатель на вектор
 *  fp - указатель на функцию
 */
static void vector_foreach (const vector *v, vector_foreach_t fp)
{
    if (v == NULL)
        return;

    char* item = v->data;

    if (item == NULL)
        return;

    size_t i = 0;

    for (i = 0; i < v->size; i++)
    {
        fp(item);
        item += v->data_size;
    }
}


/*
 * Функция пропуска пробелов и управляющих символов
 *
 * Входные данные:
 *  cursor - указатель на позицию в массиве с данными JSON файла
 */
static void skip_whitespace (const char **cursor)
{
    if (**cursor == '\0')
        return;

    while (iscntrl(**cursor) || isspace(**cursor))
        ++(*cursor);
}


/*
 * Функция проверки наличия символа в строке
 *
 * Входные данные:
 *  cursor    - указатель на позицию в массиве с данными JSON файла
 *  character - символ
 *
 * Возвращаемое значение:
 *  положительное значение при наличии символа, иначе 0
 */
static int has_char (const char **cursor, char character)
{
    skip_whitespace(cursor);
    int success = (**cursor == character);

    if (success)
        ++(*cursor);

    return success;
}


/*
 * Функция поиска объекта в JSON файле
 *
 * Входные данные:
 *  cursor - указатель на позицию в массиве с данными JSON файла
 *  parent - родительский объект
 *
 * Возвращаемое значение:
 *  положительное значение при наличии символа, иначе 0
 */
static int json_parse_object (const char **cursor, json_value *parent)
{
    json_value result = { .type = TYPE_OBJECT };
    vector_init(&result.value.object, sizeof(json_value));

    int success = 1;

    while (success && !has_char(cursor, '}'))
    {
        json_value key = { .type = TYPE_NULL };
        json_value value = { .type = TYPE_NULL };
        success = json_parse_value(cursor, &key);
        success = (success && has_char(cursor, ':'));
        success = (success && json_parse_value(cursor, &value));

        if (success)
        {
            vector_push_back(&result.value.object, &key);
            vector_push_back(&result.value.object, &value);
        }
        else
        {
            json_free_value(&key);
            break;
        }

        skip_whitespace(cursor);

        if (has_char(cursor, '}'))
            break;
        else if (has_char(cursor, ','))
            continue;
        else
            success = 0;
    }

    if (success)
    {
        *parent = result;
    }
    else
    {
        json_free_value(&result);
    }

    return success;
}


/*
 * Функция поиска массива в JSON файле
 *
 * Входные данные:
 *  cursor - указатель на позицию в массиве с данными JSON файла
 *  parent - родительский объект
 *
 * Возвращаемое значение:
 *  положительное значение при наличии символа, иначе 0
 */
static int json_parse_array (const char **cursor, json_value *parent)
{
    int success = 1;
    if (**cursor == ']')
    {
        ++(*cursor);
        return success;
    }

    while (success)
    {
        json_value new_value = { .type = TYPE_NULL };
        success = json_parse_value(cursor, &new_value);

        if (!success)
            break;

        skip_whitespace(cursor);
        vector_push_back(&parent->value.array, &new_value);
        skip_whitespace(cursor);

        if (has_char(cursor, ']'))
            break;
        else if (has_char(cursor, ','))
            continue;
        else
            success = 0;
    }

    return success;
}


/*
 * Функция освобождения объекта JSON-файла
 *
 * Входные данные:
 *  val - указатель объект
 */
void json_free_value (json_value *val)
{
    if (!val)
        return;

    switch (val->type)
    {
    case TYPE_STRING:
    {
        free(val->value.string);
        val->value.string = NULL;
        break;
    }
    case TYPE_ARRAY:
    case TYPE_OBJECT:
    {
        vector_foreach(&(val->value.array), (void (*)(void *)) json_free_value);
        vector_free(&(val->value.array));
        break;
    }
    }

    val->type = TYPE_NULL;
}


/*
 * Функция проверки наличия true, false, null в JSON-файле
 *
 * Входные данные:
 *  cursor  - указатель на позицию в массиве с данными JSON файла
 *  literal - слово для поиска
 *
 * Возвращаемое значение:
 *  1 - при наличии символа, иначе 0
 */
int json_is_literal (const char **cursor, const char *literal)
{
    size_t cnt = strlen(literal);

    if (strncmp(*cursor, literal, cnt) == 0)
    {
        *cursor += cnt;
        return 1;
    }

    return 0;
}


/*
 * Функция парсинга JSON файла
 *
 * Входные данные:
 *  cursor - указатель на позицию в массиве с данными JSON файла
 *  parent - родительский объект
 *
 * Возвращаемое значение:
 *  положительное значение при наличии символа, иначе 0
 */
int json_parse_value (const char **cursor, json_value *parent)
{
    // Eat whitespace
    int success = 0;
    skip_whitespace(cursor);

    switch (**cursor)
    {
    case '\0':
    {
        // If parse_value is called with the cursor at the end of the string
        // that's a failure
        success = 0;

        break;
    }

    case '"':
    {
        ++*cursor;
        const char *start = *cursor;
        char *end = strchr(*cursor, '"');

        if (end)
        {
            size_t len = end - start;
            char *new_string = malloc((len + 1) * sizeof(char));

            if (new_string == NULL)
            {
                success = 0;
            }
            else
            {
                memcpy(new_string, start, len);
                new_string[len] = '\0';

                if (len != strlen(new_string))
                {
                    free(new_string);
                    success = 0;
                }
                else
                {
                    parent->type = TYPE_STRING;
                    parent->value.string = new_string;

                    *cursor = end + 1;
                    success = 1;
                }
            }
        }

        break;
    }

    case '{':
    {
        ++(*cursor);
        skip_whitespace(cursor);
        success = json_parse_object(cursor, parent);

        break;
    }

    case '[':
    {
        parent->type = TYPE_ARRAY;
        vector_init(&parent->value.array, sizeof(json_value));
        ++(*cursor);
        skip_whitespace(cursor);
        success = json_parse_array(cursor, parent);

        if (!success)
        {
            vector_free(&parent->value.array);
        }

        break;
    }

    case 't':
    {
        success = json_is_literal(cursor, "true");

        if (success)
        {
            parent->type = TYPE_BOOL;
            parent->value.boolean = 1;
        }

        break;
    }

    case 'f':
    {
        success = json_is_literal(cursor, "false");

        if (success)
        {
            parent->type = TYPE_BOOL;
            parent->value.boolean = 0;
        }

        break;
    }

    case 'n':
    {
        success = json_is_literal(cursor, "null");
        break;
    }

    default:
    {
        char* end;
        double number = strtod(*cursor, &end);

        if (*cursor != end)
        {
            parent->type = TYPE_NUMBER;
            parent->value.number = number;
            *cursor = end;
            success = 1;
        }
    }
    }

    return success;
}


/*
 * Функция конвертации объекта в строку
 *
 * Входные данные:
 *  value - объект
 *
 * Возвращаемое значение:
 *  конвертированная строка
 */
char *json_value_to_string (json_value *value)
{
    if (value->type != TYPE_STRING)
        return NULL;

    return (char *)value->value.string;
}


/*
 * Функция конвертации объекта в числовое значение
 *
 * Входные данные:
 *  value - объект
 *
 * Возвращаемое значение:
 *  числовое значение
 */
double json_value_to_double (json_value *value)
{
    if (value->type != TYPE_NUMBER)
        return 0;

    return value->value.number;
}


/*
 * Функция конвертации объекта в булевое значение
 *
 * Входные данные:
 *  value - объект
 *
 * Возвращаемое значение:
 *  булевое значение
 */
int json_value_to_bool (json_value *value)
{
    if (value->type != TYPE_BOOL)
        return 0;

    return value->value.boolean;
}


/*
 * Функция конвертации объекта в вектор
 *
 * Входные данные:
 *  value - объект
 *
 * Возвращаемое значение:
 *  указатель на вектор
 */
vector *json_value_to_array (json_value *value)
{
    if (value->type != TYPE_ARRAY)
        return NULL;

    return &value->value.array;
}


/*
 * Функция конвертации объекта в объектный тип
 *
 * Входные данные:
 *  value - объект
 *
 * Возвращаемое значение:
 *  указатель на вектор объектного типа
 */
vector *json_value_to_object (json_value *value)
{
    if (value->type != TYPE_OBJECT)
        return NULL;

    return &value->value.object;
}


/*
 * Функция получения узла по индексу
 *
 * Входные данные:
 *  root  - объект
 *  index - индекс в массиве
 *
 * Возвращаемое значение:
 *  указатель на узел
 */
json_value *json_value_at (const json_value *root, size_t index)
{
    if (root->type != TYPE_ARRAY)
        return NULL;

    if (root->value.array.size >= index)
    {
        return vector_get_checked(&root->value.array,index);
    }
    else
    {
        return NULL;
    }
}


/*
 * Функция получения узла по ключу
 * Входные данные:
 *  root - объект
 *  key  - ключ
 *
 * Возвращаемое значение:
 *  указатель на узел
 */
json_value *json_value_with_key (const json_value *root, const char *key)
{
    if (root->type != TYPE_OBJECT)
        return NULL;

    json_value* data = (json_value*)root->value.object.data;
    size_t size = root->value.object.size;
    size_t i = 0;

    for (i = 0; i < size; i += 2)
    {
        if (strcmp(data[i].value.string, key) == 0)
        {
            return &data[i + 1];
        }
    }

    return NULL;
}


/*
 * Функция парсинга JSON-файла (первый этап)
 * Входные данные:
 *  input  - указатель на массив с данными JSON файла
 *  result - объект распарсенного JSON-файла
 *
 * Возвращаемое значение:
 *  положительное значение при наличии символа, иначе 0
 */
int json_parse (const char *input, json_value *result)
{
    return json_parse_value(&input, result);
}



int process_json_fcrt_settings_file (const char *file_path, const char *dest_path)
{
    int error_counter = 0;
#ifdef ETHERNET_MODE
    fc_settings_t ethernet_settings;
    ethernet_settings.regular_enabled_count = 0;
    ethernet_settings.regular_overall_count = 0;
    ethernet_settings.periodical_state = VC_OFF;
#else
    fc_settings_t fcrt_settings;
    fcrt_settings.regular_enabled_count = 0;
    fcrt_settings.regular_overall_count = 0;
    fcrt_settings.periodical_state = VC_OFF;
#endif

    int json_fd = open(file_path, O_RDONLY);

    if (json_fd >= 0)
    {
        uint32_t size = (uint32_t)get_file_size(file_path);

        if (size > 0)
        {
            // Выделение буфера под данные json-файла
            char *file_buffer = malloc(size);

            if (file_buffer)
            {
                // Чтение файла
                ssize_t read_bytes = read(json_fd, file_buffer, size);

                if (read_bytes == size)
                {
                    // Инициализация структуры и парсинг json-строки
                    json_value result = {.type = TYPE_NULL};
                    json_value root;

                    int result_parse = json_parse(file_buffer, &root);

                    if (result_parse == 1)
                    {
                        // Поиск узла REGULAR_CONFIG
                        json_value *regular_root = json_value_with_key(&root, "REGULAR_CONFIG");

                        if (regular_root != NULL)
                        {
                            if (regular_root->value.array.size > 0)
                            {
#ifdef ETHERNET_MODE
                                // Заполнение общего числа строк
                                ethernet_settings.regular_overall_count = regular_root->value.array.size;

                                // Выделение памяти под массив структур ВК регулярного сообщения
                                ethernet_settings.vc_regular_array = (vc_regular_data_t *)malloc(sizeof(vc_regular_data_t) * regular_root->value.array.size);

                                if (ethernet_settings.vc_regular_array)
#else
                                // Заполнение общего числа строк
                                fcrt_settings.regular_overall_count = regular_root->value.array.size;

                                // Выделение памяти под массив структур ВК регулярного сообщения
                                fcrt_settings.vc_regular_array = (vc_regular_data_t *)malloc(sizeof(vc_regular_data_t) * regular_root->value.array.size);

                                if (fcrt_settings.vc_regular_array)
#endif
                                {
                                    // Парсинг файла конфигурации и заполнение структур
                                    size_t i = 0;

                                    for (i = 0; i < regular_root->value.array.size; i++)
                                    {
                                        json_value *regular_vc = json_value_at(regular_root, i);

                                        if (regular_vc != NULL)
                                        {
#ifdef ETHERNET_MODE
                                            // Проверка включения ВК
                                            json_value *value = json_value_with_key(regular_vc, "active");

                                            if (value != NULL)
                                            {
                                                char *active = json_value_to_string(value);

                                                if (!strcmp(active, "ON"))
                                                {
                                                    ethernet_settings.vc_regular_array[i].enabled = VC_ON;
                                                }
                                                else
                                                {
                                                    ethernet_settings.vc_regular_array[i].enabled = VC_OFF;
                                                    continue;
                                                }
                                            }
                                            else
                                            {
                                                ethernet_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка типа ВК
                                            value = json_value_with_key(regular_vc, "type");

                                            if (value != NULL)
                                            {
                                                char *type = json_value_to_string(value);

                                                if (!strcmp(type, "LOW"))
                                                {
                                                    ethernet_settings.vc_regular_array[i].type = VC_TYPE_LOW;
                                                }
                                                else if (!strcmp(type, "HIGH"))
                                                {
                                                    ethernet_settings.vc_regular_array[i].type = VC_TYPE_HIGH;
                                                }
                                                else
                                                {
                                                    ethernet_settings.vc_regular_array[i].enabled = VC_OFF;
                                                    continue;
                                                }
                                            }
                                            else
                                            {
                                                ethernet_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка включения ВК
                                            value = json_value_with_key(regular_vc, "client_ip");

                                            if (value != NULL)
                                            {
                                                char *client_ip = json_value_to_string(value);

                                                memset(ethernet_settings.vc_regular_array[i].client_ip, 0, 16);
                                                strcpy(ethernet_settings.vc_regular_array[i].client_ip, client_ip);
                                            }
                                            else
                                            {
                                                ethernet_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка DST_ID
                                            value = json_value_with_key(regular_vc, "client_rcv_port");

                                            if (value != NULL)
                                            {
                                                ethernet_settings.vc_regular_array[i].client_rcv_port = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                ethernet_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка DST_ID
                                            value = json_value_with_key(regular_vc, "server_rcv_port");

                                            if (value != NULL)
                                            {
                                                ethernet_settings.vc_regular_array[i].server_rcv_port = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                ethernet_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка DST_ID
                                            value = json_value_with_key(regular_vc, "server_snd_port");

                                            if (value != NULL)
                                            {
                                                ethernet_settings.vc_regular_array[i].server_snd_port = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                ethernet_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "priority");

                                            if (value != NULL)
                                            {
                                                ethernet_settings.vc_regular_array[i].priority = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                ethernet_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Увеличение счетчика корректных ВК регклярного сообщения
                                            ethernet_settings.regular_enabled_count++;

#else
                                            // Проверка включения ВК
                                            json_value *value = json_value_with_key(regular_vc, "active");

                                            if (value != NULL)
                                            {
                                                char *active = json_value_to_string(value);

                                                if (!strcmp(active, "ON"))
                                                {
                                                    fcrt_settings.vc_regular_array[i].enabled = VC_ON;
                                                }
                                                else
                                                {
                                                    fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                    // continue;
                                                }
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }
                                            //копируем комментарий
                                            value = json_value_with_key(regular_vc, "comment");
                                            if (value != NULL)
                                            {
                                                char *comment = json_value_to_string(value);
                                                snprintf(fcrt_settings.vc_regular_array[i].comment, sizeof(fcrt_settings.vc_regular_array[i].comment), "\n# %s\n", comment);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }


                                            // Проверка типа ВК
                                            value = json_value_with_key(regular_vc, "type");

                                            if (value != NULL)
                                            {
                                                char *type = json_value_to_string(value);

                                                if (!strcmp(type, "LOW"))
                                                {
                                                    fcrt_settings.vc_regular_array[i].type = VC_TYPE_LOW;
                                                }
                                                else if (!strcmp(type, "HIGH"))
                                                {
                                                    fcrt_settings.vc_regular_array[i].type = VC_TYPE_HIGH;
                                                }
                                                else
                                                {
                                                    fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                    continue;
                                                }
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка DST_ID
                                            value = json_value_with_key(regular_vc, "dst_id");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].dst_id = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

#ifdef GREK_FCRT
                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "period");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].period = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }
#else

                                            // Проверка SRC_ID
                                            value = json_value_with_key(regular_vc, "src_id");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].src_id = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка входного порта
                                            value = json_value_with_key(regular_vc, "input_port");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].input_port = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "output_port");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].output_port = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }
#endif
                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "priority");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].priority = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "input_asm_id");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].input_asm_id = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "output_asm_id");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].output_asm_id = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "max_size");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].max_size = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "input_queue");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].input_queue = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "output_queue");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].output_queue = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "duplication");

                                            if (value != NULL)
                                            {
                                                char *duplication = json_value_to_string(value);

                                                if (!strcmp(duplication, "A"))
                                                {
                                                    fcrt_settings.vc_regular_array[i].duplication = VC_DUPLICATION_A;
                                                }
                                                else if (!strcmp(duplication, "B"))
                                                {
                                                    fcrt_settings.vc_regular_array[i].duplication = VC_DUPLICATION_B;
                                                }
                                                else if (!strcmp(duplication, "AB"))
                                                {
                                                    fcrt_settings.vc_regular_array[i].duplication = VC_DUPLICATION_AB;
                                                }
                                                else
                                                {
                                                    fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                    continue;
                                                }
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }

                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "channel_type");

                                            if (value != NULL)
                                            {
                                                char *channel_type = json_value_to_string(value);

                                                if (!strcmp(channel_type, "FCRT"))
                                                {
                                                    fcrt_settings.vc_regular_array[i].channel_type = VC_FCRT;
                                                }
                                                else if (!strcmp(channel_type, "ASM"))
                                                {
                                                    fcrt_settings.vc_regular_array[i].channel_type = VC_ASM;

                                                    // Проверка, что дублирование AB отключено
                                                    if (fcrt_settings.vc_regular_array[i].duplication == VC_DUPLICATION_AB)
                                                    {
                                                        fcrt_settings.vc_regular_array[i].duplication = VC_DUPLICATION_A;
                                                    }
                                                }
                                                else
                                                {
                                                    fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                    continue;
                                                }
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }
#ifndef GREK_FCRT
                                            // Проверка выходного порта
                                            value = json_value_with_key(regular_vc, "timeout_AB");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_regular_array[i].timeout_AB = (uint32_t)json_value_to_double(value);
                                            }
                                            else
                                            {
                                                fcrt_settings.vc_regular_array[i].enabled = VC_OFF;
                                                continue;
                                            }
#endif

                                            // Увеличение счетчика корректных ВК регклярного сообщения
                                            fcrt_settings.regular_enabled_count++;
#endif
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {

                        }

                        // Флаг успеха получения конфигурации
                        uint8_t periodical_success_parse = 0;

                        // По умолчанию канал отключен
#ifdef ETHERNET_MODE

#else
                        fcrt_settings.periodical_state = VC_OFF;
#endif

                        // Поиск узла PERIODICAL_CONFIG
                        json_value *periodical_root = json_value_with_key(&root, "PERIODICAL_CONFIG");

                        if (periodical_root != NULL)
                        {
                            if (periodical_root->value.array.size == 1)
                            {
                                json_value *periodical_vc = json_value_at(periodical_root, 0);

                                if (periodical_vc != NULL)
                                {
#ifdef ETHERNET_MODE
                                    // Проверка включения ВК
                                    json_value *value = json_value_with_key(periodical_vc, "active");

                                    if (value != NULL)
                                    {
                                        char *active = json_value_to_string(value);

                                        if (!strcmp(active, "ON"))
                                        {
                                            ethernet_settings.vc_periodical_array.enabled = VC_ON;

                                            // Проверка включения ВК
                                            value = json_value_with_key(periodical_vc, "server_ip");

                                            if (value != NULL)
                                            {
                                                char *client_ip = json_value_to_string(value);

                                                memset(ethernet_settings.vc_periodical_array.server_ip, 0, 16);
                                                strcpy(ethernet_settings.vc_periodical_array.server_ip, client_ip);

                                                // Проверка включения ВК
                                                value = json_value_with_key(periodical_vc, "client_ip");

                                                if (value != NULL)
                                                {
                                                    char *client_ip = json_value_to_string(value);

                                                    memset(ethernet_settings.vc_periodical_array.client_ip, 0, 16);
                                                    strcpy(ethernet_settings.vc_periodical_array.client_ip, client_ip);

                                                    value = json_value_with_key(periodical_vc, "client_rcv_port");

                                                    if (value != NULL)
                                                    {
                                                        ethernet_settings.vc_periodical_array.client_rcv_port = (uint32_t) json_value_to_double(
                                                                value);

                                                        value = json_value_with_key(periodical_vc, "server_snd_port");

                                                        if (value != NULL)
                                                        {
                                                            ethernet_settings.vc_periodical_array.server_snd_port = (uint32_t) json_value_to_double(
                                                                    value);

                                                            value = json_value_with_key(periodical_vc, "period");

                                                            if (value != NULL)
                                                            {
                                                                ethernet_settings.vc_periodical_array.period = (uint32_t) json_value_to_double(
                                                                        value);

                                                                value = json_value_with_key(periodical_vc, "max_size");

                                                                if (value != NULL)
                                                                {
                                                                    ethernet_settings.vc_periodical_array.max_size = (uint32_t) json_value_to_double(
                                                                            value);

                                                                    periodical_success_parse = 1;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

#else
                                    // Проверка включения ВК
                                    json_value *value = json_value_with_key(periodical_vc, "active");

                                    if (value != NULL)
                                    {
                                        char *active = json_value_to_string(value);

                                        if (!strcmp(active, "ON"))
                                        {
                                            fcrt_settings.periodical_state = VC_ON;
#ifndef GREK_FCRT
                                            //копируем комментарий
                                            value = json_value_with_key(periodical_vc, "comment");
                                            if (value != NULL)
                                            {
                                                char *comment = json_value_to_string(value);
                                                snprintf(fcrt_settings.vc_periodical_array.comment, sizeof(fcrt_settings.vc_periodical_array.comment), "\n# %s\n", comment);
                                            }
                                            // Проверка DST_ID
                                            value = json_value_with_key(periodical_vc, "dst_id");

                                            if (value != NULL)
                                            {
                                                fcrt_settings.vc_periodical_array.dst_id = (uint32_t) json_value_to_double(
                                                        value);

                                                // Проверка SRC_ID
                                                value = json_value_with_key(periodical_vc, "src_id");

                                                if (value != NULL)
                                                {
                                                    fcrt_settings.vc_periodical_array.src_id = (uint32_t) json_value_to_double(
                                                            value);
#endif

                                                    value = json_value_with_key(periodical_vc, "output_port");

                                                    if (value != NULL)
                                                    {
                                                        fcrt_settings.vc_periodical_array.output_port = (uint32_t) json_value_to_double(
                                                                value);

                                                        value = json_value_with_key(periodical_vc, "period");

                                                        if (value != NULL)
                                                        {
                                                            fcrt_settings.vc_periodical_array.period = (uint32_t) json_value_to_double(
                                                                    value);

                                                            value = json_value_with_key(periodical_vc, "output_asm_id");

                                                            if (value != NULL)
                                                            {
                                                                fcrt_settings.vc_periodical_array.output_asm_id = (uint32_t) json_value_to_double(
                                                                        value);

                                                                value = json_value_with_key(periodical_vc, "max_size");

                                                                if (value != NULL)
                                                                {
                                                                    fcrt_settings.vc_periodical_array.max_size = (uint32_t) json_value_to_double(
                                                                            value);

                                                                    // Проверка выходного порта
                                                                    value = json_value_with_key(periodical_vc,
                                                                                                "duplication");

                                                                    if (value != NULL)
                                                                    {
                                                                        char *duplication = json_value_to_string(value);

                                                                        if (!strcmp(duplication, "A"))
                                                                        {
                                                                            fcrt_settings.vc_periodical_array.duplication = VC_DUPLICATION_A;
                                                                            periodical_success_parse = 1;
                                                                        }
                                                                        else if (!strcmp(duplication, "B"))
                                                                        {
                                                                            fcrt_settings.vc_periodical_array.duplication = VC_DUPLICATION_B;
                                                                            periodical_success_parse = 1;
                                                                        }
                                                                        else if (!strcmp(duplication, "AB"))
                                                                        {
                                                                            fcrt_settings.vc_periodical_array.duplication = VC_DUPLICATION_AB;
                                                                            periodical_success_parse = 1;
                                                                        }
                                                                        else
                                                                        {
                                                                            fcrt_settings.periodical_state = VC_OFF;
                                                                        }

#ifdef GREK_FCRT
                                                                        if (fcrt_settings.periodical_state == VC_ON)
                                                                        {
                                                                            // Проверка выходного порта
                                                                            value = json_value_with_key(periodical_vc,
                                                                                                        "channel_type");

                                                                            if (value != NULL)
                                                                            {
                                                                                char *channel_type = json_value_to_string(
                                                                                        value);

                                                                                if (!strcmp(channel_type, "FCRT"))
                                                                                {
                                                                                    fcrt_settings.vc_periodical_array.channel_type = VC_FCRT;
                                                                                    periodical_success_parse = 1;
                                                                                }
                                                                                else if (!strcmp(channel_type, "ASM"))
                                                                                {
                                                                                    fcrt_settings.vc_periodical_array.channel_type = VC_ASM;

                                                                                    // Проверка, что дублирование AB отключено
                                                                                    if (fcrt_settings.vc_periodical_array.duplication == VC_DUPLICATION_AB)
                                                                                    {
                                                                                        fcrt_settings.vc_periodical_array.duplication = VC_DUPLICATION_A;
                                                                                    }

                                                                                    periodical_success_parse = 1;
                                                                                }
                                                                                else
                                                                                {
                                                                                    fcrt_settings.periodical_state = VC_OFF;
                                                                                }
                                                                            }
                                                                            else
                                                                            {
                                                                                fcrt_settings.periodical_state = VC_OFF;
                                                                            }
                                                                        }
#else
                                                                        if (fcrt_settings.periodical_state == VC_ON)
                                                                        {
                                                                            value = json_value_with_key(periodical_vc, "priority");

                                                                            if (value != NULL)
                                                                            {
                                                                                fcrt_settings.vc_periodical_array.priority = (uint32_t) json_value_to_double(
                                                                                        value);

                                                                                periodical_success_parse = 1;
                                                                            }
                                                                            else
                                                                            {
                                                                                periodical_success_parse = 0;
                                                                            }
                                                                        }
#endif
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
#ifndef GREK_FCRT
                                                }
                                            }
#endif
                                        }
                                    }
#endif
                                }
                            }
                        }

                        if (periodical_success_parse == 0)
                        {
#ifdef ETHERNET_MODE
                            ethernet_settings.periodical_state = VC_OFF;
#else
                            fcrt_settings.periodical_state = VC_OFF;
#endif
                        }

#ifndef ETHERNET_MODE
#ifndef GREK_FCRT
                        // Значения по умолчанию
                        fcrt_settings.deep_filter = 0;
                        fcrt_settings.reset_pause = 0;

                        // Поиск узла REGULAR_CONFIG
                        json_value *config_root = json_value_with_key(&root, "COMMON_CONFIG");

                        if (config_root != NULL)
                        {
                            if (config_root->value.array.size > 0)
                            {
                                // Парсинг файла конфигурации и заполнение структур
                                size_t i = 0;

                                for (i = 0; i < config_root->value.array.size; i++)
                                {
                                    json_value *common_config = json_value_at(config_root, i);

                                    if (common_config != NULL)
                                    {
                                        json_value *value = json_value_with_key(common_config, "name");

                                        if (value != NULL)
                                        {
                                            char *name = json_value_to_string(value);

                                            if (!strcmp(name, "pause"))
                                            {
                                                value = json_value_with_key(common_config, "value");

                                                if (value != NULL)
                                                {
                                                    fcrt_settings.reset_pause = (uint32_t) json_value_to_double(
                                                            value);
                                                }
                                            }
                                            else if (!strcmp(name, "fc_rx_err_delay"))
                                            {
                                                value = json_value_with_key(common_config, "value");

                                                if (value != NULL)
                                                {
                                                    fcrt_settings.deep_filter = (uint32_t) json_value_to_double(
                                                            value);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
#endif
#endif
                    }
                    else
                    {
                        printf("parse error\n");
                        error_counter++;
                    }

                    json_free_value(&root);
                }
                else
                {
                    printf("read file error\n");
                    error_counter++;
                }

                free(file_buffer);
            }
            else
            {
                printf("malloc error\n");
                error_counter++;
            }

            close(json_fd);
        }
        else
        {
            printf("empty file error\n");
            error_counter++;
        }
    }

    if (error_counter > 0)
    {
        return DEF_ERROR;
    }
    ///пишем данные в файл cfg
    int cfg_fd = open(dest_path, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if(cfg_fd > 0)
    {
        int i;
        #define BUFSZ 256
        char buf[BUFSZ];
        vc_regular_data_t * rd = NULL;
        vc_periodical_data_t * pd = NULL;
        rd = fcrt_settings.vc_regular_array;
        pd = &fcrt_settings.vc_periodical_array;
        ///обычные сообщения
        for(i = 0; i < fcrt_settings.regular_overall_count; i++)
        {
            snprintf(buf, BUFSZ, "%c=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
            rd->type,
            rd->dst_id, rd->src_id,
            rd->input_port, rd->output_port,
            rd->priority,
            rd->input_asm_id,rd->output_asm_id,
            rd->max_size,
            rd->input_queue, rd->output_queue,
            rd->duplication,
            rd->channel_type,
            rd->timeout_AB
            );
#if 0
            printf("%s", buf);
#endif
            write(cfg_fd, rd->comment, strlen(rd->comment));
            if(rd->enabled == VC_OFF)
                write(cfg_fd, "#", strlen("#"));

            write(cfg_fd, buf, strlen(buf));
            rd++;
        }
        ///статусное сообщение
        snprintf(buf, BUFSZ, "P=%u,%u,%u,%u,%u,%u,%u,%u\n",
        pd->dst_id, pd->src_id,
        pd->output_port,
        pd->period,
        pd->output_asm_id,
        pd->max_size,
        pd->duplication,
        pd->priority
        );

#if 0
        printf("%s", buf);
#endif
        write(cfg_fd, pd->comment, strlen(pd->comment));
        write(cfg_fd, buf, strlen(buf));
        close(cfg_fd);
    }
    else
        printf("\nError: could not open file\n");
    free(fcrt_settings.vc_regular_array);
    
    return SUCCESS;
}

char help_str[] = {
    "Using:\n\tjson_parser <path to json file> <path to converted cfg file>\n"
};

int main(int argc, char * argv[])
{
    char * json_cnf_path;
    char * cfg_cnf_path;
    if(argc != 3)
    {
        printf("%s", help_str);
        return -EINVAL;
    }
    json_cnf_path = argv[1];
    cfg_cnf_path = argv[2];

    if(process_json_fcrt_settings_file(json_cnf_path, cfg_cnf_path) != SUCCESS)
    {
        printf("\n---- Error. Could not convert json config file %s to %s file", json_cnf_path, cfg_cnf_path);
        return -EINVAL;
    }
    return 0;
}