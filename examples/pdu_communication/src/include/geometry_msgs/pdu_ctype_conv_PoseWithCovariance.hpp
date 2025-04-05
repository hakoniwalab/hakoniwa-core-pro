#ifndef _PDU_CTYPE_CONV_HAKO_geometry_msgs_PoseWithCovariance_HPP_
#define _PDU_CTYPE_CONV_HAKO_geometry_msgs_PoseWithCovariance_HPP_

#include "pdu_primitive_ctypes.h"
#include "ros_primitive_types.hpp"
#include "pdu_primitive_ctypes_conv.hpp"
/*
 * Dependent pdu data
 */
#include "geometry_msgs/pdu_ctype_PoseWithCovariance.h"
/*
 * Dependent ros data
 */
#include "geometry_msgs/msg/pose_with_covariance.hpp"

/*
 * Dependent Convertors
 */
#include "geometry_msgs/pdu_ctype_conv_Point.hpp"
#include "geometry_msgs/pdu_ctype_conv_Pose.hpp"
#include "geometry_msgs/pdu_ctype_conv_Quaternion.hpp"

/***************************
 *
 * PDU ==> ROS2
 *
 ***************************/
static inline int hako_convert_pdu2ros_PoseWithCovariance(Hako_PoseWithCovariance &src,  geometry_msgs::msg::PoseWithCovariance &dst)
{
    //struct convert
    hako_convert_pdu2ros_Pose(src.pose, dst.pose);
    //primitive array convertor
    (void)hako_convert_pdu2ros_array(
        src.covariance, M_ARRAY_SIZE(Hako_PoseWithCovariance, Hako_float64, covariance),
        dst.covariance, dst.covariance.size());
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_PoseWithCovariance(Hako_PoseWithCovariance src[], std::array<geometry_msgs::msg::PoseWithCovariance, _dst_len> &dst)
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_pdu2ros_PoseWithCovariance(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_PoseWithCovariance(Hako_PoseWithCovariance src[], std::vector<geometry_msgs::msg::PoseWithCovariance> &dst)
{
    dst.resize(_src_len);
    for (int i = 0; i < _src_len; i++) {
        (void)hako_convert_pdu2ros_PoseWithCovariance(src[i], dst[i]);
    }
    return 0;
}

/***************************
 *
 * ROS2 ==> PDU
 *
 ***************************/
static inline int hako_convert_ros2pdu_PoseWithCovariance(geometry_msgs::msg::PoseWithCovariance &src, Hako_PoseWithCovariance &dst)
{
    //struct convert
    hako_convert_ros2pdu_Pose(src.pose, dst.pose);
    //primitive array convertor
    (void)hako_convert_ros2pdu_array(
        src.covariance, src.covariance.size(),
        dst.covariance, M_ARRAY_SIZE(Hako_PoseWithCovariance, Hako_float64, covariance));
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_PoseWithCovariance(std::array<geometry_msgs::msg::PoseWithCovariance, _src_len> &src, Hako_PoseWithCovariance dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_PoseWithCovariance(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_PoseWithCovariance(std::vector<geometry_msgs::msg::PoseWithCovariance> &src, Hako_PoseWithCovariance dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_PoseWithCovariance(src[i], dst[i]);
    }
    return ret;
}

#endif /* _PDU_CTYPE_CONV_HAKO_geometry_msgs_PoseWithCovariance_HPP_ */
