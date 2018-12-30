#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#ifndef _MY_STRATEGY_H_
#define _MY_STRATEGY_H_

#include "Strategy.h"
#include "PointVectors.h"
#include "Simulation.h"
#include <string>
#include <vector>
#include <iostream>

const double EPS = 1e-5;
const double SIMULATION_DURATION = 5;
const double SIMULATION_PRECISION = 1/60.0;

struct Color {
  double r {0.0};
  double g {0.0};
  double b {0.0};
  Color() { }
  Color(double r, double g, double b) : r(r), g(g), b(b) { }
};

const Color RED    (1.0, 0.0, 0.0);
const Color GREEN  (0.0, 1.0, 0.0);
const Color BLUE   (0.0, 0.0, 1.0);
const Color VIOLET (1.0, 0.0, 1.0);
const Color YELLOW (1.0, 1.0, 0.0);
const Color TEAL   (0.0, 1.0, 1.0);
const Color WHITE  (1.0, 1.0, 1.0);
const Color BLACK  (0.0, 0.0, 0.0);
const Color LIGHT_RED  (1.0, 0.5, 0.5);
const Color LIGHT_BLUE (0.5, 0.5, 1.0);

enum Role {
    ATTACKER,              // red
    AGGRESSIVE_DEFENDER,   // light-red
    DEFENDER,              // blue
    SPECULATIVE_ATTACKER,  // violet
    SPECULATIVE_DEFENDER,  // light-blue
    BALL_CLEARER,           // yellow - unused
    DEFAULT};              // white

struct Target {
  bool exists;
  Vec2D position;
  Vec2D velocity;
};

class MyStrategy : public Strategy {
  bool initialized = false;
  double DEFENSE_BORDER;
  double CRITICAL_BORDER;
  model::Rules rules;
  model::Arena arena;
  Simulation sim;
public:
  MyStrategy();

  int prev_tick = -1;
  bool is_start_of_round = true;

  Path projected_ball_path;
  Path projected_ball_spec_path;
  std::vector<Path> projected_robot_paths;
  std::vector<Path> projected_jump_paths;

  std::vector<int> ally_ids;

  std::vector<Vec3D> target_positions = {Vec3D()};
  std::vector<Vec3D> target_velocities = {Vec3D()};
  std::vector<double> jump_speeds = {0.0};
  std::vector<Vec3D> robot_positions = {Vec3D()};
  std::vector<Vec3D> robot_velocities = {Vec3D()};
  std::vector<Role> roles = {DEFAULT};

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
      const bool &to_shift_x,
      const bool &is_speculative = false,
      const Path &avoid_path = {});
  Target calc_defend_spot(
      const Path &ball_path,
      const Vec2D &my_position);
  Target get_default_strat(
      const Vec2D &my_position,
      const Vec2D &ball_position);
  Role calc_role(
      const int &id,
      const Vec3D &my_position,
      const Vec3D &ball_position,
      const std::vector<model::Robot> &robots,
      const Target &attack,
      const Target &attack_aggro,
      const Target &attack_spec,
      const Target &cross,
      const Target &cross_spec);
  double calc_jump_speed(
      const Vec3D &my_position,
      const Vec3D &ball_position,
      const int &id);
  bool goal_scored(double z);
  bool paths_intersect(
      const Path &robot_path,
      const Path &ball_path,
      const int &till_tick);
  bool is_duplicate_target(
      const Vec2D &target_position,
      const Vec2D &my_position,
      const int &id,
      const std::vector<model::Robot> &robots);
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
  std::string draw_border_util(const double &border_z);
  std::string draw_sphere_util(
      const Vec3D &pos,
      const double &radius,
      const Color &color,
      const double &alpha);
  std::string draw_line_util(
      const Vec3D &p1,
      const Vec3D &p2,
      const double &width,
      const Color &color,
      const double &alpha);
};

#endif
