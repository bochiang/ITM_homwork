/*****************************************************************************
* Authors: Ronggang Wang <rgwang@pkusz.edu.cn>
*          Zhenyu Wang <wangzhenyu@pkusz.edu.cn>
*          Kui Fan <kuifan@pku.edu.cn>
*          Shenghao Zhang <1219759986@qq.com>
* Affiliate: Peking University Shenzhen Graduate School
*****************************************************************************/

#include "intrinsic.h"

#if COMPILE_FOR_8BIT

// ------------------ copy --------------------
void com_if_filter_cpy_sse128( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height )
{
    int row;
    for ( row = 0; row < height; row++ )
    {
        *( i64u_t* )dst = *( i64u_t* )src; // because width and height are all constant:8
        src += i_src;
        dst += i_dst;
    }
}

// ------------------ hor --------------------
void com_if_filter_hor_4_sse128( const pel_t *src,
                                 int i_src,
                                 pel_t *dst,
                                 int i_dst,
                                 int width,
                                 int height,
                                 char_t const *coeff,
                                 int max_val )
{
    int row, col;
    const int offset = FILTER_HORORVER_OFFSET;
    const int shift  = FILTER_HORORVER_SHIFT;

    //__m128i mMaxval = _mm_set1_epi16(max_val);
    //__m128i mMinval = _mm_set1_epi8(INIT_ZERO);

    __m128i mAddOffset = _mm_set1_epi16( offset );

    __m128i mSwitch1 = _mm_setr_epi8( 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6 );
    __m128i mSwitch2 = _mm_setr_epi8( 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10 );
    __m128i mCoef = _mm_set1_epi32( *( i32s_t* )coeff );
    __m128i mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );

    // set the limit val
    //mMaxval = _mm_packus_epi16(mMaxval, mMaxval);
    //mMinval = _mm_packus_epi16(mMinval, mMinval);

    src -= 1;

    for ( row = 0; row < height; row++ )
    {
        __m128i mT20, mT40, mSum, mVal;

        for ( col = 0; col < width - 7; col += 8 )
        {

            __m128i mSrc = _mm_loadu_si128( ( __m128i* )( src + col ) );

            mT20 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mCoef );
            mT40 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mCoef );

            mSum = _mm_hadd_epi16( mT20, mT40 );
            mVal = _mm_add_epi16( mSum, mAddOffset );

            mVal = _mm_srai_epi16( mVal, shift );
            mVal = _mm_packus_epi16( mVal, mVal );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, mVal);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_storel_epi64( ( __m128i* )&dst[col], mVal );
        }

        if ( col < width )
        {

            __m128i mSrc = _mm_loadu_si128( ( __m128i* )( src + col ) );

            __m128i mT20 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mCoef );
            __m128i mT40 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mCoef );

            __m128i mSum = _mm_hadd_epi16( mT20, mT40 );
            __m128i mVal = _mm_add_epi16( mSum, mAddOffset );

            mVal = _mm_srai_epi16( mVal, shift );
            mVal = _mm_packus_epi16( mVal, mVal );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, mVal);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_maskmoveu_si128( mVal, mask, ( char_t* )&dst[col] );
        }

        src += i_src;
        dst += i_dst;
    }
}

void com_if_filter_hor_6_sse128( const pel_t *src,
                                 int i_src,
                                 pel_t *dst,
                                 int i_dst,
                                 int width,
                                 int height,
                                 char_t const *coeff,
                                 int max_val )
{
    int row, col=0, i=0;
    const int offset = FILTER_HORORVER_OFFSET;
    const int shift  = FILTER_HORORVER_SHIFT;
    char_t coeff_temp[8] = {INIT_ZERO};

    __m128i mAddOffset = _mm_set1_epi16( offset );
    //__m128i mMaxval  = _mm_set1_epi16(max_val);
    //__m128i mMinval  = _mm_set1_epi8(INIT_ZERO);

    __m128i mSwitch1 = _mm_setr_epi8( 0, 1, 2, 3, 4, 5, -1, -1, 1, 2, 3, 4, 5, 6, -1, -1 );
    __m128i mSwitch2 = _mm_setr_epi8( 2, 3, 4, 5, 6, 7, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1 );
    __m128i mSwitch3 = _mm_setr_epi8( 4, 5, 6, 7, 8, 9, -1, -1, 5, 6, 7, 8, 9, 10, -1, -1 );
    __m128i mSwitch4 = _mm_setr_epi8( 6, 7, 8, 9, 10, 11, -1, -1, 7, 8, 9, 10, 11, 12, -1, -1 );
    __m128i mCoef;
    __m128i mask;

    // extend the coeff[6] to coeff_temp[8]
    for( i = 0; i<6; i++ )
    {
        coeff_temp[i] = coeff[i];
    }

    mCoef = _mm_loadl_epi64( ( __m128i* )coeff_temp );
    mask  = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );
    mCoef = _mm_unpacklo_epi64( mCoef, mCoef );

    // set the limiting val
    //mMaxval = _mm_packus_epi16(mMaxval, mMaxval);

    src -= 2;
    for ( row = 0; row < height; row++ )
    {
        __m128i T20, T40, T60, T80, s1, s2, sum, val;

        for ( col = 0; col < width - 7; col += 8 )
        {
            __m128i srcCoeff = _mm_loadu_si128( ( __m128i* )( src + col ) );

            T20 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch1 ), mCoef );
            T40 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch2 ), mCoef );
            T60 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch3 ), mCoef );
            T80 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch4 ), mCoef );

            s1 = _mm_hadd_epi16( T20, T40 );
            s2 = _mm_hadd_epi16( T60, T80 );
            sum = _mm_hadd_epi16( s1, s2 );

            val = _mm_add_epi16( sum, mAddOffset );

            val = _mm_srai_epi16( val, shift );
            val = _mm_packus_epi16( val, val );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, val);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_storel_epi64( ( __m128i* )&dst[col], val );
        }

        if ( col < width )
        {
            __m128i srcCoeff = _mm_loadu_si128( ( __m128i* )( src + col ) );

            __m128i T20 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch1 ), mCoef );
            __m128i T40 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch2 ), mCoef );
            __m128i T60 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch3 ), mCoef );
            __m128i T80 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch4 ), mCoef );

            __m128i s1 = _mm_hadd_epi16( T20, T40 );
            __m128i s2 = _mm_hadd_epi16( T60, T80 );
            __m128i sum = _mm_hadd_epi16( s1, s2 );

            __m128i val = _mm_add_epi16( sum, mAddOffset );

            val = _mm_srai_epi16( val, shift );
            val = _mm_packus_epi16( val, val );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, val);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_maskmoveu_si128( val, mask, ( char_t* )&dst[col] );
        }

        src += i_src;
        dst += i_dst;
    }
}

void com_if_filter_hor_10_sse128( const pel_t *src,
                                  int i_src,
                                  pel_t *dst,
                                  int i_dst,
                                  int width,
                                  int height,
                                  char_t const *coeff,
                                  int max_val )
{
    int row, col=0, i=0;
    const int offset = FILTER_HORORVER_OFFSET;
    const int shift = FILTER_HORORVER_SHIFT;
    char_t coeff_temp[16] = {INIT_ZERO};

    __m128i mAddOffset = _mm_set1_epi16( offset );
    //__m128i mMaxval    = _mm_set1_epi16(max_val);
    //__m128i mMinval    = _mm_set1_epi8(INIT_ZERO);

    __m128i mSwitch1 = _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1 );
    __m128i mSwitch2 = _mm_setr_epi8( 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, -1, -1, -1, -1, -1, -1 );
    __m128i mSwitch3 = _mm_setr_epi8( 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, -1, -1, -1, -1, -1, -1 );
    __m128i mSwitch4 = _mm_setr_epi8( 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, -1, -1, -1, -1, -1, -1 );
    __m128i mSwitch5 = _mm_setr_epi8( 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -1, -1, -1, -1, -1, -1 );
    __m128i mSwitch6 = _mm_setr_epi8( 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, -1, -1, -1, -1, -1, -1 );
    __m128i mSwitch7 = _mm_setr_epi8( 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1 );
    // change the srcCoeff for "16" can't appear in _mm_shuffle_epi8 function
    __m128i mSwitch8 = _mm_setr_epi8( 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1 );

    __m128i mCoef;
    __m128i mask;

    // extend the coeff[10] to coeff_temp[16]
    for( i = 0; i<10; i++ )
    {
        coeff_temp[i] = coeff[i];
    }

    mCoef = _mm_loadu_si128( ( __m128i* )coeff_temp );
    mask  = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );

    // set the limiting val
    //mMaxval = _mm_packus_epi16(mMaxval, mMaxval);

    src -= 4;
    for ( row = 0; row < height; row++ )
    {
        __m128i T20, T40, T60, T80, T100, T120, T140, T160, s1, s2, s3, s4, sum1, sum2, sum, val;

        for ( col = 0; col < width - 7; col += 8 )
        {
            __m128i srcCoeff = _mm_loadu_si128( ( __m128i* )( src + col ) );
            __m128i srcCoeff_for_8thshuffle = _mm_loadu_si128( ( __m128i* )( src + col + 1 ) );

            T20  = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch1 ), mCoef );
            T40  = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch2 ), mCoef );
            T60  = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch3 ), mCoef );
            T80  = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch4 ), mCoef );
            T100 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch5 ), mCoef );
            T120 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch6 ), mCoef );
            T140 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch7 ), mCoef );
            T160 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff_for_8thshuffle, mSwitch8 ), mCoef );

            s1 = _mm_hadd_epi16( T20, T40 );
            s2 = _mm_hadd_epi16( T60, T80 );
            s3 = _mm_hadd_epi16( T100, T120 );
            s4 = _mm_hadd_epi16( T140, T160 );

            sum1 = _mm_hadd_epi16( s1, s2 );
            sum2 = _mm_hadd_epi16( s3, s4 );

            sum = _mm_hadd_epi16( sum1, sum2 );

            val = _mm_add_epi16( sum, mAddOffset );
            val = _mm_srai_epi16( val, shift );
            val = _mm_packus_epi16( val, val );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, val);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_storel_epi64( ( __m128i* )&dst[col], val );
        }

        if ( col < width )
        {
            __m128i srcCoeff = _mm_loadu_si128( ( __m128i* )( src + col ) );
            __m128i srcCoeff_for_8thshuffle = _mm_loadu_si128( ( __m128i* )( src + col + 1 ) );

            T20  = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch1 ), mCoef );
            T40  = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch2 ), mCoef );
            T60  = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch3 ), mCoef );
            T80  = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch4 ), mCoef );
            T100 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch5 ), mCoef );
            T120 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch6 ), mCoef );
            T140 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff, mSwitch7 ), mCoef );
            T160 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff_for_8thshuffle, mSwitch8 ), mCoef );

            s1 = _mm_hadd_epi16( T20, T40 );
            s2 = _mm_hadd_epi16( T60, T80 );
            s3 = _mm_hadd_epi16( T100, T120 );
            s4 = _mm_hadd_epi16( T140, T160 );

            sum1 = _mm_hadd_epi16( s1, s2 );
            sum2 = _mm_hadd_epi16( s3, s4 );

            sum = _mm_hadd_epi16( sum1, sum2 );

            val = _mm_add_epi16( sum, mAddOffset );
            val = _mm_srai_epi16( val, shift );
            val = _mm_packus_epi16( val, val );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, val);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_maskmoveu_si128( val, mask, ( char_t* )&dst[col] );
        }

        src += i_src;
        dst += i_dst;
    }
}

// ------------------ ver --------------------
void com_if_filter_ver_4_sse128( const pel_t *src,
                                 int i_src,
                                 pel_t *dst,
                                 int i_dst,
                                 int width,
                                 int height,
                                 char_t const *coeff,
                                 int max_val )
{
    int row, col;
    pel_t const *p;
    const int offset = FILTER_HORORVER_OFFSET;
    const int shift  = FILTER_HORORVER_SHIFT;

    __m128i mAddOffset, mask, coeff0, coeff1, mVal;

    mAddOffset = _mm_set1_epi16( offset );
    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );
    coeff0  = _mm_set1_epi16( *( i32s_t* )coeff );
    coeff1  = _mm_set1_epi16( *( i32s_t* )( coeff + 2 ) );
    //mMaxval = _mm_set1_epi16(max_val);
    //mMinval = _mm_set1_epi8(INIT_ZERO);

    // set the limiting val
    //mMaxval = _mm_packus_epi16(mMaxval, mMaxval);

    src -= i_src;

    for ( row = 0; row < height; row++ )
    {
        p = src;
        for ( col = 0; col < width - 7; col += 8 )
        {
            __m128i T00, T10, T20, T30;
            T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            T10 = _mm_loadu_si128( ( __m128i* )( p + i_src ) );
            T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * i_src ) );
            T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * i_src ) );

            T00 = _mm_unpacklo_epi8( T00, T10 );
            T10 = _mm_unpacklo_epi8( T20, T30 );

            T00 = _mm_maddubs_epi16( T00, coeff0 );
            T10 = _mm_maddubs_epi16( T10, coeff1 );

            mVal = _mm_add_epi16( T00, T10 );

            mVal = _mm_add_epi16( mVal, mAddOffset );
            mVal = _mm_srai_epi16( mVal, shift );
            mVal = _mm_packus_epi16( mVal, mVal );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, mVal);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_storel_epi64( ( __m128i* )&dst[col], mVal );

            p += 8;
        }

        if ( col < width )
        {
            __m128i T00, T10, T20, T30;
            T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            T10 = _mm_loadu_si128( ( __m128i* )( p + i_src ) );
            T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * i_src ) );
            T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * i_src ) );

            T00 = _mm_unpacklo_epi8( T00, T10 );
            T10 = _mm_unpacklo_epi8( T20, T30 );

            T00 = _mm_maddubs_epi16( T00, coeff0 );
            T10 = _mm_maddubs_epi16( T10, coeff1 );

            mVal = _mm_add_epi16( T00, T10 );

            mVal = _mm_add_epi16( mVal, mAddOffset );
            mVal = _mm_srai_epi16( mVal, shift );
            mVal = _mm_packus_epi16( mVal, mVal );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, mVal);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_maskmoveu_si128( mVal, mask, ( char_t* )&dst[col] );
        }
        src += i_src;
        dst += i_dst;
    }
}

void com_if_filter_ver_6_sse128( const pel_t *src,
                                 int i_src,
                                 pel_t *dst,
                                 int i_dst,
                                 int width,
                                 int height,
                                 char_t const *coeff,
                                 int max_val )
{
    int row, col;
    pel_t const *p;
    const int offset = FILTER_HORORVER_OFFSET;
    const int shift  = FILTER_HORORVER_SHIFT;

    __m128i mAddOffset, mask, coeff0, coeff1, coeff2, mVal;

    mAddOffset = _mm_set1_epi16( offset );
    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );
    coeff0  = _mm_set1_epi16( *( i32s_t* )coeff );
    coeff1  = _mm_set1_epi16( *( i32s_t* )( coeff + 2 ) );
    coeff2  = _mm_set1_epi16( *( i32s_t* )( coeff + 4 ) );
    //mMaxval = _mm_set1_epi16(max_val);
    //mMinval = _mm_set1_epi8(INIT_ZERO);

    // set the limiting val
    //mMaxval = _mm_packus_epi16(mMaxval, mMaxval);

    src -= 2 * i_src;

    for ( row = 0; row < height; row++ )
    {
        p = src;
        for ( col = 0; col < width - 7; col += 8 )
        {
            __m128i T00, T10, T20, T30, T40, T50;

            T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            T10 = _mm_loadu_si128( ( __m128i* )( p + i_src ) );
            T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * i_src ) );
            T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * i_src ) );
            T40 = _mm_loadu_si128( ( __m128i* )( p + 4 * i_src ) );
            T50 = _mm_loadu_si128( ( __m128i* )( p + 5 * i_src ) );

            T00 = _mm_unpacklo_epi8( T00, T10 );
            T10 = _mm_unpacklo_epi8( T20, T30 );
            T20 = _mm_unpacklo_epi8( T40, T50 );

            T00 = _mm_maddubs_epi16( T00, coeff0 );
            T10 = _mm_maddubs_epi16( T10, coeff1 );
            T20 = _mm_maddubs_epi16( T20, coeff2 );

            mVal = _mm_add_epi16( T00, T10 );
            mVal = _mm_add_epi16( mVal, T20 );

            mVal = _mm_add_epi16( mVal, mAddOffset );
            mVal = _mm_srai_epi16( mVal, shift );
            mVal = _mm_packus_epi16( mVal, mVal );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, mVal);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_storel_epi64( ( __m128i* )&dst[col], mVal );

            p += 8;
        }

        if ( col < width )
        {
            __m128i T00, T10, T20, T30, T40, T50;

            T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            T10 = _mm_loadu_si128( ( __m128i* )( p + i_src ) );
            T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * i_src ) );
            T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * i_src ) );
            T40 = _mm_loadu_si128( ( __m128i* )( p + 4 * i_src ) );
            T50 = _mm_loadu_si128( ( __m128i* )( p + 5 * i_src ) );

            T00 = _mm_unpacklo_epi8( T00, T10 );
            T10 = _mm_unpacklo_epi8( T20, T30 );
            T20 = _mm_unpacklo_epi8( T40, T50 );

            T00 = _mm_maddubs_epi16( T00, coeff0 );
            T10 = _mm_maddubs_epi16( T10, coeff1 );
            T20 = _mm_maddubs_epi16( T20, coeff2 );

            mVal = _mm_add_epi16( T00, T10 );
            mVal = _mm_add_epi16( mVal, T20 );

            mVal = _mm_add_epi16( mVal, mAddOffset );
            mVal = _mm_srai_epi16( mVal, shift );
            mVal = _mm_packus_epi16( mVal, mVal );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, mVal);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_maskmoveu_si128( mVal, mask, ( char_t* )&dst[col] );
        }

        src += i_src;
        dst += i_dst;
    }
}

void com_if_filter_ver_10_sse128( const pel_t *src,
                                  int i_src,
                                  pel_t *dst,
                                  int i_dst,
                                  int width,
                                  int height,
                                  char_t const *coeff,
                                  int max_val )
{
    int row, col;
    pel_t const *p;
    const int offset = FILTER_HORORVER_OFFSET;
    const int shift  = FILTER_HORORVER_SHIFT;

    __m128i mAddOffset, mask, coeff0, coeff1, coeff2, coeff3, coeff4, mVal;

    mAddOffset = _mm_set1_epi16( offset );
    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );
    coeff0  = _mm_set1_epi16( *( i32s_t* )coeff );
    coeff1  = _mm_set1_epi16( *( i32s_t* )( coeff + 2 ) );
    coeff2  = _mm_set1_epi16( *( i32s_t* )( coeff + 4 ) );
    coeff3  = _mm_set1_epi16( *( i32s_t* )( coeff + 6 ) );
    coeff4  = _mm_set1_epi16( *( i32s_t* )( coeff + 8 ) );
    //mMaxval = _mm_set1_epi16(max_val);
    //mMinval = _mm_set1_epi8(0);

    // set the limiting val
    //mMaxval = _mm_packus_epi16(mMaxval, mMaxval);

    src -= 4 * i_src;

    for ( row = 0; row < height; row++ )
    {
        p = src;
        for ( col = 0; col < width - 7; col += 8 )
        {
            __m128i T00, T10, T20, T30, T40, T50, T60, T70, T80, T90;
            T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            T10 = _mm_loadu_si128( ( __m128i* )( p + i_src ) );
            T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * i_src ) );
            T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * i_src ) );
            T40 = _mm_loadu_si128( ( __m128i* )( p + 4 * i_src ) );
            T50 = _mm_loadu_si128( ( __m128i* )( p + 5 * i_src ) );
            T60 = _mm_loadu_si128( ( __m128i* )( p + 6 * i_src ) );
            T70 = _mm_loadu_si128( ( __m128i* )( p + 7 * i_src ) );
            T80 = _mm_loadu_si128( ( __m128i* )( p + 8 * i_src ) );
            T90 = _mm_loadu_si128( ( __m128i* )( p + 9 * i_src ) );

            T00 = _mm_unpacklo_epi8( T00, T10 );
            T10 = _mm_unpacklo_epi8( T20, T30 );
            T20 = _mm_unpacklo_epi8( T40, T50 );
            T30 = _mm_unpacklo_epi8( T60, T70 );
            T40 = _mm_unpacklo_epi8( T80, T90 );

            T00 = _mm_maddubs_epi16( T00, coeff0 );
            T10 = _mm_maddubs_epi16( T10, coeff1 );
            T20 = _mm_maddubs_epi16( T20, coeff2 );
            T30 = _mm_maddubs_epi16( T30, coeff3 );
            T40 = _mm_maddubs_epi16( T40, coeff4 );

            mVal = _mm_add_epi16( T00, T10 );
            mVal = _mm_add_epi16( mVal, T20 );
            mVal = _mm_add_epi16( mVal, T30 );
            mVal = _mm_add_epi16( mVal, T40 );

            mVal = _mm_add_epi16( mVal, mAddOffset );
            mVal = _mm_srai_epi16( mVal, shift );
            mVal = _mm_packus_epi16( mVal, mVal );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, mVal);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_storel_epi64( ( __m128i* )&dst[col], mVal );

            p += 8;
        }

        if ( col < width )
        {
            __m128i T00, T10, T20, T30, T40, T50, T60, T70, T80, T90;
            T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            T10 = _mm_loadu_si128( ( __m128i* )( p + i_src ) );
            T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * i_src ) );
            T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * i_src ) );
            T40 = _mm_loadu_si128( ( __m128i* )( p + 4 * i_src ) );
            T50 = _mm_loadu_si128( ( __m128i* )( p + 5 * i_src ) );
            T60 = _mm_loadu_si128( ( __m128i* )( p + 6 * i_src ) );
            T70 = _mm_loadu_si128( ( __m128i* )( p + 7 * i_src ) );
            T80 = _mm_loadu_si128( ( __m128i* )( p + 8 * i_src ) );
            T90 = _mm_loadu_si128( ( __m128i* )( p + 9 * i_src ) );

            T00 = _mm_unpacklo_epi8( T00, T10 );
            T10 = _mm_unpacklo_epi8( T20, T30 );
            T20 = _mm_unpacklo_epi8( T40, T50 );
            T30 = _mm_unpacklo_epi8( T60, T70 );
            T40 = _mm_unpacklo_epi8( T80, T90 );

            T00 = _mm_maddubs_epi16( T00, coeff0 );
            T10 = _mm_maddubs_epi16( T10, coeff1 );
            T20 = _mm_maddubs_epi16( T20, coeff2 );
            T30 = _mm_maddubs_epi16( T30, coeff3 );
            T40 = _mm_maddubs_epi16( T40, coeff4 );

            mVal = _mm_add_epi16( T00, T10 );
            mVal = _mm_add_epi16( mVal, T20 );
            mVal = _mm_add_epi16( mVal, T30 );
            mVal = _mm_add_epi16( mVal, T40 );

            mVal = _mm_add_epi16( mVal, mAddOffset );
            mVal = _mm_srai_epi16( mVal, shift );
            mVal = _mm_packus_epi16( mVal, mVal );

            // the val in mVal should meet: 0 <= val <= max_val
            //mRval = _mm_min_epu8(mMaxval, mVal);
            //mRval = _mm_max_epu8(mMinval, mRval);

            _mm_maskmoveu_si128( mVal, mask, ( char_t* )&dst[col] );
        }

        src += i_src;
        dst += i_dst;
    }
}

// ------------------ ver & hor --------------------
void com_if_filter_hor_ver_4_sse128( const pel_t *src,
                                     int i_src,
                                     pel_t *dst,
                                     int i_dst,
                                     int width,
                                     int height,
                                     const char_t *coeff_h,
                                     const char_t *coeff_v,
                                     int max_val )
{
    int row, col;
    int shift;
    i16s_t const *p;
    i16s_t *tmp;

    ALIGNED_16( i16s_t tmp_res[( HOR_VER_4_TMPWIDTH + FILTER_TAPNUM_4 - 1 ) * HOR_VER_4_TMPWIDTH] );

    __m128i mSwitch1, mSwitch2, mCoefx, mask, mAddOffset, coeff0, coeff1, mVal1, mVal2, mVal;

    mSwitch1 = _mm_setr_epi8( 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6 );
    mSwitch2 = _mm_setr_epi8( 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10 );
    mCoefx   = _mm_set1_epi32( *( i32s_t* )coeff_h );
    mask     = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );

    tmp = tmp_res;

    // hor
    src = src - 1 * i_src - 1;

    if ( width > 4 )
    {
        for ( row = -1; row < height + 2; row++ )
        {
            __m128i mT0, mT1, mVal;

            for ( col = 0; col < width; col += 8 )
            {
                __m128i mSrc = _mm_loadu_si128( ( __m128i* )( src + col ) );
                mT0 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mCoefx );
                mT1 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mCoefx );

                mVal = _mm_hadd_epi16( mT0, mT1 );
                _mm_store_si128( ( __m128i* )&tmp[col], mVal );
            }

            src += i_src;
            tmp += HOR_VER_4_TMPWIDTH;
        }
    }
    else
    {
        for ( row = -1; row < height + 2; row++ )
        {
            __m128i mSrc = _mm_loadu_si128( ( __m128i* )src );
            __m128i mT0  = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mCoefx );
            __m128i mVal = _mm_hadd_epi16( mT0, mT0 );
            _mm_storel_epi64( ( __m128i* )tmp, mVal );

            src += i_src;
            tmp += HOR_VER_4_TMPWIDTH;
        }
    }

    // ver
    shift      = FILTER_HORANDVER_SHIFT;
    mAddOffset = _mm_set1_epi32( FILTER_HORANDVER_OFFSET );

    tmp = tmp_res;

    coeff0 = _mm_set1_epi16( *( i16s_t* )coeff_v );
    coeff1 = _mm_set1_epi16( *( i16s_t* )( coeff_v + 2 ) );

    coeff0 = _mm_cvtepi8_epi16( coeff0 );
    coeff1 = _mm_cvtepi8_epi16( coeff1 );

    for ( row = 0; row < height; row++ )
    {
        p = tmp;

        for ( col = 0; col < width - 7; col += 8 )
        {
            __m128i T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            __m128i T10 = _mm_loadu_si128( ( __m128i* )( p + HOR_VER_4_TMPWIDTH ) );
            __m128i T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * HOR_VER_4_TMPWIDTH ) );
            __m128i T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * HOR_VER_4_TMPWIDTH ) );

            __m128i M0  = _mm_unpacklo_epi16( T00, T10 );
            __m128i M1  = _mm_unpacklo_epi16( T20, T30 );
            __m128i M2  = _mm_unpackhi_epi16( T00, T10 );
            __m128i M3  = _mm_unpackhi_epi16( T20, T30 );

            M0 = _mm_madd_epi16( M0, coeff0 );
            M1 = _mm_madd_epi16( M1, coeff1 );
            M2 = _mm_madd_epi16( M2, coeff0 );
            M3 = _mm_madd_epi16( M3, coeff1 );

            mVal1 = _mm_add_epi32( M0, M1 );
            mVal2 = _mm_add_epi32( M2, M3 );

            mVal1 = _mm_add_epi32( mVal1, mAddOffset );
            mVal2 = _mm_add_epi32( mVal2, mAddOffset );
            mVal1 = _mm_srai_epi32( mVal1, shift );
            mVal2 = _mm_srai_epi32( mVal2, shift );
            mVal  = _mm_packs_epi32( mVal1, mVal2 );
            mVal  = _mm_packus_epi16( mVal, mVal );

            _mm_storel_epi64( ( __m128i* )&dst[col], mVal );

            p += 8;
        }

        if ( col < width )
        {
            __m128i T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            __m128i T10 = _mm_loadu_si128( ( __m128i* )( p + HOR_VER_4_TMPWIDTH ) );
            __m128i T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * HOR_VER_4_TMPWIDTH ) );
            __m128i T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * HOR_VER_4_TMPWIDTH ) );

            __m128i M0 = _mm_unpacklo_epi16( T00, T10 );
            __m128i M1 = _mm_unpacklo_epi16( T20, T30 );
            __m128i M2 = _mm_unpackhi_epi16( T00, T10 );
            __m128i M3 = _mm_unpackhi_epi16( T20, T30 );

            M0 = _mm_madd_epi16( M0, coeff0 );
            M1 = _mm_madd_epi16( M1, coeff1 );
            M2 = _mm_madd_epi16( M2, coeff0 );
            M3 = _mm_madd_epi16( M3, coeff1 );

            mVal1 = _mm_add_epi32( M0, M1 );
            mVal2 = _mm_add_epi32( M2, M3 );

            mVal1 = _mm_add_epi32( mVal1, mAddOffset );
            mVal2 = _mm_add_epi32( mVal2, mAddOffset );
            mVal1 = _mm_srai_epi32( mVal1, shift );
            mVal2 = _mm_srai_epi32( mVal2, shift );
            mVal = _mm_packs_epi32( mVal1, mVal2 );
            mVal = _mm_packus_epi16( mVal, mVal );

            _mm_maskmoveu_si128( mVal, mask, ( char_t* )&dst[col] );
        }

        tmp += HOR_VER_4_TMPWIDTH;
        dst += i_dst;
    }
}


void com_if_filter_hor_ver_6_sse128( const pel_t *src,
                                     int i_src,
                                     pel_t *dst,
                                     int i_dst,
                                     int width,
                                     int height,
                                     const char_t *coeff_h,
                                     const char_t *coeff_v,
                                     int max_val )
{
    int row, col, i;
    int shift;
    i16s_t const *p;
    i16s_t *tmp;
    char_t coeff_temp[8] = { INIT_ZERO };

    ALIGNED_16( i16s_t tmp_res[( HOR_VER_6_TMPWIDTH + FILTER_TAPNUM_6 - 1 ) * HOR_VER_6_TMPWIDTH] );

    __m128i mSwitch1, mSwitch2, mSwitch3, mSwitch4, mCoefx, mask, mAddOffset, coeff0, coeff1, coeff2;

    mSwitch1 = _mm_setr_epi8( 0, 1, 2, 3, 4, 5, -1, -1, 1, 2, 3, 4, 5, 6, -1, -1 );
    mSwitch2 = _mm_setr_epi8( 2, 3, 4, 5, 6, 7, -1, -1, 3, 4, 5, 6, 7, 8, -1, -1 );
    mSwitch3 = _mm_setr_epi8( 4, 5, 6, 7, 8, 9, -1, -1, 5, 6, 7, 8, 9, 10, -1, -1 );
    mSwitch4 = _mm_setr_epi8( 6, 7, 8, 9, 10, 11, -1, -1, 7, 8, 9, 10, 11, 12, -1, -1 );

    // extend the coeff_h[6] to coeff_temp[8]
    for ( i = 0; i < 6; i++ )
    {
        coeff_temp[i] = coeff_h[i];
    }

    mCoefx = _mm_loadl_epi64( ( __m128i* )coeff_temp );
    mCoefx = _mm_unpacklo_epi64( mCoefx, mCoefx );
    mask   = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );

    tmp = tmp_res;

    // hor
    src = src - 2 * i_src - 2;

    if ( width > 4 )
    {
        for ( row = -2; row < height + 3; row++ )
        {
            for ( col = 0; col < width; col += 8 )
            {
                __m128i mSrc  = _mm_loadu_si128( ( __m128i* )( src + col ) );

                __m128i mT0   = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mCoefx );
                __m128i mT1   = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mCoefx );
                __m128i mT2   = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch3 ), mCoefx );
                __m128i mT3   = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch4 ), mCoefx );

                __m128i mVal1 = _mm_hadd_epi16( mT0, mT1 );
                __m128i mVal2 = _mm_hadd_epi16( mT2, mT3 );
                __m128i mVal  = _mm_hadd_epi16( mVal1, mVal2 );

                _mm_store_si128( ( __m128i* )&tmp[col], mVal );
            }

            src += i_src;
            tmp += HOR_VER_6_TMPWIDTH;
        }
    }
    else
    {
        for ( row = -2; row < height + 3; row++ )
        {
            __m128i mSrc = _mm_loadu_si128( ( __m128i* )src );

            __m128i mT0 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mCoefx );
            __m128i mT1 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mCoefx );

            __m128i mVal = _mm_hadd_epi16( mT0, mT1 );
            mVal = _mm_hadd_epi16( mVal, mVal );

            _mm_storel_epi64( ( __m128i* )tmp, mVal );

            src += i_src;
            tmp += HOR_VER_6_TMPWIDTH;
        }
    }

    // ver
    shift = FILTER_HORANDVER_SHIFT;
    mAddOffset = _mm_set1_epi32( FILTER_HORANDVER_OFFSET );

    tmp = tmp_res;

    coeff0 = _mm_set1_epi16( *( i16s_t* )coeff_v );
    coeff1 = _mm_set1_epi16( *( i16s_t* )( coeff_v + 2 ) );
    coeff2 = _mm_set1_epi16( *( i16s_t* )( coeff_v + 4 ) );

    coeff0 = _mm_cvtepi8_epi16( coeff0 );
    coeff1 = _mm_cvtepi8_epi16( coeff1 );
    coeff2 = _mm_cvtepi8_epi16( coeff2 );

    for ( row = 0; row < height; row++ )
    {
        p = tmp;

        for ( col = 0; col < width - 7; col += 8 )
        {
            __m128i mVal1, mVal2, mRVal;

            __m128i T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            __m128i T10 = _mm_loadu_si128( ( __m128i* )( p + HOR_VER_6_TMPWIDTH ) );
            __m128i T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * HOR_VER_6_TMPWIDTH ) );
            __m128i T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * HOR_VER_6_TMPWIDTH ) );
            __m128i T40 = _mm_loadu_si128( ( __m128i* )( p + 4 * HOR_VER_6_TMPWIDTH ) );
            __m128i T50 = _mm_loadu_si128( ( __m128i* )( p + 5 * HOR_VER_6_TMPWIDTH ) );

            __m128i M0 = _mm_unpacklo_epi16( T00, T10 );
            __m128i M1 = _mm_unpacklo_epi16( T20, T30 );
            __m128i M2 = _mm_unpacklo_epi16( T40, T50 );
            __m128i M3 = _mm_unpackhi_epi16( T00, T10 );
            __m128i M4 = _mm_unpackhi_epi16( T20, T30 );
            __m128i M5 = _mm_unpackhi_epi16( T40, T50 );

            M0 = _mm_madd_epi16( M0, coeff0 );
            M1 = _mm_madd_epi16( M1, coeff1 );
            M2 = _mm_madd_epi16( M2, coeff2 );
            M3 = _mm_madd_epi16( M3, coeff0 );
            M4 = _mm_madd_epi16( M4, coeff1 );
            M5 = _mm_madd_epi16( M5, coeff2 );

            mVal1 = _mm_add_epi32( M0, M1 );
            mVal1 = _mm_add_epi32( mVal1, M2 );
            mVal2 = _mm_add_epi32( M3, M4 );
            mVal2 = _mm_add_epi32( mVal2, M5 );

            mVal1 = _mm_add_epi32( mVal1, mAddOffset );
            mVal2 = _mm_add_epi32( mVal2, mAddOffset );

            mVal1 = _mm_srai_epi32( mVal1, shift );
            mVal2 = _mm_srai_epi32( mVal2, shift );

            mRVal = _mm_packs_epi32( mVal1, mVal2 );

            mRVal = _mm_packus_epi16( mRVal, mRVal );

            _mm_storel_epi64( ( __m128i* )&dst[col], mRVal );

            p += 8;
        }

        if ( col < width )
        {
            __m128i mVal1, mVal2, mRVal;

            __m128i T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            __m128i T10 = _mm_loadu_si128( ( __m128i* )( p + HOR_VER_6_TMPWIDTH ) );
            __m128i T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * HOR_VER_6_TMPWIDTH ) );
            __m128i T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * HOR_VER_6_TMPWIDTH ) );
            __m128i T40 = _mm_loadu_si128( ( __m128i* )( p + 4 * HOR_VER_6_TMPWIDTH ) );
            __m128i T50 = _mm_loadu_si128( ( __m128i* )( p + 5 * HOR_VER_6_TMPWIDTH ) );

            __m128i M0 = _mm_unpacklo_epi16( T00, T10 );
            __m128i M1 = _mm_unpacklo_epi16( T20, T30 );
            __m128i M2 = _mm_unpacklo_epi16( T40, T50 );
            __m128i M3 = _mm_unpackhi_epi16( T00, T10 );
            __m128i M4 = _mm_unpackhi_epi16( T20, T30 );
            __m128i M5 = _mm_unpackhi_epi16( T40, T50 );

            M0 = _mm_madd_epi16( M0, coeff0 );
            M1 = _mm_madd_epi16( M1, coeff1 );
            M2 = _mm_madd_epi16( M2, coeff2 );
            M3 = _mm_madd_epi16( M3, coeff0 );
            M4 = _mm_madd_epi16( M4, coeff1 );
            M5 = _mm_madd_epi16( M5, coeff2 );

            mVal1 = _mm_add_epi32( M0, M1 );
            mVal1 = _mm_add_epi32( mVal1, M2 );
            mVal2 = _mm_add_epi32( M3, M4 );
            mVal2 = _mm_add_epi32( mVal2, M5 );

            mVal1 = _mm_add_epi32( mVal1, mAddOffset );
            mVal2 = _mm_add_epi32( mVal2, mAddOffset );

            mVal1 = _mm_srai_epi32( mVal1, shift );
            mVal2 = _mm_srai_epi32( mVal2, shift );

            mRVal = _mm_packs_epi32( mVal1, mVal2 );

            mRVal = _mm_packus_epi16( mRVal, mRVal );

            _mm_maskmoveu_si128( mRVal, mask, ( char_t* )&dst[col] );
        }

        tmp += HOR_VER_6_TMPWIDTH;
        dst += i_dst;
    }
}


void com_if_filter_hor_ver_10_sse128( const pel_t *src,
                                      int i_src,
                                      pel_t *dst,
                                      int i_dst,
                                      int width,
                                      int height,
                                      const char_t *coeff_h,
                                      const char_t *coeff_v,
                                      int max_val )
{
    int row, col, i;
    int shift;
    i16s_t const *p;
    i16s_t *tmp;
    char_t coeff_temp[16] = { INIT_ZERO };

    ALIGNED_16( i16s_t tmp_res[( HOR_VER_10_TMPWIDTH + FILTER_TAPNUM_10 - 1 ) * HOR_VER_10_TMPWIDTH] );

    __m128i mSwitch1, mSwitch2, mSwitch3, mSwitch4, mSwitch5, mSwitch6, mSwitch7, mSwitch8, mCoefx, mask, mAddOffset, coeff0, coeff1, coeff2, coeff3, coeff4;

    mSwitch1 = _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1 );
    mSwitch2 = _mm_setr_epi8( 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, -1, -1, -1, -1, -1, -1 );
    mSwitch3 = _mm_setr_epi8( 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, -1, -1, -1, -1, -1, -1 );
    mSwitch4 = _mm_setr_epi8( 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, -1, -1, -1, -1, -1, -1 );
    mSwitch5 = _mm_setr_epi8( 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -1, -1, -1, -1, -1, -1 );
    mSwitch6 = _mm_setr_epi8( 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, -1, -1, -1, -1, -1, -1 );
    mSwitch7 = _mm_setr_epi8( 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1 );
    // change the srcCoeff for "16" can't appear in _mm_shuffle_epi8 function
    mSwitch8 = _mm_setr_epi8( 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1 );

    // extend the coeff[10] to coeff_temp[16]
    for ( i = 0; i < 10; i++ )
    {
        coeff_temp[i] = coeff_h[i];
    }

    mCoefx = _mm_loadu_si128( ( __m128i* )coeff_temp );
    mask   = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );

    tmp = tmp_res;

    // hor
    src = src - 4 * i_src - 4;

    for ( row = -4; row < height + 5; row++ )
    {
        for ( col = 0; col < width; col += 8 )
        {
            __m128i mSrc = _mm_loadu_si128( ( __m128i* )( src + col ) );
            __m128i srcCoeff_for_8thshuffle = _mm_loadu_si128( ( __m128i* )( src + col + 1 ) );

            __m128i mT0 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mCoefx );
            __m128i mT1 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch2 ), mCoefx );
            __m128i mT2 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch3 ), mCoefx );
            __m128i mT3 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch4 ), mCoefx );
            __m128i mT4 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch5 ), mCoefx );
            __m128i mT5 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch6 ), mCoefx );
            __m128i mT6 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch7 ), mCoefx );
            __m128i mT7 = _mm_maddubs_epi16( _mm_shuffle_epi8( srcCoeff_for_8thshuffle, mSwitch8 ), mCoefx );

            __m128i mVal1 = _mm_hadd_epi16( mT0, mT1 );
            __m128i mVal2 = _mm_hadd_epi16( mT2, mT3 );
            __m128i mVal3 = _mm_hadd_epi16( mT4, mT5 );
            __m128i mVal4 = _mm_hadd_epi16( mT6, mT7 );

            __m128i mRVal1 = _mm_hadd_epi16( mVal1, mVal2 );
            __m128i mRVal2 = _mm_hadd_epi16( mVal3, mVal4 );
            __m128i mRVal  = _mm_hadd_epi16( mRVal1, mRVal2 );

            _mm_store_si128( ( __m128i* )&tmp[col], mRVal );
        }

        src += i_src;
        tmp += HOR_VER_10_TMPWIDTH;
    }

    // ver
    shift = FILTER_HORANDVER_SHIFT;
    mAddOffset = _mm_set1_epi32( FILTER_HORANDVER_OFFSET );

    tmp = tmp_res;

    coeff0 = _mm_set1_epi16( *( i16s_t* )coeff_v );
    coeff1 = _mm_set1_epi16( *( i16s_t* )( coeff_v + 2 ) );
    coeff2 = _mm_set1_epi16( *( i16s_t* )( coeff_v + 4 ) );
    coeff3 = _mm_set1_epi16( *( i16s_t* )( coeff_v + 6 ) );
    coeff4 = _mm_set1_epi16( *( i16s_t* )( coeff_v + 8 ) );

    coeff0 = _mm_cvtepi8_epi16( coeff0 );
    coeff1 = _mm_cvtepi8_epi16( coeff1 );
    coeff2 = _mm_cvtepi8_epi16( coeff2 );
    coeff3 = _mm_cvtepi8_epi16( coeff3 );
    coeff4 = _mm_cvtepi8_epi16( coeff4 );

    for ( row = 0; row < height; row++ )
    {
        p = tmp;

        for ( col = 0; col < width - 7; col += 8 )
        {
            __m128i mVal1, mVal2, mRVal;

            __m128i T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            __m128i T10 = _mm_loadu_si128( ( __m128i* )( p + HOR_VER_10_TMPWIDTH ) );
            __m128i T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * HOR_VER_10_TMPWIDTH ) );
            __m128i T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * HOR_VER_10_TMPWIDTH ) );
            __m128i T40 = _mm_loadu_si128( ( __m128i* )( p + 4 * HOR_VER_10_TMPWIDTH ) );
            __m128i T50 = _mm_loadu_si128( ( __m128i* )( p + 5 * HOR_VER_10_TMPWIDTH ) );
            __m128i T60 = _mm_loadu_si128( ( __m128i* )( p + 6 * HOR_VER_10_TMPWIDTH ) );
            __m128i T70 = _mm_loadu_si128( ( __m128i* )( p + 7 * HOR_VER_10_TMPWIDTH ) );
            __m128i T80 = _mm_loadu_si128( ( __m128i* )( p + 8 * HOR_VER_10_TMPWIDTH ) );
            __m128i T90 = _mm_loadu_si128( ( __m128i* )( p + 9 * HOR_VER_10_TMPWIDTH ) );

            __m128i M0 = _mm_unpacklo_epi16( T00, T10 );
            __m128i M1 = _mm_unpacklo_epi16( T20, T30 );
            __m128i M2 = _mm_unpacklo_epi16( T40, T50 );
            __m128i M3 = _mm_unpacklo_epi16( T60, T70 );
            __m128i M4 = _mm_unpacklo_epi16( T80, T90 );

            __m128i M5 = _mm_unpackhi_epi16( T00, T10 );
            __m128i M6 = _mm_unpackhi_epi16( T20, T30 );
            __m128i M7 = _mm_unpackhi_epi16( T40, T50 );
            __m128i M8 = _mm_unpackhi_epi16( T60, T70 );
            __m128i M9 = _mm_unpackhi_epi16( T80, T90 );

            M0 = _mm_madd_epi16( M0, coeff0 );
            M1 = _mm_madd_epi16( M1, coeff1 );
            M2 = _mm_madd_epi16( M2, coeff2 );
            M3 = _mm_madd_epi16( M3, coeff3 );
            M4 = _mm_madd_epi16( M4, coeff4 );
            M5 = _mm_madd_epi16( M5, coeff0 );
            M6 = _mm_madd_epi16( M6, coeff1 );
            M7 = _mm_madd_epi16( M7, coeff2 );
            M8 = _mm_madd_epi16( M8, coeff3 );
            M9 = _mm_madd_epi16( M9, coeff4 );

            mVal1 = _mm_add_epi32( M0, M1 );
            mVal1 = _mm_add_epi32( mVal1, M2 );
            mVal1 = _mm_add_epi32( mVal1, M3 );
            mVal1 = _mm_add_epi32( mVal1, M4 );

            mVal2 = _mm_add_epi32( M5, M6 );
            mVal2 = _mm_add_epi32( mVal2, M7 );
            mVal2 = _mm_add_epi32( mVal2, M8 );
            mVal2 = _mm_add_epi32( mVal2, M9 );

            mVal1 = _mm_add_epi32( mVal1, mAddOffset );
            mVal2 = _mm_add_epi32( mVal2, mAddOffset );

            mVal1 = _mm_srai_epi32( mVal1, shift );
            mVal2 = _mm_srai_epi32( mVal2, shift );

            mRVal = _mm_packs_epi32( mVal1, mVal2 );

            mRVal = _mm_packus_epi16( mRVal, mRVal );

            _mm_storel_epi64( ( __m128i* )&dst[col], mRVal );

            p += 8;
        }

        if ( col < width )
        {
            __m128i mVal1, mVal2, mRVal;

            __m128i T00 = _mm_loadu_si128( ( __m128i* )( p ) );
            __m128i T10 = _mm_loadu_si128( ( __m128i* )( p + HOR_VER_10_TMPWIDTH ) );
            __m128i T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * HOR_VER_10_TMPWIDTH ) );
            __m128i T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * HOR_VER_10_TMPWIDTH ) );
            __m128i T40 = _mm_loadu_si128( ( __m128i* )( p + 4 * HOR_VER_10_TMPWIDTH ) );
            __m128i T50 = _mm_loadu_si128( ( __m128i* )( p + 5 * HOR_VER_10_TMPWIDTH ) );
            __m128i T60 = _mm_loadu_si128( ( __m128i* )( p + 6 * HOR_VER_10_TMPWIDTH ) );
            __m128i T70 = _mm_loadu_si128( ( __m128i* )( p + 7 * HOR_VER_10_TMPWIDTH ) );
            __m128i T80 = _mm_loadu_si128( ( __m128i* )( p + 8 * HOR_VER_10_TMPWIDTH ) );
            __m128i T90 = _mm_loadu_si128( ( __m128i* )( p + 9 * HOR_VER_10_TMPWIDTH ) );

            __m128i M0 = _mm_unpacklo_epi16( T00, T10 );
            __m128i M1 = _mm_unpacklo_epi16( T20, T30 );
            __m128i M2 = _mm_unpacklo_epi16( T40, T50 );
            __m128i M3 = _mm_unpacklo_epi16( T60, T70 );
            __m128i M4 = _mm_unpacklo_epi16( T80, T90 );

            __m128i M5 = _mm_unpackhi_epi16( T00, T10 );
            __m128i M6 = _mm_unpackhi_epi16( T20, T30 );
            __m128i M7 = _mm_unpackhi_epi16( T40, T50 );
            __m128i M8 = _mm_unpackhi_epi16( T60, T70 );
            __m128i M9 = _mm_unpackhi_epi16( T80, T90 );

            M0 = _mm_madd_epi16( M0, coeff0 );
            M1 = _mm_madd_epi16( M1, coeff1 );
            M2 = _mm_madd_epi16( M2, coeff2 );
            M3 = _mm_madd_epi16( M3, coeff3 );
            M4 = _mm_madd_epi16( M4, coeff4 );
            M5 = _mm_madd_epi16( M5, coeff0 );
            M6 = _mm_madd_epi16( M6, coeff1 );
            M7 = _mm_madd_epi16( M7, coeff2 );
            M8 = _mm_madd_epi16( M8, coeff3 );
            M9 = _mm_madd_epi16( M9, coeff4 );

            mVal1 = _mm_add_epi32( M0, M1 );
            mVal1 = _mm_add_epi32( mVal1, M2 );
            mVal1 = _mm_add_epi32( mVal1, M3 );
            mVal1 = _mm_add_epi32( mVal1, M4 );

            mVal2 = _mm_add_epi32( M5, M6 );
            mVal2 = _mm_add_epi32( mVal2, M7 );
            mVal2 = _mm_add_epi32( mVal2, M8 );
            mVal2 = _mm_add_epi32( mVal2, M9 );

            mVal1 = _mm_add_epi32( mVal1, mAddOffset );
            mVal2 = _mm_add_epi32( mVal2, mAddOffset );

            mVal1 = _mm_srai_epi32( mVal1, shift );
            mVal2 = _mm_srai_epi32( mVal2, shift );

            mRVal = _mm_packs_epi32( mVal1, mVal2 );

            mRVal = _mm_packus_epi16( mRVal, mRVal );

            _mm_maskmoveu_si128( mRVal, mask, ( char_t* )&dst[col] );
        }

        tmp += HOR_VER_10_TMPWIDTH;
        dst += i_dst;
    }
}

void get_chroma_subpix_Ext_sse128( const pel_t *src,
                                   int i_src,
                                   pel_t *dst,
                                   int i_dst,
                                   const char_t *COEF_HOR,
                                   const char_t *COEF_VER )
{
    int row, col, height, width, line;
    int shift;
    i16s_t const *p;
    i16s_t *tmp;

    ALIGNED_16( i16s_t tmp_res[( HOR_VER_4_TMPWIDTH + FILTER_TAPNUM_4 - 1 ) * HOR_VER_4_TMPWIDTH] );

    __m128i mSwitch1, mCoefx, mask, mAddOffset, coeff0, coeff1, mVal1;
    __m128i mMaxval, mMinval;

    height = width = 4;

    mSwitch1 = _mm_setr_epi8( 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6 );
    //mSwitch2 = _mm_setr_epi8(4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10);
    mCoefx = _mm_set1_epi32( *( i32s_t* )COEF_HOR );
    mask = _mm_loadu_si128( ( __m128i* )( intrinsic_mask[( width & 7 ) - 1] ) );

    mMaxval = _mm_set1_epi16( 255 );
    mMinval = _mm_set1_epi8( 0 );

    // set the limiting val
    mMaxval = _mm_packus_epi16( mMaxval, mMaxval );

    tmp = tmp_res;
    shift = FILTER_HORORVER_SHIFT;
    mAddOffset = _mm_set1_epi32( FILTER_HORORVER_OFFSET );
    col = 0;

    for ( row = 0; row < height; row++ )
    {
        __m128i mSrc, mT0, mVal, T00, T10, T20, T30, M0, M1;

        for ( line = -1; line < 3; line++ )
        {
            // hor
            mSrc = _mm_loadu_si128( ( __m128i* )( src + line *i_src - 1 ) );

            mT0 = _mm_maddubs_epi16( _mm_shuffle_epi8( mSrc, mSwitch1 ), mCoefx );
            mVal = _mm_hadd_epi16( mT0, mT0 );

            _mm_storel_epi64( ( __m128i* )tmp, mVal );

            tmp += HOR_VER_4_TMPWIDTH;
        }

        tmp = tmp_res;
        // ver
        coeff0 = _mm_set1_epi16( *( i16s_t* )COEF_VER );
        coeff1 = _mm_set1_epi16( *( i16s_t* )( COEF_VER + 2 ) );

        coeff0 = _mm_cvtepi8_epi16( coeff0 );
        coeff1 = _mm_cvtepi8_epi16( coeff1 );

        p = tmp;

        T00 = _mm_loadu_si128( ( __m128i* )( p ) );
        T10 = _mm_loadu_si128( ( __m128i* )( p + HOR_VER_4_TMPWIDTH ) );
        T20 = _mm_loadu_si128( ( __m128i* )( p + 2 * HOR_VER_4_TMPWIDTH ) );
        T30 = _mm_loadu_si128( ( __m128i* )( p + 3 * HOR_VER_4_TMPWIDTH ) );

        M0 = _mm_unpacklo_epi16( T00, T10 );
        M1 = _mm_unpacklo_epi16( T20, T30 );

        M0 = _mm_madd_epi16( M0, coeff0 );
        M1 = _mm_madd_epi16( M1, coeff1 );

        mVal1 = _mm_add_epi32( M0, M1 );

        mVal1 = _mm_add_epi32( mVal1, mAddOffset );
        mVal1 = _mm_srai_epi32( mVal1, shift );

        mVal1 = _mm_add_epi32( mVal1, mAddOffset );
        mVal1 = _mm_srai_epi32( mVal1, shift );

        mVal = _mm_packus_epi16( mVal1, mVal1 );
        mVal = _mm_packus_epi16( mVal, mVal );

        // the val in mVal should meet: 0 <= val <= 255
        mVal1 = _mm_min_epu8( mMaxval, mVal1 );
        mVal1 = _mm_max_epu8( mMinval, mVal1 );

        _mm_maskmoveu_si128( mVal, mask, ( char_t* )&dst[col] );

        src += i_src;
        dst += i_dst;
    }
}

/*
void com_if_filter_hor_ver_4_H_sse128(const pel_t *src,
                                      int i_src,
                                      int *dst,
                                      int i_dst,
                                      int width,
                                      int height,
                                      char_t const *coef_hor)
{
    int row, col, i;

    __m128i mSwitch1 = _mm_setr_epi8(0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6);
    __m128i mSwitch2 = _mm_setr_epi8(4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10);
    __m128i mBuffer  = _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    __m128i mCoef    = _mm_set1_epi32(*(i32s_t*)coef_hor);
    __m128i mask     = _mm_loadu_si128((__m128i*)(intrinsic_mask[(width & 7) - 1]));

    src -= 1;

    for (row = 0; row < height; row++) {
        __m128i mT20, mT40, mSum, mVal, mRVal1, mRVal2;

        for (col = 0; col < width - 7; col += 8) {

            __m128i mSrc = _mm_loadu_si128((__m128i*)(src + col));

            mT20 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch1), mCoef);
            mT40 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch2), mCoef);

            mSum = _mm_hadd_epi16(mT20, mT40);

            mRVal1 = _mm_unpacklo_epi16(mSum, mBuffer);
            _mm_storeu_si128((__m128i*)&dst[col], mRVal1);

            mRVal2 = _mm_unpackhi_epi16(mSum, mBuffer);
            _mm_storeu_si128((__m128i*)&dst[col+4], mRVal2);
        }

        if (col < width) {

            __m128i mSrc = _mm_loadu_si128((__m128i*)(src + col));

            mT20 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch1), mCoef);
            mT40 = _mm_maddubs_epi16(_mm_shuffle_epi8(mSrc, mSwitch2), mCoef);

            mSum = _mm_hadd_epi16(mT20, mT40);

            mRVal1 = _mm_unpacklo_epi16(mSum, mBuffer);

            ;

            _mm_maskmoveu_si128(mVal, mask, (char_t*)&dst[col]);
        }

        src += i_src;
        dst += i_dst;
    }
}

void com_if_filter_hor_ver_4_V_sse128(const int *src,
                                      int i_src,
                                      pel_t *dst,
                                      int i_dst,
                                      int width,
                                      int height,
                                      char_t const *coef_ver,
                                      int max_val)
{
    int row, col;
    int const *p;
    const int offset = FILTER_HORANDVER_OFFSET;
    const int shift  = FILTER_HORANDVER_SHIFT;

    __m128i mAddOffset, mask, coeff0, coeff1, coeff2, coeff3, mMaxval, mMinval, mVal, mRval;

    mAddOffset = _mm_set1_epi32(offset);
    mask    = _mm_loadu_si128((__m128i*)(intrinsic_mask[(width & 7) - 1]));
    coeff0  = _mm_set1_epi16(*(i32s_t*)coef_ver);
    coeff1  = _mm_set1_epi16(*(i32s_t*)(coef_ver+2));
    coeff0  = _mm_cvtepi8_epi16(coeff0);
    coeff1  = _mm_cvtepi8_epi16(coeff1);
    mMaxval = _mm_set1_epi16(max_val);
    mMinval = _mm_set1_epi8(INIT_ZERO);

    // set the limiting val
    mMaxval = _mm_packus_epi16(mMaxval, mMaxval);

    src -= i_src;

    for (row = 0; row < height; row++) {
        p = src;
        for (col = 0; col < width - 7; col += 8) {
            __m128i T00, T01, T10, T11, T20, T21, T30, T31, S00, S10, S20, S30, M0, M1, M2, M3, mVal1, mVal2;
            T00 = _mm_loadu_si128((__m128i*)(p));
            T01 = _mm_loadu_si128((__m128i*)(p+4));
            T10 = _mm_loadu_si128((__m128i*)(p + i_src));
            T11 = _mm_loadu_si128((__m128i*)(p + i_src+4));
            T20 = _mm_loadu_si128((__m128i*)(p + 2 * i_src));
            T21 = _mm_loadu_si128((__m128i*)(p + 2 * i_src+4));
            T30 = _mm_loadu_si128((__m128i*)(p + 3 * i_src));
            T31 = _mm_loadu_si128((__m128i*)(p + 3 * i_src+4));

            T00 = _mm_packus_epi32(T00, T01);
            T10 = _mm_packus_epi32(T10, T11);
            T20 = _mm_packus_epi32(T20, T21);
            T30 = _mm_packus_epi32(T30, T31);

            M0 = _mm_unpacklo_epi16(T00, T10);
            M1 = _mm_unpacklo_epi16(T20, T30);
            M2 = _mm_unpackhi_epi16(T00, T10);
            M3 = _mm_unpackhi_epi16(T20, T30);

            M0 = _mm_madd_epi16(M0, coeff0);
            M1 = _mm_madd_epi16(M1, coeff1);
            M2 = _mm_madd_epi16(M2, coeff0);
            M3 = _mm_madd_epi16(M3, coeff1);

            mVal1 = _mm_add_epi32(M0, M1);
            mVal2 = _mm_add_epi32(M2, M3);

            mVal1 = _mm_add_epi32(mVal1, mAddOffset);
            mVal2 = _mm_add_epi32(mVal2, mAddOffset);
            mVal1 = _mm_srai_epi32(mVal1, shift);
            mVal2 = _mm_srai_epi32(mVal2, shift);
            mVal  = _mm_packs_epi32(mVal1, mVal2);
            mVal  = _mm_packus_epi16(mVal, mVal);

            // the val in mVal should meet: 0 <= val <= max_val
            mRval = _mm_min_epu8(mMaxval, mVal);
            mRval = _mm_max_epu8(mMinval, mRval);

            _mm_storel_epi64((__m128i*)&dst[col], mRval);

            p += 8;
        }

        if (col < width) {
            __m128i T00, T10, T20, T30;
            T00 = _mm_loadu_si128((__m128i*)(p));
            T10 = _mm_loadu_si128((__m128i*)(p + i_src));
            T20 = _mm_loadu_si128((__m128i*)(p + 2 * i_src));
            T30 = _mm_loadu_si128((__m128i*)(p + 3 * i_src));

            T00 = _mm_unpacklo_epi8(T00, T10);
            T10 = _mm_unpacklo_epi8(T20, T30);

            T00 = _mm_maddubs_epi16(T00, coeff0);
            T10 = _mm_maddubs_epi16(T10, coeff1);

            mVal = _mm_add_epi16(T00, T10);

            mVal = _mm_add_epi16(mVal, mAddOffset);
            mVal = _mm_srai_epi16(mVal, shift);
            mVal = _mm_packus_epi16(mVal, mVal);

            // the val in mVal should meet: 0 <= val <= max_val
            mRval = _mm_min_epu8(mMaxval, mVal);
            mRval = _mm_max_epu8(mMinval, mRval);

            _mm_maskmoveu_si128(mRval, mask, (char_t*)&dst[col]);
        }
        src += i_src;
        dst += i_dst;
    }
}
*/

#endif