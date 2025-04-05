#ifndef _pdu_ctype_geometry_msgs_TransformStamped_H_
#define _pdu_ctype_geometry_msgs_TransformStamped_H_

#include "pdu_primitive_ctypes.h"
#include "builtin_interfaces/pdu_ctype_Time.h"
#include "geometry_msgs/pdu_ctype_Quaternion.h"
#include "geometry_msgs/pdu_ctype_Transform.h"
#include "geometry_msgs/pdu_ctype_Vector3.h"
#include "std_msgs/pdu_ctype_Header.h"

typedef struct {
        Hako_Header    header;
        char child_frame_id[HAKO_STRING_SIZE];
        Hako_Transform    transform;
} Hako_TransformStamped;

#endif /* _pdu_ctype_geometry_msgs_TransformStamped_H_ */
