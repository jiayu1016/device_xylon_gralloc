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


#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <asm/byteorder.h>

#include "logi3D_devtree.h"


//! ???
/*! ???
*/
static int
dt_find_path(
             const char * const searching,
             char * const result,
             const int strict
             )
{
    DIR *d;
    char cwd[MAXPATHLEN];
    struct dirent *dir;
    int found = 0;

    /* Get current working directory. */
    getcwd( cwd, MAXPATHLEN );

    d = opendir( "." );
    if( d == NULL ) 
    {
        return 1;
    }

    /* First check all files in current directory. */
    while( ( dir = readdir( d ) ) )
    {
        if( strcmp( dir->d_name, "."  ) == 0 ||
            strcmp( dir->d_name, ".." ) == 0 )
        {
            continue;
        }
        /* if */

        if(strict)
        {
            if( strcmp( dir->d_name, searching ) == 0 )
            {
                int  len;
                getcwd( result, MAXPATHLEN );
                len = strlen( result );
                snprintf( result + len, MAXPATHLEN - len, "/%s", dir->d_name );
                found = 1;
                break;
            }
        }
        else
        {
            if( strstr( dir->d_name, searching ) != 0 )
            {
                int  len;
                getcwd( result, MAXPATHLEN );
                len = strlen( result );
                snprintf( result + len, MAXPATHLEN - len, "/%s", dir->d_name );
                found = 1;
                break;
            }
            /* if */
        }
        /* if */
    }
    /* while */

    closedir( d );


    if( found == 0)
    {
        d = opendir( "." );

        /* If not found, check all directories. */
        while( ( dir = readdir( d ) ) && (found == 0) )
        {
            if( strcmp( dir->d_name, "."  ) == 0 ||
                strcmp( dir->d_name, ".." ) == 0 )
            {
                continue;
            }
            /* if */

            if( dir->d_type == DT_DIR )
            {
                chdir( dir->d_name );

                if( 0 == dt_find_path( searching, result, strict ) )
                {
                    found = 1;
                    break;
                }
                /* if */

                chdir( ".." );
            }
            /* if */

        }
        /* while */

        closedir( d );

    }
    /* if */


    /* Restore current working directory. */
    chdir(cwd);

    return (found == 0);
}
/* dt_find_path */



//! ???
/*! ???
*/
static int
dt_read_int(
            const char * const file,
            int * const out,
            int * const len
            )
{
  FILE *in;
  int i, size=0, ret=0, read;

  in = fopen ( file, "rb" );

  if ( in == NULL )
  {
    printf( "Unable to open the file" );
    ret = -1;
  }

  if(!ret)
  {
      for(i=0; i<*len; i++)
      {
          read = fread ( &out[i], 1, 4, in );
          if(read == 4)
          {
              unsigned char *pb = (unsigned char *)&out[i];
              size += read;
              out[i] = ((int)pb[0] << 24) | ((int)pb[1] << 16) | ((int)pb[2] << 8) | pb[3];
              printf( "%s() %s -> %d, 0X%x\n", __FUNCTION__, file, (int)out[i], (int)out[i]);
          }
      }
      *len = size;
      fclose ( in );
  }
  return ret;
}
/* dt_read_int */



//! ???
/*! ???
*/
static int
dt_read_char(
             const char * const file,
             char * const out,
             int  * const len
             )
{
  FILE *in;
  int size, ret=0;

  in = fopen ( file, "rb" );

  if ( in == NULL )
  {
    printf( "Unable to open the file" );
    ret = -1;
  }

  if(!ret)
  {
      /* Read a string */
      size = fread ( out, 1, *len, in );
      if(size > 0)
      {
          printf( "%s() %s -> %s\n", __FUNCTION__, file, out);
      }
      else
      {
        ret =-1;
      }
      fclose ( in );
  }
  return ret;
}
/* dt_read_char */



//! ???
/*! ???
*/
int
dt_get_path(
            const char * const root,
            char * const name,
            char * const out_path
            )
{
    int ret =-1;
    char cwd[MAXPATHLEN];

    /* Get current working directory. */
    getcwd( cwd, MAXPATHLEN );

    if(chdir( root) ==0)
    {
        if(dt_find_path( name, out_path, 0)==0)
        {
            ret = 0;
        }
    }

    /* Restore current working directory. */
    chdir(cwd);

    return ret;
}
/* dt_get_path */



//! ???
/*! ???
*/
int
dt_get_int(
           const char * const root,
           const char * const name,
           int  * const out_int
           )
{
    int ret =-1;
    char out_path[MAXPATHLEN] = { 0 };
    char cwd[MAXPATHLEN];

    /* Get current working directory. */
    getcwd( cwd, MAXPATHLEN );

    if(chdir( root) ==0)
    {
        if(dt_find_path(name, out_path, 1)==0)
        {
            int len = 1;
            if(dt_read_int(out_path, out_int, &len) == 0)
                ret = 0;
        }
    }

    /* Restore current working directory. */
    chdir(cwd);

    return ret;
}
/* dt_get_int */



//! ???
/*! ???
*/
int
dt_get_string(
              const char * const root,
              const char * const name,
              char * const out_str,
              int  * const len
              )
{
    int ret =-1;
    char out_path[MAXPATHLEN] = { 0 };
    char cwd[MAXPATHLEN];

    /* Get current working directory. */
    getcwd( cwd, MAXPATHLEN );

    if(chdir(root) == 0)
    {
        if(dt_find_path( name, out_path, 1)==0)
        {
            if(dt_read_char(out_path, out_str, len) == 0)
                ret = 0;
        }
    }

    /* Restore current working directory. */
    chdir(cwd);

    return ret;
}
/* dt_get_string */



//! ???
/*! ???
*/
int
dt_get_int_pair(
           const char * const root,
           const char * const name,
           int  * const base,
           int  * const len
           )
{
    int ret =-1;
    char out_path[MAXPATHLEN] = { 0 };
    char cwd[MAXPATHLEN];

    /* Get current working directory. */
    getcwd( cwd, MAXPATHLEN );

    if(chdir( root) ==0)
    {
        if(dt_find_path( name, out_path, 1)==0)
        {
            int lenth = 2;
            int out[2];
            if(dt_read_int(out_path, out, &lenth) == 0)
            {
                *base = out[0];
                *len  = out[1];
                ret = 0;
            }
        }
    }

    /* Restore current working directory. */
    chdir(cwd);

    return ret;
}
/* dt_get_int_pair */

