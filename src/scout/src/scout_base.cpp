#include "scout/scout_base.hpp"

#include <string>
#include <iostream>

#include "scout/scout_can_protocol.h"

namespace wescore
{
ScoutBase::~ScoutBase()
{
    if (cmd_thread_.joinable())
        cmd_thread_.join();
}

void ScoutBase::ConnectSerialPort(const std::string &port_name, int32_t baud_rate)
{
    // serial_connected_ = (scout_serial::Open_Serial(port_name, baud_rate) > 0) ? true : false;
}

void ScoutBase::ConnectCANBus(const std::string &can_if_name)
{
    can_if_ = std::make_shared<ASyncCAN>(can_if_name);

    can_if_->set_receive_callback(std::bind(&ScoutBase::ParseCANFrame, this, std::placeholders::_1));
}

void ScoutBase::StartCmdThread(int32_t period_ms)
{
    cmd_thread_ = std::thread(std::bind(&ScoutBase::ControlLoop, this, period_ms));
}

void ScoutBase::ControlLoop(int32_t period_ms)
{
    stopwatch::StopWatch ctrl_sw;
    uint8_t cmd_count = 0;
    while (true)
    {
        ctrl_sw.tic();

        MotionControlMessage msg;
        msg.data.cmd.control_mode = CMD_MODE;

        motion_cmd_mutex_.lock();
        msg.data.cmd.fault_clear_flag = static_cast<uint8_t>(current_motion_cmd_.fault_clear_flag);
        msg.data.cmd.linear_velocity_cmd = current_motion_cmd_.linear_velocity;
        msg.data.cmd.angular_velocity_cmd = current_motion_cmd_.angular_velocity;
        motion_cmd_mutex_.unlock();

        msg.data.cmd.reserved0 = 0;
        msg.data.cmd.reserved1 = 0;
        msg.data.cmd.count = cmd_count++;
        msg.data.cmd.checksum = Agilex_CANMsgChecksum(msg.id, msg.data.raw, msg.dlc);

        // send to can bus
        can_frame frame;
        frame.can_id = msg.id;
        frame.can_dlc = msg.dlc;
        std::memcpy(frame.data, msg.data.raw, msg.dlc * sizeof(uint8_t));
        can_if_->send_frame(frame);
        // ------------------

        if (ctrl_sw.toc() * 1000 > period_ms)
            std::cerr << "UpdateControl() took longer than allowable time for an update iteration" << std::endl;
        else
            ctrl_sw.sleep_until_ms(period_ms);
        std::cout << "update freq: " << 1.0 / ctrl_sw.toc() << std::endl;
    }
}

void ScoutBase::SetMotionCommand(double linear_vel, double angular_vel, ScoutMotionCmd::FaultClearFlag fault_clr_flag)
{
    if (linear_vel < ScoutMotionCmd::min_linear_velocity)
        linear_vel = ScoutMotionCmd::min_linear_velocity;
    if (linear_vel > ScoutMotionCmd::max_linear_velocity)
        linear_vel = ScoutMotionCmd::max_linear_velocity;
    if (angular_vel < ScoutMotionCmd::min_angular_velocity)
        angular_vel = ScoutMotionCmd::min_angular_velocity;
    if (angular_vel > ScoutMotionCmd::max_angular_velocity)
        angular_vel = ScoutMotionCmd::max_angular_velocity;

    std::lock_guard<std::mutex> guard(motion_cmd_mutex_);
    current_motion_cmd_.linear_velocity = static_cast<uint8_t>(linear_vel / ScoutMotionCmd::max_linear_velocity * 100.0);
    current_motion_cmd_.angular_velocity = static_cast<uint8_t>(angular_vel / ScoutMotionCmd::max_angular_velocity * 100.0);
    std::cout << "linear velocity: " << static_cast<int>(current_motion_cmd_.linear_velocity) << " , " << static_cast<int>(current_motion_cmd_.angular_velocity) << std::endl;
    current_motion_cmd_.fault_clear_flag = fault_clr_flag;
}

void ScoutBase::ParseCANFrame(can_frame *rx_frame)
{

}

} // namespace wescore
