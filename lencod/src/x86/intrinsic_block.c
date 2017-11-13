/*****************************************************************************
* Authors: Ronggang Wang <rgwang@pkusz.edu.cn>
*          Zhenyu Wang <wangzhenyu@pkusz.edu.cn>
*          Kui Fan <kuifan@pku.edu.cn>
*          Shenghao Zhang <1219759986@qq.com>
* Affiliate: Peking University Shenzhen Graduate School
*****************************************************************************/

#include "intrinsic.h"

#if COMPILE_FOR_8BIT

void post_rdoquant_sse128( const int bsize, const int *src, int *dst )
{
    int i = 0, j = 0;
    __m128i ;

    __m128i m_SRC1, m_SRC2, m_SRC3, m_SRC4, m_DST1, m_DST2, m_DST3, m_DST4;
    __m128i m_ZERO;
    __m128i m_NEGNUM;
    __m128i m_MASK;

    m_ZERO = _mm_set1_epi32( 0 );
    m_NEGNUM = _mm_set1_epi8( -1 );

    if ( 4 == bsize )
    {
        for ( i = 0; i < bsize; i++ )
        {
            m_SRC1  = _mm_lddqu_si128( ( __m128i* )src );
            m_DST1  = _mm_lddqu_si128( ( __m128i* )dst );
            m_MASK = _mm_cmpeq_epi32( m_SRC1, m_ZERO );
            _mm_maskmoveu_si128( m_NEGNUM, m_MASK, ( char * )&m_SRC1 );

            m_DST1 = _mm_sign_epi32( m_DST1, m_SRC1 );
            _mm_storeu_si128( ( __m128i * )dst, m_DST1 );

            src += bsize;
            dst += bsize;
        }
    }
    else if ( 8 == bsize )
    {
        for ( i = 0; i < bsize; i++ )
        {
            m_SRC1 = _mm_lddqu_si128( ( __m128i* )src );
            m_SRC2 = _mm_lddqu_si128( ( __m128i* )( src + 4 ) );
            m_DST1 = _mm_lddqu_si128( ( __m128i* )dst );
            m_DST2 = _mm_lddqu_si128( ( __m128i* )( dst + 4 ) );

            m_MASK = _mm_cmpeq_epi32( m_SRC1, m_ZERO );
            _mm_maskmoveu_si128( m_NEGNUM, m_MASK, ( char * )&m_SRC1 );

            m_MASK = _mm_cmpeq_epi32( m_SRC2, m_ZERO );
            _mm_maskmoveu_si128( m_NEGNUM, m_MASK, ( char * )&m_SRC2 );

            m_DST1 = _mm_sign_epi32( m_DST1, m_SRC1 );
            m_DST2 = _mm_sign_epi32( m_DST2, m_SRC2 );
            _mm_storeu_si128( ( __m128i * )dst, m_DST1 );
            _mm_storeu_si128( ( __m128i * )( dst + 4 ), m_DST2 );

            src += bsize;
            dst += bsize;
        }
    }
    else
    {
        // if(bsize == 16){...}
        for ( i = 0; i < bsize; i++ )
        {
            m_SRC1 = _mm_lddqu_si128( ( __m128i* )src );
            m_SRC2 = _mm_lddqu_si128( ( __m128i* )( src + 4 ) );
            m_SRC3 = _mm_lddqu_si128( ( __m128i* )( src + 8 ) );
            m_SRC4 = _mm_lddqu_si128( ( __m128i* )( src + 12 ) );
            m_DST1 = _mm_lddqu_si128( ( __m128i* )dst );
            m_DST2 = _mm_lddqu_si128( ( __m128i* )( dst + 4 ) );
            m_DST3 = _mm_lddqu_si128( ( __m128i* )( dst + 8 ) );
            m_DST4 = _mm_lddqu_si128( ( __m128i* )( dst + 12 ) );

            m_MASK = _mm_cmpeq_epi32( m_SRC1, m_ZERO );
            _mm_maskmoveu_si128( m_NEGNUM, m_MASK, ( char * )&m_SRC1 );

            m_MASK = _mm_cmpeq_epi32( m_SRC2, m_ZERO );
            _mm_maskmoveu_si128( m_NEGNUM, m_MASK, ( char * )&m_SRC2 );

            m_MASK = _mm_cmpeq_epi32( m_SRC3, m_ZERO );
            _mm_maskmoveu_si128( m_NEGNUM, m_MASK, ( char * )&m_SRC3 );

            m_MASK = _mm_cmpeq_epi32( m_SRC4, m_ZERO );
            _mm_maskmoveu_si128( m_NEGNUM, m_MASK, ( char * )&m_SRC4 );

            m_DST1 = _mm_sign_epi32( m_DST1, m_SRC1 );
            m_DST2 = _mm_sign_epi32( m_DST2, m_SRC2 );
            m_DST3 = _mm_sign_epi32( m_DST3, m_SRC3 );
            m_DST4 = _mm_sign_epi32( m_DST4, m_SRC4 );
            _mm_storeu_si128( ( __m128i * )dst, m_DST1 );
            _mm_storeu_si128( ( __m128i * )( dst + 4 ), m_DST2 );
            _mm_storeu_si128( ( __m128i * )( dst + 8 ), m_DST3 );
            _mm_storeu_si128( ( __m128i * )( dst + 12 ), m_DST4 );

            src += bsize;
            dst += bsize;
        }
    }

    return ;
}


void quant_B8_sse128( int qp, int mode, int *curr_blk, int bsize, int Q )  //intra_flag=>mode
{
    int y;
    int qp_const;
    int intra;
    int shift;

    __m128i m_VAL1, m_VAL2, m_VAL3, m_VAL4;
    __m128i m_TEMP1, m_TEMP2, m_TEMP3, m_TEMP4;
    __m128i m_VALUE1, m_VALUE2, m_VALUE3, m_VALUE4;
    __m128i m_Q, m_QCONST, m_SHIFT;
    __m128i m_ZERO, m_POSINUM, m_MASK;

    intra = 0;
    shift = 15;

    if ( mode > 3 ) // mode 0..inter, mode 4.. intra
    {
        intra = 1;
    }

    if ( intra )
    {
        qp_const = ( 1 << 15 ) * 10 / 31;
    }
    else
    {
        qp_const = ( 1 << 15 ) * 10 / 62;
    }

    // quantization sse
    m_Q = _mm_set1_epi32( Q );
    m_QCONST = _mm_set1_epi32( qp_const );
    m_SHIFT = _mm_set1_epi32( shift );
    m_ZERO = _mm_set1_epi32( 0 );
    m_POSINUM = _mm_set1_epi8( 1 );

    if ( 4 == bsize )
    {
        for ( y = 0; y < bsize; y++ )
        {
            m_VAL1 = _mm_lddqu_si128( ( __m128i * )curr_blk );

            m_TEMP1 = _mm_abs_epi32( m_VAL1 );

            m_VALUE1 = _mm_srli_epi32( _mm_add_epi32( _mm_mullo_epi32( m_TEMP1, m_Q ), m_QCONST ), shift );
            m_VALUE1 = _mm_abs_epi32( m_VALUE1 );

            m_MASK = _mm_cmpeq_epi32( m_VAL1, m_ZERO );
            _mm_maskmoveu_si128( m_POSINUM, m_MASK, ( char * )&m_VAL1 );

            m_VALUE1 = _mm_sign_epi32( m_VALUE1, m_VAL1 );
            _mm_storeu_si128( ( __m128i * )curr_blk, m_VALUE1 );

            // skip
            curr_blk += bsize;
        }
    }
    else if ( 8 == bsize )
    {
        for ( y = 0; y < bsize; y++ )
        {
            m_VAL1 = _mm_lddqu_si128( ( __m128i * )curr_blk );
            m_VAL2 = _mm_lddqu_si128( ( __m128i * )( curr_blk+4 ) );

            m_TEMP1 = _mm_abs_epi32( m_VAL1 );
            m_TEMP2 = _mm_abs_epi32( m_VAL2 );

            m_VALUE1 = _mm_srli_epi32( _mm_add_epi32( _mm_mullo_epi32( m_TEMP1, m_Q ), m_QCONST ), shift );
            m_VALUE1 = _mm_abs_epi32( m_VALUE1 );
            m_VALUE2 = _mm_srli_epi32( _mm_add_epi32( _mm_mullo_epi32( m_TEMP2, m_Q ), m_QCONST ), shift );
            m_VALUE2 = _mm_abs_epi32( m_VALUE2 );

            m_MASK = _mm_cmpeq_epi32( m_VAL1, m_ZERO );
            _mm_maskmoveu_si128( m_POSINUM, m_MASK, ( char * )&m_VAL1 );
            m_MASK = _mm_cmpeq_epi32( m_VAL2, m_ZERO );
            _mm_maskmoveu_si128( m_POSINUM, m_MASK, ( char * )&m_VAL2 );

            m_VALUE1 = _mm_sign_epi32( m_VALUE1, m_VAL1 );
            _mm_storeu_si128( ( __m128i * )curr_blk, m_VALUE1 );
            m_VALUE2 = _mm_sign_epi32( m_VALUE2, m_VAL2 );
            _mm_storeu_si128( ( __m128i * )( curr_blk+4 ), m_VALUE2 );

            // skip
            curr_blk += bsize;
        }
    }
    else
    {
        for ( y = 0; y < bsize; y++ )
        {
            m_VAL1 = _mm_lddqu_si128( ( __m128i * )curr_blk );
            m_VAL2 = _mm_lddqu_si128( ( __m128i * )( curr_blk + 4 ) );
            m_VAL3 = _mm_lddqu_si128( ( __m128i * )( curr_blk + 8 ) );
            m_VAL4 = _mm_lddqu_si128( ( __m128i * )( curr_blk + 12 ) );

            m_TEMP1 = _mm_abs_epi32( m_VAL1 );
            m_TEMP2 = _mm_abs_epi32( m_VAL2 );
            m_TEMP3 = _mm_abs_epi32( m_VAL3 );
            m_TEMP4 = _mm_abs_epi32( m_VAL4 );

            m_VALUE1 = _mm_srli_epi32( _mm_add_epi32( _mm_mullo_epi32( m_TEMP1, m_Q ), m_QCONST ), shift );
            m_VALUE1 = _mm_abs_epi32( m_VALUE1 );
            m_VALUE2 = _mm_srli_epi32( _mm_add_epi32( _mm_mullo_epi32( m_TEMP2, m_Q ), m_QCONST ), shift );
            m_VALUE2 = _mm_abs_epi32( m_VALUE2 );
            m_VALUE3 = _mm_srli_epi32( _mm_add_epi32( _mm_mullo_epi32( m_TEMP3, m_Q ), m_QCONST ), shift );
            m_VALUE3 = _mm_abs_epi32( m_VALUE3 );
            m_VALUE4 = _mm_srli_epi32( _mm_add_epi32( _mm_mullo_epi32( m_TEMP4, m_Q ), m_QCONST ), shift );
            m_VALUE4 = _mm_abs_epi32( m_VALUE4 );

            m_MASK = _mm_cmpeq_epi32( m_VAL1, m_ZERO );
            _mm_maskmoveu_si128( m_POSINUM, m_MASK, ( char * )&m_VAL1 );
            m_MASK = _mm_cmpeq_epi32( m_VAL2, m_ZERO );
            _mm_maskmoveu_si128( m_POSINUM, m_MASK, ( char * )&m_VAL2 );
            m_MASK = _mm_cmpeq_epi32( m_VAL3, m_ZERO );
            _mm_maskmoveu_si128( m_POSINUM, m_MASK, ( char * )&m_VAL3 );
            m_MASK = _mm_cmpeq_epi32( m_VAL4, m_ZERO );
            _mm_maskmoveu_si128( m_POSINUM, m_MASK, ( char * )&m_VAL4 );

            m_VALUE1 = _mm_sign_epi32( m_VALUE1, m_VAL1 );
            _mm_storeu_si128( ( __m128i * )curr_blk, m_VALUE1 );
            m_VALUE2 = _mm_sign_epi32( m_VALUE2, m_VAL2 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 ), m_VALUE2 );
            m_VALUE3 = _mm_sign_epi32( m_VALUE3, m_VAL3 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 ), m_VALUE3 );
            m_VALUE4 = _mm_sign_epi32( m_VALUE4, m_VAL4 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 ), m_VALUE4 );

            // skip
            curr_blk += bsize;
        }
    }
}

void inv_quant_sse128( int coef_num, int qp, int bsize, const int *IVC_SCAN, int *curr_blk, int shift, int QPI )
{
    int icoef;
    int clip1 = 0 - ( 1 << 15 );
    int clip2 = ( 1 << 15 ) - 1;
    int shift_val;

    __m128i m_VAL1, m_VAL2, m_VAL3, m_VAL4;
    __m128i m_TEMP1, m_TEMP2, m_TEMP3, m_TEMP4;
    __m128i m_VALUE1, m_VALUE2, m_VALUE3, m_VALUE4;
    __m128i m_QPI, m_SHIFT, m_SHIFT_2, m_CLIP1, m_CLIP2;

    shift_val = 1 << ( shift - 1 );
    m_QPI = _mm_set1_epi32( QPI );
    m_SHIFT = _mm_set1_epi32( shift_val );
    m_SHIFT_2 = _mm_set1_epi32( shift );
    m_CLIP1 = _mm_set1_epi32( clip1 );
    m_CLIP2 = _mm_set1_epi32( clip2 );

    // inv-quantization sse
    if ( 4 == bsize )
    {
        for ( icoef = 0; icoef < bsize; icoef++ )
        {
            m_VAL1 = _mm_lddqu_si128( ( __m128i * )curr_blk );

            m_TEMP1 = _mm_srai_epi32( _mm_add_epi32( _mm_mullo_epi32( m_VAL1, m_QPI ), m_SHIFT ), shift );

            m_VALUE1 = _mm_max_epi32( m_TEMP1, m_CLIP1 );
            m_VALUE1 = _mm_min_epi32( m_VALUE1, m_CLIP2 );

            _mm_storeu_si128( ( __m128i * )curr_blk, m_VALUE1 );

            // skip
            curr_blk += bsize;
        }
    }
    else if ( 8 == bsize )
    {
        for ( icoef = 0; icoef < bsize; icoef++ )
        {
            m_VAL1 = _mm_lddqu_si128( ( __m128i * )curr_blk );
            m_VAL2 = _mm_lddqu_si128( ( __m128i * )( curr_blk + 4 ) );

            m_TEMP1 = _mm_srai_epi32( _mm_add_epi32( _mm_mullo_epi32( m_VAL1, m_QPI ), m_SHIFT ), shift );
            m_TEMP2 = _mm_srai_epi32( _mm_add_epi32( _mm_mullo_epi32( m_VAL2, m_QPI ), m_SHIFT ), shift );

            m_VALUE1 = _mm_min_epi32( _mm_max_epi32( m_TEMP1, m_CLIP1 ), m_CLIP2 );
            m_VALUE2 = _mm_min_epi32( _mm_max_epi32( m_TEMP2, m_CLIP1 ), m_CLIP2 );

            _mm_storeu_si128( ( __m128i * )curr_blk, m_VALUE1 );
            _mm_storeu_si128( ( __m128i * )( curr_blk+4 ), m_VALUE2 );

            // skip
            curr_blk += bsize;
        }
    }
    else
    {
        for ( icoef = 0; icoef < bsize; icoef++ )
        {
            m_VAL1 = _mm_lddqu_si128( ( __m128i * )curr_blk );
            m_VAL2 = _mm_lddqu_si128( ( __m128i * )( curr_blk + 4 ) );
            m_VAL3 = _mm_lddqu_si128( ( __m128i * )( curr_blk + 8 ) );
            m_VAL4 = _mm_lddqu_si128( ( __m128i * )( curr_blk + 12 ) );

            m_TEMP1 = _mm_srai_epi32( _mm_add_epi32( _mm_mullo_epi32( m_VAL1, m_QPI ), m_SHIFT ), shift );
            m_TEMP2 = _mm_srai_epi32( _mm_add_epi32( _mm_mullo_epi32( m_VAL2, m_QPI ), m_SHIFT ), shift );
            m_TEMP3 = _mm_srai_epi32( _mm_add_epi32( _mm_mullo_epi32( m_VAL3, m_QPI ), m_SHIFT ), shift );
            m_TEMP4 = _mm_srai_epi32( _mm_add_epi32( _mm_mullo_epi32( m_VAL4, m_QPI ), m_SHIFT ), shift );

            m_VALUE1 = _mm_min_epi32( _mm_max_epi32( m_TEMP1, m_CLIP1 ), m_CLIP2 );
            m_VALUE2 = _mm_min_epi32( _mm_max_epi32( m_TEMP2, m_CLIP1 ), m_CLIP2 );
            m_VALUE3 = _mm_min_epi32( _mm_max_epi32( m_TEMP3, m_CLIP1 ), m_CLIP2 );
            m_VALUE4 = _mm_min_epi32( _mm_max_epi32( m_TEMP4, m_CLIP1 ), m_CLIP2 );

            _mm_storeu_si128( ( __m128i * )curr_blk, m_VALUE1 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 ), m_VALUE2 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 ), m_VALUE3 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 ), m_VALUE4 );

            // skip
            curr_blk += bsize;
        }
    }
}

#endif