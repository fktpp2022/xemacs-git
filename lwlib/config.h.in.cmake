#ifndef _LWLIB_CONFIG_H_
#define _LWLIB_CONFIG_H_

#include "../src/config.h"
#include "../src/compiler.h"

#cmakedefine01 NEED_MOTIF
#cmakedefine01 NEED_ATHENA
#cmakedefine01 NEED_LUCID

#cmakedefine ATHENA_Scrollbar_h_
#cmakedefine ATHENA_Dialog_h_
#cmakedefine ATHENA_Form_h_
#cmakedefine ATHENA_Command_h_
#cmakedefine ATHENA_Label_h_
#cmakedefine ATHENA_LabelP_h_
#cmakedefine ATHENA_Toggle_h_
#cmakedefine ATHENA_ToggleP_h_
#cmakedefine ATHENA_AsciiText_h_
#cmakedefine ATHENA_XawInit_h_

#endif
