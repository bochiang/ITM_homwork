#include <string.h>

#include "global.h"
#include "AEC.h"
#include "memalloc.h"
#include "elements.h"
#include "biaridecod.h"
#include "math.h"
#include "assert.h"
#include "../../common/common.h"

/***********************************************************************
* L O C A L L Y   D E F I N E D   F U N C T I O N   P R O T O T Y P E S
***********************************************************************
*/
i32u_t unary_bin_decode( Env_AEC * dep_dp,
                         BiContextTypePtr ctx,
                         int ctx_offset );


i32u_t unary_bin_max_decode( Env_AEC * dep_dp,
                             BiContextTypePtr ctx,
                             int ctx_offset,
                             i32u_t max_symbol );

i32u_t unary_exp_golomb_level_decode( Env_AEC * dep_dp,
                                      BiContextTypePtr ctx );

i32u_t unary_exp_golomb_mv_decode( Env_AEC * dep_dp,
                                   BiContextTypePtr ctx,
                                   i32u_t max_bin );

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode a pair of
*    intra prediction modes of a given MB.
************************************************************************
*/
void readIntraPredMode_AEC( CSobj *cs_aec, int *val )
{
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    BiContextTypePtr pCTX = cs_aec->tex_ctx->ipr_contexts;
    int ctx = 0, symbol = 0;

    while( biari_decode_symbol( dep_dp, pCTX+ctx )==0 )
    {
        symbol += 1;
        ctx ++;
        if( ctx>=3 )
        {
            ctx=3;
        }
#if USING_INTRA_5_9
        if (symbol == 8)
#else
        if( symbol==4 )
#endif
        {
            break;
        }
    }
    *val = symbol;
}

void readSubMBTransType_AEC( CSobj *cs_aec, int *val )
{
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    BiContextTypePtr pCTX = cs_aec->tex_ctx->qsplit_contexts;
    int symbol;

    symbol = biari_decode_symbol( dep_dp, pCTX );
    *val = symbol;
}

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the chroma
*    intra prediction mode of a given MB.
************************************************************************
*/  //GB
void readCIPredMode_AEC( CSobj *cs_aec, int *val )
{
    BiContextTypePtr pCTX = cs_aec->tex_ctx->cipr_contexts;
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    int ctx = 0, symbol = 0;

    while( biari_decode_symbol( dep_dp, pCTX+ctx )==0 )
    {
        symbol += 1;
        ctx ++;
        if( ctx>=2 )
        {
            ctx=2;
        }
        if( symbol==3 )
        {
            break;
        }
    }
    *val = symbol;//-1;
}

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the motion
*    vector data of a B-frame MB.
************************************************************************
*/
void readMVD_AEC(Macroblock *currMB, CSobj *cs_aec, int b8, int *val_1, int val_2)
{
    int a, b;
    int act_ctx;
    int act_sym;
    int mv_local_err;
    int mv_sign;
    int list_idx = val_2 & 0x01;
    int k = ( val_2>>1 ); // MVD component
    int bin_idx,sig,l,binary_symbol;
    int golomb_order=0;

    PixelPos block_a, block_b;
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    MotionInfoContexts *ctx = cs_aec->mot_ctx;
    
    getLuma8x8Neighbour(currMB, img->current_mb_nr, b8, -1,  0, &block_a );
    getLuma8x8Neighbour(currMB, img->current_mb_nr, b8, 0, -1, &block_b);

    if ( block_b.available )
    {
        b = absm( img->mb_data[block_b.mb_addr].mvd[list_idx][block_b.y][block_b.x][k] );
    }
    else
    {
        b=0;
    }

    if ( block_a.available )
    {
        a = absm( img->mb_data[block_a.mb_addr].mvd[list_idx][block_a.y][block_a.x][k] );
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

    bin_idx = 0;
    binary_symbol =0;

    if ( !biari_decode_symbol( dep_dp,&ctx->mv_res_contexts[k][act_ctx] ) )
    {
        act_sym = 0;
    }
    else if ( !biari_decode_symbol( dep_dp,&ctx->mv_res_contexts[k][3] ) )
    {
        act_sym = 1;
    }
    else if ( !biari_decode_symbol( dep_dp,&ctx->mv_res_contexts[k][4] ) )
    {
        act_sym = 2;
    }
    else if ( !biari_decode_symbol( dep_dp,&ctx->mv_res_contexts[k][5] ) ) //1110, odd
    {
        act_sym = 0;
        // get the prefix
        do
        {
            l=biari_decode_symbol_eq_prob( dep_dp );
            if ( l==0 )
            {
                act_sym += ( 1<<golomb_order );
                golomb_order++;
            }
        }
        while ( l!=1 );

        // get the suffix
        while ( golomb_order-- )
        {
            //next binary part
            sig = biari_decode_symbol_eq_prob( dep_dp );
            if ( sig==1 )
            {
                binary_symbol |= ( 1<<golomb_order );
            }
        }
        act_sym+=binary_symbol;

        act_sym=3+act_sym*2;
    }
    else //1111, even
    {
        act_sym = 0;
        do
        {
            l=biari_decode_symbol_eq_prob( dep_dp );
            if ( l==0 )
            {
                act_sym += ( 1<<golomb_order );
                golomb_order++;
            }
        }
        while ( l!=1 );

        while ( golomb_order-- )
        {
            //next binary part
            sig = biari_decode_symbol_eq_prob( dep_dp );
            if ( sig==1 )
            {
                binary_symbol |= ( 1<<golomb_order );
            }
        }
        act_sym+=binary_symbol;
        act_sym = 4+act_sym*2;

    }



    if ( act_sym!=0 )
    {
        mv_sign=biari_decode_symbol_eq_prob( dep_dp );
        act_sym =( mv_sign==0 )?act_sym:-act_sym;
    }
    *val_1 = act_sym;
}


/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the 8x8 block type.
************************************************************************
*/
void read_B8x8_PredTypeInfo_AEC( CSobj *cs_aec, int *val )
{
    int act_sym = 0;
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    MotionInfoContexts *ctx = cs_aec->mot_ctx;
    BiContextTypePtr pCTX = ctx->b8_type_contexts[0];
    int act_ctx;

    if( biari_decode_symbol( dep_dp, pCTX ) )
    {
        act_sym = 2;
        act_ctx = 3;
    }
    else
    {
        act_sym = 0;
        act_ctx = 2;
    }

    if( biari_decode_symbol( dep_dp, pCTX + act_ctx ) )
    {
        ++act_sym;
    }
    *val = act_sym;
}

void read_Bfrm_PredTypeInfo_AEC( CSobj *cs_aec, int *val )
{
    int act_ctx;
    int act_sym;

    Env_AEC *dep_dp = &cs_aec->de_AEC;
    MotionInfoContexts *ctx = cs_aec->mot_ctx;
    BiContextTypePtr pCTX = ctx->b8_type_contexts[0];

    act_sym = 0;
    if( biari_decode_symbol( dep_dp, pCTX ) )
    {
        act_sym = 2;
        act_ctx = 1;
        if ( biari_decode_symbol( dep_dp, pCTX + act_ctx ) )
        {
            ++act_sym;
        }
    }
    *val = act_sym;
}

void read_P16x16_PredTypeInfo_AEC( CSobj *cs_aec, int *val )
{
    int act_sym = 0;
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    MotionInfoContexts *ctx = cs_aec->mot_ctx;
    BiContextTypePtr pCTX = ctx->b8_type_contexts[0];
    int act_ctx;

    if( biari_decode_symbol( dep_dp, pCTX ) )
    {
        act_sym = 2;
        act_ctx = 1;
        if ( biari_decode_symbol( dep_dp, pCTX + act_ctx ) )
        {
            ++act_sym;
        }
    }
    *val = act_sym;
}


void read_Pfrm_PredTypeInfo_AEC( CSobj *cs_aec, int *val )
{
    int act_sym = 0;
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    MotionInfoContexts *ctx = cs_aec->mot_ctx;
    BiContextTypePtr pCTX = ctx->b8_type_contexts[0];
    int act_ctx;
    act_ctx = 0;
    if( biari_decode_symbol( dep_dp, pCTX + act_ctx ) )
    {
        act_sym ++;
    }
    *val = act_sym;
}


void readMBTransType_AEC( CSobj *cs_aec, int *val )
{
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    BiContextTypePtr pCTX = cs_aec->tex_ctx->tu_size_context;
    *val = biari_decode_symbol( dep_dp, pCTX );
}

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the macroblock
*    type info of a given MB.
************************************************************************
*/
void readMBPartTypeInfo_AEC( CSobj *cs_aec, int *val, int bframe, int act_ctx )
{
    int act_sym;

    Env_AEC *dep_dp = &cs_aec->de_AEC;
    MotionInfoContexts *ctx = cs_aec->mot_ctx;

    if ( bframe )
    {
        BiContextTypePtr pCTX = ctx->mb_type_contexts[2];
        if( biari_decode_symbol( dep_dp, &ctx->mb_type_contexts[2][act_ctx] ) == 0 )
        {
            act_sym = 0;
        }
        else
        {
            act_sym = 1;
            act_ctx = 3;
            while( biari_decode_symbol ( dep_dp, pCTX + act_ctx ) == 0 )
            {
                act_sym++;
                act_ctx++;
                if( act_ctx>=4 )
                {
                    act_ctx=4;
                }
            }
        }
    }
    else            // P-frame
    {
        BiContextTypePtr pCTX = ctx->mb_type_contexts[1];
        act_ctx = 0;
        act_sym = 0;
        while( biari_decode_symbol ( dep_dp, pCTX+act_ctx )==0 )
        {
            act_sym++;
            act_ctx++;
            if( act_ctx>=3 )
            {
                act_ctx=3;
            }
        }
    }
    *val = act_sym;
}

void readMBDeltaQp_AEC(CSobj *cs_aec, int *val, int prev_qp)
{
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    BiContextTypePtr pCTX = &cs_aec->tex_ctx->MBdeltaQP_contexts[0];
    int ctx_idx = (prev_qp == 0) ? 0 : 1;
    int temp_val;

    if (biari_decode_symbol(dep_dp, &pCTX[ctx_idx]))
    {
        temp_val = 0;
    }
    else
    {
        temp_val = 1;
        ctx_idx = 2;
        while (biari_decode_symbol(dep_dp, &pCTX[ctx_idx]) == 0)
        {
            temp_val++;
            ctx_idx++;
            if (ctx_idx >= 3)
            {
                ctx_idx = 3;
            }
        }
    }

    *val = temp_val;
#if TRACE
    output_trace_info("mb_qp_delta: %d", *val);
#endif
}

/*!
************************************************************************
* \brief
*    This function is used to arithmetically decode the coded
*    block pattern of a given MB.
************************************************************************
*/
void readCBP_AEC(Macroblock *currMB, CSobj *cs_aec, int *val)
{
    Env_AEC *dep_dp = &cs_aec->de_AEC;
    TextureInfoContexts *ctx = cs_aec->tex_ctx;

    int b8, b4, b8add, b4add;
    int b8x, b8y, b4x, b4y;
    int a, b;
    int curr_cbp_ctx;
    int cbp = 0;
    int cbp_bit;

    if ( currMB->mb_trans_type == TRANS_2Nx2N )
    {
        b8add = 4;
    }
    else
    {
        b8add = 1;
    }

    //  coding of luma part (bit by bit)
    for ( b8 = 0; b8 < 4; b8 += b8add )
    {
        b8x = b8 & 1;
        b8y = b8 / 2;
        if ( currMB->mb_trans_type == TRANS_2Nx2N || currMB->sub_mb_trans_type[b8] == 0 )
        {
            b4add = 4;
        }
        else
        {
            b4add = 1;
        }

        for ( b4 = 0; b4 < 4; b4 += b4add )
        {
            int b8_up = 0xF, b8_left = 0xF; // cbp in 8x8 block, 4bit
            int b8_cbp = cbp >> ( 4 * b8 );
            b4y = b4 / 2;
            b4x = b4 & 1;

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

            curr_cbp_ctx = a + 2 * b;

            if ( biari_decode_symbol( dep_dp, ctx->cbp_contexts[0] + curr_cbp_ctx ) )
            {
                if ( b4add == 4 )
                {
                    cbp += ( 0xF << ( 4 * b8 ) );
                }
                else
                {
                    cbp += ( 0x1 << ( 4 * b8 + b4 ) );
                }
            }
        }
    }

    if ( currMB->mb_trans_type == TRANS_2Nx2N && cbp )
    {
        cbp = 0xFFFF;
    }

    // chroma cbp
    curr_cbp_ctx = 0;
    cbp_bit = biari_decode_symbol( dep_dp, ctx->cbp_contexts[1] + curr_cbp_ctx );

    if ( cbp_bit )
    {
        curr_cbp_ctx = 1;
        cbp_bit = biari_decode_symbol( dep_dp, ctx->cbp_contexts[1] + curr_cbp_ctx );
        if ( cbp_bit )
        {
            cbp += 0xFF0000;
        }
        else
        {
            curr_cbp_ctx = 1;
            cbp_bit = biari_decode_symbol( dep_dp, ctx->cbp_contexts[1] + curr_cbp_ctx );
            cbp += ( cbp_bit == 1 ) ? 0xF00000 : 0xF0000;
        }
    }
    *val = cbp;
}

int DCT_Level[257];
int DCT_Run[257];
int Pair_Pos = 0;
int DCT_Pairs = -1;

void readRunLevel_AEC( CSobj *cs_aec, int bsize, int *level, int *run, int ctx_in )
{
    Env_AEC *dep_dp = &( cs_aec->de_AEC );
    const int T_Chr[5] = { 0,1,2,4,3000};
    int pairs, rank, pos;
    int Run, Level, absLevel, symbol;
    int coef_num, pos_shift_offset = 0;
    coef_num = bsize * bsize;

    switch ( bsize )
    {
        case 4:
            pos_shift_offset = 0;
            break;
        case 8:
            pos_shift_offset = 2;
            break;
        case 16:
            pos_shift_offset = 4;
            break;
    }

    //--- read coefficients for whole block ---
    if( DCT_Pairs<0 )
    {
        BiContextType ( *Primary )[NUM_MAP_CTX];
        BiContextTypePtr pCTX;
        BiContextTypePtr pCTX2;
        int ctx, ctx2, offset;
        if( ctx_in == LUMA_8x8 )
        {
            Primary = cs_aec->tex_ctx->map_contexts;
        }
        else
        {
            Primary = cs_aec->tex_ctx->last_contexts;
        }
        //! Decode
        rank = 0;
        pos = 0;
        for( pairs=0; pairs<coef_num + 1; pairs++ )
        {
            pCTX = Primary[rank];
            //! EOB
            if( rank>0 )
            {
                pCTX2 = Primary[5+( pos>>( 3+pos_shift_offset ) )];
                ctx2 = ( pos>>1 )&0x0f;
                ctx = 0;
                //if( biari_decode_symbol(dep_dp, pCTX+ctx) ) {
                if( biari_decode_symbolW( dep_dp, pCTX+ctx, pCTX2+ctx2 ) )
                {
                    break;
                }
            }
            //! Level
            ctx = 1;
            symbol = 0;
            while( biari_decode_symbol( dep_dp, pCTX+ctx )==0 )
            {
                symbol += 1;
                ctx ++;
                if( ctx>=2 )
                {
                    ctx =2;
                }
            }
            absLevel = symbol + 1;
            //! Sign
            if( biari_decode_symbol_eq_prob( dep_dp ) )
            {
                Level = - absLevel;
            }
            else
            {
                Level = absLevel;
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
            symbol = 0;
            ctx = 0;
            while( biari_decode_symbol( dep_dp, pCTX+ctx+offset )==0 )
            {
                symbol += 1;
                ctx ++;
                if( ctx>=1 )
                {
                    ctx =1;
                }
            }
            Run = symbol;
            DCT_Level[pairs] = Level;
            DCT_Run[pairs] = Run;
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
            pos += ( Run+1 );
            if( pos>=coef_num )
            {
                pos = coef_num - 1;
            }
        }
        DCT_Pairs = pairs;
        Pair_Pos = DCT_Pairs;
    }
    //--- set run and level ---
    if( DCT_Pairs>0 )
    {
        *level = DCT_Level[Pair_Pos-1];
        *run = DCT_Run[Pair_Pos-1];
        Pair_Pos --;
    }
    else
    {
        *level = 0;
        *run = 0;
    }
    //--- decrement coefficient counter and re-set position ---
    if( ( DCT_Pairs-- )==0 )
    {
        Pair_Pos = 0;
    }
}

/*!
************************************************************************
* \brief
*    decoding of unary binarization using one or 2 distinct
*    models for the first and all remaining bins; no terminating
*    "0" for max_symbol
***********************************************************************
*/
i32u_t unary_bin_max_decode( Env_AEC * dep_dp,
                             BiContextTypePtr ctx,
                             int ctx_offset,
                             i32u_t max_symbol )
{
    i32u_t l;
    i32u_t symbol;
    BiContextTypePtr ictx;

    symbol =  biari_decode_symbol( dep_dp, ctx );

    if ( symbol==0 )
    {
        return 0;
    }
    else
    {
        if ( max_symbol == 1 )
        {
            return symbol;
        }
        symbol=0;
        ictx=ctx+ctx_offset;
        do
        {
            l=biari_decode_symbol( dep_dp, ictx );
            symbol++;
        }
        while( ( l!=0 ) && ( symbol<max_symbol-1 ) );
        if ( ( l!=0 ) && ( symbol==max_symbol-1 ) )
        {
            symbol++;
        }
        return symbol;
    }
}


/*!
************************************************************************
* \brief
*    decoding of unary binarization using one or 2 distinct
*    models for the first and all remaining bins
***********************************************************************
*/
i32u_t unary_bin_decode( Env_AEC * dep_dp,
                         BiContextTypePtr ctx,
                         int ctx_offset )
{
    i32u_t l;
    i32u_t symbol;
    BiContextTypePtr ictx;

    symbol = 1 - biari_decode_symbol( dep_dp, ctx );

    if ( symbol==0 )
    {
        return 0;
    }
    else
    {
        symbol=0;
        ictx=ctx+ctx_offset;
        do
        {
            l= 1 - biari_decode_symbol( dep_dp, ictx );
            symbol++;
        }
        while( l!=0 );
        return symbol;
    }
}


/*!
************************************************************************
* \brief
*    finding end of a slice in case this is not the end of a frame
*    This bit is useless, because in the current IVC, slice is terminated by start code.
*
* \return
*    0 : the end of macroblock
*    1 : the end of slice
************************************************************************
*/
int read_terminating_bit( ImgParams *img, int eos_bit )
{
    i32u_t  bit;
    Env_AEC * dep_dp = &( img->cs_aec->de_AEC );

    if( eos_bit )
    {
        bit = biari_decode_final ( dep_dp );
    }
    else
    {
        bit = 0;
    }

    return bit;
}

/*!
************************************************************************
* \brief
*    Exp Golomb binarization and decoding of a symbol
*    with prob. of 0.5
************************************************************************
*/
i32u_t exp_golomb_decode_eq_prob( Env_AEC * dep_dp,
                                  int k )
{
    i32u_t l;
    int symbol = 0;
    int binary_symbol = 0;

    do
    {
        l=biari_decode_symbol_eq_prob( dep_dp );
        if ( l==1 )
        {
            symbol += ( 1<<k );
            k++;
        }
    }
    while ( l!=0 );

    while ( k-- )                           //next binary part
        if ( biari_decode_symbol_eq_prob( dep_dp )==1 )
        {
            binary_symbol |= ( 1<<k );
        }

    return ( i32u_t ) ( symbol+binary_symbol );
}


/*!
************************************************************************
* \brief
*    Exp-Golomb decoding for LEVELS
***********************************************************************
*/
i32u_t unary_exp_golomb_level_decode( Env_AEC * dep_dp,
                                      BiContextTypePtr ctx )
{
    i32u_t l,k;
    i32u_t symbol;
    i32u_t exp_start = 13;

    symbol = biari_decode_symbol( dep_dp, ctx );

    if ( symbol==0 )
    {
        return 0;
    }
    else
    {
        symbol=0;
        k=1;
        do
        {
            l=biari_decode_symbol( dep_dp, ctx );
            symbol++;
            k++;
        }
        while( ( l!=0 ) && ( k!=exp_start ) );
        if ( l!=0 )
        {
            symbol += exp_golomb_decode_eq_prob( dep_dp,0 )+1;
        }
        return symbol;
    }
}




/*!
************************************************************************
* \brief
*    Exp-Golomb decoding for Motion Vectors
***********************************************************************
*/
i32u_t unary_exp_golomb_mv_decode( Env_AEC * dep_dp,
                                   BiContextTypePtr ctx,
                                   i32u_t max_bin )
{
    i32u_t l,k;
    i32u_t bin=1;
    i32u_t symbol;
    i32u_t exp_start = 8;

    BiContextTypePtr ictx=ctx;

    symbol = biari_decode_symbol( dep_dp, ictx );

    if ( symbol==0 )
    {
        return 0;
    }
    else
    {
        symbol=0;
        k=1;

        ictx++;
        do
        {
            l=biari_decode_symbol( dep_dp, ictx  );
            if ( ( ++bin )==2 )
            {
                ictx++;
            }
            if ( bin==max_bin )
            {
                ictx++;
            }
            symbol++;
            k++;
        }
        while( ( l!=0 ) && ( k!=exp_start ) );
        if ( l!=0 )
        {
            symbol += exp_golomb_decode_eq_prob( dep_dp,3 )+1;
        }
        return symbol;
    }
}


/*!
************************************************************************
* \brief
*    Read one byte from AEC-partition.
*    Bitstream->read_len will be modified
*
*
* \author
*
************************************************************************
*/
void readIPCMBytes_AEC( SyntaxElement *sym, Bitstream *currStream )
{
    int read_len = currStream->read_len;
    int code_len = currStream->bs_length;
    uchar_t *buf = currStream->streamBuffer;

    sym->len=8;

    if( read_len<code_len )
    {
        sym->inf=buf[read_len++];
    }

    sym->value1=sym->inf;
    currStream->read_len=read_len;
}

void read_Reffrm_AEC( CSobj *cs_aec, int *val )
{
    int act_ctx = 0;
    int act_sym = 0;
    Env_AEC *dep_dp = &( cs_aec->de_AEC );
    BiContextTypePtr pCTX = cs_aec->tex_ctx->one_contexts[1];//ctx->mb_type_contexts[1];

    while( biari_decode_symbol( dep_dp, pCTX+act_ctx ) == 0 )
    {
        act_sym++;
        act_ctx++;
        if( act_ctx>=( img->real_ref_num-1 ) )
        {
            act_ctx=( img->real_ref_num-1 );
        }
    }
    *val = act_sym;
}