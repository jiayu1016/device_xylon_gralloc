/*
    Helper functions for parsing /proc/device-tree entries

    Copyright (C) 2012 Xylon d.o.o.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LOGI3D_DEVTREE_H
#define LOGI3D_DEVTREE_H 1

#ifdef __cplusplus
extern "C" {
#endif

/**
* Retuns the full path of the searched file or directory
* @param[in]    root is absolute path of the top folder where we start the search
* @param[in]    name is file/folder name which we search (name may be incomplete)
* @param[out]   out_path is returned absolute path if found
* @return       0 is success
******************************************************************************/
int dt_get_path(const char *const root, char* const name, char *const out_path);

/**
* Retuns the integer value read from file
* @param[in]    root is absolute path of the top folder where we start the search
* @param[in]    name is file name which we read
* @param[out]   out_int is returned number
* @return       0 is success
******************************************************************************/
int dt_get_int(const char *const root, const char* const name, int *const out_int);

/**
* Retuns the integer value read from file
* @param[in]    root is absolute path of the top folder where we start the search
* @param[in]    name is file name which we read
* @param[out]   out_str is returned string, allocation must be done outside
* @param[out]   len is number of bytes available in out_str, and number of bytes read
* @return       0 is success
******************************************************************************/
int dt_get_string(const char *const root, const char* const name, char *const out_str, int *const len);

/**
* Retuns pair of integer values read from file
* @param[in]    root is absolute path of the top folder where we start the search
* @param[in]    name is file name which we read
* @param[out]   base is returned base address
* @param[out]   len is number of bytes (length of address range)
* @return       0 is success
******************************************************************************/
int dt_get_int_pair(const char * const root, const char* const name, int *const base, int *const len);

#ifdef __cplusplus
}
#endif

#endif /* LOGI3D_DEVTREE_H */
