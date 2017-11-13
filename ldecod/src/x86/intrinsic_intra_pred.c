/*****************************************************************************
* Authors: Ronggang Wang <rgwang@pkusz.edu.cn>
*          Zhenyu Wang <wangzhenyu@pkusz.edu.cn>
*          Kui Fan <kuifan@pku.edu.cn>
*          Shenghao Zhang <1219759986@qq.com>
* Affiliate: Peking University Shenzhen Graduate School
*****************************************************************************/

#include "intrinsic.h"

#if COMPILE_FOR_8BIT

void xPredIntraLumaDC_sse128( uchar_t *pSrc,
                              pel_t *pDst,
                              int i_dst,
                              int iBlkWidth,
                              int iBlkHeight,
                              bool bAboveAvail,
                              bool bLeftAvail )
{
    int y, tmp, shift1, shift2;
    char_t param[4] = { 1, 2, 1, 0};

    __m128i mask, mIntra_DC_constvalue, mParam, mSwitch1, mSwitch2, mSwitch3, mSwitch4, mSrc, mSrc_tmp, mT0, mT1, mT2, mT3, mSum, mSum1, mSum2, mVal, mTmp;

    mParam = _mm_set1_epi32( *( i32s_t* )param );

    mSwitch1 = _mm_setr_epi8( 0, 1, 2, -1, 1, 2, 3, -1, 2, 3, 4, -1, 3, 4, 5, -1 );
    mSwitch2 = _mm_setr_epi8( 4, 5, 6, -1, 5, 6, 7, -1, 6, 7, 8, -1, 7, 8, 9, -1 );
    // change the src
    mSwitch3 = _mm_setr_epi8( 0, 1, 2, -1, 1, 2, 3, -1, 2, 3, 4, -1, 3, 4, 5, -1 );
    mSwitch4 = _mm_setr_epi8( 4, 5, 6, -1, 5, 6, 7, -1, 6, 7, 8, -1, 7, 8, 9, -1 );

    shift1 = 2;
    shift2 = 3;

    if ( !bAboveAvail && !bLeftAvail )
    {
        for ( y = 0; y<iBlkHeight; y++ )
        {

            switch ( iBlkWidth )
            {
                case 4:
                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    mIntra_DC_constvalue = _mm_set1_epi8( INTRA_DC_CONSTVALUE );
                    _mm_maskmoveu_si128( mIntra_DC_constvalue, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                    break;
                case 8:
                    mIntra_DC_constvalue = _mm_set1_epi8( INTRA_DC_CONSTVALUE );
                    _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mIntra_DC_constvalue );
                    break;
                case 16:
                    mIntra_DC_constvalue = _mm_set1_epi8( INTRA_DC_CONSTVALUE );
                    _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mIntra_DC_constvalue );
                    break;
                default:
                    break;
            }

            pDst += i_dst;
        }
    }
    else if ( bAboveAvail && !bLeftAvail )
    {
        for ( y = 0; y<iBlkHeight; y++ )
        {

            switch ( iBlkWidth )
            {
                case 4:
                    mSrc = _mm_loadu_si128( ( __m128i* )pSrc );

                    mT0  = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mParam );
                    mSum = _mm_hadd_epi16( mT0, mT0 );
                    mVal = _mm_srai_epi16( mSum, shift1 );
                    mVal = _mm_packus_epi16( mVal, mVal );

                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    _mm_maskmoveu_si128( mVal, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                    break;
                case 8:
                    mSrc = _mm_loadu_si128( ( __m128i* )pSrc );

                    mT0  = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mParam );
                    mT1  = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mParam );
                    mSum = _mm_hadd_epi16( mT0, mT1 );
                    mVal = _mm_srai_epi16( mSum, shift1 );
                    mVal = _mm_packus_epi16( mVal, mVal );

                    _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );
                    break;
                case 16:
                    mSrc     = _mm_loadu_si128( ( __m128i* )pSrc );
                    mSrc_tmp = _mm_loadu_si128( ( __m128i* )( pSrc + 8 ) );

                    mT0 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mParam );
                    mT1 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mParam );
                    mT2 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc_tmp, mSwitch3 ), mParam );
                    mT3 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc_tmp, mSwitch4 ), mParam );

                    mSum1 = _mm_hadd_epi16( mT0, mT1 );
                    mSum2 = _mm_hadd_epi16( mT2, mT3 );
                    mSum1 = _mm_srai_epi16( mSum1, shift1 );
                    mSum2 = _mm_srai_epi16( mSum2, shift1 );

                    mVal = _mm_packus_epi16( mSum1, mSum2 );

                    _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );
                    break;
                default:
                    break;
            }

            pDst += i_dst;
        }
    }
    else if ( !bAboveAvail && bLeftAvail )
    {
        for ( y = 0; y<iBlkHeight; y++ )
        {
            tmp = ( 2 * pSrc[-1 - y] + pSrc[-y] + pSrc[-2 - y] ) >> 2;

            mTmp = _mm_set1_epi32( tmp );
            mTmp = _mm_packus_epi32( mTmp, mTmp );
            mTmp = _mm_packus_epi16( mTmp, mTmp );

            switch ( iBlkWidth )
            {
                case 4:
                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    _mm_maskmoveu_si128( mTmp, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                    break;
                case 8:
                    _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mTmp );
                    break;
                case 16:
                    _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mTmp );
                    break;
                default:
                    break;
            }

            pDst += i_dst;
        }
    }
    else if ( bAboveAvail && bLeftAvail )
    {
        for ( y = 0; y<iBlkHeight; y++ )
        {
            tmp = 2 * pSrc[-1 - y] + pSrc[-y] + pSrc[-2 - y];

            mTmp = _mm_set1_epi32( tmp );
            mTmp = _mm_packus_epi32( mTmp, mTmp );
            switch ( iBlkWidth )
            {
                case 4:
                    mSrc = _mm_loadu_si128( ( __m128i* )pSrc );

                    mT0 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mParam );
                    mSum = _mm_hadd_epi16( mT0, mT0 );

                    mSum = _mm_add_epi16( mSum, mTmp );

                    mVal = _mm_srai_epi16( mSum, shift2 );
                    mVal = _mm_packus_epi16( mVal, mVal );

                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    _mm_maskmoveu_si128( mVal, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                    break;
                case 8:
                    mSrc = _mm_loadu_si128( ( __m128i* )pSrc );

                    mT0 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mParam );
                    mT1 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mParam );
                    mSum = _mm_hadd_epi16( mT0, mT1 );

                    mSum = _mm_add_epi16( mSum, mTmp );

                    mVal = _mm_srai_epi16( mSum, shift2 );
                    mVal = _mm_packus_epi16( mVal, mVal );

                    _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );
                    break;
                case 16:
                    mSrc = _mm_loadu_si128( ( __m128i* )pSrc );
                    mSrc_tmp = _mm_loadu_si128( ( __m128i* )( pSrc + 8 ) );

                    mT0 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mParam );
                    mT1 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mParam );
                    mT2 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc_tmp, mSwitch3 ), mParam );
                    mT3 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc_tmp, mSwitch4 ), mParam );

                    mSum1 = _mm_hadd_epi16( mT0, mT1 );
                    mSum2 = _mm_hadd_epi16( mT2, mT3 );

                    mSum1 = _mm_add_epi16( mSum1, mTmp );
                    mSum2 = _mm_add_epi16( mSum2, mTmp );

                    mSum1 = _mm_srai_epi16( mSum1, shift2 );
                    mSum2 = _mm_srai_epi16( mSum2, shift2 );

                    mVal = _mm_packus_epi16( mSum1, mSum2 );

                    _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );
                    break;
                default:
                    break;

            }
            pDst += i_dst;
        }
    }
}

void xPredIntraChromaDC_sse128( uchar_t *pSrc,
                                pel_t *pDst,
                                int i_dst,
                                int iBlkWidth,
                                int iBlkHeight,
                                bool bAboveAvail,
                                bool bLeftAvail )
{
    int y;
    int shift;

    __m128i mask, mIntra_DC_constvalue, mSrc, mVal, mTmp;
    shift = 1;

    if ( !bAboveAvail && !bLeftAvail )
    {
        for ( y = 0; y<iBlkHeight; y++ )
        {

            switch ( iBlkWidth )
            {
                case 4:
                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    mIntra_DC_constvalue = _mm_set1_epi8( INTRA_DC_CONSTVALUE );
                    _mm_maskmoveu_si128( mIntra_DC_constvalue, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                    break;
                case 8:
                    mIntra_DC_constvalue = _mm_set1_epi8( INTRA_DC_CONSTVALUE );
                    _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mIntra_DC_constvalue );
                    break;
                case 16:
                    mIntra_DC_constvalue = _mm_set1_epi8( INTRA_DC_CONSTVALUE );
                    _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mIntra_DC_constvalue );
                    break;
                default:
                    break;
            }

            pDst += i_dst;
        }
    }
    else if ( bAboveAvail && !bLeftAvail )
    {
        for ( y = 0; y<iBlkHeight; y++ )
        {
            mSrc = _mm_loadu_si128( ( __m128i* )( pSrc+1 ) );

            switch ( iBlkWidth )
            {
                case 4:
                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    _mm_maskmoveu_si128( mSrc, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                    break;
                case 8:
                    _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mSrc );
                    break;
                case 16:
                    _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mSrc );
                    break;
                default:
                    break;
            }

            pDst += i_dst;
        }
    }
    else if ( !bAboveAvail && bLeftAvail )
    {
        for ( y = 0; y<iBlkHeight; y++ )
        {

            mTmp = _mm_set1_epi8( pSrc[-1 - y] );

            switch ( iBlkWidth )
            {
                case 4:
                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    _mm_maskmoveu_si128( mTmp, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                    break;
                case 8:
                    _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mTmp );
                    break;
                case 16:
                    _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mTmp );
                    break;
                default:
                    break;
            }

            pDst += i_dst;
        }
    }
    else if ( bAboveAvail && bLeftAvail )
    {
        for ( y = 0; y<iBlkHeight; y++ )
        {

            switch ( iBlkWidth )
            {
                case 4:

                    mSrc = _mm_loadu_si128( ( __m128i* )( pSrc + 1 ) );
                    mTmp = _mm_set1_epi8( pSrc[-1 - y] );

                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    mSrc = _mm_cvtepu8_epi16( mSrc );
                    mTmp = _mm_cvtepu8_epi16( mTmp );

                    mVal = _mm_add_epi16( mSrc, mTmp );

                    mVal = _mm_srai_epi16( mVal, shift );

                    mVal = _mm_packus_epi16( mVal, mVal );

                    _mm_maskmoveu_si128( mVal, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                    break;
                case 8:

                    mSrc = _mm_loadu_si128( ( __m128i* )( pSrc + 1 ) );
                    mTmp = _mm_set1_epi8( pSrc[-1 - y] );

                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    mSrc = _mm_cvtepu8_epi16( mSrc );
                    mTmp = _mm_cvtepu8_epi16( mTmp );

                    mVal = _mm_add_epi16( mSrc, mTmp );

                    mVal = _mm_srai_epi16( mVal, shift );

                    mVal = _mm_packus_epi16( mVal, mVal );

                    _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );
                    break;
                case 16:

                    // first time
                    mSrc = _mm_loadu_si128( ( __m128i* )( pSrc + 1 ) );
                    mTmp = _mm_set1_epi8( pSrc[-1 - y] );

                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    mSrc = _mm_cvtepu8_epi16( mSrc );
                    mTmp = _mm_cvtepu8_epi16( mTmp );

                    mVal = _mm_add_epi16( mSrc, mTmp );

                    mVal = _mm_srai_epi16( mVal, shift );

                    mVal = _mm_packus_epi16( mVal, mVal );

                    _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );

                    // second time
                    mSrc = _mm_loadu_si128( ( __m128i* )( pSrc + 1 + 8 ) ); //!!!
                    mTmp = _mm_set1_epi8( pSrc[-1 - y] );

                    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                    mSrc = _mm_cvtepu8_epi16( mSrc );
                    mTmp = _mm_cvtepu8_epi16( mTmp );

                    mVal = _mm_add_epi16( mSrc, mTmp );

                    mVal = _mm_srai_epi16( mVal, shift );

                    mVal = _mm_packus_epi16( mVal, mVal );

                    _mm_storel_epi64( ( __m128i* )&pDst[8], mVal );

                    break;
                default:
                    break;
            }

            pDst += i_dst;
        }
    }
}

void xPredIntraVer_sse128( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight )
{
    int y;

    __m128i mSrc, mask;

    for ( y = 0; y<iBlkHeight; y++ )
    {

        mSrc = _mm_loadu_si128( ( __m128i* )( pSrc + 1 ) );

        switch ( iBlkWidth )
        {
            case 4:
                mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                _mm_maskmoveu_si128( mSrc, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                break;
            case 8:
                _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mSrc );
                break;
            case 16:
                _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mSrc );
                break;
            default:
                break;
        }

        pDst += i_dst;
    }
}

void xPredIntraHor_sse128( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight )
{
    int y;

    __m128i mTmp, mask;

    for ( y = 0; y<iBlkHeight; y++ )
    {

        mTmp = _mm_set1_epi8( pSrc[-1 - y] );

        switch ( iBlkWidth )
        {
            case 4:
                mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                _mm_maskmoveu_si128( mTmp, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                break;
            case 8:
                _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mTmp );
                break;
            case 16:
                _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mTmp );
                break;
            default:
                break;
        }

        pDst += i_dst;
    }
}

void xPredIntraDownRight_sse128( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight )
{
    int y, i;

    __m128i mTmp, mask;

    for ( y = 0; y<iBlkHeight; y++ )
    {

        mTmp = _mm_loadu_si128( ( __m128i* )( pSrc - y ) );

        switch ( iBlkWidth )
        {
            case 4:
                mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                _mm_maskmoveu_si128( mTmp, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                break;
            case 8:
                _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mTmp );
                break;
            case 16:
                _mm_storeu_si128( ( __m128i* )&pDst[INIT_POS_ZERO], mTmp );
                break;
            default:
                i = 0;
                break;
        }

        pDst += i_dst;
    }
}

void xPredIntraDownLeft_sse128( uchar_t *pSrc, pel_t *pDst, int i_dst, int iBlkWidth, int iBlkHeight )
{
    int y;
    int shift;

    __m128i mask, mSrc, mVal, mTmp;

    shift = 1;

    for ( y = 0; y<iBlkHeight; y++ )
    {

        switch ( iBlkWidth )
        {
            case 4:
                // no reverse func
                mTmp = _mm_setr_epi8( pSrc[-2 - y], pSrc[-2 - 1 - y], pSrc[-2 - 2 - y], pSrc[-2 - 3 - y], \
                                      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

                mSrc = _mm_loadu_si128( ( __m128i* )( pSrc + 2 + y ) );

                mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                mSrc = _mm_cvtepu8_epi16( mSrc );
                mTmp = _mm_cvtepu8_epi16( mTmp );

                mVal = _mm_add_epi16( mSrc, mTmp );

                mVal = _mm_srai_epi16( mVal, shift );

                mVal = _mm_packus_epi16( mVal, mVal );

                _mm_maskmoveu_si128( mVal, mask, ( char_t* )&pDst[INIT_POS_ZERO] );
                break;
            case 8:

                mTmp = _mm_setr_epi8( pSrc[-2 - y], pSrc[-2 - 1 - y], pSrc[-2 - 2 - y], pSrc[-2 - 3 - y], \
                                      pSrc[-2 - 4 - y], pSrc[-2 - 5 - y], pSrc[-2 - 6 - y], pSrc[-2 - 7 - y], \
                                      0, 0, 0, 0, 0, 0, 0, 0 );

                mSrc = _mm_loadu_si128( ( __m128i* )( pSrc + 2 + y ) );

                mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                mSrc = _mm_cvtepu8_epi16( mSrc );
                mTmp = _mm_cvtepu8_epi16( mTmp );

                mVal = _mm_add_epi16( mSrc, mTmp );

                mVal = _mm_srai_epi16( mVal, shift );

                mVal = _mm_packus_epi16( mVal, mVal );

                _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );
                break;
            case 16:

                mTmp = _mm_setr_epi8( pSrc[-2 - y], pSrc[-2 - 1 - y], pSrc[-2 - 2 - y], pSrc[-2 - 3 - y], \
                                      pSrc[-2 - 4 - y], pSrc[-2 - 5 - y], pSrc[-2 - 6 - y], pSrc[-2 - 7 - y], \
                                      0, 0, 0, 0, 0, 0, 0, 0 );

                // first time
                mSrc = _mm_loadu_si128( ( __m128i* )( pSrc + 2 + y ) );

                mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                mSrc = _mm_cvtepu8_epi16( mSrc );
                mTmp = _mm_cvtepu8_epi16( mTmp );

                mVal = _mm_add_epi16( mSrc, mTmp );

                mVal = _mm_srai_epi16( mVal, shift );

                mVal = _mm_packus_epi16( mVal, mVal );

                _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );

                // second time
                mTmp = _mm_setr_epi8( pSrc[-2 - 8 - y], pSrc[-2 - 9 - y], pSrc[-2 - 10 - y], pSrc[-2 - 11 - y], \
                                      pSrc[-2 - 12 - y], pSrc[-2 - 13 - y], pSrc[-2 - 14 - y], pSrc[-2 - 15 - y], \
                                      0, 0, 0, 0, 0, 0, 0, 0 );

                mSrc = _mm_loadu_si128( ( __m128i* )( pSrc + 2 + y + 8 ) ); //!!!

                mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                mSrc = _mm_cvtepu8_epi16( mSrc );
                mTmp = _mm_cvtepu8_epi16( mTmp );

                mVal = _mm_add_epi16( mSrc, mTmp );

                mVal = _mm_srai_epi16( mVal, shift );

                mVal = _mm_packus_epi16( mVal, mVal );

                _mm_storel_epi64( ( __m128i* )&pDst[8], mVal );

                break;
            default:
                break;
        }

        pDst += i_dst;
    }
}

void xPredIntraChromaPlane_sse128( uchar_t *pSrc,
                                   pel_t *pDst,
                                   int i_dst, int iBlkWidth,
                                   int iBlkHeight )
{
    int y, i;
    int ih, iv, ib, ic, iaa, shift;
    int max_val = 255;
    __m128i mask, mSrc, mVal, mTmp, mib, mMaxval, mMinval;

    mMaxval = _mm_set1_epi16( max_val );
    mMinval = _mm_set1_epi8( INIT_ZERO );

    mMaxval = _mm_packus_epi16( mMaxval, mMaxval );

    ih = 0;
    iv = 0;
    shift = 5;

    for ( i = 1; i<5; i++ )
    {
        ih += i*( pSrc[1 + i + 3] - pSrc[1 - i + 3] );
        iv += i*( pSrc[-1 - i - 3] - pSrc[-1 + i - 3] );
    }

    ib = ( 17 * ih + 16 ) >> 5;
    ic = ( 17 * iv + 16 ) >> 5;

    iaa = 16 * ( pSrc[1 + 7] + pSrc[-1 - 7] );

    mib = _mm_set1_epi16( ib );

    for ( y = 0; y<iBlkHeight; y++ )
    {

        switch ( iBlkWidth )
        {
            case 4:
                mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( iBlkWidth & 7 ) - 1] ) );

                mSrc = _mm_setr_epi16( - 3, 1 - 3, 2  - 3, 3 - 3, 0, 0, 0, 0 );
                mSrc = _mm_mullo_epi16( mSrc, mib );

                mTmp = _mm_set1_epi16( ( i16s_t )( iaa + ( y - 3 )*ic + 16 ) );
                mVal = _mm_add_epi16( mSrc, mTmp );
                mVal = _mm_srai_epi16( mVal, shift );

                mVal = _mm_packus_epi16( mVal, mVal );

                // the val in mVal should meet: 0 <= val <= max_val
                mVal = _mm_min_epu8( mMaxval, mVal );
                mVal = _mm_max_epu8( mMinval, mVal );

                _mm_maskmoveu_si128( mVal, mask, ( char_t * )&pDst[INIT_POS_ZERO] );
                break;
            case 8:

                mSrc = _mm_setr_epi16( - 3, 1 - 3, 2 - 3, 3 - 3, \
                                       4 - 3, 5 - 3, 6 - 3, 7 - 3 );
                mSrc = _mm_mullo_epi16( mSrc, mib );

                mTmp = _mm_set1_epi16( ( i16s_t )( iaa + ( y - 3 )*ic + 16 ) );
                mVal = _mm_add_epi16( mSrc, mTmp );
                mVal = _mm_srai_epi16( mVal, shift );

                mVal = _mm_packus_epi16( mVal, mVal );

                // the val in mVal should meet: 0 <= val <= max_val
                mVal = _mm_min_epu8( mMaxval, mVal );
                mVal = _mm_max_epu8( mMinval, mVal );

                _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );
                break;
            case 16:

                // first time
                mSrc = _mm_setr_epi16( - 3, 1 - 3, 2 - 3, 3 - 3, \
                                       4 - 3, 5 - 3, 6 - 3, 7 - 3 );
                mSrc = _mm_mullo_epi16( mSrc, mib );

                mTmp = _mm_set1_epi16( ( i16s_t )( iaa + ( y - 3 )*ic + 16 ) );
                mVal = _mm_add_epi16( mSrc, mTmp );
                mVal = _mm_srai_epi16( mVal, shift );

                mVal = _mm_packus_epi16( mVal, mVal );

                // the val in mVal should meet: 0 <= val <= max_val
                mVal = _mm_min_epu8( mMaxval, mVal );
                mVal = _mm_max_epu8( mMinval, mVal );

                _mm_storel_epi64( ( __m128i* )&pDst[INIT_POS_ZERO], mVal );

                // second time
                mSrc = _mm_setr_epi16( 8 - 3, 9 - 3, 10 - 3, 11 - 3, \
                                       12 - 3, 13 - 3, 14 - 3, 15 - 3 );
                mSrc = _mm_mullo_epi16( mSrc, mib );

                mTmp = _mm_set1_epi16( ( i16s_t )( iaa + ( y - 3 )*ic + 16 ) );
                mVal = _mm_add_epi16( mSrc, mTmp );
                mVal = _mm_srai_epi16( mVal, shift );

                mVal = _mm_packus_epi16( mVal, mVal );

                // the val in mVal should meet: 0 <= val <= max_val
                mVal = _mm_min_epu8( mMaxval, mVal );
                mVal = _mm_max_epu8( mMinval, mVal );

                _mm_storel_epi64( ( __m128i* )&pDst[8], mVal );

                break;
            default:
                break;
        }

        pDst += i_dst;
    }
}

#endif