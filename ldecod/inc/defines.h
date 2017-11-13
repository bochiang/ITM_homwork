#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <stdlib.h>
#include "../../common/commonstructs.h"

#define M38817_DATA_STUFFING 1

#define MAX_IF_BUF    (MB_SIZE + 10)
#define MAX_IF_BUF_C  (MB_SIZE/2 + 4)

#define EOS             1     //!< End Of Sequence
#define SOP             2     //!< Start Of Picture

#define DECODE_MB       1

#define IMG_PAD_SIZE (64+5+1)
#endif