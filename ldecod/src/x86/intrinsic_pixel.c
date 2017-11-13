/*****************************************************************************
* Authors: Ronggang Wang <rgwang@pkusz.edu.cn>
*          Zhenyu Wang <wangzhenyu@pkusz.edu.cn>
*          Kui Fan <kuifan@pku.edu.cn>
*          Shenghao Zhang <1219759986@qq.com>
* Affiliate: Peking University Shenzhen Graduate School
*****************************************************************************/

#include "intrinsic.h"

#if COMPILE_FOR_8BIT


void add_pel_clip_sse128( int b8, int b4, int* curr_blk, int bsize, \
                          pel_t *ppredblk, int( *pm7 )[16], int ipix_y, \
                          int iStride, int ipix_x, int ipix_c_y, int ipix_c_x, int iStrideC )
{
    int  by = 8 * ( b8 / 2 ) + 4 * ( b4 / 2 );
    int  bx = 8 * ( b8 % 2 ) + 4 * ( b4 % 2 );

    __m128i T0, T1, T2, T3;
    __m128i T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T1A, T1B, T1C, T1D, T1E, T1F;
    __m128i T20, T21, T22, T23, T24, T25, T26, T27, T28, T29, T2A, T2B, T2C, T2D, T2E, T2F;
    __m128i T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T3A, T3B, T3C, T3D, T3E, T3F;
    __m128i T40, T41, T42, T43, T44, T45, T46, T47, T48, T49, T4A, T4B, T4C, T4D, T4E, T4F;

    __m128i M10, M11, M12, M13, M14, M15, M16, M17, M18, M19, M1A, M1B, M1C, M1D, M1E, M1F;
    __m128i M20, M21, M22, M23, M24, M25, M26, M27, M28, M29, M2A, M2B, M2C, M2D, M2E, M2F;
    __m128i M30, M31, M32, M33, M34, M35, M36, M37, M38, M39, M3A, M3B, M3C, M3D, M3E, M3F;
    __m128i M40, M41, M42, M43, M44, M45, M46, M47, M48, M49, M4A, M4B, M4C, M4D, M4E, M4F;

    __m128i mTemp10, mTemp11, mTemp12, mTemp13, mTemp14, mTemp15, mTemp16, mTemp17;
    __m128i mTemp18, mTemp19, mTemp1A, mTemp1B, mTemp1C, mTemp1D, mTemp1E, mTemp1F;

    __m128i mTemp20, mTemp21, mTemp22, mTemp23, mTemp24, mTemp25, mTemp26, mTemp27;
    __m128i mTemp28, mTemp29, mTemp2A, mTemp2B, mTemp2C, mTemp2D, mTemp2E, mTemp2F;

    __m128i mTemp30, mTemp31, mTemp32, mTemp33, mTemp34, mTemp35, mTemp36, mTemp37;
    __m128i mTemp38, mTemp39, mTemp3A, mTemp3B, mTemp3C, mTemp3D, mTemp3E, mTemp3F;

    __m128i mTemp40, mTemp41, mTemp42, mTemp43, mTemp44, mTemp45, mTemp46, mTemp47;
    __m128i mTemp48, mTemp49, mTemp4A, mTemp4B, mTemp4C, mTemp4D, mTemp4E, mTemp4F;

    __m128i M0, M1, M2, M3;
    __m128i mTemp0, mTemp1, mTemp2, mTemp3;

    __m128i mMask = _mm_set_epi8( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1 );
    __m128i mMaxval  = _mm_set1_epi32( 255 );
    __m128i mMinval  = _mm_set1_epi32( 0 );
    __m128i mZero = _mm_set1_epi32( 0 );

    if ( b8 < 4 )
    {

        if ( 4 == bsize )
        {
            // get curr_blk value
            T0 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 ) );
            T1 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 ) );
            T2 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 4 ) );
            T3 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 4 ) );

            M0 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + by * 16 + bx ) ) );
            M1 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 1 ) * 16 + bx ) ) );
            M2 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 2 ) * 16 + bx ) ) );
            M3 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 3 ) * 16 + bx ) ) );

            mTemp0 = _mm_add_epi32( T0, M0 );
            mTemp1 = _mm_add_epi32( T1, M1 );
            mTemp2 = _mm_add_epi32( T2, M2 );
            mTemp3 = _mm_add_epi32( T3, M3 );

            mTemp0 = _mm_min_epi32( mMaxval, mTemp0 );
            mTemp0 = _mm_max_epi32( mMinval, mTemp0 );
            mTemp1 = _mm_min_epi32( mMaxval, mTemp1 );
            mTemp1 = _mm_max_epi32( mMinval, mTemp1 );
            mTemp2 = _mm_min_epi32( mMaxval, mTemp2 );
            mTemp2 = _mm_max_epi32( mMinval, mTemp2 );
            mTemp3 = _mm_min_epi32( mMaxval, mTemp3 );
            mTemp3 = _mm_max_epi32( mMinval, mTemp3 );

            _mm_storeu_si128( ( __m128i* )( curr_blk + 0 ), mTemp0 );
            _mm_storeu_si128( ( __m128i* )( curr_blk + 4 ), mTemp1 );
            _mm_storeu_si128( ( __m128i* )( curr_blk + 2 * 4 ), mTemp2 );
            _mm_storeu_si128( ( __m128i* )( curr_blk + 3 * 4 ), mTemp3 );

            // get pm7
            // transpose 4x4 matrix
            M0 = _mm_unpacklo_epi32( mTemp0, mTemp1 );
            M1 = _mm_unpacklo_epi32( mTemp2, mTemp3 );
            M2 = _mm_unpackhi_epi32( mTemp0, mTemp1 );
            M3 = _mm_unpackhi_epi32( mTemp2, mTemp3 );

            T0 = _mm_unpacklo_epi64( M0, M1 );
            T1 = _mm_unpackhi_epi64( M0, M1 );
            T2 = _mm_unpacklo_epi64( M2, M3 );
            T3 = _mm_unpackhi_epi64( M2, M3 );

            _mm_storeu_si128( ( __m128i* )( pm7 + 0 ), T0 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 1 ), T1 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 2 ), T2 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 3 ), T3 );

            // get imgY_rec
            T0 = _mm_packus_epi32( mTemp0, mZero );
            T1 = _mm_packus_epi32( mTemp1, mZero );
            T2 = _mm_packus_epi32( mTemp2, mZero );
            T3 = _mm_packus_epi32( mTemp3, mZero );

            T0 = _mm_packus_epi16( T0, mZero );
            T1 = _mm_packus_epi16( T1, mZero );
            T2 = _mm_packus_epi16( T2, mZero );
            T3 = _mm_packus_epi16( T3, mZero );

            _mm_maskmoveu_si128( T0, mMask, ( imgY + ( ipix_y + by )*iStride + ipix_x + bx ) );
            _mm_maskmoveu_si128( T1, mMask, ( imgY + ( ipix_y + by + 1 )*iStride + ipix_x + bx ) );
            _mm_maskmoveu_si128( T2, mMask, ( imgY + ( ipix_y + by + 2 )*iStride + ipix_x + bx ) );
            _mm_maskmoveu_si128( T3, mMask, ( imgY + ( ipix_y + by + 3 )*iStride + ipix_x + bx ) );

            /*_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by)*iStride + ipix_x + bx), T0);
            _mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 1)*iStride + ipix_x + bx), T1);
            _mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 2)*iStride + ipix_x + bx), T2);
            _mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 3)*iStride + ipix_x + bx), T3);*/
        }
        else if ( 8 == bsize )
        {
            // get curr_blk value
            T10 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 ) );
            T20 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 ) );
            T11 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 ) );
            T21 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 + 4 ) );
            T12 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 8 ) );
            T22 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 8 + 4 ) );
            T13 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 8 ) );
            T23 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 8 + 4 ) );
            T14 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 8 ) );
            T24 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 8 + 4 ) );
            T15 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 8 ) );
            T25 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 8 + 4 ) );
            T16 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 8 ) );
            T26 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 8 + 4 ) );
            T17 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 8 ) );
            T27 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 8 + 4 ) );

            M10 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + by * 16 + bx ) ) );
            M20 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + by * 16 + bx + 4 ) ) );

            M11 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 1 ) * 16 + bx ) ) );
            M21 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 1 ) * 16 + bx + 4 ) ) );

            M12 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 2 ) * 16 + bx ) ) );
            M22 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 2 ) * 16 + bx + 4 ) ) );

            M13 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 3 ) * 16 + bx ) ) );
            M23 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 3 ) * 16 + bx + 4 ) ) );

            M14 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 4 ) * 16 + bx ) ) );
            M24 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 4 ) * 16 + bx + 4 ) ) );

            M15 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 5 ) * 16 + bx ) ) );
            M25 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 5 ) * 16 + bx + 4 ) ) );

            M16 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 6 ) * 16 + bx ) ) );
            M26 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 6 ) * 16 + bx + 4 ) ) );

            M17 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 7 ) * 16 + bx ) ) );
            M27 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 7 ) * 16 + bx + 4 ) ) );

            mTemp10 = _mm_add_epi32( T10, M10 );
            mTemp20 = _mm_add_epi32( T20, M20 );
            mTemp11 = _mm_add_epi32( T11, M11 );
            mTemp21 = _mm_add_epi32( T21, M21 );
            mTemp12 = _mm_add_epi32( T12, M12 );
            mTemp22 = _mm_add_epi32( T22, M22 );
            mTemp13 = _mm_add_epi32( T13, M13 );
            mTemp23 = _mm_add_epi32( T23, M23 );
            mTemp14 = _mm_add_epi32( T14, M14 );
            mTemp24 = _mm_add_epi32( T24, M24 );
            mTemp15 = _mm_add_epi32( T15, M15 );
            mTemp25 = _mm_add_epi32( T25, M25 );
            mTemp16 = _mm_add_epi32( T16, M16 );
            mTemp26 = _mm_add_epi32( T26, M26 );
            mTemp17 = _mm_add_epi32( T17, M17 );
            mTemp27 = _mm_add_epi32( T27, M27 );

            mTemp10 = _mm_min_epi32( mMaxval, mTemp10 );
            mTemp10 = _mm_max_epi32( mMinval, mTemp10 );
            mTemp20 = _mm_min_epi32( mMaxval, mTemp20 );
            mTemp20 = _mm_max_epi32( mMinval, mTemp20 );

            mTemp11 = _mm_min_epi32( mMaxval, mTemp11 );
            mTemp11 = _mm_max_epi32( mMinval, mTemp11 );
            mTemp21 = _mm_min_epi32( mMaxval, mTemp21 );
            mTemp21 = _mm_max_epi32( mMinval, mTemp21 );

            mTemp12 = _mm_min_epi32( mMaxval, mTemp12 );
            mTemp12 = _mm_max_epi32( mMinval, mTemp12 );
            mTemp22 = _mm_min_epi32( mMaxval, mTemp22 );
            mTemp22 = _mm_max_epi32( mMinval, mTemp22 );

            mTemp13 = _mm_min_epi32( mMaxval, mTemp13 );
            mTemp13 = _mm_max_epi32( mMinval, mTemp13 );
            mTemp23 = _mm_min_epi32( mMaxval, mTemp23 );
            mTemp23 = _mm_max_epi32( mMinval, mTemp23 );

            mTemp14 = _mm_min_epi32( mMaxval, mTemp14 );
            mTemp14 = _mm_max_epi32( mMinval, mTemp14 );
            mTemp24 = _mm_min_epi32( mMaxval, mTemp24 );
            mTemp24 = _mm_max_epi32( mMinval, mTemp24 );

            mTemp15 = _mm_min_epi32( mMaxval, mTemp15 );
            mTemp15 = _mm_max_epi32( mMinval, mTemp15 );
            mTemp25 = _mm_min_epi32( mMaxval, mTemp25 );
            mTemp25 = _mm_max_epi32( mMinval, mTemp25 );

            mTemp16 = _mm_min_epi32( mMaxval, mTemp16 );
            mTemp16 = _mm_max_epi32( mMinval, mTemp16 );
            mTemp26 = _mm_min_epi32( mMaxval, mTemp26 );
            mTemp26 = _mm_max_epi32( mMinval, mTemp26 );

            mTemp17 = _mm_min_epi32( mMaxval, mTemp17 );
            mTemp17 = _mm_max_epi32( mMinval, mTemp17 );
            mTemp27 = _mm_min_epi32( mMaxval, mTemp27 );
            mTemp27 = _mm_max_epi32( mMinval, mTemp27 );

            _mm_storeu_si128( ( __m128i * )curr_blk, mTemp10 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 ), mTemp20 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 ), mTemp11 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 + 4 ), mTemp21 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 8 ), mTemp12 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 8 + 4 ), mTemp22 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 8 ), mTemp13 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 8 + 4 ), mTemp23 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 8 ), mTemp14 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 8 + 4 ), mTemp24 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 8 ), mTemp15 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 8 + 4 ), mTemp25 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 8 ), mTemp16 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 8 + 4 ), mTemp26 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 8 ), mTemp17 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 8 + 4 ), mTemp27 );

            // get pm7
            // transpose 8x8 matrix
            M10 = _mm_unpacklo_epi32( mTemp10, mTemp11 );
            M11 = _mm_unpackhi_epi32( mTemp10, mTemp11 );
            M20 = _mm_unpacklo_epi32( mTemp20, mTemp21 );
            M21 = _mm_unpackhi_epi32( mTemp20, mTemp21 );
            M12 = _mm_unpacklo_epi32( mTemp12, mTemp13 );
            M13 = _mm_unpackhi_epi32( mTemp12, mTemp13 );
            M22 = _mm_unpacklo_epi32( mTemp22, mTemp23 );
            M23 = _mm_unpackhi_epi32( mTemp22, mTemp23 );
            M14 = _mm_unpacklo_epi32( mTemp14, mTemp15 );
            M15 = _mm_unpackhi_epi32( mTemp14, mTemp15 );
            M24 = _mm_unpacklo_epi32( mTemp24, mTemp25 );
            M25 = _mm_unpackhi_epi32( mTemp24, mTemp25 );
            M16 = _mm_unpacklo_epi32( mTemp16, mTemp17 );
            M17 = _mm_unpackhi_epi32( mTemp16, mTemp17 );
            M26 = _mm_unpacklo_epi32( mTemp26, mTemp27 );
            M27 = _mm_unpackhi_epi32( mTemp26, mTemp27 );

            T10 = _mm_unpacklo_epi64( M10, M12 );
            T11 = _mm_unpackhi_epi64( M10, M12 );
            T20 = _mm_unpacklo_epi64( M14, M16 );
            T21 = _mm_unpackhi_epi64( M14, M16 );
            T12 = _mm_unpacklo_epi64( M11, M13 );
            T13 = _mm_unpackhi_epi64( M11, M13 );
            T22 = _mm_unpacklo_epi64( M15, M17 );
            T23 = _mm_unpackhi_epi64( M15, M17 );
            T14 = _mm_unpacklo_epi64( M20, M22 );
            T15 = _mm_unpackhi_epi64( M20, M22 );
            T24 = _mm_unpacklo_epi64( M24, M26 );
            T25 = _mm_unpackhi_epi64( M24, M26 );
            T16 = _mm_unpacklo_epi64( M21, M23 );
            T17 = _mm_unpackhi_epi64( M21, M23 );
            T26 = _mm_unpacklo_epi64( M25, M27 );
            T27 = _mm_unpackhi_epi64( M25, M27 );

            _mm_storeu_si128( ( __m128i* )( pm7[0] ), T10 );
            _mm_storeu_si128( ( __m128i* )( pm7[0]+4 ), T20 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 1 ), T11 );
            _mm_storeu_si128( ( __m128i* )( pm7[1]+4 ), T21 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 2 ), T12 );
            _mm_storeu_si128( ( __m128i* )( pm7[2]+4 ), T22 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 3 ), T13 );
            _mm_storeu_si128( ( __m128i* )( pm7[3]+4 ), T23 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 4 ), T14 );
            _mm_storeu_si128( ( __m128i* )( pm7[4] + 4 ), T24 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 5 ), T15 );
            _mm_storeu_si128( ( __m128i* )( pm7[5] + 4 ), T25 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 6 ), T16 );
            _mm_storeu_si128( ( __m128i* )( pm7[6] + 4 ), T26 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 7 ), T17 );
            _mm_storeu_si128( ( __m128i* )( pm7[7] + 4 ), T27 );

            // get imgY_rec
            T10 = _mm_packus_epi32( mTemp10, mZero );
            T20 = _mm_packus_epi32( mTemp20, mZero );
            T11 = _mm_packus_epi32( mTemp11, mZero );
            T21 = _mm_packus_epi32( mTemp21, mZero );
            T12 = _mm_packus_epi32( mTemp12, mZero );
            T22 = _mm_packus_epi32( mTemp22, mZero );
            T13 = _mm_packus_epi32( mTemp13, mZero );
            T23 = _mm_packus_epi32( mTemp23, mZero );
            T14 = _mm_packus_epi32( mTemp14, mZero );
            T24 = _mm_packus_epi32( mTemp24, mZero );
            T15 = _mm_packus_epi32( mTemp15, mZero );
            T25 = _mm_packus_epi32( mTemp25, mZero );
            T16 = _mm_packus_epi32( mTemp16, mZero );
            T26 = _mm_packus_epi32( mTemp26, mZero );
            T17 = _mm_packus_epi32( mTemp17, mZero );
            T27 = _mm_packus_epi32( mTemp27, mZero );

            T10 = _mm_packus_epi16( T10, mZero );
            T20 = _mm_packus_epi16( T20, mZero );
            T11 = _mm_packus_epi16( T11, mZero );
            T21 = _mm_packus_epi16( T21, mZero );
            T12 = _mm_packus_epi16( T12, mZero );
            T22 = _mm_packus_epi16( T22, mZero );
            T13 = _mm_packus_epi16( T13, mZero );
            T23 = _mm_packus_epi16( T23, mZero );
            T14 = _mm_packus_epi16( T14, mZero );
            T24 = _mm_packus_epi16( T24, mZero );
            T15 = _mm_packus_epi16( T15, mZero );
            T25 = _mm_packus_epi16( T25, mZero );
            T16 = _mm_packus_epi16( T16, mZero );
            T26 = _mm_packus_epi16( T26, mZero );
            T17 = _mm_packus_epi16( T17, mZero );
            T27 = _mm_packus_epi16( T27, mZero );

            // pack data
            T10 = _mm_unpacklo_epi32( T10, T20 );
            T11 = _mm_unpacklo_epi32( T11, T21 );
            T12 = _mm_unpacklo_epi32( T12, T22 );
            T13 = _mm_unpacklo_epi32( T13, T23 );
            T14 = _mm_unpacklo_epi32( T14, T24 );
            T15 = _mm_unpacklo_epi32( T15, T25 );
            T16 = _mm_unpacklo_epi32( T16, T26 );
            T17 = _mm_unpacklo_epi32( T17, T27 );

            _mm_storel_epi64( ( __m128i* )( imgY + ( ipix_y + by )*iStride + ipix_x + bx ), T10 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by)*iStride + ipix_x + bx+4), T20);
            _mm_storel_epi64( ( __m128i* )( imgY + ( ipix_y + by + 1 )*iStride + ipix_x + bx ), T11 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 1)*iStride + ipix_x + bx+4), T21);
            _mm_storel_epi64( ( __m128i* )( imgY + ( ipix_y + by + 2 )*iStride + ipix_x + bx ), T12 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 2)*iStride + ipix_x + bx + 4), T22);
            _mm_storel_epi64( ( __m128i* )( imgY + ( ipix_y + by + 3 )*iStride + ipix_x + bx ), T13 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 3)*iStride + ipix_x + bx + 4), T23);
            _mm_storel_epi64( ( __m128i* )( imgY + ( ipix_y + by + 4 )*iStride + ipix_x + bx ), T14 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 4)*iStride + ipix_x + bx + 4), T24);
            _mm_storel_epi64( ( __m128i* )( imgY + ( ipix_y + by + 5 )*iStride + ipix_x + bx ), T15 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 5)*iStride + ipix_x + bx + 4), T25);
            _mm_storel_epi64( ( __m128i* )( imgY + ( ipix_y + by + 6 )*iStride + ipix_x + bx ), T16 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 6)*iStride + ipix_x + bx + 4), T26);
            _mm_storel_epi64( ( __m128i* )( imgY + ( ipix_y + by + 7 )*iStride + ipix_x + bx ), T17 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 7)*iStride + ipix_x + bx + 4), T27);

        }
        else
        {
            // get curr_blk value
            T10 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 * 16 + 0 ) );
            T20 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 * 16 + 4 ) );
            T30 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 * 16 + 8 ) );
            T40 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 * 16 + 12 ) );

            T11 = _mm_loadu_si128( ( __m128i* )( curr_blk + 1 * 16 + 0 ) );
            T21 = _mm_loadu_si128( ( __m128i* )( curr_blk + 1 * 16 + 4 ) );
            T31 = _mm_loadu_si128( ( __m128i* )( curr_blk + 1 * 16 + 8 ) );
            T41 = _mm_loadu_si128( ( __m128i* )( curr_blk + 1 * 16 + 12 ) );

            T12 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 16 + 0 ) );
            T22 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 16 + 4 ) );
            T32 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 16 + 8 ) );
            T42 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 16 + 12 ) );

            T13 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 16 + 0 ) );
            T23 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 16 + 4 ) );
            T33 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 16 + 8 ) );
            T43 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 16 + 12 ) );

            T14 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 16 + 0 ) );
            T24 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 16 + 4 ) );
            T34 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 16 + 8 ) );
            T44 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 16 + 12 ) );

            T15 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 16 + 0 ) );
            T25 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 16 + 4 ) );
            T35 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 16 + 8 ) );
            T45 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 16 + 12 ) );

            T16 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 16 + 0 ) );
            T26 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 16 + 4 ) );
            T36 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 16 + 8 ) );
            T46 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 16 + 12 ) );

            T17 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 16 + 0 ) );
            T27 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 16 + 4 ) );
            T37 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 16 + 8 ) );
            T47 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 16 + 12 ) );

            T18 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 * 16 + 0 ) );
            T28 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 * 16 + 4 ) );
            T38 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 * 16 + 8 ) );
            T48 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 * 16 + 12 ) );

            T19 = _mm_loadu_si128( ( __m128i* )( curr_blk + 9 * 16 + 0 ) );
            T29 = _mm_loadu_si128( ( __m128i* )( curr_blk + 9 * 16 + 4 ) );
            T39 = _mm_loadu_si128( ( __m128i* )( curr_blk + 9 * 16 + 8 ) );
            T49 = _mm_loadu_si128( ( __m128i* )( curr_blk + 9 * 16 + 12 ) );

            T1A = _mm_loadu_si128( ( __m128i* )( curr_blk + 10 * 16 + 0 ) );
            T2A = _mm_loadu_si128( ( __m128i* )( curr_blk + 10 * 16 + 4 ) );
            T3A = _mm_loadu_si128( ( __m128i* )( curr_blk + 10 * 16 + 8 ) );
            T4A = _mm_loadu_si128( ( __m128i* )( curr_blk + 10 * 16 + 12 ) );

            T1B = _mm_loadu_si128( ( __m128i* )( curr_blk + 11 * 16 + 0 ) );
            T2B = _mm_loadu_si128( ( __m128i* )( curr_blk + 11 * 16 + 4 ) );
            T3B = _mm_loadu_si128( ( __m128i* )( curr_blk + 11 * 16 + 8 ) );
            T4B = _mm_loadu_si128( ( __m128i* )( curr_blk + 11 * 16 + 12 ) );

            T1C = _mm_loadu_si128( ( __m128i* )( curr_blk + 12 * 16 + 0 ) );
            T2C = _mm_loadu_si128( ( __m128i* )( curr_blk + 12 * 16 + 4 ) );
            T3C = _mm_loadu_si128( ( __m128i* )( curr_blk + 12 * 16 + 8 ) );
            T4C = _mm_loadu_si128( ( __m128i* )( curr_blk + 12 * 16 + 12 ) );

            T1D = _mm_loadu_si128( ( __m128i* )( curr_blk + 13 * 16 + 0 ) );
            T2D = _mm_loadu_si128( ( __m128i* )( curr_blk + 13 * 16 + 4 ) );
            T3D = _mm_loadu_si128( ( __m128i* )( curr_blk + 13 * 16 + 8 ) );
            T4D = _mm_loadu_si128( ( __m128i* )( curr_blk + 13 * 16 + 12 ) );

            T1E = _mm_loadu_si128( ( __m128i* )( curr_blk + 14 * 16 + 0 ) );
            T2E = _mm_loadu_si128( ( __m128i* )( curr_blk + 14 * 16 + 4 ) );
            T3E = _mm_loadu_si128( ( __m128i* )( curr_blk + 14 * 16 + 8 ) );
            T4E = _mm_loadu_si128( ( __m128i* )( curr_blk + 14 * 16 + 12 ) );

            T1F = _mm_loadu_si128( ( __m128i* )( curr_blk + 15 * 16 + 0 ) );
            T2F = _mm_loadu_si128( ( __m128i* )( curr_blk + 15 * 16 + 4 ) );
            T3F = _mm_loadu_si128( ( __m128i* )( curr_blk + 15 * 16 + 8 ) );
            T4F = _mm_loadu_si128( ( __m128i* )( curr_blk + 15 * 16 + 12 ) );

            M10 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 0 ) * 16 + bx ) ) );
            M20 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 0 ) * 16 + bx + 4 ) ) );
            M30 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 0 ) * 16 + bx + 8 ) ) );
            M40 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 0 ) * 16 + bx + 12 ) ) );

            M11 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 1 ) * 16 + bx ) ) );
            M21 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 1 ) * 16 + bx + 4 ) ) );
            M31 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 1 ) * 16 + bx + 8 ) ) );
            M41 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 1 ) * 16 + bx + 12 ) ) );

            M12 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 2 ) * 16 + bx ) ) );
            M22 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 2 ) * 16 + bx + 4 ) ) );
            M32 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 2 ) * 16 + bx + 8 ) ) );
            M42 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 2 ) * 16 + bx + 12 ) ) );

            M13 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 3 ) * 16 + bx ) ) );
            M23 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 3 ) * 16 + bx + 4 ) ) );
            M33 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 3 ) * 16 + bx + 8 ) ) );
            M43 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 3 ) * 16 + bx + 12 ) ) );

            M14 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 4 ) * 16 + bx ) ) );
            M24 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 4 ) * 16 + bx + 4 ) ) );
            M34 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 4 ) * 16 + bx + 8 ) ) );
            M44 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 4 ) * 16 + bx + 12 ) ) );

            M15 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 5 ) * 16 + bx ) ) );
            M25 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 5 ) * 16 + bx + 4 ) ) );
            M35 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 5 ) * 16 + bx + 8 ) ) );
            M45 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 5 ) * 16 + bx + 12 ) ) );

            M16 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 6 ) * 16 + bx ) ) );
            M26 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 6 ) * 16 + bx + 4 ) ) );
            M36 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 6 ) * 16 + bx + 8 ) ) );
            M46 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 6 ) * 16 + bx + 12 ) ) );

            M17 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 7 ) * 16 + bx ) ) );
            M27 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 7 ) * 16 + bx + 4 ) ) );
            M37 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 7 ) * 16 + bx + 8 ) ) );
            M47 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 7 ) * 16 + bx + 12 ) ) );

            M18 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 8 ) * 16 + bx ) ) );
            M28 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 8 ) * 16 + bx + 4 ) ) );
            M38 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 8 ) * 16 + bx + 8 ) ) );
            M48 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 8 ) * 16 + bx + 12 ) ) );

            M19 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 9 ) * 16 + bx ) ) );
            M29 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 9 ) * 16 + bx + 4 ) ) );
            M39 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 9 ) * 16 + bx + 8 ) ) );
            M49 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 9 ) * 16 + bx + 12 ) ) );

            M1A = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 10 ) * 16 + bx ) ) );
            M2A = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 10 ) * 16 + bx + 4 ) ) );
            M3A = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 10 ) * 16 + bx + 8 ) ) );
            M4A = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 10 ) * 16 + bx + 12 ) ) );

            M1B = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 11 ) * 16 + bx ) ) );
            M2B = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 11 ) * 16 + bx + 4 ) ) );
            M3B = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 11 ) * 16 + bx + 8 ) ) );
            M4B = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 11 ) * 16 + bx + 12 ) ) );

            M1C = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 12 ) * 16 + bx ) ) );
            M2C = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 12 ) * 16 + bx + 4 ) ) );
            M3C = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 12 ) * 16 + bx + 8 ) ) );
            M4C = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 12 ) * 16 + bx + 12 ) ) );

            M1D = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 13 ) * 16 + bx ) ) );
            M2D = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 13 ) * 16 + bx + 4 ) ) );
            M3D = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 13 ) * 16 + bx + 8 ) ) );
            M4D = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 13 ) * 16 + bx + 12 ) ) );

            M1E = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 14 ) * 16 + bx ) ) );
            M2E = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 14 ) * 16 + bx + 4 ) ) );
            M3E = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 14 ) * 16 + bx + 8 ) ) );
            M4E = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 14 ) * 16 + bx + 12 ) ) );

            M1F = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 15 ) * 16 + bx ) ) );
            M2F = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 15 ) * 16 + bx + 4 ) ) );
            M3F = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 15 ) * 16 + bx + 8 ) ) );
            M4F = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + ( by + 15 ) * 16 + bx + 12 ) ) );


            // GO NO
            mTemp10 = _mm_add_epi32( T10, M10 );
            mTemp20 = _mm_add_epi32( T20, M20 );
            mTemp30 = _mm_add_epi32( T30, M30 );
            mTemp40 = _mm_add_epi32( T40, M40 );

            mTemp11 = _mm_add_epi32( T11, M11 );
            mTemp21 = _mm_add_epi32( T21, M21 );
            mTemp31 = _mm_add_epi32( T31, M31 );
            mTemp41 = _mm_add_epi32( T41, M41 );

            mTemp12 = _mm_add_epi32( T12, M12 );
            mTemp22 = _mm_add_epi32( T22, M22 );
            mTemp32 = _mm_add_epi32( T32, M32 );
            mTemp42 = _mm_add_epi32( T42, M42 );

            mTemp13 = _mm_add_epi32( T13, M13 );
            mTemp23 = _mm_add_epi32( T23, M23 );
            mTemp33 = _mm_add_epi32( T33, M33 );
            mTemp43 = _mm_add_epi32( T43, M43 );

            mTemp14 = _mm_add_epi32( T14, M14 );
            mTemp24 = _mm_add_epi32( T24, M24 );
            mTemp34 = _mm_add_epi32( T34, M34 );
            mTemp44 = _mm_add_epi32( T44, M44 );

            mTemp15 = _mm_add_epi32( T15, M15 );
            mTemp25 = _mm_add_epi32( T25, M25 );
            mTemp35 = _mm_add_epi32( T35, M35 );
            mTemp45 = _mm_add_epi32( T45, M45 );

            mTemp16 = _mm_add_epi32( T16, M16 );
            mTemp26 = _mm_add_epi32( T26, M26 );
            mTemp36 = _mm_add_epi32( T36, M36 );
            mTemp46 = _mm_add_epi32( T46, M46 );

            mTemp17 = _mm_add_epi32( T17, M17 );
            mTemp27 = _mm_add_epi32( T27, M27 );
            mTemp37 = _mm_add_epi32( T37, M37 );
            mTemp47 = _mm_add_epi32( T47, M47 );

            mTemp18 = _mm_add_epi32( T18, M18 );
            mTemp28 = _mm_add_epi32( T28, M28 );
            mTemp38 = _mm_add_epi32( T38, M38 );
            mTemp48 = _mm_add_epi32( T48, M48 );

            mTemp19 = _mm_add_epi32( T19, M19 );
            mTemp29 = _mm_add_epi32( T29, M29 );
            mTemp39 = _mm_add_epi32( T39, M39 );
            mTemp49 = _mm_add_epi32( T49, M49 );

            mTemp1A = _mm_add_epi32( T1A, M1A );
            mTemp2A = _mm_add_epi32( T2A, M2A );
            mTemp3A = _mm_add_epi32( T3A, M3A );
            mTemp4A = _mm_add_epi32( T4A, M4A );

            mTemp1B = _mm_add_epi32( T1B, M1B );
            mTemp2B = _mm_add_epi32( T2B, M2B );
            mTemp3B = _mm_add_epi32( T3B, M3B );
            mTemp4B = _mm_add_epi32( T4B, M4B );

            mTemp1C = _mm_add_epi32( T1C, M1C );
            mTemp2C = _mm_add_epi32( T2C, M2C );
            mTemp3C = _mm_add_epi32( T3C, M3C );
            mTemp4C = _mm_add_epi32( T4C, M4C );

            mTemp1D = _mm_add_epi32( T1D, M1D );
            mTemp2D = _mm_add_epi32( T2D, M2D );
            mTemp3D = _mm_add_epi32( T3D, M3D );
            mTemp4D = _mm_add_epi32( T4D, M4D );

            mTemp1E = _mm_add_epi32( T1E, M1E );
            mTemp2E = _mm_add_epi32( T2E, M2E );
            mTemp3E = _mm_add_epi32( T3E, M3E );
            mTemp4E = _mm_add_epi32( T4E, M4E );

            mTemp1F = _mm_add_epi32( T1F, M1F );
            mTemp2F = _mm_add_epi32( T2F, M2F );
            mTemp3F = _mm_add_epi32( T3F, M3F );
            mTemp4F = _mm_add_epi32( T4F, M4F );

            mTemp10 = _mm_min_epi32( mMaxval, mTemp10 );
            mTemp10 = _mm_max_epi32( mMinval, mTemp10 );
            mTemp20 = _mm_min_epi32( mMaxval, mTemp20 );
            mTemp20 = _mm_max_epi32( mMinval, mTemp20 );
            mTemp30 = _mm_min_epi32( mMaxval, mTemp30 );
            mTemp30 = _mm_max_epi32( mMinval, mTemp30 );
            mTemp40 = _mm_min_epi32( mMaxval, mTemp40 );
            mTemp40 = _mm_max_epi32( mMinval, mTemp40 );

            mTemp11 = _mm_min_epi32( mMaxval, mTemp11 );
            mTemp11 = _mm_max_epi32( mMinval, mTemp11 );
            mTemp21 = _mm_min_epi32( mMaxval, mTemp21 );
            mTemp21 = _mm_max_epi32( mMinval, mTemp21 );
            mTemp31 = _mm_min_epi32( mMaxval, mTemp31 );
            mTemp31 = _mm_max_epi32( mMinval, mTemp31 );
            mTemp41 = _mm_min_epi32( mMaxval, mTemp41 );
            mTemp41 = _mm_max_epi32( mMinval, mTemp41 );

            mTemp12 = _mm_min_epi32( mMaxval, mTemp12 );
            mTemp12 = _mm_max_epi32( mMinval, mTemp12 );
            mTemp22 = _mm_min_epi32( mMaxval, mTemp22 );
            mTemp22 = _mm_max_epi32( mMinval, mTemp22 );
            mTemp32 = _mm_min_epi32( mMaxval, mTemp32 );
            mTemp32 = _mm_max_epi32( mMinval, mTemp32 );
            mTemp42 = _mm_min_epi32( mMaxval, mTemp42 );
            mTemp42 = _mm_max_epi32( mMinval, mTemp42 );

            mTemp13 = _mm_min_epi32( mMaxval, mTemp13 );
            mTemp13 = _mm_max_epi32( mMinval, mTemp13 );
            mTemp23 = _mm_min_epi32( mMaxval, mTemp23 );
            mTemp23 = _mm_max_epi32( mMinval, mTemp23 );
            mTemp33 = _mm_min_epi32( mMaxval, mTemp33 );
            mTemp33 = _mm_max_epi32( mMinval, mTemp33 );
            mTemp43 = _mm_min_epi32( mMaxval, mTemp43 );
            mTemp43 = _mm_max_epi32( mMinval, mTemp43 );

            mTemp14 = _mm_min_epi32( mMaxval, mTemp14 );
            mTemp14 = _mm_max_epi32( mMinval, mTemp14 );
            mTemp24 = _mm_min_epi32( mMaxval, mTemp24 );
            mTemp24 = _mm_max_epi32( mMinval, mTemp24 );
            mTemp34 = _mm_min_epi32( mMaxval, mTemp34 );
            mTemp34 = _mm_max_epi32( mMinval, mTemp34 );
            mTemp44 = _mm_min_epi32( mMaxval, mTemp44 );
            mTemp44 = _mm_max_epi32( mMinval, mTemp44 );

            mTemp15 = _mm_min_epi32( mMaxval, mTemp15 );
            mTemp15 = _mm_max_epi32( mMinval, mTemp15 );
            mTemp25 = _mm_min_epi32( mMaxval, mTemp25 );
            mTemp25 = _mm_max_epi32( mMinval, mTemp25 );
            mTemp35 = _mm_min_epi32( mMaxval, mTemp35 );
            mTemp35 = _mm_max_epi32( mMinval, mTemp35 );
            mTemp45 = _mm_min_epi32( mMaxval, mTemp45 );
            mTemp45 = _mm_max_epi32( mMinval, mTemp45 );

            mTemp16 = _mm_min_epi32( mMaxval, mTemp16 );
            mTemp16 = _mm_max_epi32( mMinval, mTemp16 );
            mTemp26 = _mm_min_epi32( mMaxval, mTemp26 );
            mTemp26 = _mm_max_epi32( mMinval, mTemp26 );
            mTemp36 = _mm_min_epi32( mMaxval, mTemp36 );
            mTemp36 = _mm_max_epi32( mMinval, mTemp36 );
            mTemp46 = _mm_min_epi32( mMaxval, mTemp46 );
            mTemp46 = _mm_max_epi32( mMinval, mTemp46 );

            mTemp17 = _mm_min_epi32( mMaxval, mTemp17 );
            mTemp17 = _mm_max_epi32( mMinval, mTemp17 );
            mTemp27 = _mm_min_epi32( mMaxval, mTemp27 );
            mTemp27 = _mm_max_epi32( mMinval, mTemp27 );
            mTemp37 = _mm_min_epi32( mMaxval, mTemp37 );
            mTemp37 = _mm_max_epi32( mMinval, mTemp37 );
            mTemp47 = _mm_min_epi32( mMaxval, mTemp47 );
            mTemp47 = _mm_max_epi32( mMinval, mTemp47 );

            mTemp18 = _mm_min_epi32( mMaxval, mTemp18 );
            mTemp18 = _mm_max_epi32( mMinval, mTemp18 );
            mTemp28 = _mm_min_epi32( mMaxval, mTemp28 );
            mTemp28 = _mm_max_epi32( mMinval, mTemp28 );
            mTemp38 = _mm_min_epi32( mMaxval, mTemp38 );
            mTemp38 = _mm_max_epi32( mMinval, mTemp38 );
            mTemp48 = _mm_min_epi32( mMaxval, mTemp48 );
            mTemp48 = _mm_max_epi32( mMinval, mTemp48 );

            mTemp19 = _mm_min_epi32( mMaxval, mTemp19 );
            mTemp19 = _mm_max_epi32( mMinval, mTemp19 );
            mTemp29 = _mm_min_epi32( mMaxval, mTemp29 );
            mTemp29 = _mm_max_epi32( mMinval, mTemp29 );
            mTemp39 = _mm_min_epi32( mMaxval, mTemp39 );
            mTemp39 = _mm_max_epi32( mMinval, mTemp39 );
            mTemp49 = _mm_min_epi32( mMaxval, mTemp49 );
            mTemp49 = _mm_max_epi32( mMinval, mTemp49 );

            mTemp1A = _mm_min_epi32( mMaxval, mTemp1A );
            mTemp1A = _mm_max_epi32( mMinval, mTemp1A );
            mTemp2A = _mm_min_epi32( mMaxval, mTemp2A );
            mTemp2A = _mm_max_epi32( mMinval, mTemp2A );
            mTemp3A = _mm_min_epi32( mMaxval, mTemp3A );
            mTemp3A = _mm_max_epi32( mMinval, mTemp3A );
            mTemp4A = _mm_min_epi32( mMaxval, mTemp4A );
            mTemp4A = _mm_max_epi32( mMinval, mTemp4A );

            mTemp1B = _mm_min_epi32( mMaxval, mTemp1B );
            mTemp1B = _mm_max_epi32( mMinval, mTemp1B );
            mTemp2B = _mm_min_epi32( mMaxval, mTemp2B );
            mTemp2B = _mm_max_epi32( mMinval, mTemp2B );
            mTemp3B = _mm_min_epi32( mMaxval, mTemp3B );
            mTemp3B = _mm_max_epi32( mMinval, mTemp3B );
            mTemp4B = _mm_min_epi32( mMaxval, mTemp4B );
            mTemp4B = _mm_max_epi32( mMinval, mTemp4B );

            mTemp1C = _mm_min_epi32( mMaxval, mTemp1C );
            mTemp1C = _mm_max_epi32( mMinval, mTemp1C );
            mTemp2C = _mm_min_epi32( mMaxval, mTemp2C );
            mTemp2C = _mm_max_epi32( mMinval, mTemp2C );
            mTemp3C = _mm_min_epi32( mMaxval, mTemp3C );
            mTemp3C = _mm_max_epi32( mMinval, mTemp3C );
            mTemp4C = _mm_min_epi32( mMaxval, mTemp4C );
            mTemp4C = _mm_max_epi32( mMinval, mTemp4C );

            mTemp1D = _mm_min_epi32( mMaxval, mTemp1D );
            mTemp1D = _mm_max_epi32( mMinval, mTemp1D );
            mTemp2D = _mm_min_epi32( mMaxval, mTemp2D );
            mTemp2D = _mm_max_epi32( mMinval, mTemp2D );
            mTemp3D = _mm_min_epi32( mMaxval, mTemp3D );
            mTemp3D = _mm_max_epi32( mMinval, mTemp3D );
            mTemp4D = _mm_min_epi32( mMaxval, mTemp4D );
            mTemp4D = _mm_max_epi32( mMinval, mTemp4D );

            mTemp1E = _mm_min_epi32( mMaxval, mTemp1E );
            mTemp1E = _mm_max_epi32( mMinval, mTemp1E );
            mTemp2E = _mm_min_epi32( mMaxval, mTemp2E );
            mTemp2E = _mm_max_epi32( mMinval, mTemp2E );
            mTemp3E = _mm_min_epi32( mMaxval, mTemp3E );
            mTemp3E = _mm_max_epi32( mMinval, mTemp3E );
            mTemp4E = _mm_min_epi32( mMaxval, mTemp4E );
            mTemp4E = _mm_max_epi32( mMinval, mTemp4E );

            mTemp1F = _mm_min_epi32( mMaxval, mTemp1F );
            mTemp1F = _mm_max_epi32( mMinval, mTemp1F );
            mTemp2F = _mm_min_epi32( mMaxval, mTemp2F );
            mTemp2F = _mm_max_epi32( mMinval, mTemp2F );
            mTemp3F = _mm_min_epi32( mMaxval, mTemp3F );
            mTemp3F = _mm_max_epi32( mMinval, mTemp3F );
            mTemp4F = _mm_min_epi32( mMaxval, mTemp4F );
            mTemp4F = _mm_max_epi32( mMinval, mTemp4F );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 0 * 16 ), mTemp10 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 0 * 16 + 4 ), mTemp20 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 0 * 16 + 8 ), mTemp30 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 0 * 16 + 12 ), mTemp40 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 1 * 16 ), mTemp11 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 1 * 16 + 4 ), mTemp21 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 1 * 16 + 8 ), mTemp31 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 1 * 16 + 12 ), mTemp41 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 16 ), mTemp12 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 16 + 4 ), mTemp22 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 16 + 8 ), mTemp32 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 16 + 12 ), mTemp42 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 16 ), mTemp13 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 16 + 4 ), mTemp23 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 16 + 8 ), mTemp33 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 16 + 12 ), mTemp43 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 16 ), mTemp14 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 16 + 4 ), mTemp24 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 16 + 8 ), mTemp34 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 16 + 12 ), mTemp44 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 16 ), mTemp15 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 16 + 4 ), mTemp25 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 16 + 8 ), mTemp35 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 16 + 12 ), mTemp45 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 16 ), mTemp16 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 16 + 4 ), mTemp26 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 16 + 8 ), mTemp36 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 16 + 12 ), mTemp46 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 16 ), mTemp17 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 16 + 4 ), mTemp27 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 16 + 8 ), mTemp37 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 16 + 12 ), mTemp47 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 * 16 ), mTemp18 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 * 16 + 4 ), mTemp28 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 * 16 + 8 ), mTemp38 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 * 16 + 12 ), mTemp48 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 9 * 16 ), mTemp19 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 9 * 16 + 4 ), mTemp29 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 9 * 16 + 8 ), mTemp39 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 9 * 16 + 12 ), mTemp49 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 10 * 16 ), mTemp1A );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 10 * 16 + 4 ), mTemp2A );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 10 * 16 + 8 ), mTemp3A );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 10 * 16 + 12 ), mTemp4A );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 11 * 16 ), mTemp1B );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 11 * 16 + 4 ), mTemp2B );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 11 * 16 + 8 ), mTemp3B );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 11 * 16 + 12 ), mTemp4B );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 * 16 ), mTemp1C );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 * 16 + 4 ), mTemp2C );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 * 16 + 8 ), mTemp3C );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 * 16 + 12 ), mTemp4C );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 13 * 16 ), mTemp1D );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 13 * 16 + 4 ), mTemp2D );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 13 * 16 + 8 ), mTemp3D );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 13 * 16 + 12 ), mTemp4D );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 14 * 16 ), mTemp1E );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 14 * 16 + 4 ), mTemp2E );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 14 * 16 + 8 ), mTemp3E );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 14 * 16 + 12 ), mTemp4E );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 15 * 16 ), mTemp1F );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 15 * 16 + 4 ), mTemp2F );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 15 * 16 + 8 ), mTemp3F );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 15 * 16 + 12 ), mTemp4F );

            // get pm7
            // transpose 16x16 matrix
            {
                __m128i tr0_0, tr0_1, tr0_2, tr0_3, tr0_4, tr0_5, tr0_6, tr0_7;

#define TRANSPOSE_8x8_16BIT(I0, I1, I2, I3, I4, I5, I6, I7, O0, O1, O2, O3, O4, O5, O6, O7) \
    tr0_0 = _mm_unpacklo_epi32(I0, I1); \
    tr0_1 = _mm_unpacklo_epi32(I2, I3); \
    tr0_2 = _mm_unpackhi_epi32(I0, I1); \
    tr0_3 = _mm_unpackhi_epi32(I2, I3); \
    tr0_4 = _mm_unpacklo_epi32(I4, I5); \
    tr0_5 = _mm_unpacklo_epi32(I6, I7); \
    tr0_6 = _mm_unpackhi_epi32(I4, I5); \
    tr0_7 = _mm_unpackhi_epi32(I6, I7); \
    O0 = _mm_unpacklo_epi64(tr0_0, tr0_1); \
    O1 = _mm_unpackhi_epi64(tr0_0, tr0_1); \
    O2 = _mm_unpacklo_epi64(tr0_2, tr0_3); \
    O3 = _mm_unpackhi_epi64(tr0_2, tr0_3); \
    O4 = _mm_unpacklo_epi64(tr0_4, tr0_5); \
    O5 = _mm_unpackhi_epi64(tr0_4, tr0_5); \
    O6 = _mm_unpacklo_epi64(tr0_6, tr0_7); \
    O7 = _mm_unpackhi_epi64(tr0_6, tr0_7); \


                TRANSPOSE_8x8_16BIT( mTemp10, mTemp11, mTemp12, mTemp13, mTemp14, mTemp15, mTemp16, mTemp17, T10, T11, T12, T13, T20, T21, T22, T23 )
                TRANSPOSE_8x8_16BIT( mTemp18, mTemp19, mTemp1A, mTemp1B, mTemp1C, mTemp1D, mTemp1E, mTemp1F, T30, T31, T32, T33, T40, T41, T42, T43 )
                TRANSPOSE_8x8_16BIT( mTemp20, mTemp21, mTemp22, mTemp23, mTemp24, mTemp25, mTemp26, mTemp27, T14, T15, T16, T17, T24, T25, T26, T27 )
                TRANSPOSE_8x8_16BIT( mTemp28, mTemp29, mTemp2A, mTemp2B, mTemp2C, mTemp2D, mTemp2E, mTemp2F, T34, T35, T36, T37, T44, T45, T46, T47 )
                TRANSPOSE_8x8_16BIT( mTemp30, mTemp31, mTemp32, mTemp33, mTemp34, mTemp35, mTemp36, mTemp37, T18, T19, T1A, T1B, T28, T29, T2A, T2B )
                TRANSPOSE_8x8_16BIT( mTemp38, mTemp39, mTemp3A, mTemp3B, mTemp3C, mTemp3D, mTemp3E, mTemp3F, T38, T39, T3A, T3B, T48, T49, T4A, T4B )
                TRANSPOSE_8x8_16BIT( mTemp40, mTemp41, mTemp42, mTemp43, mTemp44, mTemp45, mTemp46, mTemp47, T1C, T1D, T1E, T1F, T2C, T2D, T2E, T2F )
                TRANSPOSE_8x8_16BIT( mTemp48, mTemp49, mTemp4A, mTemp4B, mTemp4C, mTemp4D, mTemp4E, mTemp4F, T3C, T3D, T3E, T3F, T4C, T4D, T4E, T4F )

#undef TRANSPOSE_8x8_16BI

            }

            _mm_storeu_si128( ( __m128i* )( pm7[0] ), T10 );
            _mm_storeu_si128( ( __m128i* )( pm7[0] + 4 ), T20 );
            _mm_storeu_si128( ( __m128i* )( pm7[0] + 8 ), T30 );
            _mm_storeu_si128( ( __m128i* )( pm7[0] + 12 ), T40 );

            _mm_storeu_si128( ( __m128i* )( pm7[1] ), T11 );
            _mm_storeu_si128( ( __m128i* )( pm7[1] + 4 ), T21 );
            _mm_storeu_si128( ( __m128i* )( pm7[1] + 8 ), T31 );
            _mm_storeu_si128( ( __m128i* )( pm7[1] + 12 ), T41 );

            _mm_storeu_si128( ( __m128i* )( pm7[2] ), T12 );
            _mm_storeu_si128( ( __m128i* )( pm7[2] + 4 ), T22 );
            _mm_storeu_si128( ( __m128i* )( pm7[2] + 8 ), T32 );
            _mm_storeu_si128( ( __m128i* )( pm7[2] + 12 ), T42 );

            _mm_storeu_si128( ( __m128i* )( pm7[3] ), T13 );
            _mm_storeu_si128( ( __m128i* )( pm7[3] + 4 ), T23 );
            _mm_storeu_si128( ( __m128i* )( pm7[3] + 8 ), T33 );
            _mm_storeu_si128( ( __m128i* )( pm7[3] + 12 ), T43 );

            _mm_storeu_si128( ( __m128i* )( pm7[4] ), T14 );
            _mm_storeu_si128( ( __m128i* )( pm7[4] + 4 ), T24 );
            _mm_storeu_si128( ( __m128i* )( pm7[4] + 8 ), T34 );
            _mm_storeu_si128( ( __m128i* )( pm7[4] + 12 ), T44 );

            _mm_storeu_si128( ( __m128i* )( pm7[5] ), T15 );
            _mm_storeu_si128( ( __m128i* )( pm7[5] + 4 ), T25 );
            _mm_storeu_si128( ( __m128i* )( pm7[5] + 8 ), T35 );
            _mm_storeu_si128( ( __m128i* )( pm7[5] + 12 ), T45 );

            _mm_storeu_si128( ( __m128i* )( pm7[6] ), T16 );
            _mm_storeu_si128( ( __m128i* )( pm7[6] + 4 ), T26 );
            _mm_storeu_si128( ( __m128i* )( pm7[6] + 8 ), T36 );
            _mm_storeu_si128( ( __m128i* )( pm7[6] + 12 ), T46 );

            _mm_storeu_si128( ( __m128i* )( pm7[7] ), T17 );
            _mm_storeu_si128( ( __m128i* )( pm7[7] + 4 ), T27 );
            _mm_storeu_si128( ( __m128i* )( pm7[7] + 8 ), T37 );
            _mm_storeu_si128( ( __m128i* )( pm7[7] + 12 ), T47 );

            _mm_storeu_si128( ( __m128i* )( pm7[8] ), T18 );
            _mm_storeu_si128( ( __m128i* )( pm7[8] + 4 ), T28 );
            _mm_storeu_si128( ( __m128i* )( pm7[8] + 8 ), T38 );
            _mm_storeu_si128( ( __m128i* )( pm7[8] + 12 ), T48 );

            _mm_storeu_si128( ( __m128i* )( pm7[9] ), T19 );
            _mm_storeu_si128( ( __m128i* )( pm7[9] + 4 ), T29 );
            _mm_storeu_si128( ( __m128i* )( pm7[9] + 8 ), T39 );
            _mm_storeu_si128( ( __m128i* )( pm7[9] + 12 ), T49 );

            _mm_storeu_si128( ( __m128i* )( pm7[10] ), T1A );
            _mm_storeu_si128( ( __m128i* )( pm7[10] + 4 ), T2A );
            _mm_storeu_si128( ( __m128i* )( pm7[10] + 8 ), T3A );
            _mm_storeu_si128( ( __m128i* )( pm7[10] + 12 ), T4A );

            _mm_storeu_si128( ( __m128i* )( pm7[11] ), T1B );
            _mm_storeu_si128( ( __m128i* )( pm7[11] + 4 ), T2B );
            _mm_storeu_si128( ( __m128i* )( pm7[11] + 8 ), T3B );
            _mm_storeu_si128( ( __m128i* )( pm7[11] + 12 ), T4B );

            _mm_storeu_si128( ( __m128i* )( pm7[12] ), T1C );
            _mm_storeu_si128( ( __m128i* )( pm7[12] + 4 ), T2C );
            _mm_storeu_si128( ( __m128i* )( pm7[12] + 8 ), T3C );
            _mm_storeu_si128( ( __m128i* )( pm7[12] + 12 ), T4C );

            _mm_storeu_si128( ( __m128i* )( pm7[13] ), T1D );
            _mm_storeu_si128( ( __m128i* )( pm7[13] + 4 ), T2D );
            _mm_storeu_si128( ( __m128i* )( pm7[13] + 8 ), T3D );
            _mm_storeu_si128( ( __m128i* )( pm7[13] + 12 ), T4D );

            _mm_storeu_si128( ( __m128i* )( pm7[14] ), T1E );
            _mm_storeu_si128( ( __m128i* )( pm7[14] + 4 ), T2E );
            _mm_storeu_si128( ( __m128i* )( pm7[14] + 8 ), T3E );
            _mm_storeu_si128( ( __m128i* )( pm7[14] + 12 ), T4E );

            _mm_storeu_si128( ( __m128i* )( pm7[15] ), T1F );
            _mm_storeu_si128( ( __m128i* )( pm7[15] + 4 ), T2F );
            _mm_storeu_si128( ( __m128i* )( pm7[15] + 8 ), T3F );
            _mm_storeu_si128( ( __m128i* )( pm7[15] + 12 ), T4F );

            // get imgY_rec
            T10 = _mm_packus_epi32( mTemp10, mZero );
            T20 = _mm_packus_epi32( mTemp20, mZero );
            T30 = _mm_packus_epi32( mTemp30, mZero );
            T40 = _mm_packus_epi32( mTemp40, mZero );

            T11 = _mm_packus_epi32( mTemp11, mZero );
            T21 = _mm_packus_epi32( mTemp21, mZero );
            T31 = _mm_packus_epi32( mTemp31, mZero );
            T41 = _mm_packus_epi32( mTemp41, mZero );

            T12 = _mm_packus_epi32( mTemp12, mZero );
            T22 = _mm_packus_epi32( mTemp22, mZero );
            T32 = _mm_packus_epi32( mTemp32, mZero );
            T42 = _mm_packus_epi32( mTemp42, mZero );

            T13 = _mm_packus_epi32( mTemp13, mZero );
            T23 = _mm_packus_epi32( mTemp23, mZero );
            T33 = _mm_packus_epi32( mTemp33, mZero );
            T43 = _mm_packus_epi32( mTemp43, mZero );

            T14 = _mm_packus_epi32( mTemp14, mZero );
            T24 = _mm_packus_epi32( mTemp24, mZero );
            T34 = _mm_packus_epi32( mTemp34, mZero );
            T44 = _mm_packus_epi32( mTemp44, mZero );

            T15 = _mm_packus_epi32( mTemp15, mZero );
            T25 = _mm_packus_epi32( mTemp25, mZero );
            T35 = _mm_packus_epi32( mTemp35, mZero );
            T45 = _mm_packus_epi32( mTemp45, mZero );

            T16 = _mm_packus_epi32( mTemp16, mZero );
            T26 = _mm_packus_epi32( mTemp26, mZero );
            T36 = _mm_packus_epi32( mTemp36, mZero );
            T46 = _mm_packus_epi32( mTemp46, mZero );

            T17 = _mm_packus_epi32( mTemp17, mZero );
            T27 = _mm_packus_epi32( mTemp27, mZero );
            T37 = _mm_packus_epi32( mTemp37, mZero );
            T47 = _mm_packus_epi32( mTemp47, mZero );

            T18 = _mm_packus_epi32( mTemp18, mZero );
            T28 = _mm_packus_epi32( mTemp28, mZero );
            T38 = _mm_packus_epi32( mTemp38, mZero );
            T48 = _mm_packus_epi32( mTemp48, mZero );

            T19 = _mm_packus_epi32( mTemp19, mZero );
            T29 = _mm_packus_epi32( mTemp29, mZero );
            T39 = _mm_packus_epi32( mTemp39, mZero );
            T49 = _mm_packus_epi32( mTemp49, mZero );

            T1A = _mm_packus_epi32( mTemp1A, mZero );
            T2A = _mm_packus_epi32( mTemp2A, mZero );
            T3A = _mm_packus_epi32( mTemp3A, mZero );
            T4A = _mm_packus_epi32( mTemp4A, mZero );

            T1B = _mm_packus_epi32( mTemp1B, mZero );
            T2B = _mm_packus_epi32( mTemp2B, mZero );
            T3B = _mm_packus_epi32( mTemp3B, mZero );
            T4B = _mm_packus_epi32( mTemp4B, mZero );

            T1C = _mm_packus_epi32( mTemp1C, mZero );
            T2C = _mm_packus_epi32( mTemp2C, mZero );
            T3C = _mm_packus_epi32( mTemp3C, mZero );
            T4C = _mm_packus_epi32( mTemp4C, mZero );

            T1D = _mm_packus_epi32( mTemp1D, mZero );
            T2D = _mm_packus_epi32( mTemp2D, mZero );
            T3D = _mm_packus_epi32( mTemp3D, mZero );
            T4D = _mm_packus_epi32( mTemp4D, mZero );

            T1E = _mm_packus_epi32( mTemp1E, mZero );
            T2E = _mm_packus_epi32( mTemp2E, mZero );
            T3E = _mm_packus_epi32( mTemp3E, mZero );
            T4E = _mm_packus_epi32( mTemp4E, mZero );

            T1F = _mm_packus_epi32( mTemp1F, mZero );
            T2F = _mm_packus_epi32( mTemp2F, mZero );
            T3F = _mm_packus_epi32( mTemp3F, mZero );
            T4F = _mm_packus_epi32( mTemp4F, mZero );

            T10 = _mm_packus_epi16( T10, mZero );
            T20 = _mm_packus_epi16( T20, mZero );
            T30 = _mm_packus_epi16( T30, mZero );
            T40 = _mm_packus_epi16( T40, mZero );

            T11 = _mm_packus_epi16( T11, mZero );
            T21 = _mm_packus_epi16( T21, mZero );
            T31 = _mm_packus_epi16( T31, mZero );
            T41 = _mm_packus_epi16( T41, mZero );

            T12 = _mm_packus_epi16( T12, mZero );
            T22 = _mm_packus_epi16( T22, mZero );
            T32 = _mm_packus_epi16( T32, mZero );
            T42 = _mm_packus_epi16( T42, mZero );

            T13 = _mm_packus_epi16( T13, mZero );
            T23 = _mm_packus_epi16( T23, mZero );
            T33 = _mm_packus_epi16( T33, mZero );
            T43 = _mm_packus_epi16( T43, mZero );

            T14 = _mm_packus_epi16( T14, mZero );
            T24 = _mm_packus_epi16( T24, mZero );
            T34 = _mm_packus_epi16( T34, mZero );
            T44 = _mm_packus_epi16( T44, mZero );

            T15 = _mm_packus_epi16( T15, mZero );
            T25 = _mm_packus_epi16( T25, mZero );
            T35 = _mm_packus_epi16( T35, mZero );
            T45 = _mm_packus_epi16( T45, mZero );

            T16 = _mm_packus_epi16( T16, mZero );
            T26 = _mm_packus_epi16( T26, mZero );
            T36 = _mm_packus_epi16( T36, mZero );
            T46 = _mm_packus_epi16( T46, mZero );

            T17 = _mm_packus_epi16( T17, mZero );
            T27 = _mm_packus_epi16( T27, mZero );
            T37 = _mm_packus_epi16( T37, mZero );
            T47 = _mm_packus_epi16( T47, mZero );

            T18 = _mm_packus_epi16( T18, mZero );
            T28 = _mm_packus_epi16( T28, mZero );
            T38 = _mm_packus_epi16( T38, mZero );
            T48 = _mm_packus_epi16( T48, mZero );

            T19 = _mm_packus_epi16( T19, mZero );
            T29 = _mm_packus_epi16( T29, mZero );
            T39 = _mm_packus_epi16( T39, mZero );
            T49 = _mm_packus_epi16( T49, mZero );

            T1A = _mm_packus_epi16( T1A, mZero );
            T2A = _mm_packus_epi16( T2A, mZero );
            T3A = _mm_packus_epi16( T3A, mZero );
            T4A = _mm_packus_epi16( T4A, mZero );

            T1B = _mm_packus_epi16( T1B, mZero );
            T2B = _mm_packus_epi16( T2B, mZero );
            T3B = _mm_packus_epi16( T3B, mZero );
            T4B = _mm_packus_epi16( T4B, mZero );

            T1C = _mm_packus_epi16( T1C, mZero );
            T2C = _mm_packus_epi16( T2C, mZero );
            T3C = _mm_packus_epi16( T3C, mZero );
            T4C = _mm_packus_epi16( T4C, mZero );

            T1D = _mm_packus_epi16( T1D, mZero );
            T2D = _mm_packus_epi16( T2D, mZero );
            T3D = _mm_packus_epi16( T3D, mZero );
            T4D = _mm_packus_epi16( T4D, mZero );

            T1E = _mm_packus_epi16( T1E, mZero );
            T2E = _mm_packus_epi16( T2E, mZero );
            T3E = _mm_packus_epi16( T3E, mZero );
            T4E = _mm_packus_epi16( T4E, mZero );

            T1F = _mm_packus_epi16( T1F, mZero );
            T2F = _mm_packus_epi16( T2F, mZero );
            T3F = _mm_packus_epi16( T3F, mZero );
            T4F = _mm_packus_epi16( T4F, mZero );

            // pack the data
            T10 = _mm_unpacklo_epi32( T10, T20 );
            T20 = _mm_unpacklo_epi32( T30, T40 );
            T10 = _mm_unpacklo_epi64( T10, T20 );

            T11 = _mm_unpacklo_epi32( T11, T21 );
            T21 = _mm_unpacklo_epi32( T31, T41 );
            T11 = _mm_unpacklo_epi64( T11, T21 );

            T12 = _mm_unpacklo_epi32( T12, T22 );
            T22 = _mm_unpacklo_epi32( T32, T42 );
            T12 = _mm_unpacklo_epi64( T12, T22 );

            T13 = _mm_unpacklo_epi32( T13, T23 );
            T23 = _mm_unpacklo_epi32( T33, T43 );
            T13 = _mm_unpacklo_epi64( T13, T23 );

            T14 = _mm_unpacklo_epi32( T14, T24 );
            T24 = _mm_unpacklo_epi32( T34, T44 );
            T14 = _mm_unpacklo_epi64( T14, T24 );

            T15 = _mm_unpacklo_epi32( T15, T25 );
            T25 = _mm_unpacklo_epi32( T35, T45 );
            T15 = _mm_unpacklo_epi64( T15, T25 );

            T16 = _mm_unpacklo_epi32( T16, T26 );
            T26 = _mm_unpacklo_epi32( T36, T46 );
            T16 = _mm_unpacklo_epi64( T16, T26 );

            T17 = _mm_unpacklo_epi32( T17, T27 );
            T27 = _mm_unpacklo_epi32( T37, T47 );
            T17 = _mm_unpacklo_epi64( T17, T27 );

            T18 = _mm_unpacklo_epi32( T18, T28 );
            T28 = _mm_unpacklo_epi32( T38, T48 );
            T18 = _mm_unpacklo_epi64( T18, T28 );

            T19 = _mm_unpacklo_epi32( T19, T29 );
            T29 = _mm_unpacklo_epi32( T39, T49 );
            T19 = _mm_unpacklo_epi64( T19, T29 );

            T1A = _mm_unpacklo_epi32( T1A, T2A );
            T2A = _mm_unpacklo_epi32( T3A, T4A );
            T1A = _mm_unpacklo_epi64( T1A, T2A );

            T1B = _mm_unpacklo_epi32( T1B, T2B );
            T2B = _mm_unpacklo_epi32( T3B, T4B );
            T1B = _mm_unpacklo_epi64( T1B, T2B );

            T1C = _mm_unpacklo_epi32( T1C, T2C );
            T2C = _mm_unpacklo_epi32( T3C, T4C );
            T1C = _mm_unpacklo_epi64( T1C, T2C );

            T1D = _mm_unpacklo_epi32( T1D, T2D );
            T2D = _mm_unpacklo_epi32( T3D, T4D );
            T1D = _mm_unpacklo_epi64( T1D, T2D );

            T1E = _mm_unpacklo_epi32( T1E, T2E );
            T2E = _mm_unpacklo_epi32( T3E, T4E );
            T1E = _mm_unpacklo_epi64( T1E, T2E );

            T1F = _mm_unpacklo_epi32( T1F, T2F );
            T2F = _mm_unpacklo_epi32( T3F, T4F );
            T1F = _mm_unpacklo_epi64( T1F, T2F );

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 0 )*iStride + ipix_x + bx ), T10 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 0)*iStride + ipix_x + bx + 4), T20);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 0)*iStride + ipix_x + bx + 8), T30);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 0)*iStride + ipix_x + bx + 12), T40);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 1 )*iStride + ipix_x + bx ), T11 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 1)*iStride + ipix_x + bx + 4), T21);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 1)*iStride + ipix_x + bx + 8), T31);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 1)*iStride + ipix_x + bx + 12), T41);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 2 )*iStride + ipix_x + bx ), T12 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 2)*iStride + ipix_x + bx + 4), T22);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 2)*iStride + ipix_x + bx + 8), T32);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 2)*iStride + ipix_x + bx + 12), T42);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 3 )*iStride + ipix_x + bx ), T13 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 3)*iStride + ipix_x + bx + 4), T23);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 3)*iStride + ipix_x + bx + 8), T33);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 3)*iStride + ipix_x + bx + 12), T43);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 4 )*iStride + ipix_x + bx ), T14 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 4)*iStride + ipix_x + bx + 4), T24);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 4)*iStride + ipix_x + bx + 8), T34);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 4)*iStride + ipix_x + bx + 12), T44);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 5 )*iStride + ipix_x + bx ), T15 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 5)*iStride + ipix_x + bx + 4), T25);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 5)*iStride + ipix_x + bx + 8), T35);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 5)*iStride + ipix_x + bx + 12), T45);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 6 )*iStride + ipix_x + bx ), T16 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 6)*iStride + ipix_x + bx + 4), T26);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 6)*iStride + ipix_x + bx + 8), T36);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 6)*iStride + ipix_x + bx + 12), T46);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 7 )*iStride + ipix_x + bx ), T17 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 7)*iStride + ipix_x + bx + 4), T27);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 7)*iStride + ipix_x + bx + 8), T37);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 7)*iStride + ipix_x + bx + 12), T47);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 8 )*iStride + ipix_x + bx ), T18 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 8)*iStride + ipix_x + bx + 4), T28);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 8)*iStride + ipix_x + bx + 8), T38);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 8)*iStride + ipix_x + bx + 12), T48);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 9 )*iStride + ipix_x + bx ), T19 );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 9)*iStride + ipix_x + bx + 4), T29);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 9)*iStride + ipix_x + bx + 8), T39);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 9)*iStride + ipix_x + bx + 12), T49);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 10 )*iStride + ipix_x + bx ), T1A );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 10)*iStride + ipix_x + bx + 4), T2A);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 10)*iStride + ipix_x + bx + 8), T3A);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 10)*iStride + ipix_x + bx + 12), T4A);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 11 )*iStride + ipix_x + bx ), T1B );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 11)*iStride + ipix_x + bx + 4), T2B);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 11)*iStride + ipix_x + bx + 8), T3B);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 11)*iStride + ipix_x + bx + 12), T4B);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 12 )*iStride + ipix_x + bx ), T1C );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 12)*iStride + ipix_x + bx + 4), T2C);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 12)*iStride + ipix_x + bx + 8), T3C);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 12)*iStride + ipix_x + bx + 12), T4C);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 13 )*iStride + ipix_x + bx ), T1D );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 13)*iStride + ipix_x + bx + 4), T2D);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 13)*iStride + ipix_x + bx + 8), T3D);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 13)*iStride + ipix_x + bx + 12), T4D);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 14 )*iStride + ipix_x + bx ), T1E );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 14)*iStride + ipix_x + bx + 4), T2E);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 14)*iStride + ipix_x + bx + 8), T3E);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 14)*iStride + ipix_x + bx + 12), T4E);

            _mm_storeu_si128( ( __m128i* )( imgY + ( ipix_y + by + 15 )*iStride + ipix_x + bx ), T1F );
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 15)*iStride + ipix_x + bx + 4), T2F);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 15)*iStride + ipix_x + bx + 8), T3F);
            //_mm_storeu_si128((__m128i*)(imgY_rec + (ipix_y + by + 15)*iStride + ipix_x + bx + 12), T4F);

        }

    }
    else
    {
        if ( 4 == bsize )
        {
            // get curr_blk value
            T0 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 ) );
            T1 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 ) );
            T2 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 4 ) );
            T3 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 4 ) );

            M0 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk ) ) );
            M1 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 1 * 16 ) ) );
            M2 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 2 * 16 ) ) );
            M3 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 3 * 16 ) ) );

            mTemp0 = _mm_add_epi32( T0, M0 );
            mTemp1 = _mm_add_epi32( T1, M1 );
            mTemp2 = _mm_add_epi32( T2, M2 );
            mTemp3 = _mm_add_epi32( T3, M3 );

            mTemp0 = _mm_min_epi32( mMaxval, mTemp0 );
            mTemp0 = _mm_max_epi32( mMinval, mTemp0 );
            mTemp1 = _mm_min_epi32( mMaxval, mTemp1 );
            mTemp1 = _mm_max_epi32( mMinval, mTemp1 );
            mTemp2 = _mm_min_epi32( mMaxval, mTemp2 );
            mTemp2 = _mm_max_epi32( mMinval, mTemp2 );
            mTemp3 = _mm_min_epi32( mMaxval, mTemp3 );
            mTemp3 = _mm_max_epi32( mMinval, mTemp3 );

            _mm_storeu_si128( ( __m128i* )( curr_blk + 0 ), mTemp0 );
            _mm_storeu_si128( ( __m128i* )( curr_blk + 4 ), mTemp1 );
            _mm_storeu_si128( ( __m128i* )( curr_blk + 2 * 4 ), mTemp2 );
            _mm_storeu_si128( ( __m128i* )( curr_blk + 3 * 4 ), mTemp3 );

            // get pm7
            // transpose 4x4 matrix
            M0 = _mm_unpacklo_epi32( mTemp0, mTemp1 );
            M1 = _mm_unpacklo_epi32( mTemp2, mTemp3 );
            M2 = _mm_unpackhi_epi32( mTemp0, mTemp1 );
            M3 = _mm_unpackhi_epi32( mTemp2, mTemp3 );

            T0 = _mm_unpacklo_epi64( M0, M1 );
            T1 = _mm_unpackhi_epi64( M0, M1 );
            T2 = _mm_unpacklo_epi64( M2, M3 );
            T3 = _mm_unpackhi_epi64( M2, M3 );

            _mm_storeu_si128( ( __m128i* )( pm7 + 0 ), T0 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 1 ), T1 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 2 ), T2 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 3 ), T3 );

            // get imgV/U_rec
            T0 = _mm_packus_epi32( mTemp0, mZero );
            T1 = _mm_packus_epi32( mTemp1, mZero );
            T2 = _mm_packus_epi32( mTemp2, mZero );
            T3 = _mm_packus_epi32( mTemp3, mZero );

            T0 = _mm_packus_epi16( T0, mZero );
            T1 = _mm_packus_epi16( T1, mZero );
            T2 = _mm_packus_epi16( T2, mZero );
            T3 = _mm_packus_epi16( T3, mZero );

            if ( ( b8 - 4 ) % 2 )
            {
                _mm_maskmoveu_si128( T0, mMask, ( imgV + ( ipix_c_y )*iStrideC + ipix_c_x ) );
                _mm_maskmoveu_si128( T1, mMask, ( imgV + ( ipix_c_y + 1 )*iStrideC + ipix_c_x ) );
                _mm_maskmoveu_si128( T2, mMask, ( imgV + ( ipix_c_y + 2 )*iStrideC + ipix_c_x ) );
                _mm_maskmoveu_si128( T3, mMask, ( imgV + ( ipix_c_y + 3 )*iStrideC + ipix_c_x ) );

                /*_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y)*iStrideC + ipix_c_x), T0);
                _mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x), T1);
                _mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x), T2);
                _mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x), T3);*/
            }
            else
            {
                _mm_maskmoveu_si128( T0, mMask, ( imgU + ( ipix_c_y )*iStrideC + ipix_c_x ) );
                _mm_maskmoveu_si128( T1, mMask, ( imgU + ( ipix_c_y + 1 )*iStrideC + ipix_c_x ) );
                _mm_maskmoveu_si128( T2, mMask, ( imgU + ( ipix_c_y + 2 )*iStrideC + ipix_c_x ) );
                _mm_maskmoveu_si128( T3, mMask, ( imgU + ( ipix_c_y + 3 )*iStrideC + ipix_c_x ) );

                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y)*iStrideC + ipix_c_x), T0);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x), T1);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x), T2);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x), T3);
            }

        }
        else if ( 8 == bsize )
        {
            // get curr_blk value
            T10 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 ) );
            T20 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 ) );
            T11 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 ) );
            T21 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 + 4 ) );
            T12 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 8 ) );
            T22 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 8 + 4 ) );
            T13 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 8 ) );
            T23 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 8 + 4 ) );
            T14 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 8 ) );
            T24 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 8 + 4 ) );
            T15 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 8 ) );
            T25 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 8 + 4 ) );
            T16 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 8 ) );
            T26 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 8 + 4 ) );
            T17 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 8 ) );
            T27 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 8 + 4 ) );

            M10 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk ) ) );
            M20 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 4 ) ) );

            M11 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 1 * 16 ) ) );
            M21 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 1 * 16 + 4 ) ) );

            M12 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 2 * 16 ) ) );
            M22 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 2 * 16 +  4 ) ) );

            M13 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 3 * 16 ) ) );
            M23 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 3 * 16 +  4 ) ) );

            M14 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 4 * 16 ) ) );
            M24 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 4 * 16 +  4 ) ) );

            M15 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 5 * 16 ) ) );
            M25 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 5 * 16 +  4 ) ) );

            M16 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 6 * 16 ) ) );
            M26 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 6 * 16 +  4 ) ) );

            M17 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 7 * 16 ) ) );
            M27 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 7 * 16 +  4 ) ) );

            mTemp10 = _mm_add_epi32( T10, M10 );
            mTemp20 = _mm_add_epi32( T20, M20 );
            mTemp11 = _mm_add_epi32( T11, M11 );
            mTemp21 = _mm_add_epi32( T21, M21 );
            mTemp12 = _mm_add_epi32( T12, M12 );
            mTemp22 = _mm_add_epi32( T22, M22 );
            mTemp13 = _mm_add_epi32( T13, M13 );
            mTemp23 = _mm_add_epi32( T23, M23 );
            mTemp14 = _mm_add_epi32( T14, M14 );
            mTemp24 = _mm_add_epi32( T24, M24 );
            mTemp15 = _mm_add_epi32( T15, M15 );
            mTemp25 = _mm_add_epi32( T25, M25 );
            mTemp16 = _mm_add_epi32( T16, M16 );
            mTemp26 = _mm_add_epi32( T26, M26 );
            mTemp17 = _mm_add_epi32( T17, M17 );
            mTemp27 = _mm_add_epi32( T27, M27 );

            mTemp10 = _mm_min_epi32( mMaxval, mTemp10 );
            mTemp10 = _mm_max_epi32( mMinval, mTemp10 );
            mTemp20 = _mm_min_epi32( mMaxval, mTemp20 );
            mTemp20 = _mm_max_epi32( mMinval, mTemp20 );

            mTemp11 = _mm_min_epi32( mMaxval, mTemp11 );
            mTemp11 = _mm_max_epi32( mMinval, mTemp11 );
            mTemp21 = _mm_min_epi32( mMaxval, mTemp21 );
            mTemp21 = _mm_max_epi32( mMinval, mTemp21 );

            mTemp12 = _mm_min_epi32( mMaxval, mTemp12 );
            mTemp12 = _mm_max_epi32( mMinval, mTemp12 );
            mTemp22 = _mm_min_epi32( mMaxval, mTemp22 );
            mTemp22 = _mm_max_epi32( mMinval, mTemp22 );

            mTemp13 = _mm_min_epi32( mMaxval, mTemp13 );
            mTemp13 = _mm_max_epi32( mMinval, mTemp13 );
            mTemp23 = _mm_min_epi32( mMaxval, mTemp23 );
            mTemp23 = _mm_max_epi32( mMinval, mTemp23 );

            mTemp14 = _mm_min_epi32( mMaxval, mTemp14 );
            mTemp14 = _mm_max_epi32( mMinval, mTemp14 );
            mTemp24 = _mm_min_epi32( mMaxval, mTemp24 );
            mTemp24 = _mm_max_epi32( mMinval, mTemp24 );

            mTemp15 = _mm_min_epi32( mMaxval, mTemp15 );
            mTemp15 = _mm_max_epi32( mMinval, mTemp15 );
            mTemp25 = _mm_min_epi32( mMaxval, mTemp25 );
            mTemp25 = _mm_max_epi32( mMinval, mTemp25 );

            mTemp16 = _mm_min_epi32( mMaxval, mTemp16 );
            mTemp16 = _mm_max_epi32( mMinval, mTemp16 );
            mTemp26 = _mm_min_epi32( mMaxval, mTemp26 );
            mTemp26 = _mm_max_epi32( mMinval, mTemp26 );

            mTemp17 = _mm_min_epi32( mMaxval, mTemp17 );
            mTemp17 = _mm_max_epi32( mMinval, mTemp17 );
            mTemp27 = _mm_min_epi32( mMaxval, mTemp27 );
            mTemp27 = _mm_max_epi32( mMinval, mTemp27 );

            _mm_storeu_si128( ( __m128i * )curr_blk, mTemp10 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 ), mTemp20 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 ), mTemp11 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 + 4 ), mTemp21 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 8 ), mTemp12 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 8 + 4 ), mTemp22 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 8 ), mTemp13 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 8 + 4 ), mTemp23 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 8 ), mTemp14 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 8 + 4 ), mTemp24 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 8 ), mTemp15 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 8 + 4 ), mTemp25 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 8 ), mTemp16 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 8 + 4 ), mTemp26 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 8 ), mTemp17 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 8 + 4 ), mTemp27 );

            // get pm7
            // transpose 8x8 matrix
            M10 = _mm_unpacklo_epi32( mTemp10, mTemp11 );
            M11 = _mm_unpackhi_epi32( mTemp10, mTemp11 );
            M20 = _mm_unpacklo_epi32( mTemp20, mTemp21 );
            M21 = _mm_unpackhi_epi32( mTemp20, mTemp21 );
            M12 = _mm_unpacklo_epi32( mTemp12, mTemp13 );
            M13 = _mm_unpackhi_epi32( mTemp12, mTemp13 );
            M22 = _mm_unpacklo_epi32( mTemp22, mTemp23 );
            M23 = _mm_unpackhi_epi32( mTemp22, mTemp23 );
            M14 = _mm_unpacklo_epi32( mTemp14, mTemp15 );
            M15 = _mm_unpackhi_epi32( mTemp14, mTemp15 );
            M24 = _mm_unpacklo_epi32( mTemp24, mTemp25 );
            M25 = _mm_unpackhi_epi32( mTemp24, mTemp25 );
            M16 = _mm_unpacklo_epi32( mTemp16, mTemp17 );
            M17 = _mm_unpackhi_epi32( mTemp16, mTemp17 );
            M26 = _mm_unpacklo_epi32( mTemp26, mTemp27 );
            M27 = _mm_unpackhi_epi32( mTemp26, mTemp27 );

            T10 = _mm_unpacklo_epi64( M10, M12 );
            T11 = _mm_unpackhi_epi64( M10, M12 );
            T20 = _mm_unpacklo_epi64( M14, M16 );
            T21 = _mm_unpackhi_epi64( M14, M16 );
            T12 = _mm_unpacklo_epi64( M11, M13 );
            T13 = _mm_unpackhi_epi64( M11, M13 );
            T22 = _mm_unpacklo_epi64( M15, M17 );
            T23 = _mm_unpackhi_epi64( M15, M17 );
            T14 = _mm_unpacklo_epi64( M20, M22 );
            T15 = _mm_unpackhi_epi64( M20, M22 );
            T24 = _mm_unpacklo_epi64( M24, M26 );
            T25 = _mm_unpackhi_epi64( M24, M26 );
            T16 = _mm_unpacklo_epi64( M21, M23 );
            T17 = _mm_unpackhi_epi64( M21, M23 );
            T26 = _mm_unpacklo_epi64( M25, M27 );
            T27 = _mm_unpackhi_epi64( M25, M27 );

            _mm_storeu_si128( ( __m128i* )( pm7[0] ), T10 );
            _mm_storeu_si128( ( __m128i* )( pm7[0] + 4 ), T20 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 1 ), T11 );
            _mm_storeu_si128( ( __m128i* )( pm7[1] + 4 ), T21 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 2 ), T12 );
            _mm_storeu_si128( ( __m128i* )( pm7[2] + 4 ), T22 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 3 ), T13 );
            _mm_storeu_si128( ( __m128i* )( pm7[3] + 4 ), T23 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 4 ), T14 );
            _mm_storeu_si128( ( __m128i* )( pm7[4] + 4 ), T24 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 5 ), T15 );
            _mm_storeu_si128( ( __m128i* )( pm7[5] + 4 ), T25 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 6 ), T16 );
            _mm_storeu_si128( ( __m128i* )( pm7[6] + 4 ), T26 );
            _mm_storeu_si128( ( __m128i* )( pm7 + 7 ), T17 );
            _mm_storeu_si128( ( __m128i* )( pm7[7] + 4 ), T27 );

            // get imgY_rec
            T10 = _mm_packus_epi32( mTemp10, mZero );
            T20 = _mm_packus_epi32( mTemp20, mZero );
            T11 = _mm_packus_epi32( mTemp11, mZero );
            T21 = _mm_packus_epi32( mTemp21, mZero );
            T12 = _mm_packus_epi32( mTemp12, mZero );
            T22 = _mm_packus_epi32( mTemp22, mZero );
            T13 = _mm_packus_epi32( mTemp13, mZero );
            T23 = _mm_packus_epi32( mTemp23, mZero );
            T14 = _mm_packus_epi32( mTemp14, mZero );
            T24 = _mm_packus_epi32( mTemp24, mZero );
            T15 = _mm_packus_epi32( mTemp15, mZero );
            T25 = _mm_packus_epi32( mTemp25, mZero );
            T16 = _mm_packus_epi32( mTemp16, mZero );
            T26 = _mm_packus_epi32( mTemp26, mZero );
            T17 = _mm_packus_epi32( mTemp17, mZero );
            T27 = _mm_packus_epi32( mTemp27, mZero );

            T10 = _mm_packus_epi16( T10, mZero );
            T20 = _mm_packus_epi16( T20, mZero );
            T11 = _mm_packus_epi16( T11, mZero );
            T21 = _mm_packus_epi16( T21, mZero );
            T12 = _mm_packus_epi16( T12, mZero );
            T22 = _mm_packus_epi16( T22, mZero );
            T13 = _mm_packus_epi16( T13, mZero );
            T23 = _mm_packus_epi16( T23, mZero );
            T14 = _mm_packus_epi16( T14, mZero );
            T24 = _mm_packus_epi16( T24, mZero );
            T15 = _mm_packus_epi16( T15, mZero );
            T25 = _mm_packus_epi16( T25, mZero );
            T16 = _mm_packus_epi16( T16, mZero );
            T26 = _mm_packus_epi16( T26, mZero );
            T17 = _mm_packus_epi16( T17, mZero );
            T27 = _mm_packus_epi16( T27, mZero );

            // pack data
            T10 = _mm_unpacklo_epi32( T10, T20 );
            T11 = _mm_unpacklo_epi32( T11, T21 );
            T12 = _mm_unpacklo_epi32( T12, T22 );
            T13 = _mm_unpacklo_epi32( T13, T23 );
            T14 = _mm_unpacklo_epi32( T14, T24 );
            T15 = _mm_unpacklo_epi32( T15, T25 );
            T16 = _mm_unpacklo_epi32( T16, T26 );
            T17 = _mm_unpacklo_epi32( T17, T27 );

            if ( ( b8 - 4 ) % 2 )
            {
                _mm_storel_epi64( ( __m128i* )( imgV + ( ipix_c_y )*iStrideC + ipix_c_x ), T10 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y)*iStrideC + ipix_c_x + 4), T20);
                _mm_storel_epi64( ( __m128i* )( imgV + ( ipix_c_y + 1 )*iStrideC + ipix_c_x ), T11 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x + 4), T21);
                _mm_storel_epi64( ( __m128i* )( imgV + ( ipix_c_y + 2 )*iStrideC + ipix_c_x ), T12 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x + 4), T22);
                _mm_storel_epi64( ( __m128i* )( imgV + ( ipix_c_y + 3 )*iStrideC + ipix_c_x ), T13 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x + 4), T23);
                _mm_storel_epi64( ( __m128i* )( imgV + ( ipix_c_y + 4 )*iStrideC + ipix_c_x ), T14 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 4)*iStrideC + ipix_c_x + 4), T24);
                _mm_storel_epi64( ( __m128i* )( imgV + ( ipix_c_y + 5 )*iStrideC + ipix_c_x ), T15 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 5)*iStrideC + ipix_c_x + 4), T25);
                _mm_storel_epi64( ( __m128i* )( imgV + ( ipix_c_y + 6 )*iStrideC + ipix_c_x ), T16 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 6)*iStrideC + ipix_c_x + 4), T26);
                _mm_storel_epi64( ( __m128i* )( imgV + ( ipix_c_y + 7 )*iStrideC + ipix_c_x ), T17 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 7)*iStrideC + ipix_c_x + 4), T27);
            }
            else
            {
                _mm_storel_epi64( ( __m128i* )( imgU + ( ipix_c_y )*iStrideC + ipix_c_x ), T10 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y)*iStrideC + ipix_c_x + 4), T20);
                _mm_storel_epi64( ( __m128i* )( imgU + ( ipix_c_y + 1 )*iStrideC + ipix_c_x ), T11 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x + 4), T21);
                _mm_storel_epi64( ( __m128i* )( imgU + ( ipix_c_y + 2 )*iStrideC + ipix_c_x ), T12 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x + 4), T22);
                _mm_storel_epi64( ( __m128i* )( imgU + ( ipix_c_y + 3 )*iStrideC + ipix_c_x ), T13 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x + 4), T23);
                _mm_storel_epi64( ( __m128i* )( imgU + ( ipix_c_y + 4 )*iStrideC + ipix_c_x ), T14 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 4)*iStrideC + ipix_c_x + 4), T24);
                _mm_storel_epi64( ( __m128i* )( imgU + ( ipix_c_y + 5 )*iStrideC + ipix_c_x ), T15 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 5)*iStrideC + ipix_c_x + 4), T25);
                _mm_storel_epi64( ( __m128i* )( imgU + ( ipix_c_y + 6 )*iStrideC + ipix_c_x ), T16 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 6)*iStrideC + ipix_c_x + 4), T26);
                _mm_storel_epi64( ( __m128i* )( imgU + ( ipix_c_y + 7 )*iStrideC + ipix_c_x ), T17 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 7)*iStrideC + ipix_c_x + 4), T27);
            }

        }
        else
        {
            // get curr_blk value
            T10 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 * 16 + 0 ) );
            T20 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 * 16 + 4 ) );
            T30 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 * 16 + 8 ) );
            T40 = _mm_loadu_si128( ( __m128i* )( curr_blk + 0 * 16 + 12 ) );

            T11 = _mm_loadu_si128( ( __m128i* )( curr_blk + 1 * 16 + 0 ) );
            T21 = _mm_loadu_si128( ( __m128i* )( curr_blk + 1 * 16 + 4 ) );
            T31 = _mm_loadu_si128( ( __m128i* )( curr_blk + 1 * 16 + 8 ) );
            T41 = _mm_loadu_si128( ( __m128i* )( curr_blk + 1 * 16 + 12 ) );

            T12 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 16 + 0 ) );
            T22 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 16 + 4 ) );
            T32 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 16 + 8 ) );
            T42 = _mm_loadu_si128( ( __m128i* )( curr_blk + 2 * 16 + 12 ) );

            T13 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 16 + 0 ) );
            T23 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 16 + 4 ) );
            T33 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 16 + 8 ) );
            T43 = _mm_loadu_si128( ( __m128i* )( curr_blk + 3 * 16 + 12 ) );

            T14 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 16 + 0 ) );
            T24 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 16 + 4 ) );
            T34 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 16 + 8 ) );
            T44 = _mm_loadu_si128( ( __m128i* )( curr_blk + 4 * 16 + 12 ) );

            T15 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 16 + 0 ) );
            T25 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 16 + 4 ) );
            T35 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 16 + 8 ) );
            T45 = _mm_loadu_si128( ( __m128i* )( curr_blk + 5 * 16 + 12 ) );

            T16 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 16 + 0 ) );
            T26 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 16 + 4 ) );
            T36 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 16 + 8 ) );
            T46 = _mm_loadu_si128( ( __m128i* )( curr_blk + 6 * 16 + 12 ) );

            T17 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 16 + 0 ) );
            T27 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 16 + 4 ) );
            T37 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 16 + 8 ) );
            T47 = _mm_loadu_si128( ( __m128i* )( curr_blk + 7 * 16 + 12 ) );

            T18 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 * 16 + 0 ) );
            T28 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 * 16 + 4 ) );
            T38 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 * 16 + 8 ) );
            T48 = _mm_loadu_si128( ( __m128i* )( curr_blk + 8 * 16 + 12 ) );

            T19 = _mm_loadu_si128( ( __m128i* )( curr_blk + 9 * 16 + 0 ) );
            T29 = _mm_loadu_si128( ( __m128i* )( curr_blk + 9 * 16 + 4 ) );
            T39 = _mm_loadu_si128( ( __m128i* )( curr_blk + 9 * 16 + 8 ) );
            T49 = _mm_loadu_si128( ( __m128i* )( curr_blk + 9 * 16 + 12 ) );

            T1A = _mm_loadu_si128( ( __m128i* )( curr_blk + 10 * 16 + 0 ) );
            T2A = _mm_loadu_si128( ( __m128i* )( curr_blk + 10 * 16 + 4 ) );
            T3A = _mm_loadu_si128( ( __m128i* )( curr_blk + 10 * 16 + 8 ) );
            T4A = _mm_loadu_si128( ( __m128i* )( curr_blk + 10 * 16 + 12 ) );

            T1B = _mm_loadu_si128( ( __m128i* )( curr_blk + 11 * 16 + 0 ) );
            T2B = _mm_loadu_si128( ( __m128i* )( curr_blk + 11 * 16 + 4 ) );
            T3B = _mm_loadu_si128( ( __m128i* )( curr_blk + 11 * 16 + 8 ) );
            T4B = _mm_loadu_si128( ( __m128i* )( curr_blk + 11 * 16 + 12 ) );

            T1C = _mm_loadu_si128( ( __m128i* )( curr_blk + 12 * 16 + 0 ) );
            T2C = _mm_loadu_si128( ( __m128i* )( curr_blk + 12 * 16 + 4 ) );
            T3C = _mm_loadu_si128( ( __m128i* )( curr_blk + 12 * 16 + 8 ) );
            T4C = _mm_loadu_si128( ( __m128i* )( curr_blk + 12 * 16 + 12 ) );

            T1D = _mm_loadu_si128( ( __m128i* )( curr_blk + 13 * 16 + 0 ) );
            T2D = _mm_loadu_si128( ( __m128i* )( curr_blk + 13 * 16 + 4 ) );
            T3D = _mm_loadu_si128( ( __m128i* )( curr_blk + 13 * 16 + 8 ) );
            T4D = _mm_loadu_si128( ( __m128i* )( curr_blk + 13 * 16 + 12 ) );

            T1E = _mm_loadu_si128( ( __m128i* )( curr_blk + 14 * 16 + 0 ) );
            T2E = _mm_loadu_si128( ( __m128i* )( curr_blk + 14 * 16 + 4 ) );
            T3E = _mm_loadu_si128( ( __m128i* )( curr_blk + 14 * 16 + 8 ) );
            T4E = _mm_loadu_si128( ( __m128i* )( curr_blk + 14 * 16 + 12 ) );

            T1F = _mm_loadu_si128( ( __m128i* )( curr_blk + 15 * 16 + 0 ) );
            T2F = _mm_loadu_si128( ( __m128i* )( curr_blk + 15 * 16 + 4 ) );
            T3F = _mm_loadu_si128( ( __m128i* )( curr_blk + 15 * 16 + 8 ) );
            T4F = _mm_loadu_si128( ( __m128i* )( curr_blk + 15 * 16 + 12 ) );

            M10 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 0 * 16 ) ) );
            M20 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 0 * 16 + 4 ) ) );
            M30 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 0 * 16 + 8 ) ) );
            M40 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 0 * 16 + 12 ) ) );

            M11 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 1 * 16 ) ) );
            M21 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 1 * 16 + 4 ) ) );
            M31 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 1 * 16 + 8 ) ) );
            M41 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 1 * 16 + 12 ) ) );

            M12 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 2 * 16 ) ) );
            M22 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 2 * 16 + 4 ) ) );
            M32 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 2 * 16 + 8 ) ) );
            M42 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 2 * 16 + 12 ) ) );

            M13 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 3 * 16 ) ) );
            M23 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 3 * 16 + 4 ) ) );
            M33 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 3 * 16 + 8 ) ) );
            M43 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 3 * 16 + 12 ) ) );

            M14 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 4 * 16 ) ) );
            M24 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 4 * 16 + 4 ) ) );
            M34 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 4 * 16 + 8 ) ) );
            M44 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 4 * 16 + 12 ) ) );

            M15 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 5 * 16 ) ) );
            M25 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 5 * 16 + 4 ) ) );
            M35 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 5 * 16 + 8 ) ) );
            M45 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 5 * 16 + 12 ) ) );

            M16 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 6 * 16 ) ) );
            M26 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 6 * 16 + 4 ) ) );
            M36 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 6 * 16 + 8 ) ) );
            M46 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 6 * 16 + 12 ) ) );

            M17 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 7 * 16 ) ) );
            M27 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 7 * 16 + 4 ) ) );
            M37 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 7 * 16 + 8 ) ) );
            M47 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 7 * 16 + 12 ) ) );

            M18 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 8 * 16 ) ) );
            M28 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 8 * 16 + 4 ) ) );
            M38 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 8 * 16 + 8 ) ) );
            M48 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 8 * 16 + 12 ) ) );

            M19 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 9 * 16 ) ) );
            M29 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 9 * 16 + 4 ) ) );
            M39 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 9 * 16 + 8 ) ) );
            M49 = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 9 * 16 + 12 ) ) );

            M1A = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 10 * 16 ) ) );
            M2A = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 10 * 16 + 4 ) ) );
            M3A = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 10 * 16 + 8 ) ) );
            M4A = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 10 * 16 + 12 ) ) );

            M1B = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 11 * 16 ) ) );
            M2B = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 11 * 16 + 4 ) ) );
            M3B = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 11 * 16 + 8 ) ) );
            M4B = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 11 * 16 + 12 ) ) );

            M1C = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 12 * 16 ) ) );
            M2C = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 12 * 16 + 4 ) ) );
            M3C = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 12 * 16 + 8 ) ) );
            M4C = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 12 * 16 + 12 ) ) );

            M1D = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 13 * 16 ) ) );
            M2D = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 13 * 16 + 4 ) ) );
            M3D = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 13 * 16 + 8 ) ) );
            M4D = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 13 * 16 + 12 ) ) );

            M1E = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 14 * 16 ) ) );
            M2E = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 14 * 16 + 4 ) ) );
            M3E = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 14 * 16 + 8 ) ) );
            M4E = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 14 * 16 + 12 ) ) );

            M1F = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 15 * 16 ) ) );
            M2F = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 15 * 16 + 4 ) ) );
            M3F = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 15 * 16 + 8 ) ) );
            M4F = _mm_cvtepu8_epi32( _mm_loadu_si128( ( __m128i* )( ppredblk + 15 * 16 + 12 ) ) );


            // GO NO
            mTemp10 = _mm_add_epi32( T10, M10 );
            mTemp20 = _mm_add_epi32( T20, M20 );
            mTemp30 = _mm_add_epi32( T30, M30 );
            mTemp40 = _mm_add_epi32( T40, M40 );

            mTemp11 = _mm_add_epi32( T11, M11 );
            mTemp21 = _mm_add_epi32( T21, M21 );
            mTemp31 = _mm_add_epi32( T31, M31 );
            mTemp41 = _mm_add_epi32( T41, M41 );

            mTemp12 = _mm_add_epi32( T12, M12 );
            mTemp22 = _mm_add_epi32( T22, M22 );
            mTemp32 = _mm_add_epi32( T32, M32 );
            mTemp42 = _mm_add_epi32( T42, M42 );

            mTemp13 = _mm_add_epi32( T13, M13 );
            mTemp23 = _mm_add_epi32( T23, M23 );
            mTemp33 = _mm_add_epi32( T33, M33 );
            mTemp43 = _mm_add_epi32( T43, M43 );

            mTemp14 = _mm_add_epi32( T14, M14 );
            mTemp24 = _mm_add_epi32( T24, M24 );
            mTemp34 = _mm_add_epi32( T34, M34 );
            mTemp44 = _mm_add_epi32( T44, M44 );

            mTemp15 = _mm_add_epi32( T15, M15 );
            mTemp25 = _mm_add_epi32( T25, M25 );
            mTemp35 = _mm_add_epi32( T35, M35 );
            mTemp45 = _mm_add_epi32( T45, M45 );

            mTemp16 = _mm_add_epi32( T16, M16 );
            mTemp26 = _mm_add_epi32( T26, M26 );
            mTemp36 = _mm_add_epi32( T36, M36 );
            mTemp46 = _mm_add_epi32( T46, M46 );

            mTemp17 = _mm_add_epi32( T17, M17 );
            mTemp27 = _mm_add_epi32( T27, M27 );
            mTemp37 = _mm_add_epi32( T37, M37 );
            mTemp47 = _mm_add_epi32( T47, M47 );

            mTemp18 = _mm_add_epi32( T18, M18 );
            mTemp28 = _mm_add_epi32( T28, M28 );
            mTemp38 = _mm_add_epi32( T38, M38 );
            mTemp48 = _mm_add_epi32( T48, M48 );

            mTemp19 = _mm_add_epi32( T19, M19 );
            mTemp29 = _mm_add_epi32( T29, M29 );
            mTemp39 = _mm_add_epi32( T39, M39 );
            mTemp49 = _mm_add_epi32( T49, M49 );

            mTemp1A = _mm_add_epi32( T1A, M1A );
            mTemp2A = _mm_add_epi32( T2A, M2A );
            mTemp3A = _mm_add_epi32( T3A, M3A );
            mTemp4A = _mm_add_epi32( T4A, M4A );

            mTemp1B = _mm_add_epi32( T1B, M1B );
            mTemp2B = _mm_add_epi32( T2B, M2B );
            mTemp3B = _mm_add_epi32( T3B, M3B );
            mTemp4B = _mm_add_epi32( T4B, M4B );

            mTemp1C = _mm_add_epi32( T1C, M1C );
            mTemp2C = _mm_add_epi32( T2C, M2C );
            mTemp3C = _mm_add_epi32( T3C, M3C );
            mTemp4C = _mm_add_epi32( T4C, M4C );

            mTemp1D = _mm_add_epi32( T1D, M1D );
            mTemp2D = _mm_add_epi32( T2D, M2D );
            mTemp3D = _mm_add_epi32( T3D, M3D );
            mTemp4D = _mm_add_epi32( T4D, M4D );

            mTemp1E = _mm_add_epi32( T1E, M1E );
            mTemp2E = _mm_add_epi32( T2E, M2E );
            mTemp3E = _mm_add_epi32( T3E, M3E );
            mTemp4E = _mm_add_epi32( T4E, M4E );

            mTemp1F = _mm_add_epi32( T1F, M1F );
            mTemp2F = _mm_add_epi32( T2F, M2F );
            mTemp3F = _mm_add_epi32( T3F, M3F );
            mTemp4F = _mm_add_epi32( T4F, M4F );

            mTemp10 = _mm_min_epi32( mMaxval, mTemp10 );
            mTemp10 = _mm_max_epi32( mMinval, mTemp10 );
            mTemp20 = _mm_min_epi32( mMaxval, mTemp20 );
            mTemp20 = _mm_max_epi32( mMinval, mTemp20 );
            mTemp30 = _mm_min_epi32( mMaxval, mTemp30 );
            mTemp30 = _mm_max_epi32( mMinval, mTemp30 );
            mTemp40 = _mm_min_epi32( mMaxval, mTemp40 );
            mTemp40 = _mm_max_epi32( mMinval, mTemp40 );

            mTemp11 = _mm_min_epi32( mMaxval, mTemp11 );
            mTemp11 = _mm_max_epi32( mMinval, mTemp11 );
            mTemp21 = _mm_min_epi32( mMaxval, mTemp21 );
            mTemp21 = _mm_max_epi32( mMinval, mTemp21 );
            mTemp31 = _mm_min_epi32( mMaxval, mTemp31 );
            mTemp31 = _mm_max_epi32( mMinval, mTemp31 );
            mTemp41 = _mm_min_epi32( mMaxval, mTemp41 );
            mTemp41 = _mm_max_epi32( mMinval, mTemp41 );

            mTemp12 = _mm_min_epi32( mMaxval, mTemp12 );
            mTemp12 = _mm_max_epi32( mMinval, mTemp12 );
            mTemp22 = _mm_min_epi32( mMaxval, mTemp22 );
            mTemp22 = _mm_max_epi32( mMinval, mTemp22 );
            mTemp32 = _mm_min_epi32( mMaxval, mTemp32 );
            mTemp32 = _mm_max_epi32( mMinval, mTemp32 );
            mTemp42 = _mm_min_epi32( mMaxval, mTemp42 );
            mTemp42 = _mm_max_epi32( mMinval, mTemp42 );

            mTemp13 = _mm_min_epi32( mMaxval, mTemp13 );
            mTemp13 = _mm_max_epi32( mMinval, mTemp13 );
            mTemp23 = _mm_min_epi32( mMaxval, mTemp23 );
            mTemp23 = _mm_max_epi32( mMinval, mTemp23 );
            mTemp33 = _mm_min_epi32( mMaxval, mTemp33 );
            mTemp33 = _mm_max_epi32( mMinval, mTemp33 );
            mTemp43 = _mm_min_epi32( mMaxval, mTemp43 );
            mTemp43 = _mm_max_epi32( mMinval, mTemp43 );

            mTemp14 = _mm_min_epi32( mMaxval, mTemp14 );
            mTemp14 = _mm_max_epi32( mMinval, mTemp14 );
            mTemp24 = _mm_min_epi32( mMaxval, mTemp24 );
            mTemp24 = _mm_max_epi32( mMinval, mTemp24 );
            mTemp34 = _mm_min_epi32( mMaxval, mTemp34 );
            mTemp34 = _mm_max_epi32( mMinval, mTemp34 );
            mTemp44 = _mm_min_epi32( mMaxval, mTemp44 );
            mTemp44 = _mm_max_epi32( mMinval, mTemp44 );

            mTemp15 = _mm_min_epi32( mMaxval, mTemp15 );
            mTemp15 = _mm_max_epi32( mMinval, mTemp15 );
            mTemp25 = _mm_min_epi32( mMaxval, mTemp25 );
            mTemp25 = _mm_max_epi32( mMinval, mTemp25 );
            mTemp35 = _mm_min_epi32( mMaxval, mTemp35 );
            mTemp35 = _mm_max_epi32( mMinval, mTemp35 );
            mTemp45 = _mm_min_epi32( mMaxval, mTemp45 );
            mTemp45 = _mm_max_epi32( mMinval, mTemp45 );

            mTemp16 = _mm_min_epi32( mMaxval, mTemp16 );
            mTemp16 = _mm_max_epi32( mMinval, mTemp16 );
            mTemp26 = _mm_min_epi32( mMaxval, mTemp26 );
            mTemp26 = _mm_max_epi32( mMinval, mTemp26 );
            mTemp36 = _mm_min_epi32( mMaxval, mTemp36 );
            mTemp36 = _mm_max_epi32( mMinval, mTemp36 );
            mTemp46 = _mm_min_epi32( mMaxval, mTemp46 );
            mTemp46 = _mm_max_epi32( mMinval, mTemp46 );

            mTemp17 = _mm_min_epi32( mMaxval, mTemp17 );
            mTemp17 = _mm_max_epi32( mMinval, mTemp17 );
            mTemp27 = _mm_min_epi32( mMaxval, mTemp27 );
            mTemp27 = _mm_max_epi32( mMinval, mTemp27 );
            mTemp37 = _mm_min_epi32( mMaxval, mTemp37 );
            mTemp37 = _mm_max_epi32( mMinval, mTemp37 );
            mTemp47 = _mm_min_epi32( mMaxval, mTemp47 );
            mTemp47 = _mm_max_epi32( mMinval, mTemp47 );

            mTemp18 = _mm_min_epi32( mMaxval, mTemp18 );
            mTemp18 = _mm_max_epi32( mMinval, mTemp18 );
            mTemp28 = _mm_min_epi32( mMaxval, mTemp28 );
            mTemp28 = _mm_max_epi32( mMinval, mTemp28 );
            mTemp38 = _mm_min_epi32( mMaxval, mTemp38 );
            mTemp38 = _mm_max_epi32( mMinval, mTemp38 );
            mTemp48 = _mm_min_epi32( mMaxval, mTemp48 );
            mTemp48 = _mm_max_epi32( mMinval, mTemp48 );

            mTemp19 = _mm_min_epi32( mMaxval, mTemp19 );
            mTemp19 = _mm_max_epi32( mMinval, mTemp19 );
            mTemp29 = _mm_min_epi32( mMaxval, mTemp29 );
            mTemp29 = _mm_max_epi32( mMinval, mTemp29 );
            mTemp39 = _mm_min_epi32( mMaxval, mTemp39 );
            mTemp39 = _mm_max_epi32( mMinval, mTemp39 );
            mTemp49 = _mm_min_epi32( mMaxval, mTemp49 );
            mTemp49 = _mm_max_epi32( mMinval, mTemp49 );

            mTemp1A = _mm_min_epi32( mMaxval, mTemp1A );
            mTemp1A = _mm_max_epi32( mMinval, mTemp1A );
            mTemp2A = _mm_min_epi32( mMaxval, mTemp2A );
            mTemp2A = _mm_max_epi32( mMinval, mTemp2A );
            mTemp3A = _mm_min_epi32( mMaxval, mTemp3A );
            mTemp3A = _mm_max_epi32( mMinval, mTemp3A );
            mTemp4A = _mm_min_epi32( mMaxval, mTemp4A );
            mTemp4A = _mm_max_epi32( mMinval, mTemp4A );

            mTemp1B = _mm_min_epi32( mMaxval, mTemp1B );
            mTemp1B = _mm_max_epi32( mMinval, mTemp1B );
            mTemp2B = _mm_min_epi32( mMaxval, mTemp2B );
            mTemp2B = _mm_max_epi32( mMinval, mTemp2B );
            mTemp3B = _mm_min_epi32( mMaxval, mTemp3B );
            mTemp3B = _mm_max_epi32( mMinval, mTemp3B );
            mTemp4B = _mm_min_epi32( mMaxval, mTemp4B );
            mTemp4B = _mm_max_epi32( mMinval, mTemp4B );

            mTemp1C = _mm_min_epi32( mMaxval, mTemp1C );
            mTemp1C = _mm_max_epi32( mMinval, mTemp1C );
            mTemp2C = _mm_min_epi32( mMaxval, mTemp2C );
            mTemp2C = _mm_max_epi32( mMinval, mTemp2C );
            mTemp3C = _mm_min_epi32( mMaxval, mTemp3C );
            mTemp3C = _mm_max_epi32( mMinval, mTemp3C );
            mTemp4C = _mm_min_epi32( mMaxval, mTemp4C );
            mTemp4C = _mm_max_epi32( mMinval, mTemp4C );

            mTemp1D = _mm_min_epi32( mMaxval, mTemp1D );
            mTemp1D = _mm_max_epi32( mMinval, mTemp1D );
            mTemp2D = _mm_min_epi32( mMaxval, mTemp2D );
            mTemp2D = _mm_max_epi32( mMinval, mTemp2D );
            mTemp3D = _mm_min_epi32( mMaxval, mTemp3D );
            mTemp3D = _mm_max_epi32( mMinval, mTemp3D );
            mTemp4D = _mm_min_epi32( mMaxval, mTemp4D );
            mTemp4D = _mm_max_epi32( mMinval, mTemp4D );

            mTemp1E = _mm_min_epi32( mMaxval, mTemp1E );
            mTemp1E = _mm_max_epi32( mMinval, mTemp1E );
            mTemp2E = _mm_min_epi32( mMaxval, mTemp2E );
            mTemp2E = _mm_max_epi32( mMinval, mTemp2E );
            mTemp3E = _mm_min_epi32( mMaxval, mTemp3E );
            mTemp3E = _mm_max_epi32( mMinval, mTemp3E );
            mTemp4E = _mm_min_epi32( mMaxval, mTemp4E );
            mTemp4E = _mm_max_epi32( mMinval, mTemp4E );

            mTemp1F = _mm_min_epi32( mMaxval, mTemp1F );
            mTemp1F = _mm_max_epi32( mMinval, mTemp1F );
            mTemp2F = _mm_min_epi32( mMaxval, mTemp2F );
            mTemp2F = _mm_max_epi32( mMinval, mTemp2F );
            mTemp3F = _mm_min_epi32( mMaxval, mTemp3F );
            mTemp3F = _mm_max_epi32( mMinval, mTemp3F );
            mTemp4F = _mm_min_epi32( mMaxval, mTemp4F );
            mTemp4F = _mm_max_epi32( mMinval, mTemp4F );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 0 * 16 ), mTemp10 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 0 * 16 + 4 ), mTemp20 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 0 * 16 + 8 ), mTemp30 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 0 * 16 + 12 ), mTemp40 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 1 * 16 ), mTemp11 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 1 * 16 + 4 ), mTemp21 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 1 * 16 + 8 ), mTemp31 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 1 * 16 + 12 ), mTemp41 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 16 ), mTemp12 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 16 + 4 ), mTemp22 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 16 + 8 ), mTemp32 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 2 * 16 + 12 ), mTemp42 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 16 ), mTemp13 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 16 + 4 ), mTemp23 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 16 + 8 ), mTemp33 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 3 * 16 + 12 ), mTemp43 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 16 ), mTemp14 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 16 + 4 ), mTemp24 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 16 + 8 ), mTemp34 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 4 * 16 + 12 ), mTemp44 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 16 ), mTemp15 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 16 + 4 ), mTemp25 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 16 + 8 ), mTemp35 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 5 * 16 + 12 ), mTemp45 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 16 ), mTemp16 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 16 + 4 ), mTemp26 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 16 + 8 ), mTemp36 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 6 * 16 + 12 ), mTemp46 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 16 ), mTemp17 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 16 + 4 ), mTemp27 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 16 + 8 ), mTemp37 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 7 * 16 + 12 ), mTemp47 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 * 16 ), mTemp18 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 * 16 + 4 ), mTemp28 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 * 16 + 8 ), mTemp38 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 8 * 16 + 12 ), mTemp48 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 9 * 16 ), mTemp19 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 9 * 16 + 4 ), mTemp29 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 9 * 16 + 8 ), mTemp39 );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 9 * 16 + 12 ), mTemp49 );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 10 * 16 ), mTemp1A );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 10 * 16 + 4 ), mTemp2A );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 10 * 16 + 8 ), mTemp3A );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 10 * 16 + 12 ), mTemp4A );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 11 * 16 ), mTemp1B );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 11 * 16 + 4 ), mTemp2B );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 11 * 16 + 8 ), mTemp3B );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 11 * 16 + 12 ), mTemp4B );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 * 16 ), mTemp1C );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 * 16 + 4 ), mTemp2C );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 * 16 + 8 ), mTemp3C );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 12 * 16 + 12 ), mTemp4C );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 13 * 16 ), mTemp1D );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 13 * 16 + 4 ), mTemp2D );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 13 * 16 + 8 ), mTemp3D );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 13 * 16 + 12 ), mTemp4D );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 14 * 16 ), mTemp1E );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 14 * 16 + 4 ), mTemp2E );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 14 * 16 + 8 ), mTemp3E );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 14 * 16 + 12 ), mTemp4E );

            _mm_storeu_si128( ( __m128i * )( curr_blk + 15 * 16 ), mTemp1F );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 15 * 16 + 4 ), mTemp2F );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 15 * 16 + 8 ), mTemp3F );
            _mm_storeu_si128( ( __m128i * )( curr_blk + 15 * 16 + 12 ), mTemp4F );

            // get pm7
            // transpose 16x16 matrix
            {
                __m128i tr0_0, tr0_1, tr0_2, tr0_3, tr0_4, tr0_5, tr0_6, tr0_7;

#define TRANSPOSE_8x8_16BIT(I0, I1, I2, I3, I4, I5, I6, I7, O0, O1, O2, O3, O4, O5, O6, O7) \
    tr0_0 = _mm_unpacklo_epi32(I0, I1); \
    tr0_1 = _mm_unpacklo_epi32(I2, I3); \
    tr0_2 = _mm_unpackhi_epi32(I0, I1); \
    tr0_3 = _mm_unpackhi_epi32(I2, I3); \
    tr0_4 = _mm_unpacklo_epi32(I4, I5); \
    tr0_5 = _mm_unpacklo_epi32(I6, I7); \
    tr0_6 = _mm_unpackhi_epi32(I4, I5); \
    tr0_7 = _mm_unpackhi_epi32(I6, I7); \
    O0 = _mm_unpacklo_epi64(tr0_0, tr0_1); \
    O1 = _mm_unpackhi_epi64(tr0_0, tr0_1); \
    O2 = _mm_unpacklo_epi64(tr0_2, tr0_3); \
    O3 = _mm_unpackhi_epi64(tr0_2, tr0_3); \
    O4 = _mm_unpacklo_epi64(tr0_4, tr0_5); \
    O5 = _mm_unpackhi_epi64(tr0_4, tr0_5); \
    O6 = _mm_unpacklo_epi64(tr0_6, tr0_7); \
    O7 = _mm_unpackhi_epi64(tr0_6, tr0_7); \


                TRANSPOSE_8x8_16BIT( mTemp10, mTemp11, mTemp12, mTemp13, mTemp14, mTemp15, mTemp16, mTemp17, T10, T11, T12, T13, T20, T21, T22, T23 )
                TRANSPOSE_8x8_16BIT( mTemp18, mTemp19, mTemp1A, mTemp1B, mTemp1C, mTemp1D, mTemp1E, mTemp1F, T30, T31, T32, T33, T40, T41, T42, T43 )
                TRANSPOSE_8x8_16BIT( mTemp20, mTemp21, mTemp22, mTemp23, mTemp24, mTemp25, mTemp26, mTemp27, T14, T15, T16, T17, T24, T25, T26, T27 )
                TRANSPOSE_8x8_16BIT( mTemp28, mTemp29, mTemp2A, mTemp2B, mTemp2C, mTemp2D, mTemp2E, mTemp2F, T34, T35, T36, T37, T44, T45, T46, T47 )
                TRANSPOSE_8x8_16BIT( mTemp30, mTemp31, mTemp32, mTemp33, mTemp34, mTemp35, mTemp36, mTemp37, T18, T19, T1A, T1B, T28, T29, T2A, T2B )
                TRANSPOSE_8x8_16BIT( mTemp38, mTemp39, mTemp3A, mTemp3B, mTemp3C, mTemp3D, mTemp3E, mTemp3F, T38, T39, T3A, T3B, T48, T49, T4A, T4B )
                TRANSPOSE_8x8_16BIT( mTemp40, mTemp41, mTemp42, mTemp43, mTemp44, mTemp45, mTemp46, mTemp47, T1C, T1D, T1E, T1F, T2C, T2D, T2E, T2F )
                TRANSPOSE_8x8_16BIT( mTemp48, mTemp49, mTemp4A, mTemp4B, mTemp4C, mTemp4D, mTemp4E, mTemp4F, T3C, T3D, T3E, T3F, T4C, T4D, T4E, T4F )

#undef TRANSPOSE_8x8_16BI

            }

            _mm_storeu_si128( ( __m128i* )( pm7[0] ), T10 );
            _mm_storeu_si128( ( __m128i* )( pm7[0] + 4 ), T20 );
            _mm_storeu_si128( ( __m128i* )( pm7[0] + 8 ), T30 );
            _mm_storeu_si128( ( __m128i* )( pm7[0] + 12 ), T40 );

            _mm_storeu_si128( ( __m128i* )( pm7[1] ), T11 );
            _mm_storeu_si128( ( __m128i* )( pm7[1] + 4 ), T21 );
            _mm_storeu_si128( ( __m128i* )( pm7[1] + 8 ), T31 );
            _mm_storeu_si128( ( __m128i* )( pm7[1] + 12 ), T41 );

            _mm_storeu_si128( ( __m128i* )( pm7[2] ), T12 );
            _mm_storeu_si128( ( __m128i* )( pm7[2] + 4 ), T22 );
            _mm_storeu_si128( ( __m128i* )( pm7[2] + 8 ), T32 );
            _mm_storeu_si128( ( __m128i* )( pm7[2] + 12 ), T42 );

            _mm_storeu_si128( ( __m128i* )( pm7[3] ), T13 );
            _mm_storeu_si128( ( __m128i* )( pm7[3] + 4 ), T23 );
            _mm_storeu_si128( ( __m128i* )( pm7[3] + 8 ), T33 );
            _mm_storeu_si128( ( __m128i* )( pm7[3] + 12 ), T43 );

            _mm_storeu_si128( ( __m128i* )( pm7[4] ), T14 );
            _mm_storeu_si128( ( __m128i* )( pm7[4] + 4 ), T24 );
            _mm_storeu_si128( ( __m128i* )( pm7[4] + 8 ), T34 );
            _mm_storeu_si128( ( __m128i* )( pm7[4] + 12 ), T44 );

            _mm_storeu_si128( ( __m128i* )( pm7[5] ), T15 );
            _mm_storeu_si128( ( __m128i* )( pm7[5] + 4 ), T25 );
            _mm_storeu_si128( ( __m128i* )( pm7[5] + 8 ), T35 );
            _mm_storeu_si128( ( __m128i* )( pm7[5] + 12 ), T45 );

            _mm_storeu_si128( ( __m128i* )( pm7[6] ), T16 );
            _mm_storeu_si128( ( __m128i* )( pm7[6] + 4 ), T26 );
            _mm_storeu_si128( ( __m128i* )( pm7[6] + 8 ), T36 );
            _mm_storeu_si128( ( __m128i* )( pm7[6] + 12 ), T46 );

            _mm_storeu_si128( ( __m128i* )( pm7[7] ), T17 );
            _mm_storeu_si128( ( __m128i* )( pm7[7] + 4 ), T27 );
            _mm_storeu_si128( ( __m128i* )( pm7[7] + 8 ), T37 );
            _mm_storeu_si128( ( __m128i* )( pm7[7] + 12 ), T47 );

            _mm_storeu_si128( ( __m128i* )( pm7[8] ), T18 );
            _mm_storeu_si128( ( __m128i* )( pm7[8] + 4 ), T28 );
            _mm_storeu_si128( ( __m128i* )( pm7[8] + 8 ), T38 );
            _mm_storeu_si128( ( __m128i* )( pm7[8] + 12 ), T48 );

            _mm_storeu_si128( ( __m128i* )( pm7[9] ), T19 );
            _mm_storeu_si128( ( __m128i* )( pm7[9] + 4 ), T29 );
            _mm_storeu_si128( ( __m128i* )( pm7[9] + 8 ), T39 );
            _mm_storeu_si128( ( __m128i* )( pm7[9] + 12 ), T49 );

            _mm_storeu_si128( ( __m128i* )( pm7[10] ), T1A );
            _mm_storeu_si128( ( __m128i* )( pm7[10] + 4 ), T2A );
            _mm_storeu_si128( ( __m128i* )( pm7[10] + 8 ), T3A );
            _mm_storeu_si128( ( __m128i* )( pm7[10] + 12 ), T4A );

            _mm_storeu_si128( ( __m128i* )( pm7[11] ), T1B );
            _mm_storeu_si128( ( __m128i* )( pm7[11] + 4 ), T2B );
            _mm_storeu_si128( ( __m128i* )( pm7[11] + 8 ), T3B );
            _mm_storeu_si128( ( __m128i* )( pm7[11] + 12 ), T4B );

            _mm_storeu_si128( ( __m128i* )( pm7[12] ), T1C );
            _mm_storeu_si128( ( __m128i* )( pm7[12] + 4 ), T2C );
            _mm_storeu_si128( ( __m128i* )( pm7[12] + 8 ), T3C );
            _mm_storeu_si128( ( __m128i* )( pm7[12] + 12 ), T4C );

            _mm_storeu_si128( ( __m128i* )( pm7[13] ), T1D );
            _mm_storeu_si128( ( __m128i* )( pm7[13] + 4 ), T2D );
            _mm_storeu_si128( ( __m128i* )( pm7[13] + 8 ), T3D );
            _mm_storeu_si128( ( __m128i* )( pm7[13] + 12 ), T4D );

            _mm_storeu_si128( ( __m128i* )( pm7[14] ), T1E );
            _mm_storeu_si128( ( __m128i* )( pm7[14] + 4 ), T2E );
            _mm_storeu_si128( ( __m128i* )( pm7[14] + 8 ), T3E );
            _mm_storeu_si128( ( __m128i* )( pm7[14] + 12 ), T4E );

            _mm_storeu_si128( ( __m128i* )( pm7[15] ), T1F );
            _mm_storeu_si128( ( __m128i* )( pm7[15] + 4 ), T2F );
            _mm_storeu_si128( ( __m128i* )( pm7[15] + 8 ), T3F );
            _mm_storeu_si128( ( __m128i* )( pm7[15] + 12 ), T4F );

            // get imgY_rec
            T10 = _mm_packus_epi32( mTemp10, mZero );
            T20 = _mm_packus_epi32( mTemp20, mZero );
            T30 = _mm_packus_epi32( mTemp30, mZero );
            T40 = _mm_packus_epi32( mTemp40, mZero );

            T11 = _mm_packus_epi32( mTemp11, mZero );
            T21 = _mm_packus_epi32( mTemp21, mZero );
            T31 = _mm_packus_epi32( mTemp31, mZero );
            T41 = _mm_packus_epi32( mTemp41, mZero );

            T12 = _mm_packus_epi32( mTemp12, mZero );
            T22 = _mm_packus_epi32( mTemp22, mZero );
            T32 = _mm_packus_epi32( mTemp32, mZero );
            T42 = _mm_packus_epi32( mTemp42, mZero );

            T13 = _mm_packus_epi32( mTemp13, mZero );
            T23 = _mm_packus_epi32( mTemp23, mZero );
            T33 = _mm_packus_epi32( mTemp33, mZero );
            T43 = _mm_packus_epi32( mTemp43, mZero );

            T14 = _mm_packus_epi32( mTemp14, mZero );
            T24 = _mm_packus_epi32( mTemp24, mZero );
            T34 = _mm_packus_epi32( mTemp34, mZero );
            T44 = _mm_packus_epi32( mTemp44, mZero );

            T15 = _mm_packus_epi32( mTemp15, mZero );
            T25 = _mm_packus_epi32( mTemp25, mZero );
            T35 = _mm_packus_epi32( mTemp35, mZero );
            T45 = _mm_packus_epi32( mTemp45, mZero );

            T16 = _mm_packus_epi32( mTemp16, mZero );
            T26 = _mm_packus_epi32( mTemp26, mZero );
            T36 = _mm_packus_epi32( mTemp36, mZero );
            T46 = _mm_packus_epi32( mTemp46, mZero );

            T17 = _mm_packus_epi32( mTemp17, mZero );
            T27 = _mm_packus_epi32( mTemp27, mZero );
            T37 = _mm_packus_epi32( mTemp37, mZero );
            T47 = _mm_packus_epi32( mTemp47, mZero );

            T18 = _mm_packus_epi32( mTemp18, mZero );
            T28 = _mm_packus_epi32( mTemp28, mZero );
            T38 = _mm_packus_epi32( mTemp38, mZero );
            T48 = _mm_packus_epi32( mTemp48, mZero );

            T19 = _mm_packus_epi32( mTemp19, mZero );
            T29 = _mm_packus_epi32( mTemp29, mZero );
            T39 = _mm_packus_epi32( mTemp39, mZero );
            T49 = _mm_packus_epi32( mTemp49, mZero );

            T1A = _mm_packus_epi32( mTemp1A, mZero );
            T2A = _mm_packus_epi32( mTemp2A, mZero );
            T3A = _mm_packus_epi32( mTemp3A, mZero );
            T4A = _mm_packus_epi32( mTemp4A, mZero );

            T1B = _mm_packus_epi32( mTemp1B, mZero );
            T2B = _mm_packus_epi32( mTemp2B, mZero );
            T3B = _mm_packus_epi32( mTemp3B, mZero );
            T4B = _mm_packus_epi32( mTemp4B, mZero );

            T1C = _mm_packus_epi32( mTemp1C, mZero );
            T2C = _mm_packus_epi32( mTemp2C, mZero );
            T3C = _mm_packus_epi32( mTemp3C, mZero );
            T4C = _mm_packus_epi32( mTemp4C, mZero );

            T1D = _mm_packus_epi32( mTemp1D, mZero );
            T2D = _mm_packus_epi32( mTemp2D, mZero );
            T3D = _mm_packus_epi32( mTemp3D, mZero );
            T4D = _mm_packus_epi32( mTemp4D, mZero );

            T1E = _mm_packus_epi32( mTemp1E, mZero );
            T2E = _mm_packus_epi32( mTemp2E, mZero );
            T3E = _mm_packus_epi32( mTemp3E, mZero );
            T4E = _mm_packus_epi32( mTemp4E, mZero );

            T1F = _mm_packus_epi32( mTemp1F, mZero );
            T2F = _mm_packus_epi32( mTemp2F, mZero );
            T3F = _mm_packus_epi32( mTemp3F, mZero );
            T4F = _mm_packus_epi32( mTemp4F, mZero );

            T10 = _mm_packus_epi16( T10, mZero );
            T20 = _mm_packus_epi16( T20, mZero );
            T30 = _mm_packus_epi16( T30, mZero );
            T40 = _mm_packus_epi16( T40, mZero );

            T11 = _mm_packus_epi16( T11, mZero );
            T21 = _mm_packus_epi16( T21, mZero );
            T31 = _mm_packus_epi16( T31, mZero );
            T41 = _mm_packus_epi16( T41, mZero );

            T12 = _mm_packus_epi16( T12, mZero );
            T22 = _mm_packus_epi16( T22, mZero );
            T32 = _mm_packus_epi16( T32, mZero );
            T42 = _mm_packus_epi16( T42, mZero );

            T13 = _mm_packus_epi16( T13, mZero );
            T23 = _mm_packus_epi16( T23, mZero );
            T33 = _mm_packus_epi16( T33, mZero );
            T43 = _mm_packus_epi16( T43, mZero );

            T14 = _mm_packus_epi16( T14, mZero );
            T24 = _mm_packus_epi16( T24, mZero );
            T34 = _mm_packus_epi16( T34, mZero );
            T44 = _mm_packus_epi16( T44, mZero );

            T15 = _mm_packus_epi16( T15, mZero );
            T25 = _mm_packus_epi16( T25, mZero );
            T35 = _mm_packus_epi16( T35, mZero );
            T45 = _mm_packus_epi16( T45, mZero );

            T16 = _mm_packus_epi16( T16, mZero );
            T26 = _mm_packus_epi16( T26, mZero );
            T36 = _mm_packus_epi16( T36, mZero );
            T46 = _mm_packus_epi16( T46, mZero );

            T17 = _mm_packus_epi16( T17, mZero );
            T27 = _mm_packus_epi16( T27, mZero );
            T37 = _mm_packus_epi16( T37, mZero );
            T47 = _mm_packus_epi16( T47, mZero );

            T18 = _mm_packus_epi16( T18, mZero );
            T28 = _mm_packus_epi16( T28, mZero );
            T38 = _mm_packus_epi16( T38, mZero );
            T48 = _mm_packus_epi16( T48, mZero );

            T19 = _mm_packus_epi16( T19, mZero );
            T29 = _mm_packus_epi16( T29, mZero );
            T39 = _mm_packus_epi16( T39, mZero );
            T49 = _mm_packus_epi16( T49, mZero );

            T1A = _mm_packus_epi16( T1A, mZero );
            T2A = _mm_packus_epi16( T2A, mZero );
            T3A = _mm_packus_epi16( T3A, mZero );
            T4A = _mm_packus_epi16( T4A, mZero );

            T1B = _mm_packus_epi16( T1B, mZero );
            T2B = _mm_packus_epi16( T2B, mZero );
            T3B = _mm_packus_epi16( T3B, mZero );
            T4B = _mm_packus_epi16( T4B, mZero );

            T1C = _mm_packus_epi16( T1C, mZero );
            T2C = _mm_packus_epi16( T2C, mZero );
            T3C = _mm_packus_epi16( T3C, mZero );
            T4C = _mm_packus_epi16( T4C, mZero );

            T1D = _mm_packus_epi16( T1D, mZero );
            T2D = _mm_packus_epi16( T2D, mZero );
            T3D = _mm_packus_epi16( T3D, mZero );
            T4D = _mm_packus_epi16( T4D, mZero );

            T1E = _mm_packus_epi16( T1E, mZero );
            T2E = _mm_packus_epi16( T2E, mZero );
            T3E = _mm_packus_epi16( T3E, mZero );
            T4E = _mm_packus_epi16( T4E, mZero );

            T1F = _mm_packus_epi16( T1F, mZero );
            T2F = _mm_packus_epi16( T2F, mZero );
            T3F = _mm_packus_epi16( T3F, mZero );
            T4F = _mm_packus_epi16( T4F, mZero );

            // pack the data
            T10 = _mm_unpacklo_epi32( T10, T20 );
            T20 = _mm_unpacklo_epi32( T30, T40 );
            T10 = _mm_unpacklo_epi64( T10, T20 );

            T11 = _mm_unpacklo_epi32( T11, T21 );
            T21 = _mm_unpacklo_epi32( T31, T41 );
            T11 = _mm_unpacklo_epi64( T11, T21 );

            T12 = _mm_unpacklo_epi32( T12, T22 );
            T22 = _mm_unpacklo_epi32( T32, T42 );
            T12 = _mm_unpacklo_epi64( T12, T22 );

            T13 = _mm_unpacklo_epi32( T13, T23 );
            T23 = _mm_unpacklo_epi32( T33, T43 );
            T13 = _mm_unpacklo_epi64( T13, T23 );

            T14 = _mm_unpacklo_epi32( T14, T24 );
            T24 = _mm_unpacklo_epi32( T34, T44 );
            T14 = _mm_unpacklo_epi64( T14, T24 );

            T15 = _mm_unpacklo_epi32( T15, T25 );
            T25 = _mm_unpacklo_epi32( T35, T45 );
            T15 = _mm_unpacklo_epi64( T15, T25 );

            T16 = _mm_unpacklo_epi32( T16, T26 );
            T26 = _mm_unpacklo_epi32( T36, T46 );
            T16 = _mm_unpacklo_epi64( T16, T26 );

            T17 = _mm_unpacklo_epi32( T17, T27 );
            T27 = _mm_unpacklo_epi32( T37, T47 );
            T17 = _mm_unpacklo_epi64( T17, T27 );

            T18 = _mm_unpacklo_epi32( T18, T28 );
            T28 = _mm_unpacklo_epi32( T38, T48 );
            T18 = _mm_unpacklo_epi64( T18, T28 );

            T19 = _mm_unpacklo_epi32( T19, T29 );
            T29 = _mm_unpacklo_epi32( T39, T49 );
            T19 = _mm_unpacklo_epi64( T19, T29 );

            T1A = _mm_unpacklo_epi32( T1A, T2A );
            T2A = _mm_unpacklo_epi32( T3A, T4A );
            T1A = _mm_unpacklo_epi64( T1A, T2A );

            T1B = _mm_unpacklo_epi32( T1B, T2B );
            T2B = _mm_unpacklo_epi32( T3B, T4B );
            T1B = _mm_unpacklo_epi64( T1B, T2B );

            T1C = _mm_unpacklo_epi32( T1C, T2C );
            T2C = _mm_unpacklo_epi32( T3C, T4C );
            T1C = _mm_unpacklo_epi64( T1C, T2C );

            T1D = _mm_unpacklo_epi32( T1D, T2D );
            T2D = _mm_unpacklo_epi32( T3D, T4D );
            T1D = _mm_unpacklo_epi64( T1D, T2D );

            T1E = _mm_unpacklo_epi32( T1E, T2E );
            T2E = _mm_unpacklo_epi32( T3E, T4E );
            T1E = _mm_unpacklo_epi64( T1E, T2E );

            T1F = _mm_unpacklo_epi32( T1F, T2F );
            T2F = _mm_unpacklo_epi32( T3F, T4F );
            T1F = _mm_unpacklo_epi64( T1F, T2F );

            if ( ( b8 - 4 ) % 2 )
            {
                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 0 )*iStrideC + ipix_c_x ), T10 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 0)*iStrideC + ipix_c_x + 4), T20);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 0)*iStrideC + ipix_c_x + 8), T30);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 0)*iStrideC + ipix_c_x + 12), T40);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 1 )*iStrideC + ipix_c_x ), T11 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x + 4), T21);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x + 8), T31);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x + 12), T41);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 2 )*iStrideC + ipix_c_x ), T12 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x + 4), T22);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x + 8), T32);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x + 12), T42);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 3 )*iStrideC + ipix_c_x ), T13 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x + 4), T23);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x + 8), T33);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x + 12), T43);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 4 )*iStrideC + ipix_c_x ), T14 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 4)*iStrideC + ipix_c_x + 4), T24);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 4)*iStrideC + ipix_c_x + 8), T34);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 4)*iStrideC + ipix_c_x + 12), T44);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 5 )*iStrideC + ipix_c_x ), T15 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 5)*iStrideC + ipix_c_x + 4), T25);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 5)*iStrideC + ipix_c_x + 8), T35);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 5)*iStrideC + ipix_c_x + 12), T45);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 6 )*iStrideC + ipix_c_x ), T16 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 6)*iStrideC + ipix_c_x + 4), T26);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 6)*iStrideC + ipix_c_x + 8), T36);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 6)*iStrideC + ipix_c_x + 12), T46);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 7 )*iStrideC + ipix_c_x ), T17 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 7)*iStrideC + ipix_c_x + 4), T27);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 7)*iStrideC + ipix_c_x + 8), T37);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 7)*iStrideC + ipix_c_x + 12), T47);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 8 )*iStrideC + ipix_c_x ), T18 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 8)*iStrideC + ipix_c_x + 4), T28);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 8)*iStrideC + ipix_c_x + 8), T38);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 8)*iStrideC + ipix_c_x + 12), T48);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 9 )*iStrideC + ipix_c_x ), T19 );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 9)*iStrideC + ipix_c_x + 4), T29);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 9)*iStrideC + ipix_c_x + 8), T39);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 9)*iStrideC + ipix_c_x + 12), T49);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 10 )*iStrideC + ipix_c_x ), T1A );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 10)*iStrideC + ipix_c_x + 4), T2A);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 10)*iStrideC + ipix_c_x + 8), T3A);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 10)*iStrideC + ipix_c_x + 12), T4A);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 11 )*iStrideC + ipix_c_x ), T1B );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 11)*iStrideC + ipix_c_x + 4), T2B);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 11)*iStrideC + ipix_c_x + 8), T3B);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 11)*iStrideC + ipix_c_x + 12), T4B);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 12 )*iStrideC + ipix_c_x ), T1C );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 12)*iStrideC + ipix_c_x + 4), T2C);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 12)*iStrideC + ipix_c_x + 8), T3C);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 12)*iStrideC + ipix_c_x + 12), T4C);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 13 )*iStrideC + ipix_c_x ), T1D );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 13)*iStrideC + ipix_c_x + 4), T2D);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 13)*iStrideC + ipix_c_x + 8), T3D);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 13)*iStrideC + ipix_c_x + 12), T4D);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 14 )*iStrideC + ipix_c_x ), T1E );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 14)*iStrideC + ipix_c_x + 4), T2E);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 14)*iStrideC + ipix_c_x + 8), T3E);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 14)*iStrideC + ipix_c_x + 12), T4E);

                _mm_storeu_si128( ( __m128i* )( imgV + ( ipix_c_y + 15 )*iStrideC + ipix_c_x ), T1F );
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 15)*iStrideC + ipix_c_x + 4), T2F);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 15)*iStrideC + ipix_c_x + 8), T3F);
                //_mm_storeu_si128((__m128i*)(imgV_rec + (ipix_c_y + 15)*iStrideC + ipix_c_x + 12), T4F);
            }
            else
            {
                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 0 )*iStrideC + ipix_c_x ), T10 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 0)*iStrideC + ipix_c_x + 4), T20);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 0)*iStrideC + ipix_c_x + 8), T30);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 0)*iStrideC + ipix_c_x + 12), T40);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 1 )*iStrideC + ipix_c_x ), T11 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x + 4), T21);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x + 8), T31);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 1)*iStrideC + ipix_c_x + 12), T41);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 2 )*iStrideC + ipix_c_x ), T12 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x + 4), T22);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x + 8), T32);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 2)*iStrideC + ipix_c_x + 12), T42);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 3 )*iStrideC + ipix_c_x ), T13 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x + 4), T23);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x + 8), T33);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 3)*iStrideC + ipix_c_x + 12), T43);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 4 )*iStrideC + ipix_c_x ), T14 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 4)*iStrideC + ipix_c_x + 4), T24);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 4)*iStrideC + ipix_c_x + 8), T34);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 4)*iStrideC + ipix_c_x + 12), T44);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 5 )*iStrideC + ipix_c_x ), T15 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 5)*iStrideC + ipix_c_x + 4), T25);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 5)*iStrideC + ipix_c_x + 8), T35);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 5)*iStrideC + ipix_c_x + 12), T45);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 6 )*iStrideC + ipix_c_x ), T16 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 6)*iStrideC + ipix_c_x + 4), T26);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 6)*iStrideC + ipix_c_x + 8), T36);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 6)*iStrideC + ipix_c_x + 12), T46);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 7 )*iStrideC + ipix_c_x ), T17 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 7)*iStrideC + ipix_c_x + 4), T27);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 7)*iStrideC + ipix_c_x + 8), T37);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 7)*iStrideC + ipix_c_x + 12), T47);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 8 )*iStrideC + ipix_c_x ), T18 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 8)*iStrideC + ipix_c_x + 4), T28);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 8)*iStrideC + ipix_c_x + 8), T38);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 8)*iStrideC + ipix_c_x + 12), T48);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 9 )*iStrideC + ipix_c_x ), T19 );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 9)*iStrideC + ipix_c_x + 4), T29);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 9)*iStrideC + ipix_c_x + 8), T39);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 9)*iStrideC + ipix_c_x + 12), T49);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 10 )*iStrideC + ipix_c_x ), T1A );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 10)*iStrideC + ipix_c_x + 4), T2A);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 10)*iStrideC + ipix_c_x + 8), T3A);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 10)*iStrideC + ipix_c_x + 12), T4A);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 11 )*iStrideC + ipix_c_x ), T1B );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 11)*iStrideC + ipix_c_x + 4), T2B);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 11)*iStrideC + ipix_c_x + 8), T3B);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 11)*iStrideC + ipix_c_x + 12), T4B);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 12 )*iStrideC + ipix_c_x ), T1C );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 12)*iStrideC + ipix_c_x + 4), T2C);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 12)*iStrideC + ipix_c_x + 8), T3C);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 12)*iStrideC + ipix_c_x + 12), T4C);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 13 )*iStrideC + ipix_c_x ), T1D );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 13)*iStrideC + ipix_c_x + 4), T2D);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 13)*iStrideC + ipix_c_x + 8), T3D);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 13)*iStrideC + ipix_c_x + 12), T4D);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 14 )*iStrideC + ipix_c_x ), T1E );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 14)*iStrideC + ipix_c_x + 4), T2E);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 14)*iStrideC + ipix_c_x + 8), T3E);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 14)*iStrideC + ipix_c_x + 12), T4E);

                _mm_storeu_si128( ( __m128i* )( imgU + ( ipix_c_y + 15 )*iStrideC + ipix_c_x ), T1F );
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 15)*iStrideC + ipix_c_x + 4), T2F);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 15)*iStrideC + ipix_c_x + 8), T3F);
                //_mm_storeu_si128((__m128i*)(imgU_rec + (ipix_c_y + 15)*iStrideC + ipix_c_x + 12), T4F);
            }

        }
    }
}


void avg_pel_sse128( pel_t *dst, int i_dst, pel_t *src1, int i_src1, pel_t *src2, int i_src2, int width, int height )
{
    int i, j;

    __m128i S1, S2, D;

    if ( width & 15 )
    {
        __m128i mask = _mm_load_si128( ( const __m128i* )intrinsic_mask[( width & 15 ) - 1] );

        for ( i = 0; i < height; i++ )
        {
            for ( j = 0; j < width - 15; j += 16 )
            {
                S1 = _mm_loadu_si128( ( const __m128i* )( src1 + j ) );
                S2 = _mm_loadu_si128( ( const __m128i* )( src2 + j ) );
                D = _mm_avg_epu8( S1, S2 );
                _mm_storeu_si128( ( __m128i* )( dst + j ), D );
            }

            if ( width % 16 )
            {
                S1 = _mm_loadu_si128( ( const __m128i* )( src1 + j ) );
                S2 = _mm_loadu_si128( ( const __m128i* )( src2 + j ) );
                D = _mm_avg_epu8( S1, S2 );
                _mm_maskmoveu_si128( D, mask, ( char_t* )&dst[j] );
            }

            src1 += i_src1;
            src2 += i_src2;
            dst += i_dst;
        }
    }
    else
    {
        for ( i = 0; i < height; i++ )
        {
            for ( j = 0; j < width; j += 16 )
            {
                S1 = _mm_loadu_si128( ( const __m128i* )( src1 + j ) );
                S2 = _mm_loadu_si128( ( const __m128i* )( src2 + j ) );
                D = _mm_avg_epu8( S1, S2 );
                _mm_storeu_si128( ( __m128i* )( dst + j ), D );
            }
            src1 += i_src1;
            src2 += i_src2;
            dst += i_dst;
        }
    }
}

void padding_rows_sse128( pel_t *src, int i_src, int width, int height, int pad )
{
    int i, j;
    pel_t *p, *p1, *p2;
    int pad_lr = pad + 16 - ( pad & 0xF );

    p = src;

    // left & right
    for ( i = 0; i < height; i++ )
    {
        __m128i Val1 = _mm_set1_epi8( p[0] );
        __m128i Val2 = _mm_set1_epi8( p[width - 1] );
        p1 = p - pad_lr;
        p2 = p + width;
        for ( j = 0; j < pad_lr; j += 16 )
        {
            _mm_storeu_si128( ( __m128i* )( p1 + j ), Val1 );
            _mm_storeu_si128( ( __m128i* )( p2 + j ), Val2 );
        }
        p += i_src;
    }

    // above border
    p = src - pad;
    for ( i = 1; i <= pad; i++ )
    {
        memcpy( p - i_src * i, p, ( width + 2 * pad ) * sizeof( pel_t ) );
    }

    // bottom border
    p = src + i_src * ( height - 1 ) - pad;
    for ( i = 1; i <= pad; i++ )
    {
        memcpy( p + i_src * i, p, ( width + 2 * pad ) * sizeof( pel_t ) );
    }

}

void com_cpy_sse128( const pel_t *src, int i_src, pel_t *dst, int i_dst, int width, int height )
{
    int row, col;

    __m128i S;
    __m128i mask = _mm_load_si128( ( const __m128i* )intrinsic_mask[( width & 15 ) - 1] );

    for ( row = 0; row < height; row++ )
    {
        for ( col = 0; col < width - 15; col += 16 )
        {
            S = _mm_loadu_si128( ( const __m128i* )( src + col ) );
            _mm_storeu_si128( ( __m128i* )( dst + col ), S );
        }

        if ( width % 16 )
        {
            S = _mm_loadu_si128( ( const __m128i* )( src + col ) );
            _mm_maskmoveu_si128( S, mask, ( char_t* )&dst[col] );
        }

        src += i_src;
        dst += i_dst;
    }
}

#endif