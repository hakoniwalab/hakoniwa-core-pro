#ifndef _pdu_ctype_geometry_msgs_Pose_H_
#define _pdu_ctype_geometry_msgs_Pose_H_

#include "pdu_primitive_ctypes.h"
#include "geometry_msgs/pdu_ctype_Point.h"
#include "geometry_msgs/pdu_ctype_Quaternion.h"

typedef struct {
        Hako_Point    position;
        Hako_Quaternion    orientation;
} Hako_Pose;

#endif /* _pdu_ctype_geometry_msgs_Pose_H_ */
