// Copyright 2021 Daisuke Nishimatsu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TOPIC_TOOLS__TOOL_BASE_NODE_HPP_
#define TOPIC_TOOLS__TOOL_BASE_NODE_HPP_

#include <memory>
#include <optional>  // NOLINT : https://github.com/ament/ament_lint/pull/324
#include <string>
#include <utility>
#include <mutex>

#include "rclcpp/rclcpp.hpp"
#include "topic_tools/visibility_control.h"
#include "rosbag2_transport_backport/generic_publisher.hpp"
#include "rosbag2_transport_backport/generic_subscription.hpp"


namespace topic_tools
{
class ToolBaseNode : public rclcpp::Node
{
public:
  TOPIC_TOOLS_PUBLIC
  ToolBaseNode(const std::string & node_name, const rclcpp::NodeOptions & options);

  // Note: lifted out of rosbag2_transport_backport/{player,recorder}.hpp
  template<typename AllocatorT = std::allocator<void>>
  std::shared_ptr<rosbag2_transport::GenericSubscription> create_generic_subscription(
    const std::string & topic_name,
    const std::string & topic_type,
    const rclcpp::QoS & qos,
    std::function<void(std::shared_ptr<rclcpp::SerializedMessage>)> callback,
    const rclcpp::SubscriptionOptionsWithAllocator<AllocatorT> & options = (
      rclcpp::SubscriptionOptionsWithAllocator<AllocatorT>()
    )
  )
  {
    rclcpp::node_interfaces::NodeTopicsInterface::SharedPtr topics_interface = get_node_topics_interface();
    auto ts_lib = rosbag2_cpp::get_typesupport_library(
      topic_type, "rosidl_typesupport_cpp");

    auto subscription = std::make_shared<rosbag2_transport::GenericSubscription>(
      topics_interface->get_node_base_interface(),
      std::move(ts_lib),
      topic_name,
      topic_type,
      qos,
      callback,
      options);

    topics_interface->add_subscription(subscription, options.callback_group);

    return subscription;
  }

  template<typename AllocatorT = std::allocator<void>>
  std::shared_ptr<rosbag2_transport::GenericPublisher> create_generic_publisher(
    const std::string & topic_name,
    const std::string & topic_type,
    const rclcpp::QoS & qos,
    const rclcpp::PublisherOptionsWithAllocator<AllocatorT> & options = (
      rclcpp::PublisherOptionsWithAllocator<AllocatorT>()
    )
  )
  {
    auto ts_lib = rosbag2_cpp::get_typesupport_library(topic_type, "rosidl_typesupport_cpp");
    auto pub = std::make_shared<rosbag2_transport::GenericPublisher>(
      get_node_topics_interface()->get_node_base_interface(),
      std::move(ts_lib),
      topic_name,
      topic_type,
      qos,
      options);
    get_node_topics_interface()->add_publisher(pub, options.callback_group);
    return pub;
  }

  template<typename T>
  auto declare_required_parameter(const std::string& name) {
    auto rv = declare_parameter(name, T{});
    if (rv == T{}) throw rclcpp::exceptions::InvalidParameterValueException{name + " was not set"};
    return rv;
  }

  std::string resolve_topic_name(const std::string topic) {
    return rclcpp::expand_topic_or_service_name(topic, get_name(), get_namespace());
  }

protected:
  virtual void process_message(std::shared_ptr<rclcpp::SerializedMessage> msg) = 0;

  /// Returns an optional pair <topic type, QoS profile> of the first found source publishing
  /// on `input_topic_` if at least one source is found
  std::optional<std::pair<std::string, rclcpp::QoS>> try_discover_source();
  virtual void make_subscribe_unsubscribe_decisions();

  std::chrono::duration<float> discovery_period_ = std::chrono::milliseconds{100};
  std::optional<std::string> topic_type_;
  std::optional<rclcpp::QoS> qos_profile_;
  std::string input_topic_;
  std::string output_topic_;
  bool lazy_;
  rclcpp::TimerBase::SharedPtr discovery_timer_;
  rosbag2_transport::GenericPublisher::SharedPtr pub_;
  rosbag2_transport::GenericSubscription::SharedPtr sub_;
  std::mutex pub_mutex_;
};
}  // namespace topic_tools

#endif  // TOPIC_TOOLS__TOOL_BASE_NODE_HPP_
