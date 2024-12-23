#include "cglm/struct/vec2.h"
#include <math.h>

//
// Angles

// Constants for angle math
#define TAU (2.0f * M_PI)
#define DEG2RAD (M_PI / 180.0f)
#define RAD2DEG (180.0f / M_PI)

// Normalize angle to range [0, TAU)
float normalize_angle(float angle) {
  angle = fmodf(angle, TAU);
  return angle < 0.0f ? angle + TAU : angle;
}

// Normalize angle to range [-PI, PI)
float normalize_angle_signed(float angle) {
  angle = normalize_angle(angle);
  return angle > M_PI ? angle - TAU : angle;
}

// Calculate shortest angular distance between two angles
float angle_diff(float a, float b) {
  float diff = normalize_angle(b - a);
  return diff > M_PI ? diff - TAU : diff;
}

// Convert degrees to radians
float degrees_to_radians(float degrees) { return degrees * DEG2RAD; }

// Convert radians to degrees
float radians_to_degrees(float radians) { return radians * RAD2DEG; }

// Get angle from direction vector
float vector_to_angle(vec2s v) { return atan2f(v.y, v.x); }

// Interpolate between angles using shortest path
float lerp_angle(float start_angle, float end_angle, float t) {
  float diff = angle_diff(start_angle, end_angle);
  return normalize_angle(start_angle + diff * t);
}

// Rotate a vector by an angle (in radians)
vec2s rotate_vector(vec2s v, float angle) {
  float c = cosf(angle);
  float s = sinf(angle);
  return (vec2s){v.x * c - v.y * s, v.x * s + v.y * c};
}

//
// Interpolation

// Basic linear interpolation (lerp)
// t is expected to be between 0 and 1
float lerp(float start, float end, float t) {
  return start + t * (end - start);
}

// Vector linear interpolation
vec2s vec2s_lerp(vec2s start, vec2s end, float t) {
  return (vec2s){lerp(start.x, end.x, t), lerp(start.y, end.y, t)};
}

// Smooth step (cubic hermite interpolation)
// Smooths out the interpolation at the start and end
float smooth_step(float t) {
  // Scale and clamp t to 0-1 range
  t = fmax(0.0f, fmin(1.0f, t));
  // 3t² - 2t³ (smooth acceleration and deceleration)
  return t * t * (3.0f - 2.0f * t);
}

// Smoother step (quintic interpolation)
// Even smoother acceleration/deceleration than smooth_step
float smoother_step(float t) {
  // Scale and clamp t to 0-1 range
  t = fmax(0.0f, fmin(1.0f, t));
  // 6t⁵ - 15t⁴ + 10t³
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// Quadratic ease in
float ease_in_quad(float t) { return t * t; }

// Quadratic ease out
float ease_out_quad(float t) { return t * (2.0f - t); }

// Quadratic ease in-out
float ease_in_out_quad(float t) {
  t *= 2.0f;
  if (t < 1.0f)
    return 0.5f * t * t;
  t -= 1.0f;
  return -0.5f * (t * (t - 2.0f) - 1.0f);
}

// Cubic ease in
float ease_in_cubic(float t) { return t * t * t; }

// Cubic ease out
float ease_out_cubic(float t) {
  t -= 1.0f;
  return (t * t * t + 1.0f);
}

// Circular ease in
float ease_in_circ(float t) { return 1.0f - sqrt(1.0f - t * t); }

// Circular ease out
float ease_out_circ(float t) {
  t -= 1.0f;
  return sqrt(1.0f - (t * t));
}

// Exponential ease in
float ease_in_expo(float t) {
  return t == 0.0f ? 0.0f : powf(2.0f, 10.0f * (t - 1.0f));
}

// Exponential ease out
float ease_out_expo(float t) {
  return t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
}

// Elastic ease out
float ease_out_elastic(float t) {
  const float pi = 3.14159265359f;
  if (t == 0.0f)
    return 0.0f;
  if (t == 1.0f)
    return 1.0f;

  float p = 0.3f; // Period
  float s = p / 4.0f;

  return powf(2.0f, -10.0f * t) * sinf((t - s) * (2.0f * pi) / p) + 1.0f;
}

// Bounce ease out
float ease_out_bounce(float t) {
  if (t < (1.0f / 2.75f)) {
    return 7.5625f * t * t;
  } else if (t < (2.0f / 2.75f)) {
    t -= (1.5f / 2.75f);
    return 7.5625f * t * t + 0.75f;
  } else if (t < (2.5f / 2.75f)) {
    t -= (2.25f / 2.75f);
    return 7.5625f * t * t + 0.9375f;
  } else {
    t -= (2.625f / 2.75f);
    return 7.5625f * t * t + 0.984375f;
  }
}

// Generic interpolation using any easing function
float interpolate(float start, float end, float t, float (*ease_func)(float)) {
  return lerp(start, end, ease_func(t));
}

// Example of how to interpolate between two rotations (in radians)
float interpolate_angle(float start_angle, float end_angle, float t) {
  // Ensure angles are in reasonable range
  while (start_angle > 2.0f * M_PI)
    start_angle -= 2.0f * M_PI;
  while (start_angle < 0.0f)
    start_angle += 2.0f * M_PI;
  while (end_angle > 2.0f * M_PI)
    end_angle -= 2.0f * M_PI;
  while (end_angle < 0.0f)
    end_angle += 2.0f * M_PI;

  // Find shortest path
  float diff = end_angle - start_angle;
  if (diff > M_PI)
    diff -= 2.0f * M_PI;
  if (diff < -M_PI)
    diff += 2.0f * M_PI;

  return start_angle + diff * t;
}

//
// Bezier

// Maximum number of control points for a Bézier curve
#define BEZIER_MAX_POINTS 4

typedef struct {
  vec2s points[BEZIER_MAX_POINTS];
  int num_points;
} BezierCurve;

// Evaluate a quadratic Bézier curve (3 control points)
vec2s evaluate_quadratic_bezier(vec2s p0, vec2s p1, vec2s p2, float t) {
  float t1 = 1.0f - t;
  return (vec2s){{t1 * t1 * p0.x + 2.0f * t1 * t * p1.x + t * t * p2.x,
                  t1 * t1 * p0.y + 2.0f * t1 * t * p1.y + t * t * p2.y}};
}

// Evaluate a cubic Bézier curve (4 control points)
vec2s evaluate_cubic_bezier(vec2s p0, vec2s p1, vec2s p2, vec2s p3, float t) {
  float t1 = 1.0f - t;
  float t1_3 = t1 * t1 * t1;
  float t_3 = t * t * t;
  float t1_2_t = 3.0f * t1 * t1 * t;
  float t1_t_2 = 3.0f * t1 * t * t;

  return (vec2s){{t1_3 * p0.x + t1_2_t * p1.x + t1_t_2 * p2.x + t_3 * p3.x,
                  t1_3 * p0.y + t1_2_t * p1.y + t1_t_2 * p2.y + t_3 * p3.y}};
}

// Get tangent (direction) vector at point t
vec2s get_cubic_bezier_tangent(vec2s p0, vec2s p1, vec2s p2, vec2s p3,
                               float t) {
  float t1 = 1.0f - t;
  float t1_2 = 2.0f * t1;
  float t_2 = 2.0f * t;

  // Derivative of cubic Bézier
  vec2s tangent = {{3.0f * (t1 * t1 * (p1.x - p0.x) + t1_2 * t * (p2.x - p1.x) +
                            t * t * (p3.x - p2.x)),
                    3.0f * (t1 * t1 * (p1.y - p0.y) + t1_2 * t * (p2.y - p1.y) +
                            t * t * (p3.y - p2.y))}};

  // Normalize the tangent vector
  float length = sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
  if (length > 0.0001f) {
    tangent.x /= length;
    tangent.y /= length;
  }

  return tangent;
}

// Bezier Path

#define MAX_SEGMENTS 32
#define POINTS_PER_SEGMENT 4 // Cubic Bézier

typedef struct {
  vec2s points[MAX_SEGMENTS * POINTS_PER_SEGMENT];
  int num_segments;
  bool is_closed; // Whether the path loops back to start
} BezierPath;

typedef struct {
  int segment;
  float local_t;
} PathLocation;

// Initialize empty path
void path_init(BezierPath *path) {
  path->num_segments = 0;
  path->is_closed = false;
}

// Add a segment to the path
bool path_add_segment(BezierPath *path, vec2s p0, vec2s p1, vec2s p2,
                      vec2s p3) {
  if (path->num_segments >= MAX_SEGMENTS)
    return false;

  int base_idx = path->num_segments * POINTS_PER_SEGMENT;
  path->points[base_idx] = p0;
  path->points[base_idx + 1] = p1;
  path->points[base_idx + 2] = p2;
  path->points[base_idx + 3] = p3;

  path->num_segments++;
  return true;
}

// Close the path by connecting last point to first
void path_close(BezierPath *path) {
  if (path->num_segments < 1)
    return;

  vec2s first = path->points[0];
  vec2s last = path->points[path->num_segments * POINTS_PER_SEGMENT - 1];

  // Create control points for smooth connection
  vec2s ctrl1 = last;
  vec2s ctrl2 = first;

  path_add_segment(path, last, ctrl1, ctrl2, first);
  path->is_closed = true;
}

PathLocation get_location_from_t(const BezierPath *path, float t) {
  PathLocation loc;

  // Handle path wrapping for closed paths
  if (path->is_closed) {
    t = fmodf(t, 1.0f);
    if (t < 0.0f)
      t += 1.0f;
  } else {
    t = fmax(0.0f, fmin(1.0f, t));
  }

  float segment_t = t * path->num_segments;
  loc.segment = (int)segment_t;
  loc.local_t = segment_t - loc.segment;

  // Handle edge case at t = 1.0
  if (loc.segment == path->num_segments) {
    loc.segment--;
    loc.local_t = 1.0f;
  }

  return loc;
}

// Evaluate position at any point along the entire path
vec2s path_evaluate(const BezierPath *path, float t) {
  if (path->num_segments == 0)
    return (vec2s){{0, 0}};

  PathLocation loc = get_location_from_t(path, t);
  int base_idx = loc.segment * POINTS_PER_SEGMENT;

  return evaluate_cubic_bezier(
      path->points[base_idx], path->points[base_idx + 1],
      path->points[base_idx + 2], path->points[base_idx + 3], loc.local_t);
}

// Get tangent vector at any point along the path
vec2s path_get_tangent(const BezierPath *path, float t) {
  if (path->num_segments == 0)
    return (vec2s){{1, 0}};

  PathLocation loc = get_location_from_t(path, t);
  int base_idx = loc.segment * POINTS_PER_SEGMENT;
  float t1 = 1.0f - loc.local_t;

  // Derivative of cubic Bézier
  vec2s tangent = {
      {3.0f *
           (t1 * t1 *
                (path->points[base_idx + 1].x - path->points[base_idx].x) +
            2 * t1 * loc.local_t *
                (path->points[base_idx + 2].x - path->points[base_idx + 1].x) +
            loc.local_t * loc.local_t *
                (path->points[base_idx + 3].x - path->points[base_idx + 2].x)),
       3.0f *
           (t1 * t1 *
                (path->points[base_idx + 1].y - path->points[base_idx].y) +
            2 * t1 * loc.local_t *
                (path->points[base_idx + 2].y - path->points[base_idx + 1].y) +
            loc.local_t * loc.local_t *
                (path->points[base_idx + 3].y -
                 path->points[base_idx + 2].y))}};

  // Normalize
  float length = sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
  if (length > 0.0001f) {
    tangent.x /= length;
    tangent.y /= length;
  }

  return tangent;
}

// Get point ahead on path for smooth following
vec2s path_get_look_ahead_point(const BezierPath *path, float t,
                                float look_ahead_distance) {
  // This is a simplified version - a more accurate one would use actual path
  // distance
  float look_ahead_t;
  if (path->is_closed) {
    look_ahead_t = fmodf(t + look_ahead_distance, 1.0f);
  } else {
    look_ahead_t = fmin(t + look_ahead_distance, 1.0f);
  }

  return path_evaluate(path, look_ahead_t);
}

// Get steering vector to follow path
vec2s path_get_steering(const BezierPath *path, vec2s current_pos, float t,
                        float look_ahead_distance, float follow_speed) {
  // Get target point
  vec2s look_ahead = path_get_look_ahead_point(path, t, look_ahead_distance);

  // Get direction to target
  vec2s to_target = {
      {look_ahead.x - current_pos.x, look_ahead.y - current_pos.y}};

  // Get path direction at current point
  vec2s tangent = path_get_tangent(path, t);

  // Blend position steering and tangent alignment
  vec2s steering = {{0.8f * to_target.x + 0.2f * tangent.x,
                     0.8f * to_target.y + 0.2f * tangent.y}};

  // Normalize and scale
  float length = sqrt(steering.x * steering.x + steering.y * steering.y);
  if (length > 0.0001f) {
    steering.x = steering.x / length * follow_speed;
    steering.y = steering.y / length * follow_speed;
  }

  return steering;
}
