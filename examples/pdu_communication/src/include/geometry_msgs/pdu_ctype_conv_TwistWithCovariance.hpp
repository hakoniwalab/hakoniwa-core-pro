#ifndef _PDU_CTYPE_CONV_HAKO_geometry_msgs_TwistWithCovariance_HPP_
#define _PDU_CTYPE_CONV_HAKO_geometry_msgs_TwistWithCovariance_HPP_

#include "pdu_primitive_ctypes.h"
#include "ros_primitive_types.hpp"
#include "pdu_primitive_ctypes_conv.hpp"
/*
 * Dependent pdu data
 */
#include "geometry_msgs/pdu_ctype_TwistWithCovariance.h"
/*
 * Dependent ros data
 */
#include "geometry_msgs/msg/twist_with_covariance.hpp"

/*
 * Dependent Convertors
 */
#include "geometry_msgs/pdu_ctype_conv_Twist.hpp"
#include "geometry_msgs/pdu_ctype_conv_Vector3.hpp"

/***************************
 *
 * PDU ==> ROS2
 *
 ***************************/
static inline int hako_convert_pdu2ros_TwistWithCovariance(Hako_TwistWithCovariance &src,  geometry_msgs::msg::TwistWithCovariance &dst)
{
    //struct convert
    hako_convert_pdu2ros_Twist(src.twist, dst.twist);
    //primitive array convertor
    (void)hako_convert_pdu2ros_array(
        src.covariance, M_ARRAY_SIZE(Hako_TwistWithCovariance, Hako_float64, covariance),
        dst.covariance, dst.covariance.size());
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_TwistWithCovariance(Hako_TwistWithCovariance src[], std::array<geometry_msgs::msg::TwistWithCovariance, _dst_len> &dst)
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_pdu2ros_TwistWithCovariance(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_TwistWithCovariance(Hako_TwistWithCovariance src[], std::vector<geometry_msgs::msg::TwistWithCovariance> &dst)
{
    dst.resize(_src_len);
    for (int i = 0; i < _src_len; i++) {
        (void)hako_convert_pdu2ros_TwistWithCovariance(src[i], dst[i]);
    }
    return 0;
}

/***************************
 *
 * ROS2 ==> PDU
 *
 ***************************/
static inline int hako_convert_ros2pdu_TwistWithCovariance(geometry_msgs::msg::TwistWithCovariance &src, Hako_TwistWithCovariance &dst)
{
    //struct convert
    hako_convert_ros2pdu_Twist(src.twist, dst.twist);
    //primitive array convertor
    (void)hako_convert_ros2pdu_array(
        src.covariance, src.covariance.size(),
        dst.covariance, M_ARRAY_SIZE(Hako_TwistWithCovariance, Hako_float64, covariance));
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_TwistWithCovariance(std::array<geometry_msgs::msg::TwistWithCovariance, _src_len> &src, Hako_TwistWithCovariance dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_TwistWithCovariance(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_TwistWithCovariance(std::vector<geometry_msgs::msg::TwistWithCovariance> &src, Hako_TwistWithCovariance dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_TwistWithCovariance(src[i], dst[i]);
    }
    return ret;
}

#endif /* _PDU_CTYPE_CONV_HAKO_geometry_msgs_TwistWithCovariance_HPP_ */
