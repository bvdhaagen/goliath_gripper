#include "gripper_controller/gripper_hardware.hpp"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"

namespace gripper_controller
{
using namespace boost::placeholders;

CallbackReturn RobotSystem::on_init(const hardware_interface::HardwareInfo &info)
 {
  if (SystemInterface::on_init(info) != CallbackReturn::SUCCESS) {
    return CallbackReturn::ERROR;
  }

  port_name_ = info.hardware_parameters.at("device");
  baud_rate_ = std::stoi(info.hardware_parameters.at("baud_rate"));
  timeout_ms_ = std::stoi(info.hardware_parameters.at("timeout"));

  gripper_commands_.resize(2, 0.0);
 gripper_positions_.resize(2, 0.0);

  return CallbackReturn::SUCCESS;
 }

CallbackReturn RobotSystem::on_activate(const rclcpp_lifecycle::State &)
 {
  try {
    serial_port_ = std::make_shared<boost::asio::serial_port>(io_service_, port_name_);
    serial_port_->set_option(boost::asio::serial_port::baud_rate(baud_rate_));
    serial_port_->set_option(boost::asio::serial_port::flow_control(
      boost::asio::serial_port::flow_control::none));
      serial_port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    serial_port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    //asyncRead();
    io_thread_ = boost::thread([this]() { io_service_.run(); });
    

    RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Connected to %s at %d baud", 
                port_name_.c_str(), baud_rate_);
  } catch (const boost::system::system_error &e) {
    RCLCPP_FATAL(rclcpp::get_logger("RobotSystem"), "Port open failed: %s", e.what());
    return CallbackReturn::ERROR;
  }
  return CallbackReturn::SUCCESS;
 }

///////////////////////////////////////////////////////////////////////////////////////////
//////    this will terminate the connection with the microcontroller                   ///
///////////////////////////////////////////////////////////////////////////////////////////
CallbackReturn RobotSystem::on_deactivate(const rclcpp_lifecycle::State &)
 {
  if (serial_port_) {
    serial_port_->cancel();
    serial_port_->close();
    serial_port_.reset();
  }
  io_service_.stop();
  if (io_thread_.joinable()) io_thread_.join();
  return CallbackReturn::SUCCESS;
 }

///////////////////////////////////////////////////////////////////////////////////////////
//////     here we read from the microcontroller to pick up the joint_states            ///
///////////////////////////////////////////////////////////////////////////////////////////

return_type RobotSystem::read(const rclcpp::Time &, const rclcpp::Duration &)
{
  if (!serial_port_ || !serial_port_->is_open()) {
    return return_type::ERROR;
  }

  boost::mutex::scoped_lock lock(mutex_);
  boost::asio::streambuf buffer;
  std::istream is(&buffer);

  try {
    // Read until newline character
    boost::asio::read_until(*serial_port_, buffer, '\n');
    std::string line;
    std::getline(is, line);

    // Just print the received line
    RCLCPP_INFO(rclcpp::get_logger("RobotSystem"), "Received: %s", line.c_str());
    
    for (int i = 0; i < 2; ++i) {
    size_t pos = line.find("G" + std::to_string(i + 1) + ":");
    if (pos != std::string::npos) {
      gripper_positions_[i] = std::stod(line.substr(pos + 3));
    }
  }

  } catch (const boost::system::system_error &e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotSystem"), "Read failed: %s", e.what());
    return return_type::ERROR;
  }

  return return_type::OK;
}

///////////////////////////////////////////////////////////////////////////////////////////
//////     here we write to the microcontroller to update the position command          ///
///////////////////////////////////////////////////////////////////////////////////////////

return_type RobotSystem::write(const rclcpp::Time &, const rclcpp::Duration &)
 {
  if (!serial_port_ || !serial_port_->is_open()) {
    return return_type::ERROR;
  }

  boost::mutex::scoped_lock lock(mutex_);
  std::stringstream cmd;
  cmd << "M ";
  for (size_t i = 0; i < 2; ++i) {
    cmd << gripper_commands_[i];
    if (i < 1) cmd << ",";
  }
  cmd << "\n";

  try {
    serial_port_->write_some(boost::asio::buffer(cmd.str()));
  } catch (const boost::system::system_error &e) {
    RCLCPP_ERROR(rclcpp::get_logger("RobotSystem"), "Write failed: %s", e.what());
    return return_type::ERROR;
  }
  return return_type::OK;
 }

std::vector<hardware_interface::StateInterface> RobotSystem::export_state_interfaces()
 {
  std::vector<hardware_interface::StateInterface> state_interfaces;
  std::vector<StateInterface> state_interfaces;
 state_interfaces.emplace_back("slider_22", "position", &gripper_positions_[0]);
 state_interfaces.emplace_back("slider_23", "position", &gripper_positions_[1]);
  return state_interfaces;
 }

std::vector<hardware_interface::CommandInterface> RobotSystem::export_command_interfaces()
{
  std::vector<CommandInterface> command_interfaces;
 command_interfaces.emplace_back("slider_22", "position", &gripper_commands_[0]);
 command_interfaces.emplace_back("slider_23", "position", &gripper_commands_[1]);
  return command_interfaces;
}

}  // namespace goliath_controller

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(gripper_controller::RobotSystem, hardware_interface::SystemInterface)

