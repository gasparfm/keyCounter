/* @(#)cfileutils.h
 */

#ifndef _CFILEUTILS_H
#define _CFILEUTILS_H 1

/**
 * Returns a bytes size into a human readable form in Kb, Mb, Gb, Tb (Tera), Pb (Peta), Eb (Exa), Zb (Zetta), Yb (Yotta), Bb (Bronto)
 * 
 * @param store char* variable where to store the data
 * @param size size to work with
 *
 * @return store
 */
char *human_size(char *store, long double size);

/**
 * Calculates how many bytes correspond to a human unit like the ones used by @see human_size.
 * Units are multiple of 1024
 * 
 * @param human unit to measure
 *
 * @return byte size
 */
long long human_size_multiplier(const char *human);

/**
 * Transform a string like "10Kb", "20Mb" to bytes
 * 
 * @param human human size (number followed by two chars of unit)
 *
 * @return bytes
 */
long long human_to_bytes(const char *human);

/**
 * Calculates file size in bytes
 * 
 * @param fileName filename to check
 *
 * @return size in bytes
 */
long long file_size(char *fileName);

/**
 * Checks if a file exists in the file system. You may specify the full path.
 *
 * @param filename
 *
 * @return 0 if file is not found, 1 if file exists, -1 on error
 */
short file_exists(char *filename);

/**
 * Checks if directory exists.
 *
 * @param directory directory
 *
 * @return -2 if it's not a directory, -1 on error, 0 if directory does not exist, 1 if directory exists
 */
short directory_exists(const char *directory);

/**
 * Return current user's home directory
 *
 * @return home directory
 */
char *getHomeDir();

/**
 * Makes a path to a file, and store it all together into out
 *
 * @param out       Double pointer! It will allocate memory. 
 *                  If we input a non-null, it will try to free()
 * @param directory Directory of the path. If it not ends in /, it will be added
 * @param file      File o subdirectories + file
 *
 * @return *out
 */
char* makePath(char **out, const char *directory, const char *file);

/**
 * Copy file origin to destination
 *
 * @param origin  Origin file name
 * @param destination Destination file name
 * 
 * @return error code:
 *  · 0  success !
 *  · -1 can't open origin file
 *  · -2 can't open destination file
 *  · -3 anticipated read error
 *  · -4 write error
 *  · -5 read errorr
 */
int copyFile(char *origin, char *destination);

/**
 * Removes file nombre
 *
 * @param name  File name
 *
 * @return error code:
 *  · 0  - removed successfully
 *  · -1 - file doesn't exist
 *  · -2 - can't remove file
 */
int removeFile(char *name);

int file_get_contents(char **data, char *fileName, int freeNotNull);

/* Necesaria ¿? */
int createDir (const char *dirName, mode_t mode);
#endif /* _CFILEUTILS_H */

