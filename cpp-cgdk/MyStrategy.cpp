#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#ifndef _MY_STRATEGY_CPP_
#define _MY_STRATEGY_CPP_

#include "MyStrategy.h"

MyStrategy::MyStrategy() { }

void MyStrategy::act(
    const model::Robot &me,
    const model::Rules &rules,
    const model::Game &game,
    model::Action &action) {

  if (game.current_tick == 0 and not this->initialized)
    this->init_strategy(rules, game);

  if (this->current_tick != game.current_tick)
    this->init_tick(game);

  this->init_query(me.id, game);

  this->run_simulation(game);

  this->calc_action(action, NUM_RAYS);
}


void MyStrategy::init_strategy(
    const model::Rules &rules,
    const model::Game &game) {
  std::cout << "START!\n";

  this->RULES = rules;
  this->ARENA = rules.arena;

  this->ZONE_BORDER = 0.0;
  this->DEFENSE_BORDER = -this->ARENA.depth/8.0;
  this->CRITICAL_BORDER = -(this->ARENA.depth/2.0 - this->ARENA.top_radius);
  this->REACHABLE_HEIGHT = this->RULES.ROBOT_MAX_RADIUS +
                           geom::calc_jump_height(
                             this->RULES.ROBOT_MAX_JUMP_SPEED,
                             this->RULES.GRAVITY) +
                           this->RULES.ROBOT_MIN_RADIUS +
                           this->RULES.BALL_RADIUS;
  this->ACCEPTABLE_JUMP_DISTANCE = this->REACHABLE_HEIGHT;

  this->GOAL_LIM_LEFT  = Vec2D(  this->ARENA.goal_width/2.0 - 2*this->RULES.ROBOT_RADIUS,  this->ARENA.depth/2.0);
  this->GOAL_LIM_RIGHT = Vec2D(-(this->ARENA.goal_width/2.0 - 2*this->RULES.ROBOT_RADIUS), this->ARENA.depth/2.0);

  this->ball = Entity(game.ball, this->RULES);
  for (model::Robot robot : game.robots) {
    this->robots[robot.id] = Entity(robot, this->RULES);

    this->robot_ids.push_back(robot.id);
    (robot.is_teammate ? this->ally_ids : this->enemy_ids).push_back(robot.id);
  }

  this->sim = Simulation(rules);

  this->initialized = true;
}

void MyStrategy::init_tick(const model::Game &game) {
  assert(this->initialized);

  // std::cout<<"----------------------\n";
  // std::cout<<"current tick: "<<this->current_tick<<"\n";
  this->current_tick = game.current_tick;
  if (this->current_tick % 100 == 0)
    std::cout << this->current_tick << "\n";

  this->renderer.clear();
}

void MyStrategy::init_query(const int &me_id, const model::Game &game) {
  this->ball.update(game.ball);
  for (model::Robot robot : game.robots)
    this->robots[robot.id].update(robot);

  this->me_id = me_id;
  this->me = &this->robots[me_id];

  t_attack.exists = false;
  t_defend.exists = false;
  t_prepare.exists = false;
  t_block.exists = false;
}

void MyStrategy::run_simulation(const model::Game &game) {
  this->sim.calc_ball_path(this->ball, SIMULATION_NUM_TICKS, SIMULATION_PRECISION);

  for (int id : this->ally_ids)
    this->sim.calc_robot_path(
      this->robots[id],
      this->robots[id].projected_jump_path,
      SIMULATION_PRECISION,
      RULES.ROBOT_MAX_JUMP_SPEED,
      BY_TICK,
      0);

  for (int id : this->ally_ids)
    this->sim.calc_robot_path(
      this->robots[id],
      this->robots[id].projected_path,
      SIMULATION_PRECISION,
      RULES.ROBOT_MAX_JUMP_SPEED,
      DONT_JUMP);
}

void MyStrategy::calc_action(model::Action &action, const int &num_rays) {
  this->t_attack = this->calc_attack(num_rays, 0.75*this->RULES.ROBOT_MAX_GROUND_SPEED);
  this->t_defend = this->calc_defend();

  auto [target_exists, target_position, target_velocity] = this->t_attack;

  if (this->is_closest_to_our_goal() and int(robots.size()) > 2) {
    if (target_exists and
        this->is_closer_than_enemies(target_position) and
        target_position.z <= this->DEFENSE_BORDER) {
      // std::cout<<this->me_id<<" CLEARING BALL!\n";
      this->set_action(
        action,
        CLEARING_BALL,
        Vec3D(target_position, this->RULES.ROBOT_RADIUS),
        Vec3D(target_velocity, 0.0),
        this->calc_jump_speed(),
        false
      );
    } else {
      this->set_action(
        action,
        GOALKEEPING,
        Vec3D(this->t_defend.position, this->RULES.ROBOT_RADIUS),
        Vec3D(this->t_defend.needed_velocity, 0.0),
        this->calc_jump_speed(),
        false
      );
    }
  } else {
    // if (this->can_enemy_interrupt_before_us(0.3) or this->an_ally_is_attacking()) {
    if (this->an_ally_is_attacking()) {

    } else {

      if (target_exists) {
        this->set_action(
          action,
          ATTACKING,
          Vec3D(target_position, this->RULES.ROBOT_RADIUS),
          Vec3D(target_velocity, 0.0),
          this->calc_jump_speed(),
          false
        );
      } else {
        if (target_exists)
          target_position = geom::offset_to(target_position, this->me->position.drop(), 2*this->RULES.BALL_RADIUS);
        else {
          target_position = this->ball.bounce_positions[0].drop();
          target_position.z -= 2*this->RULES.BALL_RADIUS;
        }
        target_velocity = (target_position - this->me->position.drop()) * this->RULES.ROBOT_MAX_GROUND_SPEED;
        // std::cout<<"target: "<<target_position.str()<<"\n";
        this->set_action(
          action,
          PREPARING_TO_ATTACK,
          Vec3D(target_position, this->RULES.ROBOT_RADIUS),
          Vec3D(target_velocity, 0.0),
          this->calc_jump_speed(),
          false
        );
      }
    }
  }
}

void MyStrategy::set_action(
    model::Action &action,
    const ActionSeq &action_seq,
    const Vec3D &target_position,
    const Vec3D &target_velocity,
    const double &jump_speed,
    const bool &use_nitro) {
  // this->me->action_seq = action_seq;
  this->me->target_position = target_position;
  this->me->target_velocity = target_velocity;
  this->me->jump_speed = jump_speed;
  this->me->use_nitro = use_nitro;

  action.target_velocity_x = target_velocity.x;
  action.target_velocity_z = target_velocity.z;
  action.target_velocity_y = 0.0;
  action.jump_speed = jump_speed;
  action.use_nitro = use_nitro;
}


bool MyStrategy::is_closest_to_our_goal() {
  double my_time_to_goal =
    geom::time_to_go_to(
      this->me->position.drop(),
      this->me->velocity.drop(),
      Vec2D(this->me->position.x, -this->ARENA.depth/2.0));
  for (int id : this->ally_ids) {
    if (id == this->me_id)
      continue;

    double time_to_goal =
      geom::time_to_go_to(
        this->robots[id].position.drop(),
        this->robots[id].velocity.drop(),
        Vec2D(this->robots[id].position.x, -this->ARENA.depth/2.0));

    if (time_to_goal < my_time_to_goal)
      return false;
  }
  return true;
}

bool MyStrategy::is_closer_than_enemies(const Vec2D &pos) {
  double my_time_needed =
    geom::time_to_go_to(
      this->me->position.drop(),
      this->me->velocity.drop(),
      pos
    );
  /*
  std::cout<<"dist:\n";
  std::cout<<"pos: "<<pos.str()<<"\n";
  std::cout<<"my time needed: "<<my_time_needed<<"\n";
  */

  for (int id : this->enemy_ids) {
    double en_time_needed =
      geom::time_to_go_to(
        this->robots[id].position.drop(),
        this->robots[id].velocity.drop(),
        pos
      );

    // std::cout<<id<<" time needed: "<<en_time_needed<<"\n";
    if (en_time_needed < my_time_needed)
      return false;
  }
  return true;
}

bool MyStrategy::can_enemy_interrupt_before_us(const double &time_diff) {
  for (PosVelTime &ball_pvt : this->ball.projected_path) {
    if (this->sim.goal_scored(ball_pvt.position.z))
      break;
    if (ball_pvt.position.y > this->REACHABLE_HEIGHT)
      continue;

    double my_time_needed =
      geom::time_to_go_to(
        this->me->position.drop(),
        this->me->velocity.drop(),
        ball_pvt.position.drop()
      );
    // std::cout<<"------------------------------------\n";
    // std::cout<<"ball_pvt.time: "<<ball_pvt.time<<"\n";
    // std::cout<<"my_time_needed: "<<my_time_needed<<"\n";

    for (int id : this->enemy_ids) {
      double en_time_needed =
        geom::time_to_go_to(
          this->robots[id].position.drop(),
          this->robots[id].velocity.drop(),
          ball_pvt.position.drop()
        );
      // std::cout<<"(id: "<<en_time_needed<<")\n";

      if (en_time_needed <= ball_pvt.time and
          my_time_needed - en_time_needed >= time_diff)
        return true;
    }
  }
  return false;
}

bool MyStrategy::an_ally_is_attacking() {
  std::vector<ActionSeq> attacking_actions = {CLEARING_BALL, ATTACKING};
  for (int id : this->ally_ids) {
    if (id == this->me_id)
      continue;

    for (ActionSeq attack : attacking_actions)
      if (this->robots[id].action_seq == attack)
        return true;
  }
  return false;
}

Target MyStrategy::calc_attack(const int &num_rays, const double &min_speed) {
  Vec2D target_position;
  Vec2D target_velocity;
  for (PosVelTime &ball_pvt : this->ball.projected_path) {
    if (this->sim.goal_scored(ball_pvt.position.z))
      break;
    if (ball_pvt.position.y > this->REACHABLE_HEIGHT)
      continue;
    if (ball_pvt.time < BIG_EPS)
      continue;

    auto [direct_target, scoring_targets] =
      this->calc_targets_from(this->me->strip(), ball_pvt, num_rays);

    // target_position this->select_best_target(direct_target, scoring_targets);
    if (scoring_targets.empty())
      target_position = direct_target;
    else
      target_position = scoring_targets[int(scoring_targets.size())/2];

    if (target_position.z < this->me->position.z)
      continue;

    Vec2D delta_pos = target_position - this->me->position.drop();
    target_velocity = delta_pos / ball_pvt.time;

    double speed = target_velocity.len();

    if (min_speed <= speed and
        speed <= this->RULES.ROBOT_MAX_GROUND_SPEED) {
      this->renderer.draw_sphere(
        Vec3D(target_position, ball_pvt.position.y),
        1,
        VIOLET,
        0.25);
      this->renderer.draw_line(
        this->me->position,
        Vec3D(target_position, ball_pvt.position.y),
        10,
        VIOLET,
        0.5);
      this->renderer.draw_line(
        ball_pvt.position,
        ball_pvt.position + ball_pvt.velocity,
        10,
        VIOLET,
        0.5);
      return {true, target_position, target_velocity};
    }
  }
  return {false, Vec2D(), Vec2D()};
}

Target MyStrategy::calc_defend() {
  Vec2D target_position(
    clamp(this->ball.position.x,
          -(this->ARENA.goal_width/2.0-2*this->ARENA.bottom_radius),
          this->ARENA.goal_width/2.0-2*this->ARENA.bottom_radius),
    -this->ARENA.depth/2.0);
  Vec2D target_velocity = (target_position - this->me->position.drop()) *
                          this->RULES.ROBOT_MAX_GROUND_SPEED;

  for (PosVelTime &ball_pvt : this->ball.projected_path) {
    if (ball_pvt.time < BIG_EPS)
      continue;
    if (ball_pvt.position.z <= -this->ARENA.depth/2.0) {
      target_position.x = ball_pvt.position.x;
      Vec2D delta_pos = target_position - this->me->position.drop();
      target_velocity = delta_pos / ball_pvt.time;
      return {true, target_position, target_velocity};
    }
  }
  return {true, target_position, target_velocity};
}

std::tuple<Vec2D&, std::vector<Vec2D>& > MyStrategy::calc_targets_from(
    const PosVelTime &robot_pvt,
    const PosVelTime &ball_pvt,
    const int &num_rays) {

  Vec2D direct_target = geom::offset_to(
      ball_pvt.position.drop(),
      robot_pvt.position.drop(),
      3 - 0.1);
  std::vector<Vec2D> scoring_targets;

  /*
  // draw_targets
  this->renderer.draw_sphere(
    Vec3D(direct_target, ball_pvt.position.y), 1, BLACK, 1);
  */

  std::vector<Vec2D> tangents =
    geom::get_tangents_to_circle(
      ball_pvt.position.drop(),
      this->RULES.BALL_RADIUS + this->RULES.ROBOT_RADIUS - 0.1,
      robot_pvt.position.drop()
    );

  std::vector<Vec2D> edges;
  if (int(tangents.size()) == 0)
    return std::forward_as_tuple(direct_target, scoring_targets);
  else if (int(tangents.size()) == 1)
    edges = tangents;
  else if (int(tangents.size()) == 2) {
    Vec2D dir_tangents = tangents[1] - tangents[0];
    for (int raw = 0; raw <= num_rays; ++raw) {
      Vec2D in_point = tangents[0] + dir_tangents * (1.0*raw/num_rays);
      Vec2D edge = geom::get_segment_circle_intersection(
        ball_pvt.position.drop(),
        this->RULES.BALL_RADIUS + this->RULES.ROBOT_RADIUS - 0.1,
        in_point,
        robot_pvt.position.drop());
      edges.push_back(edge);
    }
  } else
    assert(false);        // shouldn't happen


  for (const Vec2D &edge : edges) {
    EntityLite r_dummy = this->me->lighten();
    r_dummy.position = Vec3D(edge, ball_pvt.position.y);
    r_dummy.velocity = Vec3D(edge - robot_pvt.position.drop(), 0) *
                       this->RULES.ROBOT_MAX_GROUND_SPEED;
    r_dummy.velocity.clamp(this->RULES.ROBOT_MAX_GROUND_SPEED);
    EntityLite b_dummy = this->ball.lighten();
    b_dummy.position = ball_pvt.position;
    b_dummy.velocity = ball_pvt.velocity;

    bool has_collided = sim.collide_entities(r_dummy, b_dummy);
    auto [can_score, intersection] = geom::ray_segment_intersection(
      b_dummy.position.drop(),
      b_dummy.velocity.drop(),
      this->GOAL_LIM_LEFT,
      this->GOAL_LIM_RIGHT);

    assert(has_collided);
    if (can_score)
      scoring_targets.push_back(edge);

    /*
    this->renderer.draw_sphere(
      Vec3D(edge, ball_pvt.position.y),
      1,
      (has_collided ? (can_score ? VIOLET : TEAL) : RED),
      0.25);
    this->renderer.draw_line(
      robot_pvt.position,
      Vec3D(edge, ball_pvt.position.y),
      10,
      (has_collided ? (can_score ? VIOLET : TEAL) : RED),
      0.5);
    this->renderer.draw_line(
      b_dummy.position,
      Vec3D(intersection, 1.0),
      10,
      (has_collided ? (can_score ? VIOLET : TEAL) : RED),
      0.5);
    */
  }

  return std::forward_as_tuple(direct_target, scoring_targets);
}

double MyStrategy::calc_jump_speed() {
  auto [exists, ball_pos, robot_pos] = calc_valid_jump_intercept(
     this->me->projected_jump_path,
     this->ball.projected_path,
     this->me->position);

  if (not exists)
    return 0.0;

  if (this->me->velocity.len() > this->RULES.ROBOT_MAX_GROUND_SPEED-1)
    return this->RULES.ROBOT_MAX_JUMP_SPEED;

  if (this->me->position.z < robot_pos.z and
      (ball_pos - robot_pos).len() <= 5*this->ACCEPTABLE_JUMP_DISTANCE)
    return this->RULES.ROBOT_MAX_JUMP_SPEED;

  return 0.0;
}

std::tuple<bool, Vec3D, Vec3D> MyStrategy::calc_valid_jump_intercept(
    const Path &robot_path,
    const Path &ball_path,
    const Vec3D &robot_position) {
  for (int i = 0; i < std::min(int(robot_path.size()), int(ball_path.size())); ++i) {
    assert(std::fabs(robot_path[i].time - ball_path[i].time) < BIG_EPS);
    if ((ball_path[i].position - robot_path[i].position).len() <= this->RULES.BALL_RADIUS + this->RULES.ROBOT_RADIUS) {
      if (ball_path[i].position.z >= robot_position.z and
          ball_path[i].position.z >= robot_path[i].position.z and
          ball_path[i].position.y >= robot_path[i].position.y) {
        return std::forward_as_tuple(true, ball_path[i].position, robot_path[i].position);
        // return {true, ball_path[i].position, robot_path[i].position};
      } else
        return std::forward_as_tuple(false, Vec3D(), Vec3D());
        // return {false, Vec3D(), Vec3D()};
    }
  }
  return std::forward_as_tuple(false, Vec3D(), Vec3D());
  // return {false, Vec3D(), Vec3D()};
}


std::string MyStrategy::custom_rendering() {
  // draw borders
  this->renderer.draw_border(this->DEFENSE_BORDER);
  this->renderer.draw_border(this->CRITICAL_BORDER);

  // draw goal
  this->renderer.draw_line(
    Vec3D(this->GOAL_LIM_LEFT, 1.0),
    Vec3D(this->GOAL_LIM_RIGHT, 1.0),
    10,
    RED,
    0.5);
  this->renderer.draw_sphere(Vec3D(this->GOAL_LIM_LEFT, 1.0), 1, RED, 1);
  this->renderer.draw_sphere(Vec3D(this->GOAL_LIM_RIGHT, 1.0), 1, RED, 1);

  // draw supersized ball
  this->renderer.draw_sphere(this->ball.position, 3, WHITE, 0.1);

  // draw ball path
  this->renderer.draw_path(this->ball.projected_path, 0.5, RED, 0.5);
  for (Vec3D position : this->ball.bounce_positions)
    this->renderer.draw_sphere(position, 1, BLACK, 1.0);

  // draw jump paths of the robots
  for (auto &[id, robot] : this->robots) {
    this->renderer.draw_path(robot.projected_jump_path, 0.5, YELLOW, 0.5, false);
    this->renderer.draw_line(
      robot.position,
      Vec3D(robot.position.x, robot.position.z, ARENA.height),
      10,
      (robot.position.y > RULES.ROBOT_RADIUS ? YELLOW : TEAL),
      0.5);
  }

  return this->renderer.get_json();
}


#endif
