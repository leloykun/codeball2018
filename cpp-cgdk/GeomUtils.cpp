#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#ifndef _GEOM_UTILS_CPP_
#define _GEOM_UTILS_CPP_

#include "GeomUtils.h"
#include <iostream>

namespace geom {
  std::vector<Vec2D> get_tangents_to_circle(
      const Vec2D &c_center,
      const double &c_radius,
      const Vec2D &point) {
    if (c_radius < EPS)
      return {};

    Vec2D p_point = (point - c_center) / c_radius;    // shift and scale
    double dist_sqr = p_point.len_sqr();

    if (dist_sqr < 1.0)
      return {};

    if ((dist_sqr - 1.0) < EPS)                       // point is in circumference
      return {point};

    std::vector<Vec2D> tangents;

    double D = p_point.z * std::sqrt(dist_sqr - 1);
    double tx0 = (p_point.x - D) / dist_sqr;
    double tx1 = (p_point.x + D) / dist_sqr;
    if (std::fabs(p_point.z) < EPS) {                 // point at horizontal line
      D = c_radius * std::sqrt(1 - tx0 * tx0);
      tangents.push_back(Vec2D(
        c_center.x + c_radius * tx0,
        c_center.z + D
      ));
      tangents.push_back(Vec2D(
        c_center.x + c_radius * tx1,
        c_center.z - D
      ));
    } else {
      tangents.push_back(Vec2D(
        c_center.x + c_radius * tx0,
        c_center.z + c_radius * (1 - tx0 * p_point.x) / p_point.z
      ));
      tangents.push_back(Vec2D(
        c_center.x + c_radius * tx1,
        c_center.z + c_radius * (1 - tx1 * p_point.x) / p_point.z
      ));
    }

    return tangents;
  }

  Vec2D get_segment_circle_intersection(
      const Vec2D &c_center,
      const double &c_radius,
      const Vec2D &in_point,
      const Vec2D &out_point) {
    Vec2D dir = out_point - in_point;
    double dir_len = dir.len();
    Vec2D dir_norm = dir.normalize();

    double d_ans = 0.0;

    for (double jump = dir_len; jump > EPS; jump /= 2.0) {
      while (d_ans + jump <= dir_len and
             ((in_point + dir_norm*(d_ans+jump)) - c_center).len() < c_radius) {
        d_ans += jump;
      }
    }

    return in_point + dir_norm * d_ans;
  }

  std::tuple<bool, Vec2D> ray_segment_intersection(
      const Vec2D &origin,
      const Vec2D &dir,
      const Vec2D &p1,
      const Vec2D &p2) {
    Vec2D segdir = p2 - p1;
    double d = dir.x * segdir.z - dir.z * segdir.x;
    if (std::fabs(d) > EPS) {
      double r = ((origin.z - p1.z) * segdir.x - (origin.x - p1.x) * segdir.z) / d;
      double s = ((origin.z - p1.z) * dir.x    - (origin.x - p1.x) * dir.z) / d;
      if (0 <= r and 0 <= s and s <= 1)
        return {true, p1 + segdir * s};
    }
    return {false, Vec2D()};
  }

  Vec2D offset_to(const Vec2D &origin, const Vec2D &to, const double &offset) {
    return origin + (to - origin).normalize() * offset;
  }

  double calc_jump_height(const double &jump_speed, const double &gravity) {
    return (jump_speed * jump_speed) / (2 * gravity);
  }

  double time_to_go_to(
      const Vec2D &origin,
      const Vec2D &init_velocity,
      const Vec2D &destination,
      const double &acceleration,
      const double &max_speed) {
    Vec2D dir = destination - origin;
    double dist = dir.len();
    double speed = init_velocity.dot(dir.normalize());

    double t_maxv = (max_speed - speed) / acceleration;
    double t_reach = (max_speed * max_speed - speed * speed) / (2 * acceleration);

    if (t_maxv < t_reach) {
      /*      ----------
       *     /
       *    /
       */
      double dist_left = dist - 0.5 * (speed + max_speed) * t_maxv;
      double t_left = dist_left / max_speed;
      return t_maxv + t_left;
    } else {
      return t_reach;
    }
  }
}

#endif