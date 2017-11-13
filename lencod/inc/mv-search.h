#ifndef _MV_SEARCH_H_
#define _MV_SEARCH_H_

int check_mvd( int mvd_x, int mvd_y );
int check_mv_range( int mv_x, int mv_y, int pix_x, int pix_y, int mode );
int check_mv_range_bid( int mv_x, int mv_y, int pix_x, int pix_y, int mode );

void ForwardMVSearch( Macroblock *currMB, int *fw_mcost,int *best_fw_ref, int *best_bw_ref, int mode, int block );
void ForwardMVSearchMhp( Macroblock *currMB, int *p_bid_mcost, int *best_fw_ref, int *best_bw_ref, int mode, int block );
void BidirectionalMVSearch( Macroblock *currMB, int *best_bw_ref, int *bw_mcost, int *bid_mcost, int* bid_best_fw_ref, int* bid_best_bw_ref,int mode, int block );
#endif