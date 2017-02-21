/*
 * RosHelpers.cpp
 *
 *  Created on: Jun 30, 2016
 *      Author: ai-driver
 */

#include "RosHelpers.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>
#include "PlanningHelpers.h"

namespace WayPlannerNS {

RosHelpers::RosHelpers() {

}

RosHelpers::~RosHelpers() {
}

void RosHelpers::GetTransformFromTF(const std::string parent_frame, const std::string child_frame, tf::StampedTransform &transform)
{
	static tf::TransformListener listener;

	while (1)
	{
		try
		{
			listener.lookupTransform(parent_frame, child_frame, ros::Time(0), transform);
			break;
		}
		catch (tf::TransformException& ex)
		{
			ROS_ERROR("%s", ex.what());
			ros::Duration(1.0).sleep();
		}
	}
}

void RosHelpers::ConvertFromPlannerHPointsToAutowarePathFormat(const std::vector<PlannerHNS::GPSPoint>& path,
		waypoint_follower::LaneArray& laneArray)
{
	waypoint_follower::lane l;

	for(unsigned int i=0; i < path.size(); i++)
	{
		waypoint_follower::waypoint wp;
		wp.pose.pose.position.x = path.at(i).x;
		wp.pose.pose.position.y = path.at(i).y;
		//wp.pose.pose.position.z = path.at(i).z;
		//wp.pose.pose.position.z = 5;
		wp.pose.pose.orientation = tf::createQuaternionMsgFromYaw(UtilityHNS::UtilityH::SplitPositiveAngle(path.at(i).a));

		l.waypoints.push_back(wp);
	}

	if(l.waypoints.size()>0)
		laneArray.lanes.push_back(l);
}

void RosHelpers::ConvertFromPlannerHToAutowarePathFormat(const std::vector<PlannerHNS::WayPoint>& path,
		waypoint_follower::LaneArray& laneArray)
{
	waypoint_follower::lane l;

	for(unsigned int i=0; i < path.size(); i++)
	{
		waypoint_follower::waypoint wp;
		wp.pose.pose.position.x = path.at(i).pos.x;
		wp.pose.pose.position.y = path.at(i).pos.y;
		//wp.pose.pose.position.z = path.at(i).pos.z;
		//wp.pose.pose.position.z = 5;
		wp.pose.pose.orientation = tf::createQuaternionMsgFromYaw(UtilityHNS::UtilityH::SplitPositiveAngle(path.at(i).pos.a));
		wp.twist.twist.linear.x = path.at(i).v;
		wp.twist.twist.linear.y = path.at(i).laneId;
		wp.twist.twist.linear.z = path.at(i).stopLineID;
		wp.twist.twist.angular.x = path.at(i).LeftLaneId;
		wp.twist.twist.angular.y = path.at(i).RightLaneId;
		for(unsigned int iaction = 0; iaction < path.at(i).actionCost.size(); iaction++)
		{
			if(path.at(i).actionCost.at(iaction).first == PlannerHNS::RIGHT_TURN_ACTION)
				wp.twist.twist.angular.z = 1;
			else if(path.at(i).actionCost.at(iaction).first == PlannerHNS::LEFT_TURN_ACTION)
				wp.twist.twist.angular.z = 2;
			else
				wp.twist.twist.angular.z = 0;
		}


		//wp.dtlane.dir = path.at(i).pos.a;

		//PlannerHNS::GPSPoint p = path.at(i).pos;
		//std::cout << p.ToString() << std::endl;

		l.waypoints.push_back(wp);
	}

	if(l.waypoints.size()>0)
		laneArray.lanes.push_back(l);
}

void RosHelpers::ConvertFromRoadNetworkToAutowareVisualizeMapFormat(const PlannerHNS::RoadNetwork& map,	visualization_msgs::MarkerArray& markerArray)
{
	markerArray.markers.clear();
	waypoint_follower::LaneArray map_lane_array;
	for(unsigned int i = 0; i< map.roadSegments.size(); i++)
		for(unsigned int j = 0; j < map.roadSegments.at(i).Lanes.size(); j++)
			RosHelpers::ConvertFromPlannerHToAutowarePathFormat(map.roadSegments.at(i).Lanes.at(j).points, map_lane_array);

	std_msgs::ColorRGBA total_color;
	total_color.r = 1;
	total_color.g = 0.5;
	total_color.b = 0.3;
	total_color.a = 0.85;

	visualization_msgs::Marker lane_waypoint_marker;
	  lane_waypoint_marker.header.frame_id = "map";
	  lane_waypoint_marker.header.stamp = ros::Time();
	  lane_waypoint_marker.ns = "vector_map_center_lines_rviz";
	  lane_waypoint_marker.type = visualization_msgs::Marker::LINE_STRIP;
	  lane_waypoint_marker.action = visualization_msgs::Marker::ADD;
	  lane_waypoint_marker.scale.x = 0.25;
	  lane_waypoint_marker.scale.y = 0.25;
	  lane_waypoint_marker.scale.z = 0.25;
	  lane_waypoint_marker.color = total_color;
	  //lane_waypoint_marker.frame_locked = false;

	  int count = 0;
	  for (unsigned int i=0; i<  map_lane_array.lanes.size(); i++)
	  {
	    lane_waypoint_marker.points.clear();
	    lane_waypoint_marker.id = count;

	    for (unsigned int j=0; j < map_lane_array.lanes.at(i).waypoints.size(); j++)
	    {
	      geometry_msgs::Point point;
	      point = map_lane_array.lanes.at(i).waypoints.at(j).pose.pose.position;
	      lane_waypoint_marker.points.push_back(point);
	    }
	    markerArray.markers.push_back(lane_waypoint_marker);
	    count++;
	  }


	  RosHelpers::createGlobalLaneArrayOrientationMarker(map_lane_array, markerArray);

		total_color.r = 0.99;
		total_color.g = 0.99;
		total_color.b = 0.99;
		total_color.a = 0.85;

		visualization_msgs::Marker stop_waypoint_marker;
		  stop_waypoint_marker.header.frame_id = "map";
		  stop_waypoint_marker.header.stamp = ros::Time();
		  stop_waypoint_marker.ns = "stop_lines_rviz";
		  stop_waypoint_marker.type = visualization_msgs::Marker::LINE_STRIP;
		  stop_waypoint_marker.action = visualization_msgs::Marker::ADD;
		  stop_waypoint_marker.scale.x = 0.4;
		  stop_waypoint_marker.scale.y = 0.4;
		  stop_waypoint_marker.scale.z = 0.4;
		  stop_waypoint_marker.color = total_color;
		 // stop_waypoint_marker.frame_locked = false;


		  for (unsigned int i=0; i<  map.stopLines.size(); i++)
		  {
			  waypoint_follower::LaneArray lane_array_2;
			  RosHelpers::ConvertFromPlannerHPointsToAutowarePathFormat(map.stopLines.at(i).points, lane_array_2);

			  stop_waypoint_marker.points.clear();
			  stop_waypoint_marker.id = count;
			  for (unsigned int j=0; j<  lane_array_2.lanes.size(); j++)
			  {


				for (unsigned int k=0; k < lane_array_2.lanes.at(j).waypoints.size(); k++)
				{
				  geometry_msgs::Point point;
				  point = lane_array_2.lanes.at(j).waypoints.at(k).pose.pose.position;
				  stop_waypoint_marker.points.push_back(point);
				}

				markerArray.markers.push_back(stop_waypoint_marker);
				count++;
			  }
		  }



	  //RosHelpers::createGlobalLaneArrayOrientationMarker(map_lane_array, markerArray);
}

void RosHelpers::ConvertFromPlannerHToAutowareVisualizePathFormat(const std::vector<PlannerHNS::WayPoint>& curr_path,
		const std::vector<std::vector<PlannerHNS::WayPoint> >& paths,
			visualization_msgs::MarkerArray& markerArray)
{
	visualization_msgs::Marker lane_waypoint_marker;
	lane_waypoint_marker.header.frame_id = "map";
	lane_waypoint_marker.header.stamp = ros::Time();
	lane_waypoint_marker.ns = "global_lane_array_marker";
	lane_waypoint_marker.type = visualization_msgs::Marker::LINE_STRIP;
	lane_waypoint_marker.action = visualization_msgs::Marker::ADD;
	lane_waypoint_marker.scale.x = 0.1;
	lane_waypoint_marker.scale.y = 0.1;
	std_msgs::ColorRGBA roll_color, total_color, curr_color;
	roll_color.r = 0;
	roll_color.g = 1;
	roll_color.b = 0;
	roll_color.a = 0.5;

	lane_waypoint_marker.color = roll_color;
	lane_waypoint_marker.frame_locked = false;

	int count = 0;
	for (unsigned int i = 0; i < paths.size(); i++)
	{
		lane_waypoint_marker.points.clear();
		lane_waypoint_marker.id = count;

		for (unsigned int j=0; j < paths.at(i).size(); j++)
		{
		  geometry_msgs::Point point;

		  point.x = paths.at(i).at(j).pos.x;
		  point.y = paths.at(i).at(j).pos.y;
		  //point.z = paths.at(i).at(j).pos.z;

		  lane_waypoint_marker.points.push_back(point);
		}

		markerArray.markers.push_back(lane_waypoint_marker);
		count++;
	}

	lane_waypoint_marker.points.clear();
	lane_waypoint_marker.id = count;
	lane_waypoint_marker.scale.x = 0.1;
	lane_waypoint_marker.scale.y = 0.1;
	curr_color.r = 1;
	curr_color.g = 0;
	curr_color.b = 1;
	curr_color.a = 0.9;
	lane_waypoint_marker.color = curr_color;

	for (unsigned int j=0; j < curr_path.size(); j++)
	{
	  geometry_msgs::Point point;

	  point.x = curr_path.at(j).pos.x;
	  point.y = curr_path.at(j).pos.y;
	  //point.z = curr_path.at(j).pos.z;

	  lane_waypoint_marker.points.push_back(point);
	}

	markerArray.markers.push_back(lane_waypoint_marker);
	count++;
}

void RosHelpers::ConvertFromPlannerHToAutowareVisualizePathFormat(const std::vector<std::vector<PlannerHNS::WayPoint> >& globalPaths,
			visualization_msgs::MarkerArray& markerArray)
{
	visualization_msgs::Marker lane_waypoint_marker;
	lane_waypoint_marker.header.frame_id = "map";
	lane_waypoint_marker.header.stamp = ros::Time();
	lane_waypoint_marker.ns = "global_lane_array_marker";
	lane_waypoint_marker.type = visualization_msgs::Marker::LINE_STRIP;
	lane_waypoint_marker.action = visualization_msgs::Marker::ADD;


	std_msgs::ColorRGBA roll_color, total_color, curr_color;
	lane_waypoint_marker.points.clear();
	lane_waypoint_marker.id = 1;
	lane_waypoint_marker.scale.x = 0.35;
	lane_waypoint_marker.scale.y = 0.35;
	total_color.r = 1;
	total_color.g = 0;
	total_color.b = 0;
	total_color.a = 0.5;
	lane_waypoint_marker.color = total_color;
	lane_waypoint_marker.frame_locked = false;

	int count = 0;
	for (unsigned int i = 0; i < globalPaths.size(); i++)
	{
		lane_waypoint_marker.points.clear();
		lane_waypoint_marker.id = count;

		for (unsigned int j=0; j < globalPaths.at(i).size(); j++)
		{
		  geometry_msgs::Point point;

		  point.x = globalPaths.at(i).at(j).pos.x;
		  point.y = globalPaths.at(i).at(j).pos.y;
		  //point.z = globalPaths.at(i).at(j).pos.z;

		  lane_waypoint_marker.points.push_back(point);
		}

		markerArray.markers.push_back(lane_waypoint_marker);
		count++;

//		visualization_msgs::MarkerArray tmp_marker_array;
//		visualization_msgs::Marker dir_marker;
//		  dir_marker.header.frame_id = "map";
//		  dir_marker.header.stamp = ros::Time();
//		  dir_marker.type = visualization_msgs::Marker::ARROW;
//		  dir_marker.action = visualization_msgs::Marker::ADD;
//		  dir_marker.scale.x = 0.5;
//		  dir_marker.scale.y = 0.1;
//		  dir_marker.scale.z = 0.1;
//		  dir_marker.color.r = 1.0;
//		  dir_marker.color.a = 1.0;
//		  dir_marker.frame_locked = true;
//		  dir_marker.id = count;
//		  dir_marker.ns = "direction_marker";
//
//		  for (unsigned int j=0; j < globalPaths.at(i).size(); j++)
//			{
//			  geometry_msgs::Point point;
//
//			  point.x = globalPaths.at(i).at(j).pos.x;
//			  point.y = globalPaths.at(i).at(j).pos.y;
//			  point.z = globalPaths.at(i).at(j).pos.z;
//
//			  dir_marker.pose.position = point;
//			  dir_marker.pose.orientation = tf::createQuaternionMsgFromYaw(globalPaths.at(i).at(j).pos.a);
//			  tmp_marker_array.markers.push_back(dir_marker);
//			}
//
//		  markerArray.markers.insert(markerArray.markers.end(), tmp_marker_array.markers.begin(),
//		                                           tmp_marker_array.markers.end());
//			count++;


	}
}

void RosHelpers::createGlobalLaneArrayMarker(std_msgs::ColorRGBA color,
		const waypoint_follower::LaneArray &lane_waypoints_array, visualization_msgs::MarkerArray& markerArray)
{
  visualization_msgs::Marker lane_waypoint_marker;
  lane_waypoint_marker.header.frame_id = "map";
  lane_waypoint_marker.header.stamp = ros::Time();
  lane_waypoint_marker.ns = "global_lane_array_marker";
  lane_waypoint_marker.type = visualization_msgs::Marker::LINE_STRIP;
  lane_waypoint_marker.action = visualization_msgs::Marker::ADD;
  lane_waypoint_marker.scale.x = 0.75;
  lane_waypoint_marker.scale.y = 0.75;
  lane_waypoint_marker.color = color;
  lane_waypoint_marker.frame_locked = false;

  int count = 0;
  for (unsigned int i=0; i<  lane_waypoints_array.lanes.size(); i++)
  {
    lane_waypoint_marker.points.clear();
    lane_waypoint_marker.id = count;

    for (unsigned int j=0; j < lane_waypoints_array.lanes.at(i).waypoints.size(); j++)
    {
      geometry_msgs::Point point;
      point = lane_waypoints_array.lanes.at(i).waypoints.at(j).pose.pose.position;
      lane_waypoint_marker.points.push_back(point);
    }
    markerArray.markers.push_back(lane_waypoint_marker);
    count++;
  }

}

void RosHelpers::createGlobalLaneArrayVelocityMarker(const waypoint_follower::LaneArray &lane_waypoints_array
		, visualization_msgs::MarkerArray& markerArray)
{
  visualization_msgs::MarkerArray tmp_marker_array;
  // display by markers the velocity of each waypoint.
  visualization_msgs::Marker velocity_marker;
  velocity_marker.header.frame_id = "map";
  velocity_marker.header.stamp = ros::Time();
  velocity_marker.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
  velocity_marker.action = visualization_msgs::Marker::ADD;
  //velocity_marker.scale.z = 0.4;
  velocity_marker.color.a = 0.9;
  velocity_marker.color.r = 1;
  velocity_marker.color.g = 1;
  velocity_marker.color.b = 1;
  velocity_marker.frame_locked = false;

  int count = 1;
  for (unsigned int i=0; i<  lane_waypoints_array.lanes.size(); i++)
  {

	  std::ostringstream str_count;
	  str_count << count;
    velocity_marker.ns = "global_velocity_lane_" + str_count.str();
    for (unsigned int j=0; j < lane_waypoints_array.lanes.at(i).waypoints.size(); j++)
    {
      //std::cout << _waypoints[i].GetX() << " " << _waypoints[i].GetY() << " " << _waypoints[i].GetZ() << " " << _waypoints[i].GetVelocity_kmh() << std::endl;
      velocity_marker.id = j;
      geometry_msgs::Point relative_p;
      relative_p.y = 0.5;
      velocity_marker.pose.position = calcAbsoluteCoordinate(relative_p, lane_waypoints_array.lanes.at(i).waypoints.at(j).pose.pose);
      velocity_marker.pose.position.z += 0.2;

      // double to string
      std::ostringstream str_out;
      str_out << lane_waypoints_array.lanes.at(i).waypoints.at(j).twist.twist.linear.x;
      //std::string vel = str_out.str();
      velocity_marker.text = str_out.str();//vel.erase(vel.find_first_of(".") + 2);

      tmp_marker_array.markers.push_back(velocity_marker);
    }
    count++;
  }

  markerArray.markers.insert(markerArray.markers.end(), tmp_marker_array.markers.begin(),
                                       tmp_marker_array.markers.end());
}

void RosHelpers::createGlobalLaneArrayOrientationMarker(const waypoint_follower::LaneArray &lane_waypoints_array
		, visualization_msgs::MarkerArray& markerArray)
{
  visualization_msgs::MarkerArray tmp_marker_array;
  visualization_msgs::Marker lane_waypoint_marker;
  lane_waypoint_marker.header.frame_id = "map";
  lane_waypoint_marker.header.stamp = ros::Time();
  lane_waypoint_marker.ns = "global_lane_waypoint_orientation_marker";
  lane_waypoint_marker.type = visualization_msgs::Marker::ARROW;
  lane_waypoint_marker.action = visualization_msgs::Marker::ADD;
  lane_waypoint_marker.scale.x = 1;//0.5;
  lane_waypoint_marker.scale.y = 0.3;//0.15;
  lane_waypoint_marker.scale.z = 0.3;//0.15;
  lane_waypoint_marker.color.r = 1.0;
  lane_waypoint_marker.color.a = 0.8;
  //lane_waypoint_marker.frame_locked = false;


  int count = 1;
  for (unsigned int i=0; i<  lane_waypoints_array.lanes.size(); i++)
  {
    for (unsigned int j=0; j < lane_waypoints_array.lanes.at(i).waypoints.size(); j++)
    {
    	if(lane_waypoints_array.lanes.at(i).waypoints.at(j).twist.twist.angular.z == 1)
    	{
    		lane_waypoint_marker.color.r = 0.0;
    		lane_waypoint_marker.color.g = 1.0;
    		lane_waypoint_marker.color.b = 0.0;
    	}
    	else if(lane_waypoints_array.lanes.at(i).waypoints.at(j).twist.twist.angular.z == 2)
    	{
    		lane_waypoint_marker.color.r = 0.0;
			lane_waypoint_marker.color.g = 0.0;
			lane_waypoint_marker.color.b = 1.0;
    	}
    	else
    	{
    		lane_waypoint_marker.color.r = 1.0;
			lane_waypoint_marker.color.g = 0.0;
			lane_waypoint_marker.color.b = 1.0;
    	}

      lane_waypoint_marker.id = count;
      lane_waypoint_marker.pose = lane_waypoints_array.lanes.at(i).waypoints.at(j).pose.pose;
      tmp_marker_array.markers.push_back(lane_waypoint_marker);
      count++;
    }
  }

  markerArray.markers.insert(markerArray.markers.end(), tmp_marker_array.markers.begin(),
                                         tmp_marker_array.markers.end());

}

void RosHelpers::FindIncommingBranches(const std::vector<std::vector<PlannerHNS::WayPoint> >& globalPaths, const PlannerHNS::WayPoint& currPose,const double& min_distance,
			std::vector<PlannerHNS::WayPoint*>& branches)
{
	static int detection_range = 20; // meter
	if(globalPaths.size() > 0)
	{
		int close_index = PlannerHNS::PlanningHelpers::GetClosestNextPointIndex(globalPaths.at(0), currPose);
		PlannerHNS::WayPoint closest_wp = globalPaths.at(0).at(close_index);
		double d = 0;
		for(unsigned int i=close_index+1; i < globalPaths.at(0).size(); i++)
		{
			d += hypot(globalPaths.at(0).at(i).pos.y - globalPaths.at(0).at(i-1).pos.y, globalPaths.at(0).at(i).pos.x - globalPaths.at(0).at(i-1).pos.x);

			if(d - min_distance > detection_range)
				return;

			if(d > min_distance && globalPaths.at(0).at(i).pFronts.size() > 1)
			{
				for(unsigned int j = 0; j< globalPaths.at(0).at(i).pFronts.size(); j++)
				{
					PlannerHNS::WayPoint* wp =  globalPaths.at(0).at(i).pFronts.at(j);
					bool bFound = false;
					for(unsigned int ib=0; ib< branches.size(); ib++)
					{
						if(branches.at(ib)->actionCost.at(0).first == wp->actionCost.at(0).first)
						{
							bFound = true;
							break;
						}
					}

					if(!bFound && closest_wp.laneId != wp->laneId)
						branches.push_back(wp);
				}
			}
		}
	}
}

}