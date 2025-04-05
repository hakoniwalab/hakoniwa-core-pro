#ifndef _pdu_ctype_geometry_msgs_TwistWithCovariance_H_
#define _pdu_ctype_geometry_msgs_TwistWithCovariance_H_

#include "pdu_primitive_ctypes.h"
#include "geometry_msgs/pdu_ctype_Twist.h"
#include "geometry_msgs/pdu_ctype_Vector3.h"

typedef struct {
        Hako_Twist    twist;
        Hako_float64 covariance[36];
} Hako_TwistWithCovariance;

#endif /* _pdu_ctype_geometry_msgs_TwistWithCovariance_H_ */
