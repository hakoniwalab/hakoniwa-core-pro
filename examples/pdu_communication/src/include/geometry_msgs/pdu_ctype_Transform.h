#ifndef _pdu_ctype_geometry_msgs_Transform_H_
#define _pdu_ctype_geometry_msgs_Transform_H_

#include "pdu_primitive_ctypes.h"
#include "geometry_msgs/pdu_ctype_Quaternion.h"
#include "geometry_msgs/pdu_ctype_Vector3.h"

typedef struct {
        Hako_Vector3    translation;
        Hako_Quaternion    rotation;
} Hako_Transform;

#endif /* _pdu_ctype_geometry_msgs_Transform_H_ */
