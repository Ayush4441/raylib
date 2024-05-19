/***********************************************************************************
*
*   RINI IMPLEMENTATION
*
************************************************************************************/


#include "rini.h"
#include <stdio.h>          // Required for: fopen(), feof(), fgets(), fclose(), fprintf()
#include <stdlib.h>         // Required for: malloc(), calloc(), free()
#include <string.h>         // Required for: memset(), memcpy(), strcmp(), strlen()

//----------------------------------------------------------------------------------
// Defines and macros
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Global variables definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module internal functions declaration
//----------------------------------------------------------------------------------
static int rini_read_config_key(const char *buffer, char *key); // Get key from a buffer line containing key-value-(description)
static int rini_read_config_value_text(const char *buffer, char *text, char *desc); // Get value text (and description) from a buffer line

static int rini_text_to_int(const char *text); // Convert text to int value (if possible), same as atoi()

//----------------------------------------------------------------------------------
// Module functions declaration
//----------------------------------------------------------------------------------
// Load config from file (.ini)
rini_config rini_load_config(const char *file_name)
{
    rini_config config = { 0 };
    unsigned int value_counter = 0;

    // Init config data to max capacity
    config.capacity = RINI_MAX_VALUE_CAPACITY;
    config.values = (rini_config_value *)RINI_CALLOC(RINI_MAX_VALUE_CAPACITY, sizeof(rini_config_value));

    if (file_name != NULL)
    {
        FILE *rini_file = fopen(file_name, "rt");

        if (rini_file != NULL)
        {
            char buffer[RINI_MAX_LINE_SIZE] = { 0 };    // Buffer to read every text line

            // First pass to count valid config lines
            while (!feof(rini_file))
            {
                // WARNING: fgets() keeps line endings, doesn't have any special options for converting line endings,
                // but on Windows, when reading file 'rt', line endings are converted from \r\n to just \n
                fgets(buffer, RINI_MAX_LINE_SIZE, rini_file);

                // Skip commented lines and empty lines
                // NOTE: We are also skipping sections delimiters
                if ((buffer[0] != RINI_LINE_COMMENT_DELIMITER) &&
                    (buffer[0] != RINI_LINE_SECTION_DELIMITER) &&
                    (buffer[0] != '\n') && (buffer[0] != '\0')) value_counter++;
            }

            // WARNING: We can't store more values than its max capacity
            config.count = (value_counter > RINI_MAX_VALUE_CAPACITY)? RINI_MAX_VALUE_CAPACITY : value_counter;

            if (config.count > 0)
            {
                rewind(rini_file);
                value_counter = 0;

                // Second pass to read config data
                while (!feof(rini_file))
                {
                    // WARNING: fgets() keeps line endings, doesn't have any special options for converting line endings,
                    // but on Windows, when reading file 'rt', line endings are converted from \r\n to just \n
                    fgets(buffer, RINI_MAX_LINE_SIZE, rini_file);

                    // Skip commented lines and empty lines
                    if ((buffer[0] != '#') && (buffer[0] != ';') && (buffer[0] != '\n') && (buffer[0] != '\0'))
                    {
                        // Get keyentifier string
                        memset(config.values[value_counter].key, 0, RINI_MAX_KEY_SIZE);
                        rini_read_config_key(buffer, config.values[value_counter].key);
                        rini_read_config_value_text(buffer, config.values[value_counter].text, config.values[value_counter].desc);

                        value_counter++;
                    }
                }
            }

            fclose(rini_file);
        }
    }

    return config;
}

// Save config to file (*.ini)
void rini_save_config(rini_config config, const char *file_name, const char *header)
{
    FILE *rini_file = fopen(file_name, "wt");

    if (rini_file != NULL)
    {
        if (header != NULL) fprintf(rini_file, header);

        for (int i = 0; i < config.count; i++)
        {
            // TODO: If text is not a number value, append text-quotes?

            fprintf(rini_file, "%-22s %c %6s      %c %s\n", config.values[i].key, RINI_VALUE_DELIMITER, config.values[i].text, RINI_VALUE_COMMENTS_DELIMITER, config.values[i].desc);
        }

        fclose(rini_file);
    }
}

// Unload config data
void rini_unload_config(rini_config *config)
{
    RINI_FREE(config->values);

    config->values = NULL;
    config->count = 0;
    config->capacity = 0;
}

// Get config value for provided key, returns 0 if not found or not valid
int rini_get_config_value(rini_config config, const char *key)
{
    int value = -1;

    for (int i = 0; i < config.count; i++)
    {
        if (strcmp(key, config.values[i].key) == 0)
        {
            value = rini_text_to_int(config.values[i].text);    // Returns 0 if conversion fails
            break;
        }
    }

    return value;
}

// Get config text for string id
const char *rini_get_config_value_text(rini_config config, const char *key)
{
    const char *text = NULL;

    for (int i = 0; i < config.count; i++)
    {
        if (strcmp(key, config.values[i].key) == 0)
        {
            text = config.values[i].text;
            break;
        }
    }

    return text;
}

// Get config description for string id
const char *rini_get_config_value_description(rini_config config, const char *key)
{
    const char *desc = NULL;

    for (int i = 0; i < config.count; i++)
    {
        if (strcmp(key, config.values[i].key) == 0)
        {
            desc = config.values[i].desc;
            break;
        }
    }

    return desc;
}

// Set config value and description for existing key or create a new entry
int rini_set_config_value(rini_config *config, const char *key, int value, const char *desc)
{
    int result = -1;
    char value_text[RINI_MAX_TEXT_SIZE] = { 0 };

    sprintf(value_text, "%i", value);

    result = rini_set_config_value_text(config, key, value_text, desc);

    return result;
}

// Set config value text and description for existing key or create a new entry
// NOTE: When setting a text value, if id does not exist, a new entry is automatically created
int rini_set_config_value_text(rini_config *config, const char *key, const char *text, const char *desc)
{
    int result = -1;

    if ((text == NULL) || (text[0] == '\0')) return result;

    // Try to find key and update text and description
    for (int i = 0; i < config->count; i++)
    {
        if (strcmp(key, config->values[i].key) == 0)
        {
            memset(config->values[i].text, 0, RINI_MAX_TEXT_SIZE);
            memcpy(config->values[i].text, text, strlen(text));

            memset(config->values[i].desc, 0, RINI_MAX_DESC_SIZE);
            if (desc != NULL) memcpy(config->values[i].desc, desc, strlen(desc));
            result = 0;
            break;
        }
    }

    // Key not found, we add a new entry if possible
    if (result == -1)
    {
        if (config->count < config->capacity)
        {
            // NOTE: We do a manual copy to avoid possible overflows on input data

            for (int i = 0; (i < RINI_MAX_KEY_SIZE) && (key[i] != '\0'); i++) config->values[config->count].key[i] = key[i];
            for (int i = 0; (i < RINI_MAX_TEXT_SIZE) && (text[i] != '\0'); i++) config->values[config->count].text[i] = text[i];
            for (int i = 0; (i < RINI_MAX_DESC_SIZE) && (desc[i] != '\0'); i++) config->values[config->count].desc[i] = desc[i];

            config->count++;
            result = 0;
        }
    }

    return result;
}

// Set config value description for existing key
// WARNING: Key must exist to add description, if a description exists, it is updated
int rini_set_config_value_description(rini_config *config, const char *key, const char *desc)
{
    int result = 1;

    for (int i = 0; i < config->count; i++)
    {
        if (strcmp(key, config->values[i].key) == 0)
        {
            memset(config->values[i].desc, 0, RINI_MAX_DESC_SIZE);
            if (desc != NULL) memcpy(config->values[i].desc, desc, strlen(desc));
            result = 0;
            break;
        }
    }

    return result;
}

//----------------------------------------------------------------------------------
// Module internal functions declaration
//----------------------------------------------------------------------------------
// Get string id from a buffer line containing id-value pair
static int rini_read_config_key(const char *buffer, char *key)
{
    int len = 0;
    while ((buffer[len] != '\0') && (buffer[len] != ' ') && (buffer[len] != RINI_VALUE_DELIMITER)) len++;    // Skip keyentifier

    memcpy(key, buffer, len);

    return len;
}

// Get config string-value from a buffer line containing id-value pair
static int rini_read_config_value_text(const char *buffer, char *text, char *desc)
{
    const char *buffer_ptr = buffer;

    // Expected config line structure:
    // [key][spaces?][delimiter?][spaces?]["?][textValue]["?][spaces?][[;][#]description?]
    // We need to skip spaces, check for delimiter (if required), skip spaces, and get text value

    while ((buffer_ptr[0] != '\0') && (buffer_ptr[0] != ' ')) buffer_ptr++;        // Skip keyentifier

    while ((buffer_ptr[0] != '\0') && (buffer_ptr[0] == ' ')) buffer_ptr++;        // Skip line spaces before text value or delimiter

#if defined(RINI_VALUE_DELIMITER)
    if (buffer_ptr[0] == RINI_VALUE_DELIMITER)
    {
        buffer_ptr++;       // Skip delimiter

        while ((buffer_ptr[0] != '\0') && (buffer_ptr[0] == ' ')) buffer_ptr++;    // Skip line spaces before text value
    }
#endif

    // Now buffer_ptr should be pointing to the start of value

    int len = 0;
    while ((buffer_ptr[len] != '\0') &&
        (buffer_ptr[len] != '\r') &&
        (buffer_ptr[len] != '\n')) len++;     // Get text-value and description length (to the end of line)

    // Now we got the length from text-value start to end of line

    int value_len = len;
    int desc_pos = 0;
#if defined(RINI_VALUE_COMMENTS_DELIMITER)
    // Scan text looking for text-value description (if used)
    for (; desc_pos < len; desc_pos++)
    {
        if (buffer_ptr[desc_pos] == RINI_VALUE_COMMENTS_DELIMITER)
        {
            value_len = desc_pos - 1;
            while (buffer_ptr[value_len] == ' ') value_len--;

            value_len++;

            desc_pos++;   // Skip delimiter and following spaces
            while (buffer_ptr[desc_pos] == ' ') desc_pos++;
            break;
        }
    }
#endif

    // Remove starting double quotes from text (if being used)
    if (buffer_ptr[0] == '\"')
    {
        buffer_ptr++; desc_pos--; len--; value_len--;

        // Remove ending double quotes from text (if being used)
        if (buffer_ptr[value_len - 1] == '\"') { value_len--; }
    }

    // Clear text buffers to be updated
    memset(text, 0, RINI_MAX_TEXT_SIZE);
    memset(desc, 0, RINI_MAX_DESC_SIZE);

    // Copy value-text and description to provided pointers
    memcpy(text, buffer_ptr, value_len);
    memcpy(desc, buffer_ptr + desc_pos, ((len - desc_pos) > (RINI_MAX_DESC_SIZE - 1))? (RINI_MAX_DESC_SIZE - 1) : (len - desc_pos));

    return len;
}

// Convert text to int value (if possible), same as atoi()
static int rini_text_to_int(const char *text)
{
    int value = 0;
    int sign = 1;

    if ((text[0] == '+') || (text[0] == '-'))
    {
        if (text[0] == '-') sign = -1;
        text++;
    }

    int i = 0;
    for (; ((text[i] >= '0') && (text[i] <= '9')); i++) value = value*10 + (int)(text[i] - '0');

    //if (strlen(text) != i) // Text starts with numbers but contains some text after that...

    return value*sign;
}