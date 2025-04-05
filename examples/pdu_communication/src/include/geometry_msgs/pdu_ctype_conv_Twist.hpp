#ifndef _PDU_CTYPE_CONV_HAKO_geometry_msgs_Twist_HPP_
#define _PDU_CTYPE_CONV_HAKO_geometry_msgs_Twist_HPP_

#include "pdu_primitive_ctypes.h"
#include "ros_primitive_types.hpp"
#include "pdu_primitive_ctypes_conv.hpp"
/*
 * Dependent pdu data
 */
#include "geometry_msgs/pdu_ctype_Twist.h"
/*
 * Dependent ros data
 */
#include "geometry_msgs/msg/twist.hpp"

/*
 * Dependent Convertors
 */
#include "geometry_msgs/pdu_ctype_conv_Vector3.hpp"

/***************************
 *
 * PDU ==> ROS2
 *
 ***************************/
static inline int hako_convert_pdu2ros_Twist(Hako_Twist &src,  geometry_msgs::msg::Twist &dst)
{
    //struct convert
    hako_convert_pdu2ros_Vector3(src.linear, dst.linear);
    //struct convert
    hako_convert_pdu2ros_Vector3(src.angular, dst.angular);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Twist(Hako_Twist src[], std::array<geometry_msgs::msg::Twist, _dst_len> &dst)
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_pdu2ros_Twist(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Twist(Hako_Twist src[], std::vector<geometry_msgs::msg::Twist> &dst)
{
    dst.resize(_src_len);
    for (int i = 0; i < _src_len; i++) {
        (void)hako_convert_pdu2ros_Twist(src[i], dst[i]);
    }
    return 0;
}

/***************************
 *
 * ROS2 ==> PDU
 *
 ***************************/
static inline int hako_convert_ros2pdu_Twist(geometry_msgs::msg::Twist &src, Hako_Twist &dst)
{
    //struct convert
    hako_convert_ros2pdu_Vector3(src.linear, dst.linear);
    //struct convert
    hako_convert_ros2pdu_Vector3(src.angular, dst.angular);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Twist(std::array<geometry_msgs::msg::Twist, _src_len> &src, Hako_Twist dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Twist(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Twist(std::vector<geometry_msgs::msg::Twist> &src, Hako_Twist dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Twist(src[i], dst[i]);
    }
    return ret;
}

#endif /* _PDU_CTYPE_CONV_HAKO_geometry_msgs_Twist_HPP_ */
