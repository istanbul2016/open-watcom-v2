/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  List all public symbols defined in an object file.
*
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bool.h"
#include "watcom.h"
#include "pcobj.h"
#include "misc.h"


typedef unsigned char byte;

static char         *NamePtr;
static byte         NameLen;
static unsigned_16  RecLen;
static char         *RecBuff;
static char         *RecPtr;
static unsigned_16  RecMaxLen;
static byte         RecHdr[3];

static void usage( void )
/***********************/
{
    printf( "Usage: objxdef <list of object or library files>\n" );
}

static bool IsDataRec( void )
/***************************/
{
    return( RecPtr - RecBuff < RecLen - 1 );
}

static byte GetByte( void )
/*************************/
{
    return( *RecPtr++ );
}

static unsigned_16 GetUInt( void )
/********************************/
{
    unsigned_16 word;

    word = *(unsigned_16 *)RecPtr;
    CONV_LE_16( word );
    RecPtr += sizeof( unsigned_16 );
    return( word );
}

static unsigned_32 GetOffset( void )
/**********************************/
{
    if( RecHdr[0] & 1 ) {
        unsigned_32 dword;

        dword = *(unsigned_32 *)RecPtr;
        CONV_LE_32( dword );
        RecPtr += sizeof( unsigned_32 );
        return( dword );
    } else {
        unsigned_16 word;

        word = *(unsigned_16 *)RecPtr;
        CONV_LE_16( word );
        RecPtr += sizeof( unsigned_16 );
        return( word );
    }
}

static unsigned_16 GetIndex( void )
/*********************************/
{
    unsigned_16 index;

    index = GetByte();
    if( index & 0x80 ) {
        index = ( (index & 0x7f) << 8 ) + GetByte();
    }
    return( index );
}

static char *GetName( void )
/**************************/
{
    NameLen = GetByte();
    NamePtr = RecPtr;
    RecPtr += NameLen;
    return( NamePtr );
}

static bool ExtendRecBuff( unsigned_16 size )
/*******************************************/
{
    if( RecMaxLen < size ) {
        RecMaxLen = size;
        if( RecBuff != NULL ) {
            free( RecBuff );
        }
        RecBuff = malloc( RecMaxLen );
        if( RecBuff == NULL ) {
            printf( "**FATAL** Out of memory!\n" );
            return( false );
        }
    }
    return( true );
}

static bool ReadRec( FILE *fp )
/*****************************/
{
    bool    ok;

    RecLen = RecHdr[1] | ( RecHdr[2] << 8 );
    ok = ExtendRecBuff( RecLen );
    if( ok ) {
        ok = ( fread( RecBuff, RecLen, 1, fp ) != 0 );
    }
    RecPtr = RecBuff;
    return( ok );
}


static bool ProcFile( FILE *fp )
/******************************/
{
    unsigned_16 page_len;
    unsigned_32 offset;
    bool        ok;

    page_len = 0;
    RecBuff = NULL;
    RecMaxLen = 0;
    for(;;) {
        offset = ftell( fp );
        if( fread( RecHdr, 1, 3, fp ) != 3 ) {
            ok = ( ferror( fp ) == 0 );
            break;
        }
        ok = ReadRec( fp );
        if( !ok )
            break;
        switch( RecHdr[0] & ~1 ) {
        case CMD_MODEND:
            if( page_len != 0 ) {
                offset = ftell( fp );
                offset = page_len - offset % page_len;
                if( offset != page_len ) {
                    fseek( fp, offset, SEEK_CUR );
                }
            }
            break;
        case CMD_PUBDEF:
            if( ( GetIndex() | GetIndex() ) == 0 )
                GetUInt();
            while( IsDataRec() ) {
                GetName();
                *RecPtr = '\0';
                printf( "%s\n", NamePtr );
                GetOffset();
                GetIndex();
            }
            break;
        case LIB_HEADER_REC:
            if( RecHdr[0] & 1 ) {
                fseek( fp, 0L, SEEK_END );
                page_len = 0;
            } else {
                page_len = RecLen - 1 + 4;
            }
            break;
        default:
            break;
        }
    }
    free( RecBuff );
    return( true );
}

static bool process_file_pubdef( const char *filename )
/*****************************************************/
{
    FILE    *fp;
    bool    ok;

    fp = fopen( filename, "rb" );
    if( fp == NULL ) {
        printf( "Cannot open input file: %s.\n", filename );
        return( false );
    }
    ok = ProcFile( fp );
    fclose( fp );
    return( ok );
}

int main( int argc, char *argv[] )
/********************************/
{
    int     i;
    char    *fn;
    bool    ok;

    if( argc == 1 ) {
        usage();
        return( 1 );
    }
    ok = true;
    for( i = 1; i < argc; ++i ) {
        fn = DoWildCard( argv[i] );
        while( fn != NULL ) {
            ok &= process_file_pubdef( fn );
            fn = DoWildCard( NULL );
        }
        DoWildCardClose();
    }
    return( ok ? EXIT_SUCCESS : EXIT_FAILURE );
}
