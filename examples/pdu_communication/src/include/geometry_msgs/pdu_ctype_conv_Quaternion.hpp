#ifndef _PDU_CTYPE_CONV_HAKO_geometry_msgs_Quaternion_HPP_
#define _PDU_CTYPE_CONV_HAKO_geometry_msgs_Quaternion_HPP_

#include "pdu_primitive_ctypes.h"
#include "ros_primitive_types.hpp"
#include "pdu_primitive_ctypes_conv.hpp"
/*
 * Dependent pdu data
 */
#include "geometry_msgs/pdu_ctype_Quaternion.h"
/*
 * Dependent ros data
 */
#include "geometry_msgs/msg/quaternion.hpp"

/*
 * Dependent Convertors
 */

/***************************
 *
 * PDU ==> ROS2
 *
 ***************************/
static inline int hako_convert_pdu2ros_Quaternion(Hako_Quaternion &src,  geometry_msgs::msg::Quaternion &dst)
{
    //primitive convert
    hako_convert_pdu2ros(src.x, dst.x);
    //primitive convert
    hako_convert_pdu2ros(src.y, dst.y);
    //primitive convert
    hako_convert_pdu2ros(src.z, dst.z);
    //primitive convert
    hako_convert_pdu2ros(src.w, dst.w);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Quaternion(Hako_Quaternion src[], std::array<geometry_msgs::msg::Quaternion, _dst_len> &dst)
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_pdu2ros_Quaternion(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Quaternion(Hako_Quaternion src[], std::vector<geometry_msgs::msg::Quaternion> &dst)
{
    dst.resize(_src_len);
    for (int i = 0; i < _src_len; i++) {
        (void)hako_convert_pdu2ros_Quaternion(src[i], dst[i]);
    }
    return 0;
}

/***************************
 *
 * ROS2 ==> PDU
 *
 ***************************/
static inline int hako_convert_ros2pdu_Quaternion(geometry_msgs::msg::Quaternion &src, Hako_Quaternion &dst)
{
    //primitive convert
    hako_convert_ros2pdu(src.x, dst.x);
    //primitive convert
    hako_convert_ros2pdu(src.y, dst.y);
    //primitive convert
    hako_convert_ros2pdu(src.z, dst.z);
    //primitive convert
    hako_convert_ros2pdu(src.w, dst.w);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Quaternion(std::array<geometry_msgs::msg::Quaternion, _src_len> &src, Hako_Quaternion dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Quaternion(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Quaternion(std::vector<geometry_msgs::msg::Quaternion> &src, Hako_Quaternion dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Quaternion(src[i], dst[i]);
    }
    return ret;
}

#endif /* _PDU_CTYPE_CONV_HAKO_geometry_msgs_Quaternion_HPP_ */
