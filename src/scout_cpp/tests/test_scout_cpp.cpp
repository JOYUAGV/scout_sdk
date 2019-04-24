#include <unistd.h>

#include <thread>
#include <mutex>
#include <functional>
#include <string>
#include <iostream>
#include <chrono>
#include <cmath>

#include "stopwatch/stopwatch.h"

#include "scout/scout_robot.h"

#define TEST_WITHOUT_SERIAL_HARDWARE

using namespace scout;

class ScoutMessenger
{
public:
    ScoutMessenger(ScoutRobot &robot, int32_t ctrl_freq = 10, int32_t odom_query_freq = 200)
        : robot_(robot), ctrl_freq_(ctrl_freq), odom_query_freq_(odom_query_freq){};
    ~ScoutMessenger() = default;

    using Clock = std::chrono::high_resolution_clock;
    using time_point = typename Clock::time_point;

    void PublishOdometry()
    {
        RobotState data;
        stopwatch::StopWatch swatch;
        while (true)
        {
            swatch.tic();

            // std::cout << "odometry loop" << std::endl;

            if (robot_.QueryRobotState(&data))
            {
                short linear = data.linear;
                short angular = data.angular;

                current_time_ = Clock::now();
                double dt = std::chrono::duration_cast<std::chrono::seconds>(current_time_ - last_time_).count();
                static int start_flag = 0;

                //初始化
                if (start_flag)
                {
                    last_time_ = current_time_;
                    start_flag = 1;
                    return;
                }

                double Angular = angular;
                double Linear = linear;
                Angular /= 10000;
                Linear /= 10000;

                vth = Angular; //旋转角速度，在角度很小的情况下。约等于（SPEED右 - SPEED左）/ 轮子间距
                vx = Linear;   //直线X轴的速度 =  （SPEED右 - SPEED左） / 2
                vy = 0.0;      //2轮差速车，不能左右横移

                double delta_x = (vx * std::cos(th) - vy * std::sin(th)) * dt;
                double delta_y = (vx * std::sin(th) + vy * std::cos(th)) * dt;
                double delta_th = vth * dt;

                x += delta_x;
                y += delta_y;
                th += delta_th;

                std::cout << "pose: (x,y) " << x << " , " << y << " ; yaw: " << th << std::endl;
                std::cout << "velocity: linear " << vx << " , angular " << vth << std::endl;

                last_time_ = current_time_;
            }

            swatch.sleep_until_ms(1000.0 / odom_query_freq_);
        }
    }

    void UpdateCommandCallback()
    {
        // Update linear_x, angular_z via your IPC method
        // Note: be careful about the sync (using mutex)
        std::lock_guard<std::mutex> lock(cmd_mutex_);
        cmd_linear_x_ = 0;
        cmd_angular_z_ = 0;
    }

    void SendMotionCommand()
    {
        stopwatch::StopWatch swatch;
        static unsigned char index = 0;
        while (true)
        {
            swatch.tic();

            // std::cout << "cmd loop" << std::endl;
            cmd_mutex_.lock();
#ifndef TEST_WITHOUT_SERIAL_HARDWARE
            robot_.SendCommand(cmd_linear_x_, cmd_angular_z_, index++);
#endif
            cmd_mutex_.unlock();
            swatch.sleep_until_ms(1000.0 / ctrl_freq_);
        }
    }

private:
    ScoutRobot &robot_;
    int32_t ctrl_freq_;
    int32_t odom_query_freq_;

    std::mutex cmd_mutex_;
    double cmd_linear_x_ = 0;
    double cmd_angular_z_ = 0;

    double x = 0;
    double y = 0;
    double th = 0;
    double vx = 0;
    double vy = 0;
    double vth = 0;

    time_point current_time_;
    time_point last_time_;
};

int main(int argc, char **argv)
{
    ScoutRobot scout;

#ifndef TEST_WITHOUT_SERIAL_HARDWARE
    scout.ConnectSerialPort("/dev/ttyUSB0", 115200);

    if (!scout.IsConnectionActive())
    {
        std::cerr << "Failed to connect to robot" << std::endl;
        return -1;
    }
#endif

    ScoutMessenger messenger(scout);
    std::thread odom_thread(std::bind(&ScoutMessenger::PublishOdometry, &messenger));
    std::thread cmd_thread(std::bind(&ScoutMessenger::SendMotionCommand, &messenger));

    odom_thread.join();
    cmd_thread.join();

    return 0;
}