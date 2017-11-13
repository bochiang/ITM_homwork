#ifndef _IMAGE_H_
#define _IMAGE_H_

int encode_one_frame();
void picture_data( Picture *pic, CSobj *cs_aec );
void Update_Picture_Buffers();

void find_distortion ();

#endif