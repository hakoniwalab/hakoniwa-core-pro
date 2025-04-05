#ifndef _PDU_CTYPE_CONV_HAKO_geometry_msgs_Vector3_HPP_
#define _PDU_CTYPE_CONV_HAKO_geometry_msgs_Vector3_HPP_

#include "pdu_primitive_ctypes.h"
#include "ros_primitive_types.hpp"
#include "pdu_primitive_ctypes_conv.hpp"
/*
 * Dependent pdu data
 */
#include "geometry_msgs/pdu_ctype_Vector3.h"
/*
 * Dependent ros data
 */
#include "geometry_msgs/msg/vector3.hpp"

/*
 * Dependent Convertors
 */

/***************************
 *
 * PDU ==> ROS2
 *
 ***************************/
static inline int hako_convert_pdu2ros_Vector3(Hako_Vector3 &src,  geometry_msgs::msg::Vector3 &dst)
{
    //primitive convert
    hako_convert_pdu2ros(src.x, dst.x);
    //primitive convert
    hako_convert_pdu2ros(src.y, dst.y);
    //primitive convert
    hako_convert_pdu2ros(src.z, dst.z);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Vector3(Hako_Vector3 src[], std::array<geometry_msgs::msg::Vector3, _dst_len> &dst)
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_pdu2ros_Vector3(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Vector3(Hako_Vector3 src[], std::vector<geometry_msgs::msg::Vector3> &dst)
{
    dst.resize(_src_len);
    for (int i = 0; i < _src_len; i++) {
        (void)hako_convert_pdu2ros_Vector3(src[i], dst[i]);
    }
    return 0;
}

/***************************
 *
 * ROS2 ==> PDU
 *
 ***************************/
static inline int hako_convert_ros2pdu_Vector3(geometry_msgs::msg::Vector3 &src, Hako_Vector3 &dst)
{
    //primitive convert
    hako_convert_ros2pdu(src.x, dst.x);
    //primitive convert
    hako_convert_ros2pdu(src.y, dst.y);
    //primitive convert
    hako_convert_ros2pdu(src.z, dst.z);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Vector3(std::array<geometry_msgs::msg::Vector3, _src_len> &src, Hako_Vector3 dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Vector3(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Vector3(std::vector<geometry_msgs::msg::Vector3> &src, Hako_Vector3 dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Vector3(src[i], dst[i]);
    }
    return ret;
}

#endif /* _PDU_CTYPE_CONV_HAKO_geometry_msgs_Vector3_HPP_ */
