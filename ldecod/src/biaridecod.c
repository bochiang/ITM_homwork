/*!
*************************************************************************************
* \file biaridecod.c
*
* \brief
*    binary arithmetic decoder routines
* \date
*
* \author
*    see contributors.h 
*************************************************************************************
*/
#include <assert.h>

#include "math.h"
#include "global.h"
#include "memalloc.h"
#include "../../common/common.h"

extern BbvBuffer_t *pBbv;

// Get one byte from the decoder stream
void get_byte( Env_AEC * dep )
{
    dep->Dbuffer = dep->Dcodestrm[( *dep->Dcodestrm_len )++];
    dep->bits_to_go = 7;
    if ( input->check_BBV_flag && pBbv )
    {
        pBbv->frame_code_bits += 8;
    }
}

/*!
************************************************************************
* \brief
*    Allocates memory for the Env_AEC struct
* \return DecodingContextPtr
*    allocates memory
************************************************************************
*/
Env_AEC * arideco_create_decoding_environment()
{
    Env_AEC * dep;

    if ( ( dep = ( Env_AEC * )calloc( 1,sizeof( Env_AEC ) ) ) == NULL )
    {
        no_mem_exit( "arideco_create_decoding_environment: dep" );
    }
    return dep;
}

/*!
************************************************************************
* \brief
*    Initializes the Env_AEC for the arithmetic coder
************************************************************************
*/
void arideco_start_decoding( Env_AEC *dep, uchar_t *cpixcode, int firstbyte, int *cpixcode_len )
{
    int i;
    dep->Dcodestrm = cpixcode;
    dep->Dcodestrm_len = cpixcode_len;
    *dep->Dcodestrm_len = firstbyte;

    dep->E_s1 = 0;
    dep->E_t1 = QUARTER - 1;

    dep->value_s = 0;
    dep->value_t = 0;
    dep->bits_to_go = 0;

    for ( i = 0; i < B_BITS - 1 ; i++ )
    {
        if ( --dep->bits_to_go < 0 )
        {
            get_byte( dep );
        }
        // append the first bit of Dbuffer to the end of value_t, 9-bits are obtained and stored in value_t
        dep->value_t = ( dep->value_t << 1 )  | ( ( dep->Dbuffer >> dep->bits_to_go ) & 0x01 );
    }

    while ( dep->value_t < QUARTER )
    {
        if ( --dep->bits_to_go < 0 )
        {
            get_byte( dep );
        }
        // Shift in next bit and add to value
        dep->value_t = ( dep->value_t << 1 ) | ( ( dep->Dbuffer >> dep->bits_to_go ) & 0x01 );
        dep->value_s++;
    }
    dep->value_t = dep->value_t & 0xff; // get the low 8-bits of value_t
}



/*!
************************************************************************
* \brief
*    arideco_bits_read
************************************************************************
*/
int arideco_bits_read( Env_AEC * dep )
{
    return 8 * ( ( *dep->Dcodestrm_len ) - 1 ) + ( 8 - dep->bits_to_go );
}


/*!
************************************************************************
* \brief
*    arideco_done_decoding():
************************************************************************
*/
void arideco_done_decoding( Env_AEC * dep )
{
    if ( dep->bits_to_go != 8 )
    {
        currStream->read_len --;
        //arienc_start_encoding
        currStream->byte_offset += ( currStream->read_len ) * 8; //multiple slice
        dep->bits_to_go = 8;
    }
    else
    {
        currStream->read_len -= 2;
        currStream->byte_offset += ( currStream->read_len ) * 8; //multiple slice
    }
}

i32u_t biari_decode_symbol( Env_AEC * dep, BiContextTypePtr bi_ct )
{
    uchar_t bit;
    uchar_t s_flag, is_LPS=0;
    uchar_t cwr, cycno = bi_ct->cycno;
    i32u_t  lg_pmps= bi_ct->LG_PMPS;
    i32u_t  t_rlps;
    i32u_t  s1, t1, s2, t2;

    s1 = dep->E_s1;
    t1 = dep->E_t1;
    bit = bi_ct->MPS;

    cwr = ( cycno <= 1 ) ? 3 : ( cycno == 2 ) ? 4 : 5; //FAST ADAPTION PARAMETER


    if ( t1 >= ( lg_pmps >> LG_PMPS_SHIFTNO ) )
    {
        s2 = s1;
        t2 = t1 - ( lg_pmps >> LG_PMPS_SHIFTNO ); //8bits
        s_flag = 0;
    }
    else
    {
        s2 = s1 + 1;
        t2 = 256 + t1 - ( lg_pmps >> LG_PMPS_SHIFTNO ); //8bits
        s_flag = 1;
    }


    if ( s2 > dep->value_s || ( s2 == dep->value_s && dep->value_t >= t2 ) ) //LPS
    {
        is_LPS = 1;
        bit = !bit; //LPS

        t_rlps = ( s_flag == 0 ) ? ( lg_pmps >> LG_PMPS_SHIFTNO )
                 : ( t1 + ( lg_pmps >> LG_PMPS_SHIFTNO ) );

        if ( s2 == dep->value_s )
        {
            dep->value_t = ( dep->value_t - t2 );
        }
        else
        {
            if ( --dep->bits_to_go < 0 )
            {
                get_byte( dep );
            }
            // Shift in next bit and add to value
            dep->value_t = ( dep->value_t << 1 ) | ( ( dep->Dbuffer >> dep->bits_to_go ) & 0x01 );
            dep->value_t = 256 + dep->value_t - t2;
        }

        //restore range
        while ( t_rlps < QUARTER )
        {
            t_rlps = t_rlps << 1;
            if ( --dep->bits_to_go < 0 )
            {
                get_byte( dep );
            }
            // Shift in next bit and add to value
            dep->value_t = ( dep->value_t << 1 ) | ( ( dep->Dbuffer >> dep->bits_to_go ) & 0x01 );
        }

        s1 = 0;
        t1 = t_rlps & 0xff;

        //restore value
        dep->value_s = 0;
        while ( dep->value_t < QUARTER )
        {
            int j;
            if ( --dep->bits_to_go < 0 )
            {
                get_byte( dep );
            }
            j = ( dep->Dbuffer >> dep->bits_to_go ) & 0x01;
            // Shift in next bit and add to value

            dep->value_t = ( dep->value_t << 1 ) | j;
            dep->value_s++;
        }
        dep->value_t = dep->value_t & 0xff;
    }
    else     //MPS
    {
        s1 = s2;
        t1 = t2;
    }

    dep->E_s1 = s1;
    dep->E_t1 = t1;

    //update other parameters
    if ( is_LPS )
    {
        cycno = ( cycno <= 2 ) ? ( cycno + 1 ) : 3;
    }
    else if ( cycno == 0 )
    {
        cycno = 1;
    }
    bi_ct->cycno = cycno;

    //update probability estimation
    if ( is_LPS )
    {
        switch ( cwr )
        {
            case 3:
                lg_pmps = lg_pmps + 197;
                break;
            case 4:
                lg_pmps = lg_pmps + 95;
                break;
            default:
                lg_pmps = lg_pmps + 46;
        }

        if ( lg_pmps >= ( 256 << LG_PMPS_SHIFTNO ) )
        {
            lg_pmps = ( 512 << LG_PMPS_SHIFTNO ) - 1 - lg_pmps;
            bi_ct->MPS = !( bi_ct->MPS );
        }
    }
    else
    {
        lg_pmps = lg_pmps - ( i32u_t )( lg_pmps>>cwr ) - ( i32u_t )( lg_pmps>>( cwr+2 ) );
    }

    bi_ct->LG_PMPS = lg_pmps;
    assert( lg_pmps <= 1023 );
    assert( s1 <= 255 );
    assert( t1 <= 255 );
    assert( dep->value_t <= 1023 );

    return ( bit );
}

i32u_t biari_decode_symbolW( Env_AEC *dep, BiContextTypePtr bi_ct1, BiContextTypePtr bi_ct2 )
{
    uchar_t bit1, bit2;
    uchar_t pred_MPS, bit;
    i32u_t  lg_pmps;
    uchar_t cwr1, cycno1 = bi_ct1->cycno;
    uchar_t cwr2, cycno2 = bi_ct2->cycno;
    i32u_t  lg_pmps1 = bi_ct1->LG_PMPS, lg_pmps2 = bi_ct2->LG_PMPS;
    i32u_t  t_rlps;
    uchar_t s_flag, is_LPS = 0;
    i32u_t  s1, t1, s2, t2;

    bit1 = bi_ct1->MPS;
    bit2 = bi_ct2->MPS;


    cwr1 = ( cycno1 <= 1 ) ? 3 : ( cycno1 == 2 ) ? 4 : 5;
    cwr2 = ( cycno2 <= 1 ) ? 3 : ( cycno2 == 2 ) ? 4 : 5;

    if ( bit1 == bit2 )
    {
        pred_MPS = bit1;
        lg_pmps = ( lg_pmps1 + lg_pmps2 ) / 2;
    }
    else
    {
        if ( lg_pmps1 < lg_pmps2 )
        {
            pred_MPS = bit1;
            lg_pmps = ( 256 << LG_PMPS_SHIFTNO ) - 1 - ( ( lg_pmps2 - lg_pmps1 ) >> 1 );
        }
        else
        {
            pred_MPS = bit2;
            lg_pmps = ( 256 << LG_PMPS_SHIFTNO ) - 1 - ( ( lg_pmps1 - lg_pmps2 ) >> 1 );
        }
    }

    s1 = dep->E_s1;
    t1 = dep->E_t1;

    if ( t1 >= ( lg_pmps >> LG_PMPS_SHIFTNO ) )
    {
        s2 = s1;
        t2 = t1 - ( lg_pmps >> LG_PMPS_SHIFTNO );
        s_flag = 0;
    }
    else
    {
        s2 = s1 + 1;
        t2 = 256 + t1 - ( lg_pmps >> LG_PMPS_SHIFTNO );
        s_flag = 1;
    }

    bit = pred_MPS;
    if ( s2 > dep->value_s || ( s2 == dep->value_s && dep->value_t >= t2 ) ) //LPS
    {
        is_LPS = 1;
        bit = !bit; //LPS
        t_rlps = ( s_flag == 0 ) ? ( lg_pmps >> LG_PMPS_SHIFTNO ) : ( t1 + ( lg_pmps >> LG_PMPS_SHIFTNO ) );

        if ( s2 == dep->value_s )
        {
            dep->value_t = ( dep->value_t - t2 );
        }
        else
        {
            if ( --dep->bits_to_go < 0 )
            {
                get_byte( dep );
            }
            // Shift in next bit and add to value
            dep->value_t = ( dep->value_t << 1 ) | ( ( dep->Dbuffer >> dep->bits_to_go ) & 0x01 );
            dep->value_t = 256 + dep->value_t - t2;
        }

        //restore range
        while ( t_rlps < QUARTER )
        {
            t_rlps = t_rlps << 1;
            if ( --dep->bits_to_go < 0 )
            {
                get_byte( dep );
            }
            // Shift in next bit and add to value
            dep->value_t = ( dep->value_t << 1 ) | ( ( dep->Dbuffer >> dep->bits_to_go ) & 0x01 );
        }
        s1 = 0;
        t1 = t_rlps & 0xff;

        //restore value
        dep->value_s = 0;
        while ( dep->value_t < QUARTER )
        {
            int j;
            if ( --dep->bits_to_go < 0 )
            {
                get_byte( dep );
            }
            j = ( dep->Dbuffer >> dep->bits_to_go ) & 0x01;
            // Shift in next bit and add to value

            dep->value_t = ( dep->value_t << 1 ) | j;
            dep->value_s++;
        }
        dep->value_t = dep->value_t & 0xff;
    }//--LPS
    else   //MPS
    {
        s1 = s2;
        t1 = t2;
    }

    if ( bit != bit1 )
    {
        cycno1 = ( cycno1 <= 2 ) ? ( cycno1 + 1 ) : 3; //LPS occurs
    }
    else
    {
        if ( cycno1 == 0 )
        {
            cycno1 = 1;
        }
    }

    if ( bit != bit2 )
    {
        cycno2 = ( cycno2 <= 2 ) ? ( cycno2 + 1 ) : 3; //LPS occurs
    }
    else
    {
        if ( cycno2 == 0 )
        {
            cycno2 = 1;
        }
    }
    bi_ct1->cycno = cycno1;
    bi_ct2->cycno = cycno2;

    dep->E_s1 = s1;
    dep->E_t1 = t1;

    //update probability estimation
    {
        //bi_ct1
        if ( bit == bit1 )
        {
            lg_pmps1 = lg_pmps1 - ( i32u_t )( lg_pmps1>>cwr1 ) - ( i32u_t )( lg_pmps1>>( cwr1+2 ) );
        }
        else
        {
            switch ( cwr1 )
            {
                case 3:
                    lg_pmps1 = lg_pmps1 + 197;
                    break;
                case 4:
                    lg_pmps1 = lg_pmps1 + 95;
                    break;
                default:
                    lg_pmps1 = lg_pmps1 + 46;
            }

            if ( lg_pmps1 >= ( 256 << LG_PMPS_SHIFTNO ) )
            {
                lg_pmps1 = ( 512 << LG_PMPS_SHIFTNO ) - 1 - lg_pmps1;
                bi_ct1->MPS = !( bi_ct1->MPS );
            }
        }
        bi_ct1->LG_PMPS = lg_pmps1;

        //bi_ct2
        if ( bit == bit2 )
        {
            lg_pmps2 = lg_pmps2 - ( i32u_t )( lg_pmps2>>cwr2 ) - ( i32u_t )( lg_pmps2>>( cwr2+2 ) );
        }
        else
        {
            switch ( cwr2 )
            {
                case 3:
                    lg_pmps2 = lg_pmps2 + 197;
                    break;
                case 4:
                    lg_pmps2 = lg_pmps2 + 95;
                    break;
                default:
                    lg_pmps2 = lg_pmps2 + 46;
            }

            if ( lg_pmps2 >= ( 256 << LG_PMPS_SHIFTNO ) )
            {
                lg_pmps2 = ( 512 << LG_PMPS_SHIFTNO ) - 1 - lg_pmps2;
                bi_ct2->MPS = !( bi_ct2->MPS );
            }
        }
        bi_ct2->LG_PMPS = lg_pmps2;
    }

    return ( bit );
}


/*!
************************************************************************
* \brief
*    biari_decode_symbol_eq_prob():
* \return
*    the decoded symbol
************************************************************************
*/
i32u_t biari_decode_symbol_eq_prob( Env_AEC * dep )
{
    i32u_t bit;
    BiContextType octx;
    BiContextTypePtr ctx = &octx;

    ctx->LG_PMPS = ( QUARTER << LG_PMPS_SHIFTNO ) - 1;
    ctx->MPS = 0;
    bit = biari_decode_symbol( dep,ctx );

    return ( bit );
}

i32u_t biari_decode_final( Env_AEC * dep )
{
    i32u_t bit;
    BiContextType octx;
    BiContextTypePtr ctx = &octx;

    ctx->LG_PMPS = 1 << LG_PMPS_SHIFTNO;
    ctx->MPS = 0;
    bit = biari_decode_symbol( dep,ctx );
    return ( bit );
}