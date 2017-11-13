/*!
*************************************************************************************
* \file AEC.c
*
* \brief
*    AEC entropy coding routines
*
* \author
*    Main contributors (see contributors.h)
**************************************************************************************
*/

#include <math.h>
#include <memory.h>
#include <assert.h>
#include "AEC.h"
#include "image.h"
#include "../../common/common.h"

int last_dquant = 0;
/***********************************************************************
* L O C A L L Y   D E F I N E D   F U N C T I O N   P R O T O T Y P E S
***********************************************************************
*/

void exp_golomb_encode_eq_prob( Env_AEC* eep_dp,
                                i32u_t symbol,
                                int k );

void unary_bin_encode( Env_AEC* eep_frame,
                       i32u_t symbol,
                       BiContextTypePtr ctx,
                       int ctx_offset );

void unary_bin_max_encode( Env_AEC* eep_frame,
                           i32u_t symbol,
                           BiContextTypePtr ctx,
                           int ctx_offset,
                           i32u_t max_symbol );

void unary_exp_golomb_level_encode( Env_AEC* eep_dp,
                                    i32u_t symbol,
                                    BiContextTypePtr ctx );

void unary_exp_golomb_mv_encode( Env_AEC* eep_dp,
                                 i32u_t symbol,
                                 BiContextTypePtr ctx,
                                 i32u_t max_bin );

/*!
***************************************************************************
* \brief
*    This function is used to arithmetically encode the run length
*    info to buffer
***************************************************************************
*/
int writeRunLengthInfo2Buffer_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    int symbol = val;
    BiContextTypePtr pCTX;
    int ctx;
    pCTX = cs_aec->tex_ctx->one_contexts[0];
    ctx = 0;
    while( symbol >= 1 )
    {
        symbol -= 1;
        biari_encode_symbol( eep_dp, 0, pCTX+ctx );
        ctx ++;
        if( ctx>=3 )
        {
            ctx=3;
        }
    }
    biari_encode_symbol( eep_dp, 1, pCTX+ctx );

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

/*!
***************************************************************************
* \brief
*    This function is used to arithmetically encode the macroblock
*    transform type(2Nx2N/NxN) of a given MB.
***************************************************************************
*/
int writeMBTransType_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    BiContextTypePtr pCTX = &cs_aec->tex_ctx->tu_size_context[0];
    biari_encode_symbol ( eep_dp, ( uchar_t )val, pCTX );

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

int writeMBDeltaQp_AEC(CSobj *cs_aec, int val, int prev_qp)
{
    Env_AEC* eep_dp = &(cs_aec->ee_AEC);
    int curr_len = arienco_bits_written(eep_dp);

    BiContextTypePtr pCTX = &cs_aec->tex_ctx->MBdeltaQP_contexts[0];
    int abs_val = abs(val);
    int sign_val = (val > 0) ? 0 : 1;
    int ctx_idx = (prev_qp == 0) ? 0 : 1;

#if TRACE
    output_trace_info("mb_qp_delta: %d", val);
#endif

    if (abs_val == 0)
    {
        biari_encode_symbol(eep_dp, 1, &pCTX[ctx_idx]);
    }
    else
    {
        biari_encode_symbol(eep_dp, 0, &pCTX[ctx_idx]);
        abs_val--;
        ctx_idx = 2;
        while (abs_val >= 1)
        {
            biari_encode_symbol(eep_dp, 0, pCTX + ctx_idx);
            abs_val--;
            ctx_idx++;
            if (ctx_idx >= 3)
            {
                ctx_idx = 3;
            }
        }
        biari_encode_symbol(eep_dp, 1, pCTX + ctx_idx);
    }

    biari_encode_symbol_eq_prob(eep_dp, sign_val);

    return (arienco_bits_written(eep_dp) - curr_len);
}

/*!
***************************************************************************
* \brief
*    This function is used to arithmetically encode the macroblock
*    type info of a given MB.
***************************************************************************
*/

int writeMBPartTypeInfo_AEC( Macroblock *currMB, CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    int a, b;
    int act_ctx = 0;
    int act_sym;
    MotionInfoContexts *ctx         = cs_aec->mot_ctx;

    if( img->type==B_IMG )
    {
        BiContextTypePtr pCTX = ctx->mb_type_contexts[2];

        if( currMB->mb_available_up==NULL )
        {
            b = 0;
        }
        else
        {
            b = ( ( ( currMB->mb_available_up )->mb_type != 0 ) ? 1 : 0 );
        }
        if ( currMB->mb_available_left == NULL )
        {
            a = 0;
        }
        else
        {
            a = ( ( ( currMB->mb_available_left )->mb_type != 0 ) ? 1 : 0 );
        }
        act_ctx = a + b;

        switch( val )
        {
            case PSKIP:
            case P2Nx2N:
                act_sym = currMB->mb_type;
                break; // 2Nx2N
            case P2NxN:
                act_sym = 3;
                break; // 2NxN
            case PNx2N:
                act_sym = 2;
                break; // Nx2N
            case PNxN:
                act_sym = 5;
                break; // NxN
            case I_MB:
                act_sym = 4;
                break; // I_MB
            default:
                printf( "error input MB mode!\n" );
                exit( 0 );
                break;
        }

        if( act_sym == 0 )
        {
            biari_encode_symbol ( eep_dp, 0, &ctx->mb_type_contexts[2][act_ctx] );
        }
        else
        {
            biari_encode_symbol ( eep_dp, 1, &ctx->mb_type_contexts[2][act_ctx] );
            act_sym--;
            act_ctx = 3;
            while( act_sym>=1 )
            {
                biari_encode_symbol ( eep_dp, 0, pCTX + act_ctx );
                act_sym--;
                act_ctx++;
                if( act_ctx>=4 )
                {
                    act_ctx=4;
                }
            }
            biari_encode_symbol ( eep_dp, 1, pCTX + act_ctx );
        }
    }

    if( img->type==P_IMG )
    {
        BiContextTypePtr pCTX = ctx->mb_type_contexts[1];
        switch( val )
        {
            case PSKIP:
            case P2Nx2N:
                act_sym = 0;
                break; // 2Nx2N and SKIP, which are distinguished by the prediction mode
            case P2NxN:
                act_sym = 2;
                break; // 2NxN
            case PNx2N:
                act_sym = 1;
                break; // Nx2N
            case PNxN:
                act_sym = 4;
                break; // NxN
            case I_MB:
                act_sym = 3;
                break; // I_MB
            default:
                printf( "error input MB mode!\n" );
                exit( 0 );
                break;
        }

        act_ctx = 0;
        while( act_sym>=1 )
        {
            biari_encode_symbol ( eep_dp, 0, pCTX + act_ctx );
            act_sym--;
            act_ctx++;
            if( act_ctx>=3 )
            {
                act_ctx=3;
            }
        }
        biari_encode_symbol ( eep_dp, 1, pCTX + act_ctx );
    }

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

/*!
***************************************************************************
* \brief
*    This function is used to arithmetically encode the 8x8 block
*    type info
***************************************************************************
*/
int write_B8x8_PredTypeInfo_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    int act_ctx;
    int act_sym;
    MotionInfoContexts *ctx    = cs_aec->mot_ctx;
    BiContextTypePtr pCTX = ctx->b8_type_contexts[0];

    act_sym = val;
    if( act_sym & 0x02 )
    {
        biari_encode_symbol ( eep_dp, 1, pCTX );
        act_ctx = 3;
    }
    else
    {
        biari_encode_symbol ( eep_dp, 0, pCTX );
        act_ctx = 2;
    }
    biari_encode_symbol ( eep_dp, ( uchar_t )( act_sym & 0x01 ), pCTX + act_ctx );

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

int write_Bfrm_PredTypeInfo_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    int act_ctx;
    int act_sym;
    MotionInfoContexts *ctx    = cs_aec->mot_ctx;
    BiContextTypePtr pCTX = ctx->b8_type_contexts[0];

    act_sym = val;

    if ( act_sym == 0 )
    {
        biari_encode_symbol ( eep_dp, 0, pCTX );
    }
    else
    {
        biari_encode_symbol ( eep_dp, 1, pCTX );
        act_ctx = 1;
        biari_encode_symbol ( eep_dp, ( uchar_t )( act_sym & 0x01 ), pCTX + act_ctx );
    }

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

int write_P16x16_PredTypeInfo_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    int act_ctx;
    int act_sym;
    MotionInfoContexts *ctx    = cs_aec->mot_ctx;
    BiContextTypePtr pCTX = ctx->b8_type_contexts[0];

    act_sym = val;
    act_ctx = 0;
    if ( act_sym == 0 )
    {
        biari_encode_symbol ( eep_dp, 0, pCTX + act_ctx );
    }
    else
    {
        biari_encode_symbol ( eep_dp, 1, pCTX + act_ctx );
        act_ctx = 1;
        biari_encode_symbol ( eep_dp, ( uchar_t )( act_sym & 0x01 ), pCTX + act_ctx );
    }

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

int write_Pfrm_PredTypeInfo_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    int act_ctx;
    int act_sym;
    MotionInfoContexts *ctx    = cs_aec->mot_ctx;
    BiContextTypePtr pCTX = ctx->b8_type_contexts[0];

    act_sym = val;
    act_ctx = 0;
    biari_encode_symbol ( eep_dp, ( uchar_t )( act_sym & 0x01 ), pCTX + act_ctx );
    return ( arienco_bits_written( eep_dp ) - curr_len );
}

/*!
****************************************************************************
* \brief
*    This function is used to arithmetically encode a pair of
*    intra prediction modes of a given MB.
* \author
*
****************************************************************************
*/
int  writeIntraPredMode_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    BiContextTypePtr pCTX;
    int ctx;
    int symbol = val;
    int value = symbol;
    pCTX = cs_aec->tex_ctx->ipr_contexts;
    ctx = 0;
    while( symbol >= 1 )
    {
        symbol -= 1;
        biari_encode_symbol( eep_dp, 0, pCTX+ctx );
        ctx ++;
        if( ctx>=3 )
        {
            ctx=3;
        }
    }
    if( value<4 )
    {
        biari_encode_symbol( eep_dp, 1, pCTX+ctx );
    }

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

int writeSubMBTransType_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    BiContextTypePtr pCTX;
    int symbol = val;
    int value = symbol;
    pCTX = cs_aec->tex_ctx->qsplit_contexts;
    biari_encode_symbol( eep_dp, ( uchar_t )value, pCTX );

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

/*!
****************************************************************************
* \brief
*    This function is used to arithmetically encode the chroma
*    intra prediction mode of an 8x8 block
****************************************************************************
*/
int writeCIPredMode_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    BiContextTypePtr pCTX;
    int ctx;
    int symbol = val;
    int value = symbol;
    pCTX = cs_aec->tex_ctx->cipr_contexts;
    ctx = 0;
    while( symbol >= 1 )
    {
        symbol -= 1;
        biari_encode_symbol( eep_dp, 0, pCTX+ctx );
        ctx ++;
        if( ctx>=2 )
        {
            ctx=2;
        }
    }
    if( value<3 )
    {
        biari_encode_symbol( eep_dp, 1, pCTX+ctx );
    }

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

/*!
****************************************************************************
* \brief
*    This function is used to arithmetically encode the motion
*    vector data of a B-frame MB.
****************************************************************************
*/
int writeMVD_AEC( Macroblock *currMB, CSobj *cs_aec, int b8, int val_1, int val_2 )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    int a, b;
    int act_ctx;
    i32u_t act_sym;
    int mv_pred_res;
    int mv_local_err;
    int mv_sign;
    int list_idx = val_2 & 0x01;
    int k = ( val_2 >> 1 );
    int exp_golomb_order=0;
    Macroblock *tmpMB;
    PixelPos block_a, block_b;
    MotionInfoContexts *ctx = cs_aec->mot_ctx;

    getLuma8x8Neighbour( currMB, img->current_mb_nr, b8, -1,  0, &block_a );
    getLuma8x8Neighbour( currMB, img->current_mb_nr, b8,  0, -1, &block_b );

    if ( block_b.available )
    {
        if( block_b.mb_addr == img->current_mb_nr )
        {
            tmpMB = currMB;
        }
        else
        {
            tmpMB = &img->mb_data[block_b.mb_addr];
        }
        b = absm( tmpMB->mvd[list_idx][block_b.y][block_b.x][k] );
    }
    else
    {
        b=0;
    }

    if ( block_a.available )
    {
        if( block_a.mb_addr == img->current_mb_nr )
        {
            tmpMB = currMB;
        }
        else
        {
            tmpMB = &img->mb_data[block_a.mb_addr];
        }
        a = absm( tmpMB->mvd[list_idx][block_a.y][block_a.x][k] );
    }
    else
    {
        a = 0;
    }


    if ( ( mv_local_err=a )<2 )
    {
        act_ctx = 0;
    }
    else if ( mv_local_err<16 )
    {
        act_ctx = 1;
    }
    else
    {
        act_ctx = 2;
    }

    mv_pred_res = val_1;
    act_sym = absm( mv_pred_res );

    if ( act_sym< 3 ) // 0, 1, 2
    {
        if ( act_sym == 0 )
        {
            biari_encode_symbol( eep_dp, 0, &ctx->mv_res_contexts[k][act_ctx] );
        }
        else if ( act_sym == 1 )
        {
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][act_ctx] );
            biari_encode_symbol( eep_dp, 0, &ctx->mv_res_contexts[k][3] );
        }
        else if ( act_sym == 2 )
        {
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][act_ctx] );
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][3] );
            biari_encode_symbol( eep_dp, 0, &ctx->mv_res_contexts[k][4] );
        }

    }
    else
    {
        if ( act_sym% 2==1 ) //odds >3
        {
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][act_ctx] );
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][3] );
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][4] );
            biari_encode_symbol( eep_dp, 0, &ctx->mv_res_contexts[k][5] );
            act_sym = ( act_sym - 3 )/2;

        }
        else //even >3
        {
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][act_ctx] );
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][3] );
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][4] );
            biari_encode_symbol( eep_dp, 1, &ctx->mv_res_contexts[k][5] );
            act_sym = ( act_sym - 4 )/2;
        }
        // exp_golomb part
        while( 1 )
        {
            if ( act_sym >= ( i32u_t )( 1<<exp_golomb_order ) )
            {
                biari_encode_symbol_eq_prob( eep_dp,0 );
                act_sym = act_sym - ( 1<<exp_golomb_order );
                exp_golomb_order++;
            }
            else
            {
                biari_encode_symbol_eq_prob( eep_dp,1 );
                while ( exp_golomb_order-- ) //next binary part
                {
                    biari_encode_symbol_eq_prob( eep_dp,( uchar_t )( ( act_sym>>exp_golomb_order )&1 ) );
                }
                break;
            }
        }
    }

    if ( mv_pred_res!=0 )
    {
        mv_sign= ( mv_pred_res>=0 ) ? 0:1;
        biari_encode_symbol_eq_prob( eep_dp,( uchar_t ) mv_sign );
    }

    return ( arienco_bits_written( eep_dp ) - curr_len );
}
/*!
****************************************************************************
* \brief
*    This function is used to arithmetically encode the coded
*    block pattern of an 8x8 block
****************************************************************************
*/
void writeCBP_BIT_AEC ( Macroblock* currMB, CSobj *cs_aec, int b8, int b4, int bit, int cbp, Env_AEC* eep_dp )
{
    int a, b;

    int b8x = b8 % 2;
    int b8y = b8 / 2;
    int b4x = b4 % 2;
    int b4y = b4 / 2;

    int b8_up = 0xF, b8_left = 0xF; // cbp in 8x8 block, 4bit
    int b8_cbp = cbp >> ( 4 * b8 );

    // 1st. get 8x8 neighbour's cbp
    if ( b8y == 0 )
    {
        if ( currMB->mb_available_up != NULL )
        {
            b8_up &= ( ( currMB->mb_available_up )->cbp ) >> ( 4 * ( 2 + b8x ) );
        }
    }
    else
    {
        b8_up &= cbp >> ( 4 * b8x );
    }

    if ( b8x == 0 )
    {
        if ( currMB->mb_available_left != NULL )
        {
            b8_left &= ( ( currMB->mb_available_left )->cbp ) >> ( 4 * ( 2 * b8y + 1 ) );
        }
    }
    else
    {
        b8_left &= cbp >> ( 4 * 2 * b8y );
    }

    // 2nd. get a & b
    if ( b4y == 0 )
    {
        b = ( b8_up & ( 1 << ( 2 + b4x ) ) ) == 0 ? 1 : 0; //VG-ADD
    }
    else
    {
        b = ( b8_cbp & ( 1 << b4x ) ) == 0 ? 1 : 0;
    }

    if ( b4x == 0 )
    {
        a = ( b8_left & ( 1 << ( 2 * b4y + 1 ) ) ) == 0 ? 1 : 0; //VG-ADD
    }
    else
    {
        a = ( b8_cbp & ( 1 << ( 2 * b4y ) ) ) == 0 ? 1 : 0;
    }

    //===== WRITE BIT =====
    biari_encode_symbol ( eep_dp, ( uchar_t ) bit, cs_aec->tex_ctx->cbp_contexts[0] + a+2*b );
}

/*!
****************************************************************************
* \brief
*    This function is used to arithmetically encode the coded
*    block pattern of a macroblock
***************************************************************************
*/
int writeCBP_AEC( Macroblock *currMB, CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    TextureInfoContexts *ctx = cs_aec->tex_ctx;
    int curr_cbp_ctx;
    int cbp = val; // symbol to encode
    int cbp_bit;
    int b8, b4;
    int b8Split;

    // luma
    if ( currMB->trans_type == TRANS_2Nx2N )
    {
        writeCBP_BIT_AEC( currMB, cs_aec, 0, 0, ( cbp & ( 0xFFFF ) ) != 0, cbp, eep_dp );
    }
    else
    {
        for ( b8 = 0; b8 < 4; b8++ )
        {
            b8Split = IS_INTRA( currMB )? currMB->sub_mb_trans_type[b8] : 0;
            if ( b8Split )
            {
                for ( b4 = 0; b4 < 4; b4++ )
                {
                    writeCBP_BIT_AEC( currMB, cs_aec, b8, b4, ( cbp&( 0x1 << ( 4 * b8 + b4 ) ) ) != 0, cbp, eep_dp );
                }
            }
            else
            {
                writeCBP_BIT_AEC( currMB, cs_aec, b8, 0, ( cbp&( 0xF << ( 4 * b8 ) ) ) != 0, cbp, eep_dp );
            }
        }
    }

    // chroma
    cbp_bit = ( cbp > 0xFFFF ) ? 1 : 0;
    curr_cbp_ctx = 0;
    biari_encode_symbol( eep_dp, ( uchar_t ) cbp_bit, ctx->cbp_contexts[1] + curr_cbp_ctx );
    if ( cbp > 0xFFFF )
    {
        cbp_bit = ( cbp >= 0xFF0000 ) ? 1 : 0;
        curr_cbp_ctx = 1;
        biari_encode_symbol( eep_dp, ( uchar_t ) cbp_bit, ctx->cbp_contexts[1] + curr_cbp_ctx );
        if( cbp < 0xFF0000 )
        {
            cbp_bit = ( ( cbp>>16 ) == 0xF0 ) ? 1 : 0;
            biari_encode_symbol( eep_dp, ( uchar_t ) cbp_bit, ctx->cbp_contexts[1] + curr_cbp_ctx );
        }
    }

    if ( !cbp )
    {
        last_dquant=0;
    }

    return ( arienco_bits_written( eep_dp ) - curr_len );
}

/*!
****************************************************************************
* \brief
*    Write coefficients
****************************************************************************
*/

int writeRunLevel_AEC(CSobj *cs_aec, int tu_size, int* levels, int* runs, int DCT_pairs, int ctx_in)
{
    const int T_Chr[5] = {0, 1, 2, 4, 3000};
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    int pairs=0, rank=0, pos=0;
    int Run=0, Level=0, absLevel=0, symbol=0;
    int coef_num = tu_size * tu_size;
    int Primary_shift_offset;

    switch ( tu_size )
    {
        case 4:
            Primary_shift_offset = 0;
            break;
        case 8:
            Primary_shift_offset = 2;
            break;
        case 16:
            Primary_shift_offset = 4;
            break;
        default:
            Primary_shift_offset = -1;
            assert( 0 );
    }

    if (DCT_pairs>0)
    {
        BiContextType ( *Primary )[NUM_MAP_CTX] = NULL;
        BiContextTypePtr pCTX;
        BiContextTypePtr pCTX2;
        int ctx=0, offset=0, ctx2=0;

        if( ctx_in == LUMA_8x8 )
        {
            Primary = cs_aec->tex_ctx->map_contexts;
        }
        else
        {
            Primary = cs_aec->tex_ctx->last_contexts;
        }
        rank = 0;
        pos  = 0;
        for (pairs = DCT_pairs; pairs >= 0; pairs--)
        {
            if( pairs==0 ) // at the end of one block, (0,0) is written.
            {
                Level = 0;
                Run = 0;
            }
            else
            {
                Level = levels[pairs - 1];
                Run = runs[pairs-1];
            }
            absLevel = abs( Level );
            pCTX = Primary[rank];
            //! EOB
            if( rank>0 )
            {
                pCTX2 = Primary[5+( pos>>( 3+Primary_shift_offset ) )];
                ctx2 = ( pos>>1 ) & 0x0f;
                ctx = 0;
                if( absLevel==0 )
                {
                    biari_encode_symbolW( eep_dp, 1, pCTX+ctx, pCTX2+ctx2 );
                    break;
                }
                else
                {
                    biari_encode_symbolW( eep_dp, 0, pCTX+ctx, pCTX2+ctx2 );
                }
            }
            //! Level
            symbol = absLevel-1;
            ctx = 1;
            while( symbol>=1 )
            {
                symbol -= 1;
                biari_encode_symbol( eep_dp, 0, pCTX+ctx );
                ctx ++;
                if( ctx>=2 )
                {
                    ctx =2;
                }
            }
            biari_encode_symbol( eep_dp, 1, pCTX+ctx );

            if( Level<0 )
            {
                biari_encode_symbol_eq_prob( eep_dp, 1 );
            }
            else
            {
                biari_encode_symbol_eq_prob( eep_dp, 0 );
            }
            //! Run
            if( absLevel==1 )
            {
                offset = 4;
            }
            else
            {
                offset = 6;
            }
            symbol = Run;
            ctx = 0;
            while( symbol>=1 )
            {
                symbol -= 1;
                biari_encode_symbol( eep_dp, 0, pCTX + ctx + offset );
                ctx ++;
                if( ctx>=1 )
                {
                    ctx =1;
                }
            }
            biari_encode_symbol( eep_dp, 1, pCTX + ctx + offset );
            //! Update Rank
            if( absLevel>T_Chr[rank] )
            {
                if( absLevel<=2 )
                {
                    rank = absLevel;
                }
                else if( absLevel<=4 )
                {
                    rank = 3;
                }
                else
                {
                    rank = 4;
                }
            }
            //! Update position
            pos += ( Run+1 );
            if( pos>=coef_num )
            {
                pos = coef_num - 1;
            }
        }
    }

    return ( arienco_bits_written( eep_dp ) - curr_len );
}







/*!
************************************************************************
* \brief
*    Unary binarization and encoding of a symbol by using
*    one or two distinct models for the first two and all
*    remaining bins
*
************************************************************************/
void unary_bin_encode( Env_AEC* eep_dp,
                       i32u_t symbol,
                       BiContextTypePtr ctx,
                       int ctx_offset )
{
    i32u_t l;
    BiContextTypePtr ictx;

    if ( symbol==0 )
    {
        biari_encode_symbol( eep_dp, 1, ctx );
        return;
    }
    else
    {
        biari_encode_symbol( eep_dp, 0, ctx );
        l=symbol;
        ictx=ctx+ctx_offset;
        while ( ( --l )>0 )
        {
            biari_encode_symbol( eep_dp, 0, ictx );
        }
        biari_encode_symbol( eep_dp, 1, ictx );
    }
    return;
}

/*!
************************************************************************
* \brief
*    Unary binarization and encoding of a symbol by using
*    one or two distinct models for the first two and all
*    remaining bins; no terminating "0" for max_symbol
*    (finite symbol alphabet)
************************************************************************
*/
void unary_bin_max_encode( Env_AEC* eep_dp,
                           i32u_t symbol,
                           BiContextTypePtr ctx,
                           int ctx_offset,
                           i32u_t max_symbol )
{
    i32u_t l;
    BiContextTypePtr ictx;

    if ( symbol==0 )
    {
        biari_encode_symbol( eep_dp, 0, ctx );
        return;
    }
    else
    {
        biari_encode_symbol( eep_dp, 1, ctx );
        l=symbol;
        ictx=ctx+ctx_offset;
        while ( ( --l )>0 )
        {
            biari_encode_symbol( eep_dp, 1, ictx );
        }
        if ( symbol<max_symbol )
        {
            biari_encode_symbol( eep_dp, 0, ictx );
        }
    }
    return;
}



/*!
************************************************************************
* \brief
*    Exp Golomb binarization and encoding
************************************************************************
*/
void exp_golomb_encode_eq_prob( Env_AEC* eep_dp,
                                i32u_t symbol,
                                int k )
{
    while( 1 )
    {
        if ( symbol >= ( i32u_t )( 1<<k ) )
        {
            biari_encode_symbol_eq_prob( eep_dp, 1 ); //first unary part
            symbol = symbol - ( 1<<k );
            k++;
        }
        else
        {
            biari_encode_symbol_eq_prob( eep_dp, 0 ); //now terminated zero of unary part
            while ( k-- )                             //next binary part
            {
                biari_encode_symbol_eq_prob( eep_dp, ( uchar_t )( ( symbol>>k )&1 ) );
            }
            break;
        }
    }

    return;
}

/*!
************************************************************************
* \brief
*    Exp-Golomb for Level Encoding
*
************************************************************************/
void unary_exp_golomb_level_encode( Env_AEC* eep_dp,
                                    i32u_t symbol,
                                    BiContextTypePtr ctx )
{
    i32u_t l,k;
    i32u_t exp_start = 13; // 15-2 : 0,1 level decision always sent

    if ( symbol==0 )
    {
        biari_encode_symbol( eep_dp, 0, ctx );
        return;
    }
    else
    {
        biari_encode_symbol( eep_dp, 1, ctx );
        l=symbol;
        k=1;
        while ( ( ( --l )>0 ) && ( ++k <= exp_start ) )
        {
            biari_encode_symbol( eep_dp, 1, ctx );
        }
        if ( symbol < exp_start )
        {
            biari_encode_symbol( eep_dp, 0, ctx );
        }
        else
        {
            exp_golomb_encode_eq_prob( eep_dp,symbol-exp_start,0 );
        }
    }
    return;
}



/*!
************************************************************************
* \brief
*    Exp-Golomb for MV Encoding
*
************************************************************************/
void unary_exp_golomb_mv_encode( Env_AEC* eep_dp,
                                 i32u_t symbol,
                                 BiContextTypePtr ctx,
                                 i32u_t max_bin )
{
    i32u_t l,k;
    i32u_t bin=1;
    BiContextTypePtr ictx=ctx;
    i32u_t exp_start = 8; // 9-1 : 0 mvd decision always sent

    if ( symbol==0 )
    {
        biari_encode_symbol( eep_dp, 0, ictx );
        return;
    }
    else
    {
        biari_encode_symbol( eep_dp, 1, ictx );
        l=symbol;
        k=1;
        ictx++;
        while ( ( ( --l )>0 ) && ( ++k <= exp_start ) )
        {
            biari_encode_symbol( eep_dp, 1, ictx  );
            if ( ( ++bin )==2 )
            {
                ictx++;
            }
            if ( bin==max_bin )
            {
                ictx++;
            }
        }
        if ( symbol < exp_start )
        {
            biari_encode_symbol( eep_dp, 0, ictx );
        }
        else
        {
            exp_golomb_encode_eq_prob( eep_dp,symbol-exp_start,3 );
        }
    }
    return;
}

int write_Reffrm_AEC( CSobj *cs_aec, int val )
{
    Env_AEC* eep_dp = &( cs_aec->ee_AEC );
    int curr_len = arienco_bits_written( eep_dp );

    int act_ctx = 0;
    int act_sym;
    MotionInfoContexts *ctx         = cs_aec->mot_ctx;
    Macroblock         *currMB      = &img->mb_data[img->current_mb_nr];
    BiContextTypePtr pCTX = cs_aec->tex_ctx->one_contexts[1];//ctx->mb_type_contexts[1];

    act_sym = val;
    act_ctx = 0;
    while( act_sym>=1 )
    {
        biari_encode_symbol ( eep_dp, 0, pCTX + act_ctx );
        act_sym--;
        act_ctx++;
        // if( act_ctx>=4 ) act_ctx=4;
        if( act_ctx >= ( img->real_ref_num - 1 ) )
        {
            act_ctx=( img->real_ref_num-1 );
        }
    }

    biari_encode_symbol ( eep_dp, 1, pCTX + act_ctx );

    return ( arienco_bits_written( eep_dp ) - curr_len );
}