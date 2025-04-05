#ifndef _PDU_CTYPE_CONV_HAKO_geometry_msgs_Pose_HPP_
#define _PDU_CTYPE_CONV_HAKO_geometry_msgs_Pose_HPP_

#include "pdu_primitive_ctypes.h"
#include "ros_primitive_types.hpp"
#include "pdu_primitive_ctypes_conv.hpp"
/*
 * Dependent pdu data
 */
#include "geometry_msgs/pdu_ctype_Pose.h"
/*
 * Dependent ros data
 */
#include "geometry_msgs/msg/pose.hpp"

/*
 * Dependent Convertors
 */
#include "geometry_msgs/pdu_ctype_conv_Point.hpp"
#include "geometry_msgs/pdu_ctype_conv_Quaternion.hpp"

/***************************
 *
 * PDU ==> ROS2
 *
 ***************************/
static inline int hako_convert_pdu2ros_Pose(Hako_Pose &src,  geometry_msgs::msg::Pose &dst)
{
    //struct convert
    hako_convert_pdu2ros_Point(src.position, dst.position);
    //struct convert
    hako_convert_pdu2ros_Quaternion(src.orientation, dst.orientation);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Pose(Hako_Pose src[], std::array<geometry_msgs::msg::Pose, _dst_len> &dst)
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_pdu2ros_Pose(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_Pose(Hako_Pose src[], std::vector<geometry_msgs::msg::Pose> &dst)
{
    dst.resize(_src_len);
    for (int i = 0; i < _src_len; i++) {
        (void)hako_convert_pdu2ros_Pose(src[i], dst[i]);
    }
    return 0;
}

/***************************
 *
 * ROS2 ==> PDU
 *
 ***************************/
static inline int hako_convert_ros2pdu_Pose(geometry_msgs::msg::Pose &src, Hako_Pose &dst)
{
    //struct convert
    hako_convert_ros2pdu_Point(src.position, dst.position);
    //struct convert
    hako_convert_ros2pdu_Quaternion(src.orientation, dst.orientation);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Pose(std::array<geometry_msgs::msg::Pose, _src_len> &src, Hako_Pose dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Pose(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_Pose(std::vector<geometry_msgs::msg::Pose> &src, Hako_Pose dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_Pose(src[i], dst[i]);
    }
    return ret;
}

#endif /* _PDU_CTYPE_CONV_HAKO_geometry_msgs_Pose_HPP_ */
