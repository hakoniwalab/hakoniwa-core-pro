#ifndef _pdu_ctype_geometry_msgs_Twist_H_
#define _pdu_ctype_geometry_msgs_Twist_H_

#include "pdu_primitive_ctypes.h"
#include "geometry_msgs/pdu_ctype_Vector3.h"

typedef struct {
        Hako_Vector3    linear;
        Hako_Vector3    angular;
} Hako_Twist;

#endif /* _pdu_ctype_geometry_msgs_Twist_H_ */
