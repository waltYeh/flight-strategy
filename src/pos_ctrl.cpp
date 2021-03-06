#include "ros/ros.h"
#include <math.h>
#include "geometry_msgs/Twist.h"
#include "geometry_msgs/PoseStamped.h"
#include "nav_msgs/Odometry.h"
#include "std_msgs/Empty.h"
#include "Eigen/Dense"
#include "ardrone_autonomy/Navdata.h"
#include "ardrone_control/ROI.h"
#include <flight_strategy/ctrl.h>

#define LOOP_RATE 20
using namespace std;
using namespace Eigen;
int constrain(int a, int b, int c){return ((a)<(b)?(b):(a)>(c)?c:a);}
int dead_zone(int a, int b){return ((a)>(b)?(a):(a)<(-b)?(a):0);}
float constrain_f(float a, float b, float c){return ((a)<(b)?(b):(a)>(c)?c:a);}
float dead_zone_f(float a, float b){return ((a)>(b)?(a):(a)<(-b)?(a):0);}
int minimum(int a, int b){return (a>b?b:a);}
int maximum(int a, int b){return (a>b?a:b);}
int absolute(int a){return (a>0?a:-a);}
float absolute_f(float a){return (a>0?a:-a);}
struct raw_state
{
	Vector3f pos_b;
	Vector3f pos_f;
	Vector3f vel_b;
	Vector3f vel_f;
	float yaw;
};
struct image_state
{
	/* data */
};

struct control
{
	bool flag_arrived;
	int mode;
	bool pos_halt[3];
	float pos_sp[3];
	bool disabled;
};
struct output
{
	float vel_sp[3];
};
raw_state raw_state;
struct control ctrl = {false, 0,{false,false,false},{0,0,0},false};
struct output output = {{0,0,0}};
void ctrl_sp_callback(const flight_strategy::ctrl msg)
{
	for(int i=0;i<3;i++){
		ctrl.pos_sp[i] = msg.pos_sp[i];
		if(msg.pos_halt[i])
			ctrl.pos_halt[i] = true;
		else
			ctrl.pos_halt[i] = false;
	}
}
void odometryCallback(const nav_msgs::Odometry &msg)
{
//	pos_w(2) = msg.pose.pose.position.z;
}
void rawpos_f_Callback(const geometry_msgs::PoseStamped &msg)
{
	raw_state.pos_f(0)=msg.pose.position.x;
	raw_state.pos_f(1)=msg.pose.position.y;
	raw_state.pos_f(2)=msg.pose.position.z;
}
void rawpos_b_Callback(const geometry_msgs::PoseStamped &msg)
{
	raw_state.pos_b(0)=msg.pose.position.x;
	raw_state.pos_b(1)=msg.pose.position.y;
	raw_state.pos_b(2)=msg.pose.position.z;
}
void navCallback(const ardrone_autonomy::Navdata &msg)
{

}
bool inaccurate_control_1D(float pos_sp, float pos, float *vel_sp)
{
	float speed = 0.7;
	bool is_arrived;
	float err = pos_sp - pos;
	float dist = absolute_f(err);
	if(dist > 0.15){
		float direction = err / dist;
		*vel_sp = direction * speed;
		is_arrived = false;
	//	ROS_INFO("\npos(%f, %f)\nsetpt(%f, %f)\nvelsp(%f, %f)\n",_pos(0),_pos(1),_pos_sp(0),_pos_sp(1),vel_sp(0),vel_sp(1));
	}
	else{
		is_arrived = true;
	}
	return is_arrived;
}
bool inaccurate_control_3D(const Vector3f& pos_sp, const Vector3f& pos, Vector3f& vel_sp)
{
	float speed = 0.1;
	bool is_arrived;
	Vector3f err = pos_sp - pos;
	err(2) = 0;
	float dist = err.norm();
	if(dist > 0.3){
		Vector3f direction = err / dist;
		vel_sp = direction * speed;

		is_arrived = false;
	//	ROS_INFO("\npos(%f, %f)\nsetpt(%f, %f)\nvelsp(%f, %f)\n",_pos(0),_pos(1),_pos_sp(0),_pos_sp(1),vel_sp(0),vel_sp(1));
	}
	else{
		is_arrived = true;
	}
	return is_arrived;
}
bool accurate_control(const Vector3f& image_pos, float pos_z, Vector3f& vel_sp)
{
	static Vector2f err_last;
	static Vector2f err_int;
	static bool new_start = true;
	float P_pos = 0.0001, D_pos = 0.00005, I_pos = 0;
	Vector2f vel_sp_2d;
	Vector2f image_center(320.0,180.0);
	Vector2f image_pos_2d;
	image_pos_2d(0) = image_pos(0);
	image_pos_2d(1) = image_pos(1);
	Vector2f err = image_pos_2d - image_center;
	if(new_start){
		err_last = err;
		err_int(0) = 0;
		err_int(1) = 0;
		new_start = false;
	}
	Vector2f err_d = (err - err_last) * LOOP_RATE;
	vel_sp_2d = err * P_pos + err_d * D_pos + err_int * I_pos;
	
	vel_sp(0) = -vel_sp_2d(1);
	vel_sp(1) = -vel_sp_2d(0);
	vel_sp(2) = 0;
	vel_sp(0)=constrain_f(vel_sp(0), -0.08, 0.08);
	vel_sp(1)=constrain_f(vel_sp(1), -0.08, 0.08);
	
	bool is_arrived;
	float dist = err.norm();
	if(dist > 30.0){
		is_arrived = false;
	}
	else{
		is_arrived = true;
	}
	err_last = err;
	err_int += err / LOOP_RATE;
	return is_arrived;
}
int main(int argc, char **argv)
{
	ros::init(argc, argv, "position_controler");
	ros::NodeHandle n;
//	ros::Subscriber image_pos_sub = n.subscribe("/ROI", 1, imagepositionCallback);
	ros::Publisher cmd_pub = n.advertise<geometry_msgs::Twist>("/cmd_vel", 1);
	ros::Subscriber nav_sub = n.subscribe("/ardrone/navdata", 1, navCallback);
	ros::Subscriber odometry_sub = n.subscribe("/ardrone/odometry", 1, odometryCallback);
	ros::Subscriber rawpos_f_sub = n.subscribe("/ardrone/rawpos_f", 1, rawpos_f_Callback);
	ros::Subscriber rawpos_b_sub = n.subscribe("/ardrone/rawpos_b", 1, rawpos_b_Callback);
	ros::Subscriber ctrl_sub = n.subscribe("ctrl", 1, ctrl_sp_callback);
	ros::Rate loop_rate(LOOP_RATE);
	bool arrived;
	while(ros::ok()){
		for(int i=0;i<3;i++){//xyz axis
			if(ctrl.pos_halt[i]){
				output.vel_sp[i] = 0;
			}
			else{
				float vel_sp_1D;
				arrived = inaccurate_control_1D(ctrl.pos_sp[i], raw_state.pos_f(i), &vel_sp_1D);
				if(arrived){
					output.vel_sp[i] = 0;
				}
				else{
					output.vel_sp[i] = vel_sp_1D;
				}
				
			}

		}
		geometry_msgs::Twist cmd;
		cmd.linear.x = output.vel_sp[0];
		cmd.linear.y = output.vel_sp[1];
		cmd.linear.z = output.vel_sp[2];
		cmd_pub.publish(cmd);
		ros::spinOnce();
		loop_rate.sleep();
	}
	return 0;
}