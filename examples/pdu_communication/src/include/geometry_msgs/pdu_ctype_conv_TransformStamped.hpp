#ifndef _PDU_CTYPE_CONV_HAKO_geometry_msgs_TransformStamped_HPP_
#define _PDU_CTYPE_CONV_HAKO_geometry_msgs_TransformStamped_HPP_

#include "pdu_primitive_ctypes.h"
#include "ros_primitive_types.hpp"
#include "pdu_primitive_ctypes_conv.hpp"
/*
 * Dependent pdu data
 */
#include "geometry_msgs/pdu_ctype_TransformStamped.h"
/*
 * Dependent ros data
 */
#include "geometry_msgs/msg/transform_stamped.hpp"

/*
 * Dependent Convertors
 */
#include "builtin_interfaces/pdu_ctype_conv_Time.hpp"
#include "geometry_msgs/pdu_ctype_conv_Quaternion.hpp"
#include "geometry_msgs/pdu_ctype_conv_Transform.hpp"
#include "geometry_msgs/pdu_ctype_conv_Vector3.hpp"
#include "std_msgs/pdu_ctype_conv_Header.hpp"

/***************************
 *
 * PDU ==> ROS2
 *
 ***************************/
static inline int hako_convert_pdu2ros_TransformStamped(Hako_TransformStamped &src,  geometry_msgs::msg::TransformStamped &dst)
{
    //struct convert
    hako_convert_pdu2ros_Header(src.header, dst.header);
    //string convertor
    dst.child_frame_id = (const char*)src.child_frame_id;
    //struct convert
    hako_convert_pdu2ros_Transform(src.transform, dst.transform);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_TransformStamped(Hako_TransformStamped src[], std::array<geometry_msgs::msg::TransformStamped, _dst_len> &dst)
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_pdu2ros_TransformStamped(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_pdu2ros_array_TransformStamped(Hako_TransformStamped src[], std::vector<geometry_msgs::msg::TransformStamped> &dst)
{
    dst.resize(_src_len);
    for (int i = 0; i < _src_len; i++) {
        (void)hako_convert_pdu2ros_TransformStamped(src[i], dst[i]);
    }
    return 0;
}

/***************************
 *
 * ROS2 ==> PDU
 *
 ***************************/
static inline int hako_convert_ros2pdu_TransformStamped(geometry_msgs::msg::TransformStamped &src, Hako_TransformStamped &dst)
{
    //struct convert
    hako_convert_ros2pdu_Header(src.header, dst.header);
    //string convertor
    (void)hako_convert_ros2pdu_array(
        src.child_frame_id, src.child_frame_id.length(),
        dst.child_frame_id, M_ARRAY_SIZE(Hako_TransformStamped, char, child_frame_id));
    //struct convert
    hako_convert_ros2pdu_Transform(src.transform, dst.transform);
    return 0;
}

template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_TransformStamped(std::array<geometry_msgs::msg::TransformStamped, _src_len> &src, Hako_TransformStamped dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_TransformStamped(src[i], dst[i]);
    }
    return ret;
}
template<int _src_len, int _dst_len>
int hako_convert_ros2pdu_array_TransformStamped(std::vector<geometry_msgs::msg::TransformStamped> &src, Hako_TransformStamped dst[])
{
    int ret = 0;
    int len = _dst_len;
    if (_dst_len > _src_len) {
        len = _src_len;
        ret = -1;
    }
    for (int i = 0; i < len; i++) {
        (void)hako_convert_ros2pdu_TransformStamped(src[i], dst[i]);
    }
    return ret;
}

#endif /* _PDU_CTYPE_CONV_HAKO_geometry_msgs_TransformStamped_HPP_ */
