/**********************************************************************************************
*
*   rini v1.0 - A simple and easy-to-use config init files reader and writer
*
*   DESCRIPTION:
*       Load and save config init properties
*
*   FEATURES:
*       - Config files reading and writing
*       - Supported value types: int, string
*       - Support custom line comment character
*       - Support custom property delimiters
*       - Support inline comments with custom delimiter
*       - Support multi-word text values w/o quote delimiters
*       - Minimal C standard lib dependency (optional)
*       - Customizable maximum config values capacity
*
*   LIMITATIONS:
*       - Config [sections] not supported
*       - Saving config file requires complete rewrite
*
*   POSSIBLE IMPROVEMENTS:
*       - Support config [sections]
*       - Support disabled entries recognition
*       - Config values update over existing files
*
*   CONFIGURATION:
*       #define RINI_IMPLEMENTATION
*           Generates the implementation of the library into the included file.
*           If not defined, the library is in header only mode and can be included in other headers
*           or source files without problems. But only ONE file should hold the implementation.
*
*       #define RINI_MAX_LINE_SIZE
*           Defines the maximum size of line buffer to read from file.
*           Default value: 512 bytes (considering [key + text + desc] is 256 max size by default)
*
*       #define RINI_MAX_KEY_SIZE
*           Defines the maximum size of value key
*           Default value: 64 bytes
*
*       #define RINI_MAX_TEXT_SIZE
*           Defines the maximum size of value text
*           Default value: 64 bytes
*
*       #define RINI_MAX_DESC_SIZE
*           Defines the maximum size of value description
*           Default value: 128 bytes
*
*       #define RINI_MAX_VALUE_CAPACITY
*           Defines the maximum number of values supported
*           Default value: 32 entries support
*
*       #define RINI_VALUE_DELIMITER
*           Defines a key value delimiter, in case it is defined.
*           Most .ini files use '=' as value delimiter but ':' is also used or just whitespace
*           Default value: ' '
*
*       #define RINI_LINE_COMMENT_DELIMITER
*           Define character used to comment lines, placed at beginning of line
*           Most .ini files use semicolon ';' but '#' is also used
*           Default value: '#'
*
*       #define RINI_VALUE_COMMENTS_DELIMITER
*           Defines a property line-end comment delimiter
*           This implementation allows adding inline comments after the value.
*           Default value: '#'
*
*       #define RINI_LINE_SECTION_DELIMITER
*           Defines section lines start character
*           Sections loading is not supported, lines are just skipped for now
*
*   DEPENDENCIES: C standard library:
*       - stdio.h: fopen(), feof(), fgets(), fclose(), fprintf()
*       - stdlib.h: malloc(), calloc(), free()
*       - string.h: memset(), memcpy(), strcmp(), strlen()
*
*   VERSIONS HISTORY:
*       1.0 (18-May-2023) First release, basic read/write functionality
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2023 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#ifndef RINI_H
#define RINI_H

#define RINI_VERSION    "1.0"

// Function specifiers in case library is build/used as a shared library (Windows)
// NOTE: Microsoft specifiers to tell compiler that symbols are imported/exported from a .dll
#if defined(_WIN32)
    #if defined(BUILD_LIBTYPE_SHARED)
        #define RINIAPI __declspec(dllexport)     // We are building the library as a Win32 shared library (.dll)
    #elif defined(USE_LIBTYPE_SHARED)
        #define RINIAPI __declspec(dllimport)     // We are using the library as a Win32 shared library (.dll)
    #endif
#endif

// Function specifiers definition
#ifndef RINIAPI
    #define RINIAPI       // Functions defined as 'extern' by default (implicit specifiers)
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Allow custom memory allocators
#ifndef RINI_MALLOC
    #define RINI_MALLOC(sz)       malloc(sz)
#endif
#ifndef RINI_CALLOC
    #define RINI_CALLOC(n,sz)     calloc(n,sz)
#endif
#ifndef RINI_FREE
    #define RINI_FREE(p)          free(p)
#endif

// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define RINI_SUPPORT_LOG_INFO
#if defined(RINI_SUPPORT_LOG_INFO)
  #define RINI_LOG(...)           printf(__VA_ARGS__)
#else
  #define RINI_LOG(...)
#endif

#if !defined(RINI_MAX_LINE_SIZE)
    #define RINI_MAX_LINE_SIZE              512
#endif

#if !defined(RINI_MAX_KEY_SIZE)
    #define RINI_MAX_KEY_SIZE                64
#endif

#if !defined(RINI_MAX_TEXT_SIZE)
    #define RINI_MAX_TEXT_SIZE               64
#endif

#if !defined(RINI_MAX_DESC_SIZE)
    #define RINI_MAX_DESC_SIZE              128
#endif

#if !defined(RINI_MAX_VALUE_CAPACITY)
    #define RINI_MAX_VALUE_CAPACITY          32
#endif

#if !defined(RINI_VALUE_DELIMITER)
    #define RINI_VALUE_DELIMITER            ' '
#endif
#if !defined(RINI_LINE_COMMENT_DELIMITER)
    #define RINI_LINE_COMMENT_DELIMITER     '#'
#endif
#if !defined(RINI_VALUE_COMMENTS_DELIMITER)
    #define RINI_VALUE_COMMENTS_DELIMITER   '#'
#endif
#if !defined(RINI_LINE_SECTION_DELIMITER)
    #define RINI_LINE_SECTION_DELIMITER     '['
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Config value entry
typedef struct {
    char key[RINI_MAX_KEY_SIZE];    // Config value key identifier
    char text[RINI_MAX_TEXT_SIZE];  // Config value text
    char desc[RINI_MAX_DESC_SIZE];  // Config value description
} rini_config_value;

// Config data
typedef struct {
    rini_config_value *values;      // Config values array
    unsigned int count;             // Config values count
    unsigned int capacity;          // Config values capacity
} rini_config;

#if defined(__cplusplus)
extern "C" {                    // Prevents name mangling of functions
#endif

//------------------------------------------------------------------------------------
// Functions declaration
//------------------------------------------------------------------------------------
RINIAPI rini_config rini_load_config(const char *file_name);            // Load config from file (*.ini) or create a new config object (pass NULL)
RINIAPI void rini_unload_config(rini_config *config);                   // Unload config data from memory
RINIAPI void rini_save_config(rini_config config, const char *file_name, const char *header); // Save config to file, with custom header

RINIAPI int rini_get_config_value(rini_config config, const char *key); // Get config value int for provided key, returns -1 if not found
RINIAPI const char *rini_get_config_value_text(rini_config config, const char *key); // Get config value text for provided key
RINIAPI const char *rini_get_config_value_description(rini_config config, const char *key); // Get config value description for provided key

// Set config value int/text and description for existing key or create a new entry
// NOTE: When setting a text value, if id does not exist, a new entry is automatically created
RINIAPI int rini_set_config_value(rini_config *config, const char *key, int value, const char *desc);
RINIAPI int rini_set_config_value_text(rini_config *config, const char *key, const char *text, const char *desc);

// Set config value description for existing key
// WARNING: Key must exist to add description, if a description exists, it is updated
RINIAPI int rini_set_config_value_description(rini_config *config, const char *key, const char *desc);

#ifdef __cplusplus
}
#endif

#endif // RINI_H