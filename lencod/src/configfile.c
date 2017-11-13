
#define INCLUDED_BY_CONFIGFILE_C

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <IO.H>
#include <fcntl.h>

#include "global.h"
#include "configfile.h"

#define MAX_ITEMS_TO_PARSE  10000
#define _S_IREAD            0000400         /* read permission, owner */
#define _S_IWRITE           0000200         /* write permission, owner */

static char *GetConfigFileContent ( char *Filename );
static void ParseContent ( char *buf, int bufsize );
static int ParameterNameToMapIndex ( char *s );
static void PatchInp ();

/*
*************************************************************************
* Function:Parse the command line parameters and read the config files.
* Input: ac
number of command line parameters
av
command line parameters
* Output:
* Return:
* Attention:
*************************************************************************
*/

void Configure ( int ac, char *av[] )
{
    char *content;
    int CLcount, ContentLen, NumberParams;

    memset( &configinput, 0, sizeof ( InpParams ) );

    CLcount = 1;
    while ( CLcount < ac )
    {
        if ( 0 == strncmp( av[CLcount], "-f", 2 ) ) // A parameter file
        {
            content = GetConfigFileContent( av[CLcount + 1] );
            printf( "Parsing Configfile %s", av[CLcount + 1] );
            ParseContent( content, (int)strlen( content ) );
            printf( "\n" );
            free( content );
            CLcount += 2;
        }
        else
        {
            if ( 0 == strncmp( av[CLcount], "-p", 2 ) ) // A configuration parameter
            {
                // Collect all data until next parameter (starting with -<x> (x is any character)),
                // put it into content, and parse content.
                CLcount++;
                ContentLen = 0;
                NumberParams = CLcount;

                // determine the necessary size for content
                while ( NumberParams < ac && av[NumberParams][0] != '-' )
                {
                    ContentLen += (int)strlen( av[NumberParams++] );    // Space for all the strings
                }

                ContentLen += 1000;                     // Additional 1000 bytes for spaces and \0s

                if ( ( content = malloc( ContentLen ) ) == NULL )
                {
                    no_mem_exit( "Configure: content" );
                }

                content[0] = '\0';

                // concatenate all parameters itendified before
                while ( CLcount < NumberParams )
                {
                    char *source = &av[CLcount][0];
                    char *destin = &content[strlen( content )];

                    while ( *source != '\0' )
                    {
                        if ( *source == '=' ) // The Parser expects whitespace before and after '='
                        {
                            *destin++ = ' ';
                            *destin++ = '=';
                            *destin++ = ' ';  // Hence make sure we add it
                        }
                        else
                        {
                            *destin++ = *source;
                        }

                        source++;
                    }

                    *destin++ = ' ';            // add a space to support multiple config items
                    *destin = '\0';
                    CLcount++;
                }
                printf( "Parsing command line string '%s'", content );
                ParseContent( content, (int)strlen( content ) );
                free( content );
                printf( "\n" );
            }
            else
            {
                fprintf( stderr, "Error in command line, ac %d, around string '%s', missing -f or -p parameters?", CLcount, av[CLcount] );
                exit(1);
            }
        }
    }

    PatchInp();
    printf( "\n" );

    // parameters should be configured in the encoder software
    input->fixed_picture_qp = 1; // fixed_picture_qp can vary in different frames
    if (!input->fixed_picture_qp)
    {
        input->fixed_slice_qp = 1;
    }
}

/*
*************************************************************************
* Function: Alocate memory buf, opens file Filename in f, reads contents into
buf and returns buf
* Input:name of config file
* Output:
* Return:
* Attention:
*************************************************************************
*/
char *GetConfigFileContent ( char *Filename )
{
    unsigned FileSize;
    FILE *f;
    char *buf;

    if ( NULL == ( f = fopen ( Filename, "r" ) ) )
    {
        fprintf(stderr, "Cannot open configuration file %s.\n", Filename);
        exit(1);
    }

    if ( 0 != fseek ( f, 0, SEEK_END ) )
    {
        fprintf(stderr, "Cannot fseek in configuration file %s.\n", Filename);
        exit(1);
    }

    FileSize = ftell ( f );

    if ( FileSize < 0 || FileSize > 60000 )
    {
        fprintf(stderr, "Unreasonable Filesize %d reported by ftell for configuration file %s.\n", FileSize, Filename);
        exit(1);
    }

    if ( 0 != fseek ( f, 0, SEEK_SET ) )
    {
        fprintf(stderr, "Cannot fseek in configuration file %s.\n", Filename);
        exit(1);
    }

    if ( ( buf = malloc ( FileSize + 1 ) )==NULL )
    {
        no_mem_exit( "GetConfigFileContent: buf" );
    }

    // Note that ftell() gives us the file size as the file system sees it.  The actual file size,
    // as reported by fread() below will be often smaller due to CR/LF to CR conversion and/or
    // control characters after the dos EOF marker in the file.

    FileSize = (int)fread ( buf, 1, FileSize, f );
    buf[FileSize] = '\0';

    fclose ( f );

    return buf;
}

/*
*************************************************************************
* Function: Parses the character array buf and writes global variable input, which is defined in
configfile.h.  This hack will continue to be necessary to facilitate the addition of
new parameters through the Map[] mechanism (Need compiler-generated addresses in map[]).
* Input:  buf
buffer to be parsed
bufsize
buffer size of buffer
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void ParseContent ( char *buf, int bufsize )
{
    char *items[MAX_ITEMS_TO_PARSE];
    int MapIdx;
    int item = 0;
    int InString = 0;
    int InItem = 0;
    char *p = buf;
    char *bufend = &buf[bufsize];
    int IntContent;
    int i;

    // Stage one: Generate an argc/argv-type list in items[], without comments and whitespace.
    // This is context insensitive and could be done most easily with lex(1).

    while ( p < bufend )
    {
        switch ( *p )
        {
            case 13:
                p++;
                break;
            case '#':                 // Found comment
                *p = '\0';              // Replace '#' with '\0' in case of comment immediately following integer or string
                while ( *p != '\n' && p < bufend ) // Skip till EOL or EOF, whichever comes first
                {
                    p++;
                }
                InString = 0;
                InItem = 0;
                break;
            case '\n':
                InItem = 0;
                InString = 0;
                *p++ = '\0';
                break;
            case ' ':
            case '\t':              // Skip whitespace, leave state unchanged
                if ( InString )
                {
                    p++;
                }
                else
                {
                    // Terminate non-strings once whitespace is found
                    *p++ = '\0';
                    InItem = 0;
                }
                break;
            case '"':               // Begin/End of String
                *p++ = '\0';
                if ( !InString )
                {
                    items[item++] = p;
                    InItem = ~InItem;
                }
                else
                {
                    InItem = 0;
                }
                InString = ~InString; // Toggle
                break;
            default:
                if ( !InItem )
                {
                    items[item++] = p;
                    InItem = ~InItem;
                }
                p++;
        }
    }

    item--;

    for ( i = 0; i<item; i += 3 )
    {
        if ( 0 >( MapIdx = ParameterNameToMapIndex( items[i] ) ) )
        {
            fprintf(stderr, " Parsing error in config file: Parameter Name '%s' not recognized.", items[i]);
            exit(1);
        }
        if ( strcmp( "=", items[i + 1] ) )
        {
            fprintf(stderr, " Parsing error in config file: '=' expected as the second token in each line.");
            exit(1);
        }

        // Now interprete the Value, context sensitive...
        switch ( Map[MapIdx].Type )
        {
            case 0:           // Numerical
                if ( 1 != sscanf( items[i + 2], "%d", &IntContent ) )
                {
                    fprintf(stderr, " Parsing error: Expected numerical value for Parameter of %s, found '%s'.", items[i], items[i + 2]);
                    error( errortext, 300 );
                }
                *( int * )( Map[MapIdx].Place ) = IntContent;
                printf( "." );
                break;
            case 1:
                strcpy( ( char * )Map[MapIdx].Place, items[i + 2] );
                printf( "." );
                break;
            default:
                assert( "Unknown value type in the map definition of configfile.h" );
        }
    }

    memcpy( input, &configinput, sizeof ( InpParams ) );
}

/*
*************************************************************************
* Function:Return the index number from Map[] for a given parameter name.
* Input:parameter name string
* Output:
* Return: the index number if the string is a valid parameter name,         \n
-1 for error
* Attention:
*************************************************************************
*/

static int ParameterNameToMapIndex ( char *s )
{
    int i = 0;

    while ( Map[i].TokenName != NULL )
        if ( 0==strcmp ( Map[i].TokenName, s ) )
        {
            return i;
        }
        else
        {
            i++;
        }

    return -1;
}

/*
*************************************************************************
* Function:Checks the input parameters for consistency.
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/

static void PatchInp ()
{
    // consistency check no_multpred
    if (input->inp_ref_num < 1)
    {
        input->inp_ref_num = 1;
    }

    // consistency check of QPs
    input->fixed_picture_qp = 1;

    if ( input->qp_1st_frm > MAX_QP || input->qp_1st_frm < MIN_QP )
    {
        fprintf(stderr, "Error input parameter quant_0,check configuration file");
        error( errortext, 400 );
    }

    if ( input->qp_P_frm > MAX_QP || input->qp_P_frm < MIN_QP )
    {
        fprintf(stderr, "Error input parameter quant_n,check configuration file");
        error( errortext, 400 );
    }

    if ( input->qp_B_frm > MAX_QP || input->qp_B_frm < MIN_QP )
    {
        fprintf(stderr, "Error input parameter quant_B,check configuration file");
        error( errortext, 400 );
    }

    if ( input->successive_Bframe == 0 )
    {
        input->low_delay = 1;
    }
    else
    {
        input->low_delay = 0;
    }

    // Open Files
    if ( strlen( input->infile ) > 0 && ( p_in = _open( input->infile, O_RDONLY | O_BINARY ) ) == -1 )
    {
      sprintf(errortext, "\n>>>>>>>>>> Input file %s does not exist <<<<<<<<<<\n", input->infile);
      error(errortext, 400);
    }

    if (input->output_enc_pic) //output_enc_pic
    {
        if (strlen(input->ReconFile) > 0 && (p_dec = _open(input->ReconFile, _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC, _S_IREAD | _S_IWRITE)) == -1)
        {
            fprintf(stderr, "Error open file %s", input->ReconFile);
        }
    }

#if TRACE
    if ( (p_trace = fopen( ENC_TRACE_FILE, "w" )) == NULL )
    {
        fprintf(stderr, "Error open file %s", ENC_TRACE_FILE );
        error( errortext,500 );
    }
#endif

    if ( input->seqheader_period != 0 && input->intra_period == 0 )
    {
        if ( input->intra_period == 0 )
        {
            fprintf(stderr, "\nintra_period  should not equal %d when seqheader_period equal %d", input->intra_period, input->seqheader_period);
        }
        error( errortext, 400 );
    }

    // input intra period and Seqheader check Add cjw
    if ( input->seqheader_period == 0 && input->vec_period != 0 )
    {
        fprintf(stderr, "\nvec_period  should not equal %d when seqheader_period equal %d", input->intra_period, input->seqheader_period);
        error( errortext, 400 );
    }
    {
        int ref_num[10] = { 1, 2, 4, 8, 12, 16, 20, 24, 28, 32 };
        if ( input->inp_ref_num < 1 || input->inp_ref_num > 10 )
        {
            printf( "Check the number of reference frames in encoder.cfg\n" );
            exit( 0 );
        }
        img->real_ref_num = ref_num[input->inp_ref_num - 1];
    }

}

/*
******************************************************************************
*  Function: Determine the MVD's value (1/4 pixel) is legal or not.
*  Input:
*  Output:
*  Return: 0: out of the legal mv range; 1: in the legal mv range
*  Attention:
*  Author:
******************************************************************************
*/
void DecideMvRange()
{
    switch( input->level_id )
    {
        case 0x10:
            Min_V_MV = -512;
            Max_V_MV =  511;
            Min_H_MV = -8192;
            Max_H_MV =  8191;
            break;
        case 0x20:
            Min_V_MV = -1024;
            Max_V_MV =  1023;
            Min_H_MV = -8192;
            Max_H_MV =  8191;
            break;
        case 0x40:
            Min_V_MV = -2048;
            Max_V_MV =  2047;
            Min_H_MV = -8192;
            Max_H_MV =  8191;
            break;
    }
}

/*
******************************************************************************
*  Function: Check Tools in Current Profile.
*  Input:
*  Output:
*  Return:
*  Attention:
*  Author:
*  Date:
******************************************************************************
*/
void CheckToolsInProfile()
{
    if( input->frame_rate_code>10 )
    {
        if( input->frame_rate_code==24 )
        {
            input->frame_rate_code=2;
        }
        else if( input->frame_rate_code==30 )
        {
            input->frame_rate_code=5;
        }
        else if( input->frame_rate_code==50 )
        {
            input->frame_rate_code=6;
        }
        else if( input->frame_rate_code==60 )
        {
            input->frame_rate_code=8;
        }
    }
}