#ifndef _PDU_CTYPE_CONV_HAKO_geometry_msgs_Transform_HPP_
#define _PDU_CTYPE_CONV_HAKO_geometry_msgs_Transform_HPP_

#include "pdu_primitive_ctypes.h"
#include "ros_primitive_types.hpp"
#include "pdu_primitive_ctypes_conv.hpp"
/*
 * Dependent pdu data
 */
#include "geometry_msgs/pdu_ctype_Transform.h"
/*
 * Dependent ros data
 */
#include "geometry_msgs/msg/transform.hpp"

/*
 * Dependent Convertors
 */
#include "geometry_msgs/pdu_ctype_conv_Quaternion.hpp"
#include "geometry_msgs/pdu_ctype_conv_Vector3.hpp"

/***************************
 *
 * PDU ==> ROS2
 *
 ***************************/
static inline int hako_convert_pdu2ros_Transform(Hako_Transform &src,  geometry_msgs::msg::Transform &dst)
{
    //struct convert
    hako_convert_pdu2ros_Vector3(src.translation, dst.translation);
    //struct convert
    hako_convert_pdu2ros_Quaternion(src.rotation, dst.rotation);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Transform(Hako_Transform src[], std::array<geometry_msgs::msg::Transform, _dst_len> &dst)
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_pdu2ros_Transform(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Transform(Hako_Transform src[], std::vector<geometry_msgs::msg::Transform> &dst)
{
    dst.resize(_src_len);
    for (int i = 0; i < _src_len; i++) {
        (void)hako_convert_pdu2ros_Transform(src[i], dst[i]);
    }
    return 0;
}

/***************************
 *
 * ROS2 ==> PDU
 *
 ***************************/
static inline int hako_convert_ros2pdu_Transform(geometry_msgs::msg::Transform &src, Hako_Transform &dst)
{
    //struct convert
    hako_convert_ros2pdu_Vector3(src.translation, dst.translation);
    //struct convert
    hako_convert_ros2pdu_Quaternion(src.rotation, dst.rotation);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Transform(std::array<geometry_msgs::msg::Transform, _src_len> &src, Hako_Transform dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Transform(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Transform(std::vector<geometry_msgs::msg::Transform> &src, Hako_Transform dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Transform(src[i], dst[i]);
    }
    return ret;
}

#endif /* _PDU_CTYPE_CONV_HAKO_geometry_msgs_Transform_HPP_ */
