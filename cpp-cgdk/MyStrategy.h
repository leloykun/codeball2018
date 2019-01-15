#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#ifndef _MY_STRATEGY_H_
#define _MY_STRATEGY_H_

#include "Strategy.h"
#include "PointVectors.h"
#include "Simulation.h"
#include "RenderUtil.h"
#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include <algorithm>
/*
#include <queue>
#include <utility>

typedef std::pair<double, double> JumpTime;
typedef std::priority_queue<JumpTime, std::vector<JumpTime>, std::greater<JumpTime> > JumpTimePQ;
*/
const double SIMULATION_DURATION = 4;
const double SIMULATION_PRECISION = 1/60.0;

enum Role {
    ATTACKER,              // red
    AGGRESSIVE_DEFENDER,   // light-red
    DEFENDER,              // blue
    BALL_CLEARER,          // yellow - unused
    DEFAULT};              // white

struct Target {
  bool exists;
  Vec2D position;
  Vec2D velocity;
};

struct TargetJump {
  bool exists;
  Vec3D ball_pos;
  Vec3D robot_pos;
};

struct PositionAndDist {
  Vec2D position;
  double distance;
  bool operator<(const PositionAndDist &other) const {
    return this->distance < other.distance;
  }
};

class MyStrategy : public Strategy {
  bool initialized = false;
  model::Rules rules;
  model::Arena arena;
  double DEFENSE_BORDER;
  double CRITICAL_BORDER;

  Simulation sim;
  RenderUtil renderer;

  // initialized after init_strategy()
  std::vector<model::Robot> robots;
public:
  MyStrategy();

  int prev_tick = -1;
  bool is_start_of_round = true;

  std::vector<Vec3D> ball_bounce_positions;
  Path speculative_ball_path;

  Path projected_ball_path;
  std::vector<Path> projected_robot_paths;
  std::vector<Path> projected_jump_paths;

  std::vector<int> ally_ids;
  std::vector<int> enemy_ids;

  std::vector<Vec3D> target_positions = {Vec3D()};
  std::vector<Vec3D> target_velocities = {Vec3D()};
  std::vector<double> jump_speeds = {0.0};
  std::vector<Vec3D> robot_positions = {Vec3D()};
  std::vector<Vec3D> robot_velocities = {Vec3D()};
  std::vector<Role> roles = {DEFAULT};

  Target attack;
  Target attack_aggro;
  Target cross;
  Target default_strat;

  void act(
      const model::Robot& me,
      const model::Rules& rules,
      const model::Game& game,
      model::Action& action) override;
  void init_strategy(
      const model::Rules &rules,
      const model::Game &game);
  void run_simulation(const model::Game &game);
  Target calc_intercept_spot(
      const Path &ball_path,
      const Vec2D &my_position,
      const double &acceptable_height,
      const bool &to_shift_x);
  PositionAndDist calc_optimal_intercept_target(
      const Vec2D &p1,
      const Vec2D &p2,
      const double &offset);
  double robots_dist_to_line_segment(const Vec2D &p1, const Vec2D &p2);
  Target calc_defend_spot(
      const Path &ball_path,
      const Vec2D &my_position);
  Target get_default_strat(
      const Vec2D &my_position,
      const Vec2D &ball_position);
  int get_id_nearest_to(const Vec2D &position);
  Role calc_role(
      const int &id,
      const Vec3D &my_position,
      const Vec3D &ball_position);
  double calc_jump_speed(
      const Vec3D &my_position,
      const Vec3D &ball_position,
      const Vec3D &ball_velocity,
      const int &id);
  bool goal_scored(double z);
  TargetJump calc_jump_intercept(
      const Path &robot_path,
      const Path &ball_path,
      const Vec3D &my_position);
  bool is_duplicate_target(
      const Vec2D &target_position,
      const Vec2D &my_position,
      const int &id);
  bool is_attacker(const Role &role);
  bool is_defender(const Role &role);
  void set_action(
      model::Action &action,
      const int &id,
      const Vec3D &target_position,
      const Vec3D &target_velocity,
      const double &jump_speed,
      const bool &use_nitro);

  std::string custom_rendering() override;
};

#endif
