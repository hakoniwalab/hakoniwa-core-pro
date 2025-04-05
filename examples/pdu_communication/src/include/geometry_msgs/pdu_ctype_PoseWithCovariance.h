#ifndef _pdu_ctype_geometry_msgs_PoseWithCovariance_H_
#define _pdu_ctype_geometry_msgs_PoseWithCovariance_H_

#include "pdu_primitive_ctypes.h"
#include "geometry_msgs/pdu_ctype_Point.h"
#include "geometry_msgs/pdu_ctype_Pose.h"
#include "geometry_msgs/pdu_ctype_Quaternion.h"

typedef struct {
        Hako_Pose    pose;
        Hako_float64 covariance[36];
} Hako_PoseWithCovariance;

#endif /* _pdu_ctype_geometry_msgs_PoseWithCovariance_H_ */
